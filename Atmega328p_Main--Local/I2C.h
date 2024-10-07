#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void i2c_init(uint8_t);
uint8_t i2c_io(uint8_t, uint8_t *, uint16_t, uint8_t *, uint16_t);

void sci_init(uint8_t);

// Find divisors for the UART0 and I2C baud rates
#define FOSC 7372800            // Clock frequency = Oscillator freq.
#define BAUD 9600               // UART0 baud rate
#define MYUBRR FOSC/16/BAUD-1   // Value for UBRR0 register
#define BDIV (FOSC / 100000 - 16) / 2 + 1    // Puts I2C rate just below 100kHz

