/*********************************************************************
*       EE 459 Embeded System Laboratory
*               Spring 2024
*   Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*               05/06/2024
* 
*********************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "lcd.h"
#include <util/delay.h>

volatile int count = 0;
volatile int barcode_ready = 0;
volatile char barcode[7];
#// Setup SPI
void SPI_SlaveInit(void) {
  DDRB |= (1<<4);  // MISO as OUTPUT
  SPCR |= ((1<<SPE) | (1<<SPIE));  // Enable SPI and SPI interrupt
  //SPCR |= (1<<CPHA); // set spi mode to mode 1

}

// SPI Serial Transfer Complete ISR
ISR(SPI_STC_vect) {
    //uint8_t received = SPDR;
    
    //char str_value[2];  // Buffer to hold the string
    //sprintf(str_value, "%c", received);  // Convert uint8_t to string
    //lcd_stringout(str_value);
    
    //SPDR = received;  // Echo received data

    char rx_char = SPDR; // Read the received data
    SPDR = rx_char;

    if (rx_char == 0x00)
    {
        if (count > 0)
        {
            barcode[count] = '\0'; // Null-terminate the string
            barcode_ready = 1;     // Set the flag indicating that a full barcode is ready
            count = 0;             // Prepare for the next barcode
        }
    }
    else
    {
        barcode[count++] = rx_char; // Store the character
    }
}

int main(void) {
  SPI_SlaveInit();  // Initialize as SPI slave
  lcd_init();
  lcd_writecommand(1);
  lcd_writecommand(2);
  sei();            // Enable global interrupts

  // Main loop does nothing, SPI is interrupt driven
  while(1) {
    if(barcode_ready==1){
      lcd_stringout(barcode);
      barcode_ready=0;
    }
  }
}