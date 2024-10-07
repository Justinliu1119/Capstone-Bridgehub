/*********************************************************************
*       EE 459 Embeded System Laboratory
*               Spring 2024
*   Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*               05/06/2024
* 
*********************************************************************/

/*
  lcd.c - Routines for sending data and commands to the LCD shield
*/

#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"                // Declarations of the LCD functions

/* This function not declared in lcd.h since
   should only be used by the routines in this file. */
void lcd_writenibble(unsigned char);

/* Define a couple of masks for the bits in Port B and Port D */
#define DATA_BITS ((1 << PD7)|(1 << PD6)|(1 << PD5)|(1 << PD4))
#define Register_select (1 << PB0) // Rigister Select
#define Enable_signal (1 << PC0) // Enable Signal

/*
  lcd_init - Do various things to initialize the LCD display
*/
void lcd_init(void)
{
    DDRD |= (DATA_BITS);// Set the DDR register bits for ports B and D
    DDRB |= (Register_select);
    DDRC |= (Enable_signal);
                                // Take care not to affect any unnecessary bits

    _delay_ms(15);              // Delay at least 15ms

    lcd_writenibble(0x30);      // Use lcd_writenibble to send 0b0011
    _delay_ms(5);               // Delay at least 4msec

    lcd_writenibble(0x30);      // Use lcd_writenibble to send 0b0011
    _delay_us(120);             // Delay at least 100usec

    lcd_writenibble(0x30);      // Use lcd_writenibble to send 0b0011, no delay needed

    lcd_writenibble(0x20);      // Use lcd_writenibble to send 0b0010
    _delay_ms(2);               // Delay at least 2ms
    
    lcd_writecommand(0x28);     // Function Set: 4-bit interface, 2 lines

    lcd_writecommand(0x0f);     // Display and cursor on
}

/*
  lcd_moveto - Move the cursor to the row and column given by the arguments.
  Row is 0 or 1, column is 0 - 15.
*/
void lcd_moveto(unsigned char row, unsigned char col)
{
    unsigned char pos=0x00;
    if(row == 0) {
        pos = 0x80 + col;       // 1st row locations start at 0x80
    }
    else if(row==2){            // 3rd row locations start at 0x94
      pos = 0x94 + col;
    }
    else if(row==1){
        pos = 0xc0 + col;       // 2nd row locations start at 0xc0
    }
    else if(row==3){
        pos = 0xd4 + col;       // 4th row locations start at 0xc0
    }
    lcd_writecommand(pos);      // Send command
}

/*
  lcd_stringout - Print the contents of the character string "str"
  at the current cursor position.
*/
void lcd_stringout(char *str)
{
    int i = 0;
    while (str[i] != '\0') {    // Loop until next charater is NULL byte
        lcd_writedata(str[i]);  // Send the character
        i++;
    }
}

/*
  lcd_writecommand - Output a byte to the LCD command register.
*/
void lcd_writecommand(unsigned char cmd)
{
    /* Clear PB0 to 0 for a command transfer */
    PORTB &= ~(1<<0);

    /* Call lcd_writenibble to send UPPER four bits of "cmd" argument */
    lcd_writenibble(cmd);

    /* Call lcd_writenibble to send LOWER four bits of "cmd" argument */
    lcd_writenibble(cmd<<4);

    /* Delay 2ms */
    _delay_ms(2); 
    

}

/*
  lcd_writedata - Output a byte to the LCD data register
*/
void lcd_writedata(unsigned char dat)
{
    /* Set PB0 to 1 for a data transfer */
    PORTB |= (1<<0);

    /* Call lcd_writenibble to send UPPER four bits of "dat" argument */
    lcd_writenibble(dat);

    /* Call lcd_writenibble to send LOWER four bits of "dat" argument */
    lcd_writenibble(dat<<4);

    /* Delay 2ms */
    _delay_ms(2);

}

/*
  lcd_writenibble - Output the UPPER four bits of "lcdbits" to the LCD
*/
void lcd_writenibble(unsigned char lcdbits)
{
    /* Load PORTD, bits 7-4 with bits 7-4 of "lcdbits" */
    PORTD &= ~(DATA_BITS);
    lcdbits &= DATA_BITS;
    PORTD |= lcdbits; 


    /* Make E signal (PC0) go to 1 and back to 0 */

    PORTC |= (1<<0);
    PORTC |= (1<<0);
    PORTC |= (1<<0);
    PORTC &= ~(1<<0);

}
