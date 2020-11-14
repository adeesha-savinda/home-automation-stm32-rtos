#include <string.h>
#include "stm32f4_discovery_lcd.h"
#include "definitions.h"
#include "tm_stm32f4_ds1307.h"

//global variables declaration
char print_array[41];

//Theme structure definition for lcd_elements.h
typedef struct
{
	unsigned short color1;
	unsigned short color1_text;
	unsigned short color2;
	unsigned short color2_text;
	unsigned short background;
}lcd_theme;

//Define the status structure
typedef struct {
	int temperature;
	uint8_t no_lights;
	uint8_t no_fan;
	char message[10];
} status_struct;


lcd_theme def_theme = {LCD_COLOR_BLUE,LCD_COLOR_WHITE,LCD_COLOR_BLUE2,LCD_COLOR_BLACK,LCD_COLOR_CYAN};
status_struct status = {0,0,0,"nothing"};

char* get_day(uint8_t day){
	if(day == 1)
		return "Monday";
	else if(day == 2)
		return "Tuesday";
	else if(day == 3)
		return "Wednesday";
	else if(day == 4)
		return "Thursday";
	else if(day == 5)
		return "Friday";
	else if(day == 6)
		return "Saturday";
	else if(day == 7)
		return "Sunday";
	return "error";
}
void debug_line(char* text){
	LCD_SetFont(&Font16x24);
	LCD_DisplayStringLine(Line8,(uint8_t*)text);
}


//Draw the date time bar
void draw_date_time_bar(TM_DS1307_Time_t time){
	LCD_SetFont(&Font12x12);
	LCD_SetColors(def_theme.color1_text,def_theme.color1);
	
	sprintf(print_array," %02d:%02d %9s %02d:%02d:%4d",time.hours,time.minutes,get_day(time.day),time.date,time.month,2000+time.year);
	LCD_DisplayStringLine(LCD_LINE_0,(uint8_t*)print_array);
	LCD_SetFont(&Font16x24);
	
}
//Draw the status bar
void draw_status_bar(status_struct status){
	char temp_light[4];
	char temp_fan[4];
	
	if(status.no_lights == 0)
		strcpy(temp_light,"off");
	else
		sprintf(temp_light,"%3d",status.no_lights);
	
	if(status.no_fan == 0)
		strcpy(temp_fan,"off");
	else
		sprintf(temp_fan,"%3d",status.no_fan);
	
	LCD_SetFont(&Font8x12);
	sprintf(print_array,"Tem:%2d'C Fan: %s bulb: %s : %10s",status.temperature,temp_fan,temp_light,status.message);
	LCD_DisplayStringLine(LCD_LINE_19,(uint8_t*)print_array);
	LCD_SetFont(&Font16x24);
}
//splash screen
void splash_screen(){
	LCD_Clear(LCD_COLOR_BLACK);
	LCD_SetColors(White,LCD_COLOR_BLACK);
	LCD_DisplayStringLine(Line4,(uint8_t*)"        HOME");
	LCD_DisplayStringLine(Line5,(uint8_t*)"     AUTOMATION");
	LCD_DrawCircle(160,120,100);
	LCD_DrawCircle(160,120,101);
}

void draw_caption(int row, int col, int mode, char* caption){
	int i;
	int lim;
	//check mode
	if(mode == MODE2X2){
		lim =8;
		LCD_SetFont(&Font16x24);
		//check rows and cols
		if(row == 1){
			row = 168;
		}
		else{
			row = 84;
		}
		if(col == 1){
			col = 176;
		}
		else{
			col = 16;
		}
	}
	if(mode == MODENX1){
		row = (row*24)+Line3;
		col = 16*3;
		lim = 14;
	}
	//lim defines the no of characters, if mode is 0, only 8 characters
	for(i=0; i<lim; i++){
		//else if tree used to get rid of edge overlapping with characters
		if(i == 0){
			col++;
		}
		else if(i == 1){
			col--;
		}
		else if(i == 7){
			col--;
		}
		LCD_DisplayChar(row,col+(i*16),caption[i]);
	}
}

void button(int row, int col, int mode, int btn, char* caption){
	
	LCD_SetFont(&Font16x24);
	
	//Button up or down will change the text and back color
	if(btn == BTN_UP)
		LCD_SetColors(def_theme.color1_text,def_theme.color2);
	else if(btn == BTN_DWN)
		LCD_SetColors(def_theme.color2_text,def_theme.background);
	
	//drawing button rectangles according to up or down and row and column and mode
	if(mode == MODE2X2){
			if(btn == BTN_UP){
				LCD_DrawFullRect(16+(col*160),60+(row*84),128,72);
				LCD_SetTextColor(def_theme.color1_text);
				LCD_DrawRect(16+(col*160),60+(row*84),128,72);
			}
			else if(btn == BTN_DWN){
				LCD_DrawFullRect(16+(col*160),60+(row*84),128,72);
				LCD_SetTextColor(def_theme.color2_text);
				LCD_DrawRect(16+(col*160),60+(row*84),128,72);
			}
		draw_caption(row,col,mode,caption);
	}
	if(mode == MODENX1){
		draw_caption(row,col,mode,caption);
	}
}

void draw_menu(int row, int col,int mode,char** menu_items){
	int i,j,lim_i,lim_j,item = 0;
	//menu_items[0] will be the title of the menu
	LCD_SetColors(def_theme.color2_text,def_theme.background);
	LCD_DisplayStringLine(Line1,(uint8_t*)menu_items[item++]);
	
	if(mode == MODE2X2){
		lim_i = 2;
		lim_j = 2;
	}
	else if(mode == MODE1X2){
		lim_i = 1;
		lim_j = 2;
	}
	//If it is a list, then the variable col will decide the no of list items
	else if(mode == MODENX1){
		lim_i = col;
		lim_j = 1;
		col = 0;
	}
	
	for(i=0; i<lim_i; i++){
		for(j=0; j<lim_j; j++){
			if(strlen(menu_items[item])){
				if(i==row && j == col){
					button(i,j,mode,BTN_UP,menu_items[item++]);
					continue;
				}
				button(i,j,mode,BTN_DWN,menu_items[item++]);
			}
		}
	}
	item = 0;
}

void change_theme(uint8_t theme){
	if(theme == THEME_DEF){
		def_theme.color1 = LCD_COLOR_BLUE;
		def_theme.color2 = LCD_COLOR_BLUE2;
		def_theme.background = LCD_COLOR_CYAN;
	}
	else if(theme == THEME1){
		def_theme.color1 = LCD_COLOR_RED;
		def_theme.color2 = LCD_COLOR_MAGENTA;
		def_theme.background = LCD_COLOR_PINK;
	}
	else if(theme == THEME2){
		def_theme.color1 = LCD_COLOR_BLACK;
		def_theme.color2 = LCD_COLOR_GREY2;
		def_theme.background = LCD_COLOR_WHITE;
	}
	else if(theme == THEME3){
		def_theme.color1 = LCD_COLOR_GREEN;
		def_theme.color2 = LCD_COLOR_CYAN;
		def_theme.background = LCD_COLOR_WHITE;
	}
	LCD_SetBackColor(def_theme.background);
	LCD_DrawFullRect(0,12,320,216);
	draw_date_time_bar(time);
	draw_status_bar(status);
}
