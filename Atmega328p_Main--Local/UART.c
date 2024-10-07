/*********************************************************************
*       EE 459 Embeded System Laboratory
*               Spring 2024
*   Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*               05/06/2024
* 
*********************************************************************/

/*
serial_init - Initialize the USART port
EE 459Lx
*/
#include <avr/io.h>
#include <util/delay.h>
#include "UART.h"
#include <avr/interrupt.h>  

//For receiving data, the program monitors the RXC0 (Receive Complete) bit in status register A. When
//it becomes a one, the program then reads the received byte from the data register, which also clears the
//RXC0 bit.
/*
serial_in - Read a byte from the USART0 and return it
*/
char serial_receive(void) {
    // Wait for the receive complete flag to be set
    while (!(UCSR0A & (1 << RXC0))) {
        ; // Do nothing until data is received
    }
    return UDR0; // Return the received data
}

void serial_init(unsigned short ubrr) {
    UBRR0 = ubrr;  // Set baud rate
    UCSR0B |= (1 << RXEN0) | (1 << RXCIE0); // Enable receiver and RX Complete interrupt
    UCSR0C = (3 << UCSZ00); // Set frame format: 8 data, no parity, 1 stop bit
    sei(); // Enable global interrupts
}



//new
int serial_available(void) {
    return (UCSR0A & (1 << RXC0));
}


