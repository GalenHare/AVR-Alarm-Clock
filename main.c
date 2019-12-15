#include "lcd.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/eeprom.h>

//bug with the long press
//increase pin pd3
//decrease pin pd4
//mode pin pd2
/* Variables to keep track of time*/
int mill = 0;
int sec = 0;
int min = 0;
int hr = 12;
//==============================================//

/*Variables to keep track of time being set by user*/
int tempSec=0;
int tempMin =0;
int tempHr =0;
//=================================================//

/*Variables to keep track of alarm times (both alarm 1 and 2)*/
int alarmSec=0;
int alarmMin =0;
int alarmHr =0;
int alarmSec2=0;
int alarmMin2 =0;
int alarmHr2 =0;
int alarmSet = 0;
//==========================================================//

int longPress = 0; //keeps track of how long a button has been pressed
int prevMode=0; //keeps track of previous mode
unsigned char mode =1; //keeps track of current mode
unsigned char selected=0; //keeps track of whether the user has selected a unit to increment or decrement
unsigned char selection=1; //keeps track of which unit the user has selected to increment or decrement
char str[12]; //character buffer used to print integers to the lcd
int displaySet = 0;
int counter =0;

//function declarations//
void displayMode();
void setTime();
void selectUnit();
void placeArrow();
void checkValues();
void displayLCD();
void alarmRoutine();
void toDisplay();
void EEPROM_write (unsigned int ucAddress, unsigned int ucData);
unsigned int EEPROM_read(unsigned int ucaddress);

int main(void){
	lcd_init();
	DDRB = 0xFF;//Set pin B to output
	DDRD = 0x00;//set pin D to input

	TIMSK |= (1 << TOIE0); //enabling timer overflow interrupt enable
	TCCR0B = 0x03;//prescale by 64
	TCNT0 = 131;

	MCUCR = 0b00001010;//The falling edge of INT0 generates an interrupt request
	GIMSK = 0b01000000;//External Interrupt Request 0 Enable
	sei();//enable global interrupts
	clear_lcd();
	lcd_gotoxy(1,1);
	if(EEPROM_read(16)!=255){
		alarmHr = EEPROM_read(19);
		alarmMin = EEPROM_read(20);
		alarmSec = EEPROM_read(21);
		alarmHr2 = EEPROM_read(22);
		alarmMin2 = EEPROM_read(23);
		alarmSec2 = EEP ROM_read(24);
	}
	displayMode();
	while(1){
		if(mode==1){//Normal Mode
			if(prevMode==1){
				//Previous mode was set time mode, then set all time variables to user set variables
				hr = tempHr;
				min = tempMin;
				sec = tempSec;
				tempHr = 0;
				tempMin = 0;
				tempSec = 0;
				prevMode=0;
			}else if(prevMode==2){
				//Previous mode was set alarm 1 mode then set all alarm 1 variables to user set variables
				prevMode=0;
				alarmHr = tempHr;
				alarmMin = tempMin;
				alarmSec = tempSec;
				EEPROM_write(19,alarmHr);
				EEPROM_write(20,alarmMin);
				EEPROM_write(21,alarmSec);
				tempHr = 0;
				tempMin = 0;
				tempSec = 0;
				alarmSet = 1;
			}else if(prevMode==3){
				//Previous mode was set alarm 2 mode then set all alarm 2 variables to user set variables
				prevMode=0;
				alarmHr2 = tempHr;
				alarmMin2 = tempMin;
				alarmSec2 = tempSec;
				EEPROM_write(22,alarmHr2);
				EEPROM_write(23,alarmMin2);
				EEPROM_write(24,alarmSec2);
				tempHr = 0;
				tempMin = 0;
				tempSec = 0;
				alarmSet = 1;
			}
			selected=0; //reset selected to 0. So that the user may reselect a unit
		}else if(mode==2){
			//If mode is 2 then set time
			setTime(hr,min,sec);
		}else if(mode==3){
			//If mode is 3 then set alarm 1
			setTime(alarmHr,alarmMin,alarmSec);
		}else if(mode==4){
			//If mode is 4 then set alarm 2
			setTime(alarmHr2,alarmMin2,alarmSec2);
		}
		displayMode();
		displayLCD(mode);//used to display time or tempTime based on mode
	}
}

