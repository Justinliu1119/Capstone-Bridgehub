// UART.h
#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <avr/interrupt.h>

// Define constants
#define F_CPU 7372800
#define BAUD 9600
#define UBRR_VALUE ((F_CPU/16/BAUD)-1)

// Function declarations
void serial_init(unsigned short ubrr);
char serial_receive(void);
int serial_available(void);

#endif // UART_H



