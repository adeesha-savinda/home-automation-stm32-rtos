#define osObjectsPublic
#include "osObjects.h" 
#include "main.h"
#include "main_menu.h"

char debug_array[20] = "";

/******global variable definitions******/
int joy = IDLE;
int i = 0;
int speed = 0;
int fan = 0;
uint8_t intruder = 0;

/******imported variables and functions******/
extern int timer1;
extern int timer2;
extern void Init_Timers(void);

//main thread reads joystick, reads time from RTC module and updates the window
void main_thread(void const* argument){
	while(1){
		main_menu();
	}
}

osThreadDef(main_thread, osPriorityIdle, 1, 0);

/*****************get date time and update status thread****************/
void update_bars(void const* args){
	uint16_t text_color;
	uint16_t back_color;
	
	while(1){
		osSignalWait(1,osWaitForever);
		if(timer2){
			LCD_GetColors(&text_color,&back_color);
			timer2 = 0;
			
			time.seconds = TM_DS1307_GetSeconds();
			time.minutes = TM_DS1307_GetMinutes();
			time.hours = TM_DS1307_GetHours();
			time.day = TM_DS1307_GetDay();
			time.date = TM_DS1307_GetDate();
			time.month = TM_DS1307_GetMonth();
			time.year = TM_DS1307_GetYear();
			
			draw_date_time_bar(time);
			draw_status_bar(status);
			LCD_SetColors(text_color,back_color);
		}
		
		osSignalClear (thread_id, 1);
	}
}

osThreadDef(update_bars, osPriorityAboveNormal, 1, 0);

/*****************lights on/off automation thread******************/
void automation_thread(void const* args){
	while(1){
		osSignalWait(2,osWaitForever);
		if(room1.light1 == ON)
			GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_RESET);
		if(room1.light1 == OFF)
			GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_SET);
		if(room1.light2 == ON)
			GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
		if(room1.light2 == OFF)
			GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
		
		status.no_lights = room1.light1+room1.light2;
		status.no_fan = room1.fan;
		
		osSignalClear (thread_automation, 2);
	}	
}
osThreadDef(automation_thread, osPriorityRealtime, 1, 0);

/******************polling inputs thread******************/
void polling_thread(void const* args){
	uint8_t light1 = 0;
	uint8_t light2 = 0;
	uint8_t fan = 0;
	uint8_t buzzer = 0;

	while(1){
		k = GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_11);
		
		x = adc_read(joy_x, ADC_SampleTime_480Cycles);
		y = adc_read(joy_y, ADC_SampleTime_480Cycles);
		
		temp_value = adc_read(temp, ADC_SampleTime_480Cycles);
		ldr_value = adc_read(ldr, ADC_SampleTime_480Cycles);
		
		
		pir_value = GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_12);
		
		status.temperature = (temp_value*300/4095);
		
		if(timer == OFF){
			if(manual == MANUAL_NO){
				if(ldr_value > LDR_VAL_NIGHT){
					light1 = ON;
					light2 = ON;
					room1.light1 = light1;
					room1.light2 = light2;
					status.no_lights = light1+light2;
					osSignalSet (thread_pir,4);
					osSignalSet (thread_automation, 2);
					
				}
				if(ldr_value < LDR_VAL_MORNING){
					light1 = OFF;
					light2 = OFF;
					room1.light1 = light1;
					room1.light2 = light2;
					osSignalSet (thread_automation, 2);
					status.no_lights = light1+light2;
				}
			}
			if(manual == MANUAL_YES){
				if(ldr_value < LDR_VAL_MORNING){
					manual = MANUAL_NO;
				}
				if(room1.light1 == ON || room1.light2 == OFF || room1.fan != FAN0){
					osSignalSet(thread_pir,4);
				}
			}
		}
		else if(timer == ON){
			if(timer_main.hours == time.hours && timer_main.minutes == time.minutes){
				light1 = ON;
				light2 = ON;
				room_struct_apply(ROOM1,light1,light2,fan);
				osSignalSet (thread_automation, 2);
			}
		}
		switch(room1.fan){
			case FAN1:
				speed = 7;
				break;
			case FAN2:
				speed = 5;
				break;
			case FAN3:
				speed = 3;
				break;
			case FAN4:
				speed = 2;
				break;
		}
		if(intruder){
			if(timer1){
				timer1 = 0;
				if(buzzer%2 == 0){
					buzzer++;
					GPIO_WriteBit(GPIOA,GPIO_Pin_10,Bit_RESET);
					
				}
				else{
					buzzer++;
					GPIO_WriteBit(GPIOA,GPIO_Pin_10,Bit_SET);
				}
			}
			if(buzzer == 100){
				buzzer = 0;
				intruder = 0;
				timer1 = 0;
			}
		}
	}
}
osThreadDef(polling_thread, osPriorityIdle, 1, 0);

/****************fan dimmer thread*********************/
void fan_thread(void const* args){
	
	while(1){
		//wait for signal
		osSignalWait(3,osWaitForever);

		if(room1.fan != FAN0){
			osDelay(speed);
			GPIO_WriteBit(GPIOA,GPIO_Pin_8, Bit_SET);
			//small delay of 10uS
			for(i = 0; i < 400 ; i++);
			GPIO_WriteBit(GPIOA,GPIO_Pin_8, Bit_RESET);
		}
		osSignalClear(thread_fan,3);
	}
}
osThreadDef(fan_thread, osPriorityRealtime, 1, 0);

