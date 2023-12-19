#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include "stdio.h"
#include "i2c.h"
#include "klaw.h"
#include "leds.h"
#include "tsi.h"


//#include <string.h>
//#include <stdlib.h>

#define	ZYXDR_Mask	1<<3	// Maska bitu ZYXDR w rejestrze STATUS
static uint8_t arrayXYZ[6] = {0,0,0,0,0,0};
static uint8_t sens;
static uint8_t accelStatus;

volatile uint8_t C1_press=0;
volatile uint8_t C2_press=0;	// "1" - klawisz zostal wcisniety "0" - klawisz "skonsumowany"
volatile uint8_t C3_press=0;
volatile uint8_t C4_press=0;

void PORTA_IRQHandler(void) {	// Podprogram obslugi przerwania od klawiszy C2, C3 i C4
	uint32_t buf;
	buf=PORTA->ISFR & (C1_MASK | C2_MASK | C3_MASK | C4_MASK);

	switch(buf) {
		case C1_MASK:	DELAY(10)
									if(!(PTA->PDIR&C1_MASK))		// Minimalizacja drgan zestyków
									{
										if(!(PTA->PDIR&C1_MASK))	// Minimalizacja drgan zestyków (c.d.)
										{
											if(!C1_press)
											{
												C1_press=1;
												readKeyboard(0);
											}
										}
									}
									break;
		case C2_MASK:	DELAY(10)
									if(!(PTA->PDIR&C2_MASK))		// Minimalizacja drgan zestyków
									{
										if(!(PTA->PDIR&C2_MASK))	// Minimalizacja drgan zestyków (c.d.)
										{
											if(!C2_press)
											{
												C2_press=1;
												readKeyboard(1);
											}
										}
									}
									break;
		case C3_MASK:	DELAY(10)
									if(!(PTA->PDIR&C3_MASK))		// Minimalizacja drgan zestyków
									{
										if(!(PTA->PDIR&C3_MASK))	// Minimalizacja drgan zestyków (c.d.)
										{
											if(!C3_press)
											{
												C3_press=1;
												readKeyboard(2);
											}
										}
									}
									break;
		case C4_MASK:	DELAY(10)
									if(!(PTA->PDIR&C4_MASK))		// Minimalizacja drgan zestyków
									{
										if(!(PTA->PDIR&C4_MASK))	// Minimalizacja drgan zestyków (c.d.)
										{
											if(!C4_press)
											{
												C4_press=1;
												readKeyboard(3);
											}
										}
									}
									break;
		default:			
			LCD1602_ClearAll();
			LCD1602_Print("error1");
			break;
	}	
	PORTA->ISFR |=  C1_MASK | C2_MASK | C3_MASK | C4_MASK;	// Kasowanie wszystkich bitów ISF
	NVIC_ClearPendingIRQ(PORTA_IRQn);
}


uint8_t refreshScreen = 1;
char screenUp[17];
char screenDown[17];
char passwordBuffer[4] = {' ', ' ', ' ', ' '};
int16_t passwordIndex = 0;

uint8_t passwordCompare(char p1[4], char p2[4]) {
	for(int i=0; i<4; ++i) {
		if(p1[i] != p2[i])
			return 0;
	}
	return 1;
}

uint8_t passwordWrite(char pass[4]) {
	switch(key) {
		case '#':	//	enter
			refreshScreen = 1;
			if(passwordIndex == 4) {
				if(passwordCompare(passwordBuffer, pass))
					return 1;
				else {
					sprintf(screenDown, "%s %s", passwordBuffer, "zle haslo");
					return 0;
				}
			}
			else {
				sprintf(screenDown, "%s %s", passwordBuffer, "za krotkie");
			}
			break;
			
		case '*':	//	backspace
			refreshScreen = 1;
			if(passwordIndex > 0) {
				--passwordIndex;
				passwordBuffer[passwordIndex] = ' ';
				//--passwordIndex;
			}
			sprintf(screenDown, "%s %s", passwordBuffer, "");
			break;
			
		case 'X':	//	ERROR
			refreshScreen = 1;
			break;
		
		case '0':	//	numbers
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			refreshScreen = 1;
			if(passwordIndex > 3) {
				sprintf(screenDown, "%s %s", passwordBuffer, "za dlugie");
				break;
			}
			else {
				passwordBuffer[passwordIndex] = key;
				++passwordIndex;
				sprintf(screenDown, "%s %s", passwordBuffer, "");
			}
			break;
		default:
			break;
		}
	return 0;
}