ISR(TIMER0_OVF_vect){
	//Interrupt triggered every millisecond
	mill++;
	TCNT0 = 131;
	checkValues();//used to check time passing
}

ISR(INT0_vect)
{
	//used to switch between modes
	if(mode==1){
		mode = 2;
	}else if(mode==2){
		mode = 3;
	}else if(mode==3){
		mode=4;
	}else{
		mode=1;
	}
	displaySet=0;
	_delay_ms(100);
	while((PIND & (1<<PD2)) ==0){
		counter=60;
		_delay_ms(200);
	}
}

void displayMode(){
	if(displaySet == 0){
		lcd_gotoxy(9,2);
		lcd_print("              ");
		displaySet=1;
	}
	if(mode==1){
		lcd_gotoxy(9,2);
		_delay_ms(30);
		lcd_print("Normal");
	}else if(mode==2){
		lcd_gotoxy(9,2);
		_delay_ms(30);
		lcd_print("Set");
	}else if(mode==3){
		lcd_gotoxy(9,2);
		_delay_ms(30);
		lcd_print("Alarm 1");
	}else{
		lcd_gotoxy(9,2);
		_delay_ms(30);
		lcd_print("Alarm 2");
	}
}

void setTime(int tempHr,int tempMin, int tempSec){
	if(selected==0){
		//If user has not selected a unit then allow user to do so
		selectUnit();
	}else{
		//based on unit selected by user increment or decrement corresponding variable
		if((PIND & (1<<PD3))==0){
			if(selection== 1){
				tempHr++;
				if(tempHr>23){
					tempHr=0;
				}
			}
			if(selection== 2){
				tempMin++;
				if(tempMin>59){
					tempMin=0;
				}
			}
			if(selection==3){
				tempSec++;
				if(tempSec>59){
					tempSec=0;
				}
			}
			while((PIND & (1<<PD3))==0){
				_delay_ms(100);
				longPress++;//incremented every 100 ms
			}
			if(longPress>=10){//if value is >=10. Then 10*100ms = 1s has passed. Allow user to reselect a unit
				selected=0;
				switch(selection){
					case 1:
					tempHr--;
					if(tempHr<0){
						tempHr=23;
					}
					break;
					case 2:
					tempMin--;
					if(tempMin<0){
						tempMin=59;
					}
					break;
					case 3:
					tempSec--;
					if(tempSec<0){
						tempSec=59;
					}
					break;
				}
			}
			longPress=0;
		}
		if((PIND & (1<<PD4))==0){
			if(selection== 1){
				tempHr--;
				if(tempHr<0){
					tempHr=23;
				}
			}
			if(selection== 2){
				tempMin--;
				if(tempMin<0){
					tempMin=59;
				}
			}
			if(selection== 3){
				tempSec--;
				if(tempSec<0){
					tempSec=59;
				}
			}
			while((PIND & (1<<PD4))==0){
				_delay_ms(100);
				longPress++;
			}
			if(longPress>=10){//if value is >=10. Then 10*100ms = 1s has passed. Allow user to reselect a unit
				selected=0;
				switch(selection){
					case 1:
						tempHr++;
						if(tempHr>23){
							tempHr=0;
						}
						break;
					case 2:
						tempMin++;
						if(tempMin>59){
							tempMin=0;
						}
						break;
					case 3:
						tempSec++;
						if(tempSec>59){
							tempSec=0;
						}
						break;
				}
			}
			longPress=0;
		}
		
		//Set indicator of the mode which the user selected a unit in. Able to set correct variables in main loop
		if(selected==1){
			prevMode = 1;
		}else if(selected==2){
			prevMode = 2;
		}else if(selected==3){
			prevMode=3;
		}
	}
}

