#ifndef KLAW_H
#define KLAW_H

#include "MKL05Z4.h"
#define C1	7								// Numer bitu dla kolumny 1
#define C2	10							// Numer bitu dla kolumny 2
#define C3	11							// Numer bitu dla kolumny 3
#define C4	12							// Numer bitu dla kolumny 4
#define R1	0	//b0					// Numer bitu dla wiersza 1
#define R2	1	//b1					// Numer bitu dla wiersza 2
#define R3	2	//b2					// Numer bitu dla wiersza 3
#define R4	7	//b7					// Numer bitu dla wiersza 4

#define C1_MASK	(1<<C1)		// Maska dla kolumny 1
#define C2_MASK	(1<<C2)		// Maska dla kolumny 2
#define C3_MASK	(1<<C3)		// Maska dla kolumny 3
#define C4_MASK	(1<<C4)		// Maska dla kolumny 4
#define R1_MASK	(1<<R1)		// Maska dla wiersza 1
#define R2_MASK	(1<<R2)		// Maska dla wiersza 2
#define R3_MASK	(1<<R3)		// Maska dla wiersza 3
#define R4_MASK	(1<<R4)		// Maska dla wiersza 4

extern uint16_t keys;
extern char key;

void Klaw_Init(void);
void Klaw_S1_4_Int(void);
void readKeyboard(uint16_t col);

#endif  /* KLAW_H */