uint8_t passwordSet(char * password) {
	switch(key) {
		case '#':	//	enter
			refreshScreen = 1;
			if(passwordIndex == 4) {
				//sprintf(password, passwordBuffer);
				password[0] = passwordBuffer[0];
				password[1] = passwordBuffer[1];
				password[2] = passwordBuffer[2];
				password[3] = passwordBuffer[3];
				return 1;
			}
			else {
				sprintf(screenDown, "%s %s", passwordBuffer, "za krotkie");
			}
			break;
			
		case '*':	//	backspace
			refreshScreen = 1;
			if(passwordIndex > 0) {
				--passwordIndex;
				passwordBuffer[passwordIndex] = ' ';
				//--passwordIndex;
			}
			sprintf(passwordBuffer, "%s %s", password, "");
			sprintf(screenDown, "%s %s", passwordBuffer, "");
			break;
			
		case 'X':	//	ERROR
			refreshScreen = 1;
			break;
		
		case '0':	//	numbers
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			refreshScreen = 1;
			if(passwordIndex > 3) {
				sprintf(screenDown, "%s %s", passwordBuffer, "za dlugie");
				break;
			}
			else {
				passwordBuffer[passwordIndex] = key;
				++passwordIndex;
				sprintf(screenDown, "%s %s", passwordBuffer, "");
			}
			break;
		default:
			break;
		}
	return 0;
}


