/*
Alarm CN-1
Tworca: Cristian Niwelt
23/01/2024
Projekt alarmu z wykorzystaniem czujnika natezenia swiatla i akcelerometru.
*/
#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include "stdio.h"
#include "i2c.h"
#include "klaw.h"
#include "leds.h"
#include "tsi.h"
#include "ADC.h"
#include "DAC.h"
#include "math.h"

// Wartosci stale - parametry
#define ALARM_SIREN_MAX_F 40.0f
#define ALARM_SIREN_MIN_F 20.0f
#define ALARM_SIREN_DELTA_F 0.005f;

// Zmienne dot. akcelerometu
#define	ZYXDR_Mask	1<<3	// Maska bitu ZYXDR w rejestrze STATUS
static uint8_t arrayXYZ[6] = {0,0,0,0,0,0};
static uint8_t sens;
static uint8_t accelStatus;

// Zmienne dot. klawiatury
volatile uint8_t C1_press=0;
volatile uint8_t C2_press=0;	// "1" - klawisz zostal wcisniety "0" - klawisz "skonsumowany"
volatile uint8_t C3_press=0;
volatile uint8_t C4_press=0;

// Zmienne dot. czujnika natezenia swiatla
float adc_volt_coeff = ((float)(((float)2.91) / 4096) );			// Wspólczynnik korekcji wyniku, w stosunku do napiecia referencyjnego przetwornika
uint8_t wynik_ok=0;
uint16_t temp;
float wynik;
float lightAveraged = 0.0f;

// Zmienne dot. przetwornika DAC - dzwiek
#define DIV_CORE	8192	// Przerwanie co 0.120ms - fclk=8192Hz df=8Hz
#define MASK_9_BIT 0x1FF
//uint16_t slider;
uint16_t dac;
int16_t Sinus[1024];
uint16_t faza = 0, mod = (uint16_t)ALARM_SIREN_MAX_F, freq, df=DIV_CORE/MASK_9_BIT;;
float modFloat = ALARM_SIREN_MAX_F;

// Przerwania od SysTick - dzwiek
void SysTick_Handler(void) {	// Podprogram obslugi przerwania od SysTick'a 
	//PTB->PTOR |= LED_R_MASK;
	dac = Sinus[faza] + 0x0800;					// Przebieg sinusoidalny
	DAC_Load_Trig(dac);
	faza += mod;		// faza - generator cyfrowej fazy
	faza &= MASK_9_BIT;	// rejestr sterujacy przetwornikiem, liczacy modulo 1024 (N=10 bitów)
	mod = (uint16_t)modFloat;
	modFloat -= ALARM_SIREN_DELTA_F;
	if(modFloat < ALARM_SIREN_MIN_F)
		modFloat = ALARM_SIREN_MAX_F;
}

// Przerwania od przetwornika ADC - pomiar natezenia swiatla
void ADC0_IRQHandler() {	
	temp = ADC0->R[0];		// Odczyt danej i skasowanie flagi COCO
	if(!wynik_ok)					// Sprawdz, czy wynik skonsumowany przez petle glówna
	{
		wynik = temp;				// Wyslij wynik do petli glównej
		wynik_ok=1;
	}
}

// Przerwania od portu A - klawiatura
void PORTA_IRQHandler(void) {	// Podprogram obslugi przerwania od klawiszy C1, C2, C3 i C4
	uint32_t buf;
	buf=PORTA->ISFR & (C1_MASK | C2_MASK | C3_MASK | C4_MASK);
if(key == 0)
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
	//LCD1602_SetCursor(0,1);
	//LCD1602_Print(&key);
	PORTA->ISFR |=  C1_MASK | C2_MASK | C3_MASK | C4_MASK;	// Kasowanie wszystkich bitów ISF
	NVIC_ClearPendingIRQ(PORTA_IRQn);
}

// Rozne zmienne
uint8_t refreshScreen = 1;
uint8_t refreshTop = 1;
uint8_t refreshBottom = 1;
char screenUp[17];
char screenDown[17];
char passwordBuffer[4] = {' ', ' ', ' ', ' '};
int16_t passwordIndex = 0;

char keyCopy = 0;	// to copy key value when using it(to quickly delete it so it can get new value)

