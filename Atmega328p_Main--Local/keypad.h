
#ifndef KEYPAD_H
#define KEYPAD_H

#include <avr/io.h>

// Define the pins corresponding to keypad rows and columns
#define ROW1 PC1
#define ROW2 PC2
#define ROW3 PC3
#define ROW4 PB1

#define COL1 PD2
#define COL2 PD3
#define COL3 PB3

// Define bitmasks for the rows and columns
#define ROWS_MASK ((1 << ROW1) | (1 << ROW2) | (1 << ROW3) | (1 << ROW4))
#define COLS_MASK ((1 << COL1) | (1 << COL2) | (1 << COL3))

// Initialize the keypad
void keypad_init(void);

// Scan the keypad and return the pressed key
char keypad_scan(void);

#endif // KEYPAD_H