/***************pir check thread************************/
void pir_thread(void const* args){
	int i;
	int detections = 0;
	while(1){
		osSignalWait(4,osWaitForever);
		debug_line("pir thread");
		for(i=0 ; i < 10 ; i++){
			if(pir_value)
				detections++;
			osDelay(1000);
		}
		if(detections < 3){
			debug_line("det < 3");
			detections = 0;
			for(i=0 ; i<20 ; i++){
				if(pir_value)
					detections++;
				osDelay(1000);
			}
			if(detections < 10){
				debug_line("det < 10");
				room1.light1 = OFF;
				room1.light2 = OFF;
				room1.fan = FAN0;
				manual = MANUAL_YES;
				
				status.no_fan = room1.fan;
				strcpy(status.message,"no motion");
				osSignalSet(thread_automation,2);
			}
		}
		else{
			debug_line("det motion on");
		}
		osSignalClear(thread_pir,4);
	}
}
osThreadDef(pir_thread, osPriorityIdle, 1, 0);

int main (void) {
	// initialize CMSIS-RTOS
	osKernelInitialize ();                    

  // initialize peripherals here
	init_LCD();
	splash_screen();
	adc_init(ldr);
	adc_init(temp);
	init_joystick();
	init_RTC();
	init_lights();
	Init_Timers();
	init_back();
	init_room_struct();
	init_USART6();
	init_fan();
	init_buzzer();
	init_pir();
	LCD_Clear(def_theme.background);
	
  // create 'thread' functions that start executing,
	osThreadCreate (osThread(main_thread),NULL);
	thread_pir = 				osThreadCreate (osThread(pir_thread),NULL);
	thread_id = 				osThreadCreate (osThread(update_bars),NULL);
	thread_automation = osThreadCreate (osThread(automation_thread),NULL);
	osThreadCreate (osThread(polling_thread),NULL);
	thread_fan = 				osThreadCreate (osThread(fan_thread),NULL);
	
	
	osSignalSet (thread_id, 1);
	
	// start thread execution 
	osKernelStart ();       
}

void EXTI0_IRQHandler(void) {
    // Make sure the interrupt flag is set for EXTI0
    if(EXTI_GetITStatus(EXTI_Line0) != RESET){
        if(!back){
					back = 1;
				}
        // Clear the interrupt flag
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
void EXTI15_10_IRQHandler(void) { 
		/* Make sure that interrupt flag is set */
    if (EXTI_GetITStatus(EXTI_Line15) != RESET) {
      /* Do your stuff when PB15 is changed */
			if(room1.fan == 5)
				GPIO_WriteBit(GPIOA,GPIO_Pin_8, Bit_SET);
			else if(room1.fan == 0)
				GPIO_WriteBit(GPIOA,GPIO_Pin_8, Bit_RESET);
			else{
				GPIO_WriteBit(GPIOA,GPIO_Pin_8, Bit_RESET);
				osSignalSet(thread_fan,3);
			}
			EXTI_ClearITPendingBit(EXTI_Line15);
    }
}

void USART6_IRQHandler(void){
	// Check the Interrupt status to ensure the Rx interrupt was triggered, not Tx
	
  if(USART_GetITStatus(USART6, USART_IT_RXNE)){
    static int cnt = 0;
		// Get the byte that was transferred
    char ch = USART6->DR;
    // Check for '.' character, or Maximum characters
    if((ch != '.') && (cnt < MAX_WORDLEN)){
      received_str[cnt++] = ch;
    }
    else{
      received_str[cnt] = '\0';
      cnt = 0;
			
			if(strcmp(received_str,"Successfully Connected") == 0){
				strcpy(status.message,"Bluetooth");
			}
			else if(strstr(received_str,"Intruder") != NULL){
				strcpy(status.message,"INTRUDER");
				USART_puts(USART6, "OK.");
				intruder = 1;
			}
			else if(strcmp(received_str,"L1") == 0){
				if(room1.light1 == OFF){
					manual = MANUAL_YES;
					room1.light1 = ON;
				}
				else if(room1.light1 == ON){
					manual = MANUAL_YES;
					room1.light1 = OFF;
				}
				osSignalSet (thread_automation, 2);
			}
			else if(strcmp(received_str,"L2") == 0){
				if(room1.light2 == OFF){
					manual = MANUAL_YES;
					room1.light2 = ON;
				}
				else if(room1.light2 == ON){
					manual = MANUAL_YES;
					room1.light2 = OFF;
				}
				osSignalSet (thread_automation, 2);
			}
			else if(strcmp(received_str,"F") == 0){
				if(fan == 0){
					fan = 1;
				}
				else{
					fan = 0;
					room1.fan = FAN0;
				}
			}
			else if(strcmp(received_str,"1") == 0){
				if(fan){
					room1.fan = FAN1;
				}
			}
			else if(strcmp(received_str,"2") == 0){
				if(fan){
					room1.fan = FAN2;
				}
			}
			else if(strcmp(received_str,"3") == 0){
				if(fan){
					room1.fan = FAN3;
				}
			}
			else if(strcmp(received_str,"4") == 0){
				if(fan){
					room1.fan = FAN4;
				}
			}
			else if(strcmp(received_str,"5") == 0){
				if(fan){
					room1.fan = FAN5;
				}
			}
			else{
				strcpy(received_str,"");
			}
		}
	}
}