// Ta funkcja porownuje dwa hasla
uint8_t passwordCompare(char p1[4], char p2[4]) {
	for(int i=0; i<4; ++i) {
		if(p1[i] != p2[i])
			return 0;
	}
	return 1;
}

// Ta funkcja porownuje haslo wpisywane do tego wskazanego przez pass
uint8_t passwordWrite(char pass[4]) {
	switch(key) {
		case '#':	//	enter
			key = 0;
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
			key = 0;
			if(passwordIndex > 0) {
				refreshBottom = 1;
				--passwordIndex;
				passwordBuffer[passwordIndex] = ' ';
				sprintf(screenDown, "                ");
				sprintf(screenDown, "%s %s", passwordBuffer, "");
				//--passwordIndex;
			}
			break;
			
		case 'X':	//	ERROR
			key = 0;
			//refreshScreen = 1;
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
			keyCopy = key;
			key = 0;
			refreshBottom = 1;
			if(passwordIndex > 3) {
				sprintf(screenDown, "%s %s", passwordBuffer, "za dlugie");
				break;
			}
			else {
				passwordBuffer[passwordIndex] = keyCopy;
				++passwordIndex;
				sprintf(screenDown, "%s %s", passwordBuffer, "            ");
			}
			break;
		default:
			//key = 0;
			break;
		}
	return 0;
}

// Ta funkcja zmienia haslo wskazane przez wkaznik password
uint8_t passwordSet(char * password) {
	switch(key) {
		case '#':	//	enter
			key = 0;
			refreshScreen = 1;
			//refreshBottom = 1;
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
				refreshBottom = 1;
			}
			break;
			
		case '*':	//	backspace
			key = 0;
			//refreshScreen = 1;
			refreshBottom = 1;
			if(passwordIndex > 0) {
				--passwordIndex;
				passwordBuffer[passwordIndex] = ' ';
				//LCD1602_SetCursor(passwordIndex,1);
				//LCD1602_Print(" ");
				//--passwordIndex;
				sprintf(screenDown, "%s %s", passwordBuffer, "");
			}
			//else {
			//	sprintf(screenDown, "%s %s", passwordBuffer, "");
			//}
			//sprintf(passwordBuffer, "%s %s", password, "");
			
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
			keyCopy = key;
			key = 0;
			//refreshScreen = 1;
			refreshBottom = 1;
			if(passwordIndex > 3) {
				//sprintf(screenDown, "%s %s", passwordBuffer, "za dlugie");
				break;
			}
			else {
				passwordBuffer[passwordIndex] = keyCopy;
				//LCD1602_SetCursor(passwordIndex,1);
				//LCD1602_Print(&keyCopy);
				++passwordIndex;
				sprintf(screenDown, "%s %s", passwordBuffer, "");
			}
			break;
		case 'X':	//error
			key = 0;
			break;
		default:
			break;
		}
	return 0;
}

