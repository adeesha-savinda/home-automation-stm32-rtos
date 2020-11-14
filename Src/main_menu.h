#include <string.h>
#include "lcd_elements.h"
#include "definitions.h"

//ids of threads created in main.c
osThreadId thread_id;
osThreadId thread_automation;
osThreadId thread_fan;
osThreadId thread_pir;

int back = 0;
//function prototypes
void check_joy(int joy,int* joy_row,int* joy_col,int* key,int* lock,int* change,int mode);
int lighting_menu(void);
int timer_menu(void);
int settings_menu(void);
int settings_theme_submenu(void);
int lighting_room_submenu(int room);

//main menu
void main_menu(){
	int joy = IDLE;
	int joy_row = 0;
	int joy_col = 0;
	int lock = 0;
	int change = 1;
	int key = 0;
	int menu_sel = 0;
	int skip_joy = 0;
	char* menu_items[5] = {"     Main  Menu     ","Security","Controls","SetTimer","Settings"};
	
	do{
		if(!skip_joy){
			joy = read_joy();
			check_joy(joy,&joy_row,&joy_col,&key,&lock,&change,MODE2X2);
		}
		else{
			LCD_SetBackColor(def_theme.background);
			LCD_DrawFullRect(0,12,320,216);
		}
		if(key && !lock){
			change = 1;
			lock = 1;
			key = 0;
			if(joy_row == 0 && joy_col == 0){
				menu_sel = 1;
			}
			else if(joy_row == 0 && joy_col == 1){
				menu_sel = 2;
			}
			else if(joy_row == 1 && joy_col == 0){
				menu_sel = 3;
			}
			else if(joy_row == 1 && joy_col == 1){
				menu_sel = 4;
			}
		}
		switch(menu_sel){
			case 0:
				if(change && !key){
					skip_joy = 0;
					change = 0;
					back = 0;
					draw_menu(joy_row,joy_col,MODE2X2,menu_items);
				}
				break;
			case 1:
				if(change && !key){
					change = 1;
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
			case 2:
				if(change && !key){
					change = 0;
					change = lighting_menu();
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
			case 3:
				if(change && !key){
					change = 0;
					change = timer_menu();
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
			case 4:
				if(change && !key){
					change = 0;
					change = settings_menu();
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
		}
		if(!menu_sel && back){
			back = 0;
		}
	}while(1);
}
int lighting_menu(){
	
	int menu_sel = 0;
	int joy = PRESS;
	int joy_row = 0;
	int joy_col = 0;
	int lock = 1;
	int change = 1;
	int key = 0;
	uint8_t skip_joy = 0;
	char* menu_items[5] = {"Lighting:Choose Room"," Room 1 "," Room 2 "," Room 3 ",""};

	LCD_Clear(def_theme.background);
	osSignalSet(thread_id,1);
	draw_menu(joy_row,joy_col,MODE2X2,menu_items);
	

	do{
		if(!skip_joy){
			joy = read_joy();
			check_joy(joy,&joy_row,&joy_col,&key,&lock,&change,MODE2X2);
		}
		if(joy_row && joy_col){
			joy_row = 1;
			joy_col = 0;
			if(joy == DOWN)
				change = 1;
			else
				change = 0;
		}
		//update joy_row and joy_col according to input of joystick
		if(key && !lock){
			change = 1;
			lock = 1;
			if(joy_row == 0 && joy_col == 0){
				key = 0;
				menu_sel = 1;
			}
			else if(joy_row == 0 && joy_col == 1){
				key = 0;
				menu_sel = 2;
			}
			else if(joy_row == 1 && joy_col == 0){
				key = 0;
				menu_sel = 3;
			}
			else if(joy_row == 1 && joy_col == 1){
				key = 0;
				menu_sel = 4;
			}
		}
		switch(menu_sel){
			case 0:
				if(change && !key){
					skip_joy = 0;
					change = 0;
					back = 0;
					draw_menu(joy_row,joy_col,MODE2X2,menu_items);
				}
				break;
			case 1:
				if(change && !key){
					change = 0;
					change = lighting_room_submenu(ROOM1);
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
			default:
				if(change && !key){
					change = 0;
					back = 1;
				}
				break;
		}
	}while(!back);
	
	return 1;
}

int timer_menu(void){
	
	int joy = PRESS;
	int joy_row = 0;
	int joy_col = 0;
	int lock = 0;
	int change = 1;
	uint8_t set = 0;
	
	int minutes = 0;
	int hours = 0;
	
	char minutes_str[20];
	char hours_str[20];

	LCD_Clear(def_theme.background);
	LCD_SetColors(def_theme.color2_text,def_theme.background);
	LCD_DisplayStringLine(Line1,(uint8_t*)"     Set  Timer");
	osSignalSet(thread_id,1);

	do{
		joy = read_joy();

		if(joy == LEFT && !lock){
			lock = 1;	
			if(--joy_col < 0)
				joy_col = 0;
			else
				change = 1;	
		}
		else if(joy == RIGHT && !lock){
			lock = 1;
			if(++joy_col > 1)
				joy_col = 1;
			else
				change = 1;
		}
		else if(joy == DOWN && !lock){
			lock = 1;
			if(joy_col == 0){
				if(--hours < 0)
					hours = 0;
				else
					change = 1;
			}
			else if(joy_col == 1){
				if(--minutes < 0)
					minutes = 0;
				else
					change = 1;
			}
		}
		else if(joy == UP && !lock){
			lock = 1;
			if(joy_col == 0){
				if(++hours > 23)
					hours = 23;
				else
					change = 1;
			}
			else if(joy_col == 1){
				if(++minutes > 59)
					minutes = 59;
				else
					change = 1;
			}
		}
		else if(joy == IDLE && lock){
			lock = 0;
			change = 0;
		}
		else if(joy == PRESS){
			set = 1;
			osDelay(5);
		}
		
		switch(joy_col){
			case 0:
				if(change){
					change = 0;
					
					LCD_SetTextColor(def_theme.color2_text);
					LCD_SetBackColor(def_theme.background);
					LCD_DisplayChar(Line4,16*9,':');
					sprintf(minutes_str,"%02d",minutes);
					LCD_DisplayChar(Line4,16*10,minutes_str[0]);
					LCD_DisplayChar(Line4,16*11,minutes_str[1]);
					
					LCD_SetTextColor(def_theme.color1_text);
					LCD_SetBackColor(def_theme.color2);
					sprintf(hours_str,"%02d",hours);
					LCD_DisplayChar(Line4,16*7,hours_str[0]);
					LCD_DisplayChar(Line4,16*8,hours_str[1]);
				}
				break;
			case 1:
				if(change){
					
					change = 0;
					
					LCD_SetTextColor(def_theme.color1_text);
					LCD_SetBackColor(def_theme.color2);
					sprintf(minutes_str,"%02d",minutes);
					LCD_DisplayChar(Line4,16*10,minutes_str[0]);
					LCD_DisplayChar(Line4,16*11,minutes_str[1]);
					
					LCD_SetTextColor(def_theme.color2_text);
					LCD_SetBackColor(def_theme.background);
					LCD_DisplayChar(Line4,16*9,':');
					sprintf(hours_str,"%02d",hours);
					LCD_DisplayChar(Line4,16*7,hours_str[0]);
					LCD_DisplayChar(Line4,16*8,hours_str[1]);
				}
				break;
		}	
	}while(!back && !set);
	
	if(!back){
		timer_main.minutes = minutes;
		timer_main.hours = hours;
		timer = ON;
	}
	return 1;
}

int settings_menu(void){
	int menu_sel = 0;
	int joy = PRESS;
	int joy_row = 0;
	int joy_col = 1;
	int lock = 1;
	int change = 0;
	int key = 0;
	uint8_t skip_joy = 0;
	
	char* menu_items[3] = {"      Settings","Set Date& Time","   Set Theme  "};

	LCD_Clear(def_theme.background);
	osSignalSet(thread_id,1);
	draw_menu(joy_row,2,MODENX1,menu_items);

	do{
		if(!skip_joy){
			joy = read_joy();
			check_joy(joy,&joy_row,&joy_col,&key,&lock,&change,MODENX1);
		}
		joy_col = 1;
		
		//update joy_row and joy_col according to input of joystick
		if(key && !lock){
			change = 1;
			lock = 1;
			if(joy_row == 0){
				key = 0;
				menu_sel = 1;
			}
			else if(joy_row == 1){
				key = 0;
				menu_sel = 2;
			}
		}
		switch(menu_sel){
			case 0:
				if(change && !key){
					skip_joy = 0;
					change = 0;
					back = 0;
					draw_menu(joy_row,2,MODENX1,menu_items);
				}
				break;
			case 1:
				if(change && !key){
					change = 0;
					back = 1;
				}
				break;
			case 2:
				if(change && !key){
					change = 0;
					change = settings_theme_submenu();
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
		}
	}while(!back);
	return 1;
}

int settings_theme_submenu(){
	int menu_sel = 0;
	int joy = PRESS;
	int joy_row = 0;
	int joy_col = 0;
	int lock = 1;
	int change = 1;
	int key = 0;
	uint8_t skip_joy = 0;
	//set is used to set the theme
	uint8_t set = 0;
	lcd_theme temp = def_theme;
	
	char* menu_items[5] = {"Lighting:Choose Room","Default ","Custom 1","Custom 2","Custom 3"};

	change_theme(THEME_DEF);
	LCD_Clear(def_theme.background);
	osSignalSet(thread_id,1);
	draw_menu(joy_row,joy_col,MODE2X2,menu_items);
	

	do{
		if(!skip_joy){
			joy = read_joy();
			check_joy(joy,&joy_row,&joy_col,&key,&lock,&change,MODE2X2);
		}
		switch(joy_row){
			case 0:
				if(change && !joy_col){	
					change_theme(THEME_DEF);
				}
				if(change && joy_col){
					change_theme(THEME1);
				}
				break;
			case 1:
				if(change && !joy_col){
					change_theme(THEME2);
				}
				if(change && joy_col){
					change_theme(THEME3);
				}
				break;
			case 3:
				
				break;
		}
		//update joy_row and joy_col according to input of joystick
		if(key && !lock){
			change = 1;
			lock = 1;
			if(joy_row == 0 && joy_col == 0){
				key = 0;
				menu_sel = 1;
			}
			else if(joy_row == 0 && joy_col == 1){
				key = 0;
				menu_sel = 2;
			}
			else if(joy_row == 1 && joy_col == 0){
				key = 0;
				menu_sel = 3;
			}
			else if(joy_row == 1 && joy_col == 1){
				key = 0;
				menu_sel = 4;
			}
		}
		switch(menu_sel){
			case 0:
				if(change && !key){
					skip_joy = 0;
					change = 0;
					back = 0;
					draw_menu(joy_row,joy_col,MODE2X2,menu_items);
				}
				break;
			case 1:
				if(change && !key){
					change_theme(THEME_DEF);
					set = 1;
				}
				break;
			case 2:
				if(change && !key){
					change_theme(THEME1);
					set = 1;
				}
				break;
			case 3:
				if(change && !key){
					change_theme(THEME2);
					set = 1;
				}
				break;
			case 4:
				if(change && !key){
					change_theme(THEME3);
					set = 1;
				}
				break;
		}
		
	}while(!set && !back);
	if(back){
		def_theme = temp;
	}
	return 1;
}

int lighting_room_submenu(int room){
	int menu_sel = 0;
	int joy = PRESS;
	int joy_row = 0;
	int joy_col = 2;
	int lock = 1;
	int change = 0;
	int key = 0;
	uint8_t skip_joy = 0;
	char* menu_items[4] = {"       Room 1" , "Light 1  : off" , "Light 2  : off" , "Fan Speed: off"};
	
	uint8_t light1;
	uint8_t light2;
	uint8_t fan;
	char print_value[15];
	
	if(room == ROOM1){
		light1 = room1.light1;
		light2 = room1.light2;
		fan = room1.fan;
	}
	if(light1 == ON)
		menu_items[1] = "Light 1  :  on";
	if(light2 == ON)
		menu_items[2] = "Light 2  :  on";
	if(fan != FAN0){
		sprintf(print_value,"Fan Speed:   %d",fan);
		menu_items[3] = print_value;
	}

	LCD_Clear(def_theme.background);
	osSignalSet(thread_id,1);
	draw_menu(joy_row,3,MODENX1,menu_items);

	do{
		if(room1.light1 == ON)
			menu_items[1] = "Light 1  :  on";
		if(room1.light2 == ON)
			menu_items[2] = "Light 2  :  on";
		if(room1.fan != FAN0){
			sprintf(print_value,"Fan Speed:   %d",room1.fan);
			menu_items[3] = print_value;
		}
		if(!skip_joy){
			joy = read_joy();
			check_joy(joy,&joy_row,&joy_col,&key,&lock,&change,MODENX1);
		}
		
		joy_col = 2;
		//update joy_row and joy_col according to input of joystick
		if(key && !lock){
			change = 1;
			lock = 1;
			if(joy_row == 0){
				key = 0;
				menu_sel = 1;
			}
			else if(joy_row == 1){
				key = 0;
				menu_sel = 2;
			}
			else if(joy_row == 2){
				key = 0;
				menu_sel = 3;
			}
		}
		switch(menu_sel){
			case 0:
				if(change && !key){
					skip_joy = 0;
					change = 0;
					back = 0;
					draw_menu(joy_row,3,MODENX1,menu_items);
				}
				break;
			case 1:
				if(change && !key){
					change =1;
					if(room1.light1 == OFF){
						light1 = ON;
						menu_items[1] = "Light 1  :  on";
					}
					else if(room1.light1 == ON){
						light1 = OFF;
						menu_items[1] = "Light 1  : off";
					}
					room1.light1 = light1;
					manual = MANUAL_YES;
					osSignalSet (thread_automation, 2);
					
					//initialize menu routine
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
			case 2:
				if(change && !key){
					change = 1;
					if(room1.light2 == OFF){
						light2 = ON;
						menu_items[2] = "Light 2  :  on";
					}
					else if(room1.light2 == ON){
						light2 = OFF;
						menu_items[2] = "Light 2  : off";
					}
					room1.light2 = light2;
					manual = MANUAL_YES;
					osSignalSet(thread_automation, 2);
					
					//initialize menu routine
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
			case 3:
				if(change && !key){
					change = 1;
					if(++room1.fan > FAN5){
						room1.fan = FAN0;
					}
					if(room1.fan == FAN0){
						menu_items[3] = "Fan Speed: off";
					}
					else{
						sprintf(menu_items[3],"Fan Speed: %3d",room1.fan);
					}
					status.no_fan = room1.fan;
					//initialize menu routine
					menu_sel = 0;
					lock = 1;
					skip_joy = 1;
				}
				break;
		}
	}while(!back);
	return 1;
}


//fuction used to check and return joystick position and condition
void check_joy(int joy,int* joy_row,int* joy_col,int* key,int* lock,int* change,int mode){
	int row_lim;
	int col_lim;
	if(mode == MODE2X2){
		row_lim = 1;
		col_lim = 1;
	}
	if(mode == MODENX1){
		row_lim = *joy_col;
		col_lim = 0;
	}
	//update joy_row and joy_col according to input of joystick
	if(joy == LEFT && !*lock){
		*lock = 1;	
		if(--*joy_col < 0)
			*joy_col = 0;
		else
			*change = 1;	
	}
	else if(joy == RIGHT && !*lock){
		*lock = 1;
		if(++*joy_col > col_lim)
			*joy_col = col_lim;
		else
			*change = 1;
	}
	else if(joy == UP && !*lock){
		*lock = 1;
		if(--*joy_row < 0)
			*joy_row = 0;
		else
			*change = 1;
	}
	else if(joy == DOWN && !*lock){
		*lock = 1;
		if(++*joy_row > row_lim)
			*joy_row = row_lim;
		else
			*change = 1;
	}
	else if(joy == IDLE && *lock){
		*lock = 0;
		*change = 0;
	}
	else if(joy == PRESS){
			*key = 1;
			*lock = 1;
			osDelay(5);
	}
}

