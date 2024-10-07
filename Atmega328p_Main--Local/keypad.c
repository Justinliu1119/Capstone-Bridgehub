/*********************************************************************
*       EE 459 Embeded System Laboratory
*               Spring 2024
*   Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*               05/06/2024
* 
*********************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include "keypad.h"

// Define the pins for keypad rows and columns
#define ROWS_MASK ((1 << PC1) | (1 << PC2) | (1 << PC3))
#define ROWS_DDR DDRC
#define ROWS_PORT PORTC
#define ROWS_PIN PINC

#define COL1 PD2
#define COL2 PD3
#define COL3 PB7  
#define COLS_DDR DDRD
#define COLS_PORT PORTD
#define COLS_PIN PIND

#define COL3_PORT PORTB  // Define PORTB for Column 3
#define COL3_DDR DDRB    // Define DDRB for Column 3

#define ROW4 PB1
#define ROW4_DDR DDRB
#define ROW4_PORT PORTB
#define ROW4_PIN PINB

void keypad_init(void) {
    // Set rows as inputs with pull-up resistors
    ROWS_DDR &= ~ROWS_MASK;
    ROWS_PORT |= ROWS_MASK;

    // Set the fourth row (PB1) as input with pull-up
    ROW4_DDR &= ~(1 << ROW4);
    ROW4_PORT |= (1 << ROW4);

    // Set columns 1 and 2 as outputs, initially high
    COLS_DDR |= (1 << COL1) | (1 << COL2);
    COLS_PORT |= (1 << COL1) | (1 << COL2);

    // Set column 3 (PB7) as output, initially high
    COL3_DDR |= (1 << COL3);
    COL3_PORT |= (1 << COL3); 
}

char keypad_scan(void) {
    const char keys[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };

    // Scan columns
    for (int col = 0; col < 3; col++) {
        // Set all columns to high before grounding one
        COLS_PORT |= (1 << COL1) | (1 << COL2);
        COL3_PORT |= (1 << COL3);  

        // Ground the current column
        if (col == 0) {
            COLS_PORT &= ~(1 << COL1);
        } else if (col == 1) {
            COLS_PORT &= ~(1 << COL2);
        } else {
            COL3_PORT &= ~(1 << COL3);  
        }

        // Scan rows
        for (int row = 0; row < 4; row++) {
            _delay_ms(5); // Delay for debouncing

            if (row < 3) {
                if (!(ROWS_PIN & (1 << (PC1 + row)))) {
                    return keys[row][col];
                }
            } else {
                if (!(ROW4_PIN & (1 << ROW4))) {
                    return keys[row][col];
                }
            }
        }
        
        // Set column pins back to high after scanning
        COLS_PORT |= (1 << COL1) | (1 << COL2);
        COL3_PORT |= (1 << COL3);
    }
    return '\0';
}
