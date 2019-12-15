/*
 * lcd.c
 *
 * Created: 11/26/2014 11:27:11 AM
 *  Author: Lindon
 */ 
#include "lcd.h"

//-----------------------------------------------------------------------------------------//
void lcdCommand(unsigned char cmnd)
{
	_delay_us(100);
	
	LCD_PRT = (LCD_PRT & 0x0F) | (cmnd & 0xF0);		//Send high nibble d4-d7 to data port
	LCD_PRT &= ~(1<<LCD_RS);		//RS=0 for command
	LCD_PRT &= ~(1<<LCD_RW);		//RW=0 for write
	LCD_PRT |= (1<<LCD_EN);			//EN=1 for H=to-L pulse
	_delay_us(1);					//Wait to make enable wide
	LCD_PRT &= ~(1<<LCD_EN);		//EN = 0 for H-to-L pulse
	
	_delay_us(20);					//Wait to make enale wide

	LCD_PRT = (LCD_PRT & 0x0F) | (cmnd << 4);		//Send low nibble d4-d7 to data port
	LCD_PRT |= (1<<LCD_EN);			//EN=1 for H=to-L pulse
	_delay_us(1);					//Wait to make enable wide
	LCD_PRT &= ~(1<<LCD_EN);		//EN = 0 for H-to-L pulse
}
//*************************************************************************************
void lcdData(unsigned char data)
{
	_delay_us(100);  //*****NEW****
	
	LCD_PRT = (LCD_PRT & 0x0F) | (data & 0xF0);		//Send high nibble d4-d7 to data port
	LCD_PRT |= (1<<LCD_RS);			//RS=1 for data
	LCD_PRT &= ~(1<<LCD_RW);		//RW=0 for write
	LCD_PRT |= (1<<LCD_EN);			//EN=1 for H=to-L pulse
	_delay_us(1);					//Wait to make enable wide
	LCD_PRT &= ~(1<<LCD_EN);		//EN = 0 for H-to-L pulse
	
	_delay_us(20);					//Wait to make enale wide

	LCD_PRT = (LCD_PRT & 0x0F) | (data << 4);		//Send LOW nibble d4-d7 to data port
	LCD_PRT |= (1<<LCD_EN);			//EN=1 for H=to-L pulse
	_delay_us(1);					//Wait to make enable wide
	LCD_PRT &= ~(1<<LCD_EN);		//EN = 0 for H-to-L pulse

}
//*************************************************************************************
void lcd_init()
{
	LCD_DDR = 0xFE;					//LCD Port is output

	LCD_PRT &= ~(1<<LCD_EN);		//LCD_EN = 0
	_delay_us(2000);					//Wait for stable power.
	lcdCommand(0x33);				//$33 for 4-bit mode
	_delay_us(100);
	lcdCommand(0x32);				//$32 for 4-bit mode
	_delay_us(100);
	lcdCommand(0x28);				//$28 for 4-bit mode
	_delay_us(100);					//wait
	lcdCommand(0x0e);				//Display on, cursor on
	_delay_us(100);
	lcdCommand(0x01);				//Clear LCD
	_delay_us(2000);
	lcdCommand(0x06);				//Shift cursor right
	_delay_us(100);					//wait
}
//*************************************************************************************
void lcd_gotoxy(unsigned char x, unsigned char y)
{
	unsigned char firstCharAdr[] = {0x80, 0xC0, 0x94, 0xD4};	//Table 12-5

	lcdCommand(firstCharAdr[y-1] + x-1);
	_delay_us(100);
}
//*************************************************************************************
void lcd_print(char *str)
{
	unsigned char i=0;

	while(str[i]!=0)
	{
		lcdData(str[i]);
		i++;
	}
	lcdCommand(0x0C); //Remove cursor.
}
//*************************************************************************************
void clear_lcd()
{
	lcdCommand(0x01);				//Clear LCD
	_delay_us(2000);					//wait
}
//*************************************************************************************
//Copies string from flash memory to LCD at x y position
//const uint8_t welcomeln1[] PROGMEM="AVR LCD DEMO\0";
//CopyStringtoLCD(welcomeln1, 3, 1);	
void CopyStringtoLCD(const char* FlashLoc)
{
	uint8_t i;
	
	for(i=0;(uint8_t)pgm_read_byte(&FlashLoc[i]);i++)
	{
		lcdData((uint8_t)pgm_read_byte(&FlashLoc[i]));
	}
}