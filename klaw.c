#include "klaw.h"

void Klaw_Init(void) {
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;		// Wlaczenie portu A
	PORTA->PCR[C1] |= PORT_PCR_MUX(1);
	PORTA->PCR[C2] |= PORT_PCR_MUX(1);
	PORTA->PCR[C3] |= PORT_PCR_MUX(1);
	PORTA->PCR[C4] |= PORT_PCR_MUX(1);
	PORTA->PCR[C1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[C2] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[C3] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[C4] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;		// Wlaczenie portu B
	PORTB->PCR[R1] |= PORT_PCR_MUX(1);
	PORTB->PCR[R2] |= PORT_PCR_MUX(1);
	PORTB->PCR[R3] |= PORT_PCR_MUX(1);
	PORTB->PCR[R4] |= PORT_PCR_MUX(1);
	//PTB->PDDR |= R1_MASK | R2_MASK | R3_MASK | R4_MASK;				//output
	PTB->PDDR |= (1<<R1);
	PTB->PDDR |= (1<<R2);
	PTB->PDDR |= (1<<R3);
	PTB->PDDR |= (1<<R4);
	PTB->PCOR |= (1<<R1);
	PTB->PCOR |= (1<<R2);
	PTB->PCOR |= (1<<R3);
	PTB->PCOR |= (1<<R4);
	//PTB->PDDR |= R2_MASK;
	//PTB->PDOR |= R1_MASK | R2_MASK | R3_MASK | R4_MASK;				//off
	//PTB->PCOR |= R1_MASK | R2_MASK | R3_MASK | R4_MASK;
}

void Klaw_S1_4_Int(void) {
	PORTA -> PCR[C1] |= PORT_PCR_IRQC(0xa);
	PORTA -> PCR[C2] |= PORT_PCR_IRQC(0xa);		//0x8 - poziom "0"; 0x9 - zbocze narastające; 0xa - zbocze opadające; 0xb - obydwa zbocza
	PORTA -> PCR[C3] |= PORT_PCR_IRQC(0xa);		
	PORTA -> PCR[C4] |= PORT_PCR_IRQC(0xa);
	NVIC_ClearPendingIRQ(PORTA_IRQn);
	NVIC_EnableIRQ(PORTA_IRQn);
}



const uint16_t rowMasks[4] = {R1_MASK, R2_MASK, R3_MASK, R4_MASK};
const uint16_t colMasks[4] = {C1_MASK, C2_MASK, C3_MASK, C4_MASK};
uint16_t keys = 0;	//	bity klawiszy
char key = 0;

void readKeyboard(uint16_t col) {
	if(col>3) return ;	//assert
	
	keys = 0;
	
	for(int i=0; i<4; i++) {
		//PTB->PSOR |= R1_MASK | R2_MASK | R3_MASK | R4_MASK;
		//PTB->PCOR |= rowMasks[i];
		PTB->PSOR |= R1_MASK | R2_MASK | R3_MASK | R4_MASK;
		__nop();__nop();__nop();__nop();__nop();__nop();
		PTB->PDOR ^= rowMasks[i];
		
		//for(int i = 0; i < 300; i++)__nop();
		
		if( PTA->PDIR & colMasks[col] ) {
			keys |= 1<<( i + col*4 );
			//keys = i + 4*col;
			//break;
		}
	}
	
	PTB->PCOR |= R1_MASK | R2_MASK | R3_MASK | R4_MASK;
	
	switch(keys) {
		case 28672:
			key = 'A';
			break;
		case 45056:
			key = 'B';
			break;
		case 53248:
			key = 'C';
			break;
		case 57344:
			key = 'D';
			break;
		case 112:
			key = '1';
			break;
		case 1792:
			key = '2';
			break;
		case 7:
			key = '3';
			break;
		case 176:
			key = '4';
			break;
		case 2816:
			key = '5';
			break;
		case 11:
			key = '6';
			break;
		case 208:
			key = '7';
			break;
		case 3328:
			key = '8';
			break;
		case 13:
			key = '9';
			break;
		case 3584:
			key = '0';
			break;
		case 224:
			key = '*';
			break;
		case 14:
			key = '#';
			break;
		default:
			key = 'X';
			break;
	}
}