// Main
int main(void){
	DAC_Init();
	for(faza=0;faza<256;faza++)
		Sinus[faza]=(sin((double)faza*2*3.1415/512)*2047.0); // zaladowanie tablicy sinusa
	freq=mod*df;
	SysTick_Config(1); // wylaczenie syreny
	
	LED_Init ();
	
	uint8_t initOK = ADC_Init();
	while(initOK) PTB->PTOR |= LED_R_MASK;
	ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(8);
	//NVIC_EnableIRQ(ADC0_IRQn);
	
	// ekran powitalny
	LCD1602_Init();		 					// Inicjalizacja LCD
	LCD1602_Backlight(TRUE);  	// Wlaczenie podswietlenia
	LCD1602_ClearAll();
	LCD1602_Print("Centr. alarmowa");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("CN-1");

	Klaw_Init();
	Klaw_S1_4_Int();
	
	I2C_WriteReg(0x1d, 0x2a, 0x0);	// ACTIVE=0 - stan czuwania
	I2C_WriteReg(0x1d, 0xe, 2);	 		// Ustawienie czulosci akcelerometry na 8g
	I2C_WriteReg(0x1d, 0x2a, 0x1);	// ACTIVE=1 - stan aktywny
	
	for(int i=0; i<10000000; ++i) __nop();
	// todo: tryb wykrywania zaniku zasilania
	
	double X=0.0, Y=0.0, Z=0.0;
	
	uint16_t globalStatus = 0;
	#define ALARM		1<<1
	#define WANT_TO_ARM 1<<3
	#define	ARMED		1<<2
	#define WANT_TO_DISARM 1<<4
	
	#define ADMIN_MODE 	1<<0
	#define WANT_ADMIN_MODE 1<<5
	
	
	//globalStatus ^= WANT_TO_DISARM;
	//globalStatus |= ARMED;
	
	// status poczatkowy alarmu - nie uzbrojony(kolor diody zielony)
	sprintf(screenUp, "Nie uzbrojono");
	sprintf(screenDown, "");
	PTB->PSOR |= LED_R_MASK;
	PTB->PCOR |= LED_G_MASK;
	PTB->PSOR |= LED_B_MASK;
	
	// tablica z haslami dostepu
	char passwords[4][4] = {
		"1234",	//admin
		"1111",	//unarm
		"2222",	//arm
		"3333"	//alarm
	};
	#define PASSWORD_ADMIN 0
	#define PASSWORD_UNARM 1
	#define PASSWORD_ARM 2
	#define PASSWORD_ALARM_OFF 3
	
	double accelLevel = 1.5;				// prog alarmu od akcelerometru
	uint8_t accelBool = 1;					// czy odbierac alarm od akcelerometru
	
	float lightIntegral = 0.09f;		// jak szybko sie zmienia wartosc lightAveraged
	uint8_t lightBool = 1;					// czy odbierac alarm od czujnika swiatla
	NVIC_EnableIRQ(ADC0_IRQn);			// odbieraj przerwania od przetwornika ADC
	float lightSensitivity = 0.01f;	// prog wlaczania alarmu dla czujnika swiatla
	
	uint16_t adminMenu = 0;
	uint8_t insideMenu = 0;
	uint8_t changedAdminMenu = 1;
	char adminMenuData[17] = "DEBUG";
	Klaw_Init();
	Klaw_S1_4_Int();
	const char adminMenuTitles[7][17] = {
		"Haslo administr.",
		"Haslo odzbrajani",
		"Haslo uzbrajania",
		"Haslo alarmu    ",
		"Czulosc akceler.",
		"Zmiennosc swietl",
		"Czulosc swietlna",
	};
	
	while(1) {	//	main loop 
		if(globalStatus & ADMIN_MODE) {	//	ADMIN MODE
			//wysw menu
			if(insideMenu == 1) {			//	insideMenu
				
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
						case '7':
						case '8':
						case '9':
							keyCopy = key;
							key = 0;
							accelBool = 1;
							accelLevel = ( (float)(keyCopy - '0') + 1.0 ) * 0.5;
							sprintf(adminMenuData, "                ");
							sprintf(adminMenuData, "Czulosc:%4.2fg", accelLevel);
							changedAdminMenu = 1;
							//refreshBottom = 1;
							break;
						case '0':
							keyCopy = key;
							key = 0;
							accelBool = 0;
							//accelLevel = ( (float)(keyCopy - '0') + 1.0 ) * 0.5;
							sprintf(adminMenuData, "                ");
							sprintf(adminMenuData, "Akcel. wylaczony");
							//refreshBottom = 1;
							changedAdminMenu = 1;
							break;
						case '#':
							key = 0;
							insideMenu = 0;
							changedAdminMenu = 1;
							break;
						default:
							key = 0;
							break;
					}
					if(changedAdminMenu == 1) {
						if(insideMenu == 1) {
							sprintf(screenUp, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenDown, "%s", adminMenuData);
						} else {
							sprintf(screenDown, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenUp, "%s", "[A]Wybierz menu");
						}
						refreshScreen = 1;
						changedAdminMenu = 0;
					}
				}
				else if(adminMenu == 5) {		// light integral menu
					if(lightBool && wynik_ok) {							// light read
						wynik = wynik*adc_volt_coeff;
						float deltaLight = (wynik - lightAveraged) * lightIntegral;
						if(__fabs(deltaLight) > lightSensitivity) {
							PTB->PCOR |= LED_R_MASK;
							PTB->PSOR |= LED_B_MASK;
						} else {
							PTB->PSOR |= LED_R_MASK;
							PTB->PCOR |= LED_B_MASK;
						}
						lightAveraged += deltaLight;
						//lightAveraged = wynik + lightAveraged * (1.0f-0.01f);
						//refreshBottom = 1;
						wynik_ok = 0;
					}
					if(lightBool == 1) sprintf(screenDown, "Zm:%4.2f Sw:%4.2fg", lightIntegral, lightAveraged);
					else sprintf(screenDown, "Cz. swiatla wyl.");
					refreshBottom = 1;
					switch(key) {		//	input
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							keyCopy = key;
							key = 0;
							lightBool = 1;
							NVIC_EnableIRQ(ADC0_IRQn);
							lightIntegral = ( (float)(keyCopy - '0') ) * 0.01f;
							//sprintf(adminMenuData, "Czulosc:%4.2fg", lightIntegral);
							//sprintf(screenDown, "Zmiennosc %4.2fg", lightIntegral);
							refreshBottom = 1;
							break;
						case '0':
							keyCopy = key;
							key = 0;
							lightBool = 0;
							NVIC_DisableIRQ(ADC0_IRQn);
							//accelLevel = ( (float)(keyCopy - '0') + 1.0 ) * 0.5;
							//sprintf(screenDown, "Cz. swiatla wyl.");
							refreshBottom = 1;
							break;
						case '#':
							key = 0;
							insideMenu = 0;
							changedAdminMenu = 1;
							break;
						default:
							key = 0;
							break;
					}
					if(changedAdminMenu == 1) {
						if(insideMenu == 1) {
							sprintf(screenUp, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenDown, "%s", adminMenuData);
						} else {
							sprintf(screenDown, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenUp, "%s", "[A]Wybierz menu");
						}
						refreshScreen = 1;
						changedAdminMenu = 0;
					}
				}
				else if(adminMenu == 6) {		// light sensitivity menu
					if(lightBool && wynik_ok) {							// light read
						wynik = wynik*adc_volt_coeff;
						float deltaLight = (wynik - lightAveraged) * lightIntegral;
						if(__fabs(deltaLight) > lightSensitivity) {
							PTB->PCOR |= LED_R_MASK;
							PTB->PSOR |= LED_B_MASK;
						} else {
							PTB->PSOR |= LED_R_MASK;
							PTB->PCOR |= LED_B_MASK;
						}
						lightAveraged += deltaLight;
						//lightAveraged = wynik + lightAveraged * (1.0f-0.01f);
						//refreshBottom = 1;
						wynik_ok = 0;
					}
					if(lightBool == 1) sprintf(screenDown, "Cz:%4.2f Sw:%4.2fg", lightSensitivity, lightAveraged);
					else sprintf(screenDown, "Cz. swiatla wyl.");
					refreshBottom = 1;
					switch(key) {		//	input
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							keyCopy = key;
							key = 0;
							lightBool = 1;
							NVIC_EnableIRQ(ADC0_IRQn);
							lightSensitivity = ( (float)(keyCopy - '0') ) * 0.01f;
							
							break;
						case '0':
							keyCopy = key;
							key = 0;
							lightBool = 0;
							NVIC_DisableIRQ(ADC0_IRQn);
							//accelLevel = ( (float)(keyCopy - '0') + 1.0 ) * 0.5;
							sprintf(screenDown, "Cz. swiatla wyl.");
							refreshBottom = 1;
							break;
						case '#':
							key = 0;
							insideMenu = 0;
							PTB->PSOR |= LED_R_MASK;
							PTB->PCOR |= LED_B_MASK;
							changedAdminMenu = 1;
							break;
						default:
							key = 0;
							break;
					}
					if(changedAdminMenu == 1) {
						if(insideMenu == 1) {
							sprintf(screenUp, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenDown, "%s", adminMenuData);
						} else {
							sprintf(screenDown, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenUp, "%s", "[A]Wybierz menu");
						}
						refreshScreen = 1;
						changedAdminMenu = 0;
					}
					if (key == 'D') {	//	leave menu
						key = 0;
						insideMenu = 0;
						changedAdminMenu = 1;
						goto END_INPUT;
					}
					
				}
			}													//	insideMenu end
			else {										//	!insideMenu
				if(key == 'D') {			// exit ADMIN_MODE
					key = 0;
					globalStatus ^= ADMIN_MODE;
					sprintf(screenUp, "Nie uzbrojono");
					sprintf(screenDown, "");
					PTB->PSOR |= LED_R_MASK;//	green for unarmed
					PTB->PCOR |= LED_G_MASK;
					PTB->PSOR |= LED_B_MASK;
					refreshScreen = 1;
					goto END_INPUT;
				}
				
				//if(changedAdminMenu == 0) {										//	!insideMenu
					switch(key) {		//	input
						case '1':	// menu select
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
							keyCopy = key;
							key = 0;
							adminMenu = keyCopy - '1';
							changedAdminMenu = 1;
							break;
						case '#':	//enter menu
							key = 0;
							insideMenu = 1;
							changedAdminMenu = 1;
							sprintf(adminMenuData, "");
							sprintf(passwordBuffer, "    ");
							passwordIndex = 0;
							if(adminMenu == 4) sprintf(adminMenuData, "Czulosc:%4.2fg", accelLevel);	//accel
							if(adminMenu == 5) sprintf(adminMenuData, "Calkowanie%4.2fg", lightIntegral);	//light
							break;
						default:
							key = 0;
						break;
					}
					if(changedAdminMenu == 1) {
						if(insideMenu == 1) {
							sprintf(screenUp, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenDown, "%s", adminMenuData);
						} else {
							sprintf(screenDown, "%s", adminMenuTitles[adminMenu]);
							sprintf(screenUp, "%s", "[A]Wybierz menu");
						}
						refreshScreen = 1;
						changedAdminMenu = 0;
					}
			}												//	!insideMenu end
		}														//	END ADMIN MODE
		else {											//	USER MODE
			if(globalStatus & ARMED) {		//	ARMED
				if(globalStatus & ALARM) {			//	ALARM
					
					//mod = (uint16_t)modFloat;
					//modFloat -= ALARM_SIREN_DELTA_F;
					//if(modFloat < ALARM_SIREN_MIN_F)
					//	modFloat = ALARM_SIREN_MAX_F;
					if(key != 0) {
						//key = 0;
						sprintf(screenUp, "Haslo alarmu");
						sprintf(screenDown, "");
						if( passwordWrite(passwords[PASSWORD_ALARM_OFF]) ) {
							SysTick_Config(1);
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
						//key = 0;
					}
				}																//	ALARM end
				else {													//	!ALARM
					if(accelBool == 1) {
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
								SysTick_Config(SystemCoreClock/DIV_CORE);	// enable alarm siren
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
					}	// accelBool end
					if(lightBool && wynik_ok) {							// light read
						wynik = wynik*adc_volt_coeff;
						float deltaLight = (wynik - lightAveraged) * lightIntegral;
						if( __fabs(deltaLight) > lightSensitivity ) {	// light detect
								globalStatus |= ALARM;
								refreshScreen = 1;
								SysTick_Config(SystemCoreClock/DIV_CORE);	// enable alarm siren
								PTB->PCOR |= LED_R_MASK;//	red for alarm
								PTB->PSOR |= LED_G_MASK;
								PTB->PSOR |= LED_B_MASK;
								sprintf(passwordBuffer, "    ");
								passwordIndex = 0;
								sprintf(screenUp, "ALARM!");
								sprintf(screenDown, "ZMIANA SWIATLA");
								goto END_INPUT;
							}
						lightAveraged += deltaLight;
						wynik_ok = 0;
					}
					if(globalStatus & WANT_TO_DISARM) {	//	WANT_TO_DISARM
						if( passwordWrite(passwords[PASSWORD_UNARM]) ) {	//disarm achieved
							PTB->PSOR |= LED_R_MASK;//	green for disarmed
							PTB->PCOR |= LED_G_MASK;
							PTB->PSOR |= LED_B_MASK;
							NVIC_DisableIRQ(ADC0_IRQn);
							globalStatus ^= WANT_TO_DISARM;
							globalStatus ^= ARMED;
							sprintf(screenUp, "Nie uzbrojono");
							sprintf(screenDown, "");
							refreshScreen = 1;
						}
						else if(key == 'D') {	//cancel disarm
							key = 0;
							globalStatus ^= WANT_TO_DISARM;
							sprintf(screenUp, "Uzbrojono");
							sprintf(screenDown, "");
							refreshScreen = 1;
							goto END_INPUT;
						}
						else key = 0;
						
						
					}																		//	WANT_TO_DISARM end
					else {															//	!WANT_TO_DISARM
						if(key != 0) {	// want to disarm																	NIE DZIALA
							key = 0;
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
						switch(key) {
							case 'A':	// want to arm
								key = 0;
								//if(lightBool == 1)	//get environment light measurement during arming process
								//	SysTick_Config(SystemCoreClock/DIV_CORE);
								globalStatus |= WANT_TO_ARM;
								sprintf(screenUp, "Haslo uzbrojenia");
								sprintf(screenDown, "");
								sprintf(passwordBuffer, "    ");
								passwordIndex = 0;
								refreshScreen = 1;
								break;
							case 'C':	// want to enter ADMIN MODE
								key = 0;
								globalStatus |= WANT_ADMIN_MODE;
								sprintf(screenUp, "Haslo admin.");
								sprintf(screenDown, "");
								sprintf(passwordBuffer, "    ");
								passwordIndex = 0;
								refreshScreen = 1;
								break;
							default:
								key = 0;
								break;
						}
						goto END_INPUT;
					}																						//	!WANT_ADMIN_MODE end
					else {																			//	WANT_ADMIN_MODE
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
						else if(key == 'D') {		//	cancel WANT_ADMIN_MODE
							key = 0;
							globalStatus ^= WANT_ADMIN_MODE;
							sprintf(screenUp, "Nie uzbrojono");
							sprintf(screenDown, "");
							refreshScreen = 1;
							goto END_INPUT;
						}
						else key = 0;
					}																						//	WANT_ADMIN_MODE end
				}																			//	!WANT_TO_ARM end
				else {																//	WANT_TO_ARM
					if(lightBool && wynik_ok) {							// light read
						wynik = wynik*adc_volt_coeff;
						float deltaLight = (wynik - lightAveraged) * lightIntegral;
						lightAveraged += deltaLight;
						wynik_ok = 0;
					}
					if( passwordWrite(passwords[PASSWORD_ARM]) ) {
						PTB->PCOR |= LED_R_MASK;//	yellow for armed
						PTB->PCOR |= LED_G_MASK;
						PTB->PSOR |= LED_B_MASK;
						if(lightBool == 1) {
							NVIC_EnableIRQ(ADC0_IRQn);
						} else {
							NVIC_DisableIRQ(ADC0_IRQn);
						}
						globalStatus ^= WANT_TO_ARM;
						globalStatus |= ARMED;
						sprintf(screenUp, "Uzbrojono!");
						sprintf(screenDown, "");
						refreshScreen = 1;
					}
					else if(key == 'D') {
						key = 0;
						//SysTick_Config(1);	//disable light measurement 
						globalStatus ^= WANT_TO_ARM;
						sprintf(screenUp, "Nie uzbrojono");
						sprintf(screenDown, "");
						refreshScreen = 1;
						goto END_INPUT;
					}
					else key = 0;
				}																			//	WANT_TO_ARM end
				//key = 0;
			}															//	END NOT ARMED
		}															//	END USER MODE
		END_INPUT:
		
		C1_press = 0; C2_press = 0; C3_press = 0; C4_press = 0;
		
		// pisanie do ekranu
		if(refreshScreen) {
			LCD1602_ClearAll();
			LCD1602_Print(screenUp);
			LCD1602_SetCursor(0,1);
			//screenDown[14] = key;
			LCD1602_Print(screenDown);
			refreshScreen = 0;
		}
		// pisanie do dolntej czesci ekranu - dane
		if(refreshBottom) {
			//LCD1602_ClearAll();
			//LCD1602_Print(screenUp);
			LCD1602_SetCursor(0,1);
			//screenDown[14] = key;
			LCD1602_Print(screenDown);
			refreshBottom = 0;
		}
	}			//	while(1) end
}	//	main end