int main(void){
	LED_Init ();
	
	//char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

	LCD1602_Init();		 					// Inicjalizacja LCD
	LCD1602_Backlight(TRUE);  	// Wlaczenie podswietlenia

	//TSI_Init();									// Inicjalizacja panelu dotykowego
	
	Klaw_Init();
	Klaw_S1_4_Int();
	
	sens=2;	// Wybór czulosci: 0 - 2g; 1 - 4g; 2 - 8g
	I2C_WriteReg(0x1d, 0x2a, 0x0);	// ACTIVE=0 - stan czuwania
	I2C_WriteReg(0x1d, 0xe, sens);	 		// Ustaw czulosc zgodnie ze zmienna sens
	I2C_WriteReg(0x1d, 0x2a, 0x1);	 		// ACTIVE=1 - stan aktywny
	
	LCD1602_ClearAll();
	LCD1602_Print("Centr. alarmowa");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("CN-1");
//	while(key==0);
	// moze tryb wykrywania braku zasilania?
	double X=0.0, Y=0.0, Z=0.0;
	
	uint16_t globalStatus = 0;
	#define ALARM		1<<1
	#define WANT_TO_ARM 1<<3
	#define	ARMED		1<<2
	#define WANT_TO_DISARM 1<<4
	
	#define ADMIN_MODE 	1<<0
	#define WANT_ADMIN_MODE 1<<5
	
	
	//globalStatus ^= WANT_TO_DISARM;
	globalStatus |= ADMIN_MODE;
	
	{
		sprintf(screenUp, "Nie uzbrojono");
		sprintf(screenDown, "");
		PTB->PSOR |= LED_R_MASK;//	green for unarmed
		PTB->PCOR |= LED_G_MASK;
		PTB->PSOR |= LED_B_MASK;
	}
	
	char passwords[4][4] = {
		/*{'1', '2', '3', '4'},
		{'1', '1', '1', '1'},
		{'2', '2', '2', '2'},
		{'3', '3', '3', '3'}*/
		"1234",
		"1111",
		"2222",
		"3333"
	};
	#define PASSWORD_ADMIN 0
	#define PASSWORD_UNARM 1
	#define PASSWORD_ARM 2
	#define PASSWORD_ALARM_OFF 3
	
	double accelLevel = 1.5;
	uint16_t adminMenu = 0;
	uint8_t insideMenu = 0;
	uint8_t changedAdminMenu = 1;
	char adminMenuData[17] = "DEBUG";
	
	const char adminMenuTitles[6][17] = {
		"Haslo administr.",
		"Haslo uzbrajania",
		"Haslo odzbrajani",
		"Haslo alarmu",
		"Czulosc akceler.",
		"Czulosc swietlna",
	};
	
	while(1) {	//	main loop
		if(globalStatus & ADMIN_MODE) {	//	ADMIN MODE
			//wysw menu
			if(insideMenu == 1) {			//	insideMenu
				
				if(changedAdminMenu == 1) {
					sprintf(screenUp, "%s", adminMenuTitles[adminMenu]);
					sprintf(screenDown, "%s", adminMenuData);
					refreshScreen = 1;
					changedAdminMenu = 0;
				}
				
				if(key == 'D') {	//	leave menu
					insideMenu = 0;
					changedAdminMenu = 1;
					
					goto END_INPUT;
				}
				
				if( adminMenu < 4 ) {	// set passwords
					if( passwordSet( passwords[adminMenu] ) ) {
						insideMenu = 0;
						changedAdminMenu = 1;
						//adminMenuData = "";
					}
				}	// end admin menu < 4
				else if(adminMenu == 4) {		// accel menu
					switch(key) {		//	input
						case '1':	// accel level
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
							accelLevel = ( (float)(key - '0') + 1.0 ) * 0.5;
							sprintf(adminMenuData, "Czulosc:%4.2fg", accelLevel);
							changedAdminMenu = 1;
							break;
						case '#':
							insideMenu = 0;
							changedAdminMenu = 1;
							break;
					}
				}
				else if(adminMenu == 5) {		// light menu
					// TODO
				}
						
			}													//	insideMenu end
			else {										//	!insideMenu
				if(key == 'D') {			// exit ADMIN_MODE
					globalStatus ^= ADMIN_MODE;
					sprintf(screenUp, "Nie uzbrojono");
					sprintf(screenDown, "");
					PTB->PSOR |= LED_R_MASK;//	green for unarmed
					PTB->PCOR |= LED_G_MASK;
					PTB->PSOR |= LED_B_MASK;
					refreshScreen = 1;
					goto END_INPUT;
				}
				if(changedAdminMenu == 1) {
					sprintf(screenUp, "[A]Wybierz menu");
					sprintf(screenDown, "%s", adminMenuTitles[adminMenu]);
					//sprintf(passwordBuffer, "    ");
					//passwordIndex = 0;
					refreshScreen = 1;
					changedAdminMenu = 0;
				}
				else {										//	!insideMenu
					switch(key) {		//	input
						case '1':	// menu
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
							adminMenu = key - '1';
							changedAdminMenu = 1;
							break;
						case '#':
							insideMenu = 1;
							changedAdminMenu = 1;
							sprintf(adminMenuData, "");
							sprintf(passwordBuffer, "    ");
							passwordIndex = 0;
							if(adminMenu == 4) sprintf(adminMenuData, "Czulosc:%4.2fg", accelLevel);	//accel
							if(adminMenu == 5) sprintf(adminMenuData, "swiatlo%f", accelLevel);	//light
							break;
					}
				}													//	!insideMenu end
			}
		}														//	END ADMIN MODE
		else {											//	USER MODE
			
			if(globalStatus & ARMED) {		//	ARMED
				if(globalStatus & ALARM) {			//	ALARM
					if(key != 0) {
						sprintf(screenUp, "Haslo alarmu");
						if( passwordWrite(passwords[PASSWORD_ALARM_OFF]) ) {
							PTB->PSOR |= LED_R_MASK;//	green for unarmed
							PTB->PCOR |= LED_G_MASK;
							PTB->PSOR |= LED_B_MASK;
							globalStatus ^= ALARM;
							globalStatus ^= ARMED;
							sprintf(screenUp, "Nie uzbrojono");
							sprintf(screenDown, "Alarm OFF");
							refreshScreen = 1;
							goto END_INPUT;
						}
					}
				}																//	ALARM end
				else {													//	!ALARM
					
					//	accelerometer read
					I2C_ReadReg(0x1d, 0x0, &accelStatus);
					accelStatus&=ZYXDR_Mask;
					if(accelStatus)	{		// Czy dane gotowe do odczytu?
						I2C_ReadRegBlock(0x1d, 0x1, 6, arrayXYZ);
						X=((double)((int16_t)((arrayXYZ[0]<<8)|arrayXYZ[1])>>2)/(4096>>sens));
						Y=((double)((int16_t)((arrayXYZ[2]<<8)|arrayXYZ[3])>>2)/(4096>>sens));
						Z=((double)((int16_t)((arrayXYZ[4]<<8)|arrayXYZ[5])>>2)/(4096>>sens));
						
						if( __fabs(X) > accelLevel || __fabs(Y) > accelLevel || __fabs(Z) > accelLevel ) {// accel detect
							globalStatus |= ALARM;
							refreshScreen = 1;
							PTB->PCOR |= LED_R_MASK;//	red for alarm
							PTB->PSOR |= LED_G_MASK;
							PTB->PSOR |= LED_B_MASK;
							sprintf(passwordBuffer, "    ");
							passwordIndex = 0;
							sprintf(screenUp, "ALARM!");
							sprintf(screenDown, "WYKRYTO RUCH");
							goto END_INPUT;
						}
					}//	accelerometer read end
					
					if(globalStatus & WANT_TO_DISARM) {	//	WANT_TO_DISARM
						if(key == 'D') {	//cancel disarm
						globalStatus ^= WANT_TO_DISARM;
						sprintf(screenUp, "Uzbrojono");
						sprintf(screenDown, "");
						refreshScreen = 1;
						goto END_INPUT;
						}
						
						if( passwordWrite(passwords[PASSWORD_UNARM]) ) {	//disarm achieved
							PTB->PSOR |= LED_R_MASK;//	green for disarmed
							PTB->PCOR |= LED_G_MASK;
							PTB->PSOR |= LED_B_MASK;
							globalStatus ^= WANT_TO_DISARM;
							globalStatus ^= ARMED;
							sprintf(screenUp, "Nie uzbrojono");
							sprintf(screenDown, "");
							refreshScreen = 1;
						}
					}																		//	WANT_TO_DISARM end
					else {															//	!WANT_TO_DISARM
						if(key != 0) {	// want to disarm																	NIE DZIALA
							//PTB->PTOR |= LED_R_MASK;
							globalStatus |= WANT_TO_DISARM;
							sprintf(screenUp, "Has. odzbrojenia");
							sprintf(screenDown, "");
							sprintf(passwordBuffer, "    ");
							passwordIndex = 0;
							refreshScreen = 1;
							goto END_INPUT;
						}
					}
					
				}																//	!ALARM end
			}															//	END ARMED
			else {												//	NOT ARMED
				if( !(globalStatus & WANT_TO_ARM) ) {	//	!WANT_TO_ARM
					if( !(globalStatus & WANT_ADMIN_MODE) ) {		//	!WANT_ADMIN_MODE
						if(key == 'A') {	// want to arm
							globalStatus |= WANT_TO_ARM;
							sprintf(screenUp, "Haslo uzbrojenia");
							sprintf(screenDown, "");
							sprintf(passwordBuffer, "    ");
							passwordIndex = 0;
							refreshScreen = 1;
						}
						if(key == 'C') {	// want to enter ADMIN MODE
							globalStatus |= WANT_ADMIN_MODE;
							sprintf(screenUp, "Haslo admin.");
							sprintf(screenDown, "");
							sprintf(passwordBuffer, "    ");
							passwordIndex = 0;
							refreshScreen = 1;
						}
					}																						//	!WANT_ADMIN_MODE end
					else {																			//	WANT_ADMIN_MODE
						if(key == 'D') {		//	cancel WANT_ADMIN_MODE
							globalStatus ^= WANT_ADMIN_MODE;
							sprintf(screenUp, "Nie uzbrojono");
							sprintf(screenDown, "");
							refreshScreen = 1;
							goto END_INPUT;
						}
						
						if( passwordWrite(passwords[PASSWORD_ADMIN]) ) {	//	ADMIN_MODE achieved
							PTB->PSOR |= LED_R_MASK;//	blue for ADMIN_MODE
							PTB->PSOR |= LED_G_MASK;
							PTB->PCOR |= LED_B_MASK;
							globalStatus ^= WANT_ADMIN_MODE;
							globalStatus |= ADMIN_MODE;
							adminMenu = 0;
							insideMenu = 0;
							//sprintf(screenUp, "[A]Wybierz menu");
							//sprintf(screenDown, "");
							changedAdminMenu = 1;
							refreshScreen = 1;
						}
					}																						//	WANT_ADMIN_MODE end
				}																			//	!WANT_TO_ARM end
				else {																//	WANT_TO_ARM

					if(key == 'D') {
						globalStatus ^= WANT_TO_ARM;
						sprintf(screenUp, "Nie uzbrojono");
						sprintf(screenDown, "");
						refreshScreen = 1;
						goto END_INPUT;
					}
					//sprintf(screenUp, "Haslo uzbrojenia");
					if( passwordWrite(passwords[PASSWORD_ARM]) ) {
							PTB->PCOR |= LED_R_MASK;//	yellow for armed
							PTB->PCOR |= LED_G_MASK;
							PTB->PSOR |= LED_B_MASK;
						globalStatus ^= WANT_TO_ARM;
						globalStatus |= ARMED;
						sprintf(screenUp, "Uzbrojono!");
						sprintf(screenDown, "");
						refreshScreen = 1;
					}
				}																			//	WANT_TO_ARM end
			}															//	END NOT ARMED
		}															//	END USER MODE
		END_INPUT:
		
		//for(int i = 0; i<3000000; ++i) __nop();
		//if(key != 0) {
		key = 0;
		//}
		C1_press = 0; C2_press = 0; C3_press = 0; C4_press = 0;
		
		if(refreshScreen) {
			LCD1602_ClearAll();
			LCD1602_Print(screenUp);
			LCD1602_SetCursor(0,1);
			//screenDown[14] = key;
			LCD1602_Print(screenDown);
			refreshScreen = 0;
		}
		
	}			//	while(1) end
}	//	main end