void selectUnit(){
	if((PIND & (1<<PD3)) ==0){
		if(selection==1){
			selection=2;
		}else if(selection==2){
			selection=3;
		}else{
			selection=1;
		}
		while((PIND & (1<<PD3)) ==0){
			_delay_ms(100);
		}
	}
	if((PIND & (1<<PD4)) ==0){
		//set selected variable, indicating the user has selected a unit
		if(mode==2){
			selected = 1;
		}else if(mode==3){
			selected=2;
		}else if(mode==4){
			selected=3;
		}
		while((PIND & (1<<PD4)) ==0){
			_delay_ms(100);
		}
	}
	placeArrow(selection); //used to place the arrow underneath the unit which the user is at
}

void placeArrow(int x){
	lcd_gotoxy(1,2);
	lcd_print("        ");
	if(x==1){
		lcd_gotoxy(2,2);
		}else if(x==2){
		lcd_gotoxy(5,2);
		}else{
		lcd_gotoxy(8,2);
	}
	lcd_print("^");
}

void checkValues(void){
	if(mill>=1000){
		sec++;
		mill = 0;
	}
	if(sec >= 60){
		min++;
		sec = 0;
		mill = 0;
	}
	if(min >= 60){
		hr++;
		min = 0;
		sec = 0;
		mill = 0;
	}
}

void displayLCD(int x){
	if(x==1 || x==2){
		toDisplay(hr,min,sec);
	}else if(x==3){
		toDisplay(alarmHr,alarmMin,alarmSec);
	}else if(x==4){
		toDisplay(alarmHr2,alarmMin2,alarmSec2);
	}
	if(((hr==alarmHr && sec==alarmSec && min == alarmMin)||(hr==alarmHr2 && sec==alarmSec2 && min == alarmMin2)) && alarmSet==1){
		alarmRoutine();
	}
	lcd_gotoxy(13,1);
	lcd_print("  ");
	lcd_gotoxy(3,1);
	lcd_print(":");
	lcd_gotoxy(6,1);
	lcd_print(":");
	lcd_gotoxy(9,1);
}

void alarmRoutine(){
	createSymbol();
	lcd_gotoxy(14,1);
	lcdData(0);
	// alarmHr = 0;
	// alarmSec=0;
	// alarmMin=0;
	// alarmSet=0;
	PORTB |= (1 << PB0);
	for(int i =0; i <60;i++){
		if((PIND & (1<<PD3))==0){
			break;
		}
		if((PIND & (1<<PD4))==0){
			break;
		}
		if((PIND & (1<<PD2))==0){
			break;
		}
		_delay_ms(1000);
	}
	PORTB &= ~(1<<PB0);
}

void toDisplay(int x, int y, int z){
	itoa(x,str,10);
	if(x<10){
		lcd_gotoxy(1,1);
		lcd_print("0");
		lcd_gotoxy(2,1);
		lcd_print(str);
		}else{
		lcd_gotoxy(1,1);
		lcd_print(str);
	}
	itoa(y,str,10);
	if(y<10){
		lcd_gotoxy(4,1);
		lcd_print("0");
		lcd_gotoxy(5,1);
		lcd_print(str);
		}else{
		lcd_gotoxy(4,1);
		lcd_print(str);
	}
	itoa(z,str,10);
	if(z<10){
		lcd_gotoxy(7,1);
		lcd_print("0");
		lcd_gotoxy(8,1);
		lcd_print(str);
		}else{
		lcd_gotoxy(7,1);
		lcd_print(str);
	}
}

void EEPROM_write(unsigned int ucAddress, unsigned int ucData)
{
	while (EECR & (1<<EEPE)); //wait for completion of previous write
	EECR = (0 << EEPM1) | (0<<EEPM0); //set programming mode
	EEAR = ucAddress;	//set up address
	EEDR = ucData;	//set up data registers
	EECR |= (1<<EEMPE);
	EECR |= (1 << EEPE); //start EEPROM
}

unsigned int EEPROM_read(unsigned int ucaddress)
{
	while(EECR & (1 << EEPE));	//wait for completion of previous write
	EEAR = ucaddress; //set up address register
	EECR |= (1 << EERE); // start eeprom
	return EEDR;
}

void createSymbol(){
	lcdCommand(0x40);
	lcdData(0x04);
	lcdData(0x0E);
	lcdData(0x0E);
	lcdData(0x0E);
	lcdData(0x1F);
	lcdData(0x00);
	lcdData(0x04);
	lcdData(0x00);
}
