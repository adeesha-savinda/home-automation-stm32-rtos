//Std Periph Driver includes
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "misc.h"
//SHU library includes
#include "adc.h"
#include "time.h"
//BB support includes
#include "stm32f4_discovery_lcd.h"
//TM library includes
#include "tm_stm32f4_i2c.h"
#include "tm_stm32f4_ds1307.h"
//Own includes
#include "definitions.h"
//#include "lcd_elements.h"

#define MAX_WORDLEN 25

//definitions of state variables
uint8_t manual = MANUAL_NO;
uint8_t timer = OFF;

//variables of inputs
int ldr_value = 0;
int temp_value = 0;
uint8_t pir_value = 0;
int x,y,k;

char received_str[MAX_WORDLEN + 1];

const GPIO_pin_t joy_x = {"PA3", GPIOA, GPIO_Pin_3, GPIO_PinSource3};
const GPIO_pin_t joy_y = {"PA5", GPIOA, GPIO_Pin_5, GPIO_PinSource5};
const GPIO_pin_t ldr = {"PC3", GPIOC, GPIO_Pin_3, GPIO_PinSource3};
const GPIO_pin_t temp = {"PC2", GPIOC, GPIO_Pin_2, GPIO_PinSource2};


//define the structure of room controls
typedef struct {
	uint8_t light1;
	uint8_t light2;
	uint8_t fan;
} room_struct;

room_struct room1;

typedef struct {
	uint8_t hours;
	uint8_t minutes;
} timer_struct;

timer_struct timer_main;

void init_LCD(){
	STM32f4_Discovery_LCD_Init();
}

void init_joystick(){
	GPIO_InitTypeDef gpio_init_struct;
	//ADC initialize to get joystick values  and y
	adc_init(joy_x);
	adc_init(joy_y);
	//GPIO initialize to get joystick key
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	GPIO_StructInit(&gpio_init_struct);
	gpio_init_struct.GPIO_Mode = GPIO_Mode_IN;
	gpio_init_struct.GPIO_Pin = GPIO_Pin_11;
	gpio_init_struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOD,&gpio_init_struct);
	
	//dedicated back button as PA0
	GPIO_StructInit(&gpio_init_struct);
	gpio_init_struct.GPIO_Mode = GPIO_Mode_IN;
	gpio_init_struct.GPIO_Pin = GPIO_Pin_0;
	gpio_init_struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOA,&gpio_init_struct);
}

int read_joy(){
	if(y > 3900)
		return DOWN; 
	if(y < 500)
		return UP;
	if(x < 500)
		return LEFT; 
	if(x > 3900)
		return RIGHT;
	if(k == 0)
		return PRESS;
	if((x < 3000 && x > 2000) || (x < 3000 && x > 2000))
		return IDLE;
}
/////////////////////////////////////////////////////////////////////
//Init Dedicated back button
void init_back(){
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
 /* Configure PA0 pin as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;		// PA0 is connected to high, so use pulldown resistor
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Connect EXTI Line0 to PA0 pin (i.e. EXTI0CR[0])*/
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
	// SYSCFG->EXTICR[0] &= SYSCFG_EXTICR1_EXTI0_PA;		// Same as above, but with direct register access
	
	/* Configure EXTI Line0 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;              // PA0 is connected to EXTI0
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;     // Interrupt mode
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; 	// Trigger on Rising edge (Just as user presses btn)
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;               // Enable the interrupt
	EXTI_Init(&EXTI_InitStructure);                         // Initialize EXTI
	
	/* Enable and set priorities for the EXTI0 in NVIC */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;                // Function name for EXTI_Line0 interrupt handler
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;    // Set priority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;           // Set sub priority
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                 // Enable the interrupt
	NVIC_Init(&NVIC_InitStructure);                                 // Add to NVIC
}
/////////////////////////////////////////////////////////////////////
// Initialize RTC
TM_DS1307_Time_t time; //time structure global variable

void init_RTC(){
	if (TM_DS1307_Init() != TM_DS1307_Result_Ok) {
    LCD_DisplayStringLine(Line7,(uint8_t*)"RTC init FAIL");
		while (1);
  }
	time.hours = 5;
	time.minutes = 38;
	time.seconds = 0;
	time.date = 9;
	time.day = 5;
	time.month = 10;
	time.year = 15;
	
	TM_DS1307_SetSeconds(time.seconds);
	TM_DS1307_SetMinutes(time.minutes);
	TM_DS1307_SetHours(time.hours);
	TM_DS1307_SetDay(time.day);
	TM_DS1307_SetDate(time.date);
	TM_DS1307_SetMonth(time.month);
	TM_DS1307_SetYear(time.year);
}

/////////////////////////////////////////////////////////////////////
//Initialize Light on off
void init_lights(){
	GPIO_InitTypeDef gpio_init_struct;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	GPIO_StructInit(&gpio_init_struct);
	gpio_init_struct.GPIO_Mode = GPIO_Mode_OUT;
	gpio_init_struct.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_14;
	gpio_init_struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOB,&gpio_init_struct);
	GPIO_WriteBit(GPIOB, GPIO_Pin_10|GPIO_Pin_14, Bit_SET);
}

void init_room_struct(){
	room1.light1 = OFF;
	room1.light2 = OFF;
	room1.fan = FAN0;
}

void room_struct_apply(uint8_t room, uint8_t light1, uint8_t light2, uint8_t fan){
	if(room == ROOM1){
		room1.light1 = light1;
		room1.light2 = light2;
		room1.fan = fan;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//Init USART
void init_USART6(){
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//Enable the periph clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
	//enable GPIOA clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	//pc6 - tx
	//pc7 - rx
	//Setup the GPIO pins for Tx and Rx
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// Map USART6 to A2 and A3
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

	// Initialize USART
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	/* Configure USART */
	USART_Init(USART6, &USART_InitStructure);

	//NVIC setup
	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitStructure);
	
	//Enable USART
	USART_Cmd(USART6, ENABLE);
}

volatile char* USART_RX_OUT(USART_TypeDef* USARTx, volatile char *str){
	return str;
}

void USART_puts(USART_TypeDef* USARTx, volatile char *str){
	while(*str){
		//while(!(USARTx->SR & 0x040));
		while(USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET);
		USART_SendData(USARTx, *str);
		*str++;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////
//Init fan
void init_fan(){
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
 /* Configure PA0 pin as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;		// PA0 is connected to high, so use pulldown resistor
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Connect EXTI Line0 to PA0 pin (i.e. EXTI0CR[0])*/
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource15);
	// SYSCFG->EXTICR[0] &= SYSCFG_EXTICR1_EXTI0_PA;		// Same as above, but with direct register access
	
	/* Configure EXTI Line0 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line15;              // PA0 is connected to EXTI0
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;     // Interrupt mode
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 	// Trigger on Rising edge (Just as user presses btn)
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;               // Enable the interrupt
	EXTI_Init(&EXTI_InitStructure);                         // Initialize EXTI
	
	/* Enable and set priorities for the EXTI0 in NVIC */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;                // Function name for EXTI_Line0 interrupt handler
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;    // Set priority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;           // Set sub priority
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                 // Enable the interrupt
	NVIC_Init(&NVIC_InitStructure);                                 // Add to NVIC
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOA,&GPIO_InitStructure);
}

void init_buzzer(){
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_WriteBit(GPIOA,GPIO_Pin_10, Bit_SET);
}

void init_pir(){
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOC,&GPIO_InitStructure);
}
