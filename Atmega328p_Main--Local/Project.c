/*********************************************************************
*           EE 459 Embeded System Laboratory
*                   Spring 2024
*   Created by Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
*                   05/06/2024
* 
*********************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "UART.h"
#include "lcd.h"
#include "I2C.h"
#include "keypad.h"

#define MAX_BARCODE_LEN 25
#define CARRIAGE_RETURN 0x0D // ASCII value for carriage return
#define Tempsensor_ADDR 0x90 // Define address of the temperature sensor
#define MAX_TEMPERATURE_LEN 5 // max for temp0erature buffer length


//variable used in barcode interrupt 
volatile char barcode[MAX_BARCODE_LEN + 1];
volatile int barcode_count = 0; // counter for counting barcode character
volatile int barcode_print = 0; // flag for barcode display 
volatile int barcode_ready = 0; // used for spi not yet used in this program
volatile int barcode_date = 0; // flag for going to entering date state 

///////////////SPI variable 
volatile char Send_message[24];
volatile int message_ready = 0;
volatile char send_buffer[MAX_BARCODE_LEN + 1];
volatile int send_index = 0;
volatile int send_flag = 0;

////SPI TEMP
char temp_buffer[MAX_BARCODE_LEN];
volatile int temp_index = 0;
volatile temp_high = 0;
volatile int count = 0;
volatile char temp_test[2];




// Receiving the item information
#define MAX_BARCODE_BACK 23
volatile char response_buffer[MAX_BARCODE_BACK];
volatile int response_index = 0;
volatile int response_complete = 0;
volatile char received_message[23];
// variable for volatile temperature
volatile int temperature;

// Setup SPI
void SPI_SlaveInit(void) {
  DDRB |= (1<<4);  // MISO as OUTPUT
  SPCR |= ((1<<SPE) | (1<<SPIE));  // Enable SPI and SPI interrupt
}


//state declaration
volatile enum states {Add, Delete, Temperature_Display, Item_Info, Exp_date, Date_disp, Location};
volatile int state = Add;

// defining struct type called product 
struct Product{
    int item_num;
    char barcode[14];
    char info[20];
    char exp_date[11];
    char loc_fridge[20];
};

//hard code the differnet product 
struct Product Butter = {0, "641548121712", "Asiago Chive", "xxxx-xx-xx", "Butter: freezer"}; // to be continue
struct Product AZ_tea = {0, "613008715267", "Arizona Tea", "xxxx-xx-xx", "Tea: fridge"};
struct Product Espresso = {0, "810019263231", "Black Rifle Coffee", "xxxx-xx-xx", "Coffee: frige"};
struct Product Biryani = {0, "788821122124", "Shan Biryani", "xxxx-xx-xx", "Biryani: fridge"};


int getTemperature() // function that returns the temperature 
{
    uint8_t wdata[20];
    uint8_t rdata[20];
    uint8_t status;
    int temperature;

    // Set the sensor to read temperature
    wdata[0] = 0xAA;                                      // Command to read temperature
    status = i2c_io(Tempsensor_ADDR, wdata, 1, rdata, 2); // Assuming Tempsensor_ADDR is defined as before

    unsigned int c2 = rdata[0] * 2;
    if (rdata[1] != 0)
    {
        c2++;
    }
    temperature = (c2 * 9) / 10 + 32; // Convert to Fahrenheit 
    return temperature;
}

void displayTemperature()
{
    int temperature = getTemperature(); // Get the current temperature from the sensor
    char displayBuffer[32];

    if (temperature < 100)
    {                        // Check if reading was successful
        lcd_writecommand(1); // Clear display
        lcd_writecommand(2);
        sprintf(displayBuffer, "Temperature: %dF", temperature);
        lcd_stringout(displayBuffer);
        
    }
    else
    {
        // Handle the error, maybe display an error message
        lcd_writecommand(1); // Clear display
        lcd_writecommand(2);
        sprintf(displayBuffer, "Temperature: %dF, too high!", temperature);
        lcd_stringout(displayBuffer);
        // Decide whether to retry or transition to another state
    }
}
void displayWelcome()
{
    lcd_writecommand(1); // Clear display
    lcd_writecommand(2);
    lcd_stringout("Welcome to FridgeHub");
    _delay_ms(2000);                          // Display for 2 seconds
}
void temp_LED_init()
{
    DDRD |= (1<<1); //set the TXD port to output for temp_LED
}
void combineBuffers(char* buffer1, char* buffer2, char* combinedBuffer) {
    // Copy the contents of buffer1 to combinedBuffer
    strcpy(combinedBuffer, buffer1);

    // Find the length of buffer1
    size_t len1 = strlen(buffer1);

    // Add a comma as a separator after buffer1
    combinedBuffer[len1] = ',';
    combinedBuffer[len1 + 1] = '\0'; // Null-terminate after the comma

    // Concatenate buffer2 to combinedBuffer after the comma
    strcat(combinedBuffer, buffer2);
}
bool isDigit(char c) {
    return ((c >= '0' && c <= '9')||c=='-');
}
void splitString(char* A, char* str_1, char* date) {
    size_t len = strlen(A);
    size_t i;

    // Find the index of the first character that is not a digit
    for (i = 0; i < len; i++) {
        if (!isDigit(A[i])) {
            break;
        }
    }
    // Copy the characters before the first non-digit to str_1
    strncpy(str_1, A, i);
    str_1[i] = '\0';  // Null-terminate

    // Copy the remaining characters to date
    strcpy(date, A + i+1);
}

// SPI Serial Transfer Complete ISR
ISR(SPI_STC_vect) {

    /////////Receiving the stuff from RPI
    char received = SPDR;  // Read the received data first, crucial for full-duplex SPI

    // Process the received data
    if (received == '\0') 
    {
        //lcd_stringout("B");
        if(response_index > 0)
        {
            //lcd_stringout("A");
            response_buffer[response_index] = '\0'; // Null-terminate the string
            response_complete = 1;
            strcpy(received_message, response_buffer);
            response_index = 0;  // Reset index for next message
        }
    } 
    
   //else if (received != 0xFF) {  // Ignore the dummy bytes

    else if(received != 0xFF) 
    {
        if (response_index < MAX_BARCODE_BACK)
            response_buffer[response_index++] = received;
    }
    
    /////////Sending from MCU to RPI
    // Let's prioritize temp

    // Add Item to APP
    if (message_ready) {
        strncpy(send_buffer, Send_message, MAX_BARCODE_LEN - 1);
        send_buffer[MAX_BARCODE_LEN - 1] = '\0'; // Correctly terminate within bounds
        //barcode_ready = 0;
        send_index = 0;
        message_ready = 0;
    }

    // Delete Item on APP
     else if (barcode_ready && (state == Delete)) {
        // Place '!' at the start of the send_buffer
        
        send_buffer[0] = '!';
        
        strncpy(send_buffer + 1, barcode, MAX_BARCODE_LEN - 1);

        send_buffer[13] = '\0';

        barcode_ready = 0;
        send_index = 0;
    }

    if (temp_high && count == 0) {
        //send_buffer[1] = 'T';  // Properly use the first character for 'T'
        //snprintf(send_buffer + 2, MAX_TEMPERATURE_LEN, "%02d", temperature); // Ensure snprintf does not overflow
        // sprintf(temp_test, "%d", temperature);
        // strcpy(send_buffer+1, "temperature is high");

        strcpy(send_buffer, "   Temperature: "); // Copy the initial part of the message
        int length = strlen(send_buffer); // Get the length of the string already in send_buffer
        sprintf(send_buffer + length, "%d", temperature); // Append the temperature

        //  // Copy the initial part of the message
        // send_buffer[0] = '?';
        // int length = strlen(send_buffer); // Get the length of the string already in send_buffer
        // sprintf(send_buffer + length, "%d", temperature); // Append the temperature

        send_buffer[20] = '\0'; // Correctly terminate within bounds
        //temp_high = 0;
        count ++;
        send_index = 0;
        barcode_date = 0;
    }


    if (send_index < MAX_BARCODE_LEN) {
        SPDR = send_buffer[send_index++];
    }
    
    else {

        SPDR = '\0';  // End transmission if nothing to send
        memset(send_buffer, 0x01, sizeof(send_buffer));  // Clear the buffer after the message is completely sent
        send_index = 0;  // Reset index for next message
        count = 0;
        
    }

}

/////////////////////




ISR(USART_RX_vect)
{
    char rx_char = UDR0; // Read the received data

    if (rx_char == CARRIAGE_RETURN)
    {
        if (barcode_count > 0)
        {
            barcode[barcode_count] = '\0'; // Null-terminate the string
            barcode_ready = 1;     // Set the flag indicating that a full barcode is ready
            barcode_print = 1;     // the barcode is ready to print  
            barcode_date = 1;      // set flag to enter the entering date state
            barcode_count = 0;             // Prepare for the next barcode
        }
    }
    else if (barcode_count < MAX_BARCODE_LEN)
    {
        barcode[barcode_count++] = rx_char; // Store the character
    }
}

int main(void)
{
    lcd_init();              // Initialize the LCD
    lcd_writecommand(1);     // Clear LCD
    i2c_init(BDIV);          // Initialize the I2C port
    serial_init(UBRR_VALUE); // Initialize the UART for RX interrupts
    keypad_init();           // Keypad initialization
    SPI_SlaveInit();  // Initialize as SPI slave
    sei();            // Enable global interrupts
    // write and read buffer for I2C
    uint8_t wdata[20], rdata[20]; 
    // define variable for I2C
    uint8_t status;
    //variable for the scanning of the keypad
    char key;
    // // variable for temperature
    // int temperature;
    // counter for temperature display so the screen would not flash too often 
    int disp_count = 0;
    // flag for printing
    int flag_print = 0;
    // counter for moving the cursor 
    int enter_track;
    // row and col for display
    int row;
    int col;
    // item processing number 
    int item_processing = 0;
    // temporary barcode and date to compare from the receving message
    char temp_barcode[13];
    char temp_date[11];
    // Temperature sensor initialization
    wdata[0] = 0xAC; // Set config for active high = 1 and continuous acquisitions
    status = i2c_io(Tempsensor_ADDR, wdata, 2, NULL, 0);
    wdata[0] = 0x51; // Start conversions
    status = i2c_io(Tempsensor_ADDR, wdata, 1, NULL, 0);
    // welcome message
    displayWelcome();
    // temperature LED initializaton
    temp_LED_init();
    

    while(1){
        // emergengy light logic
        temperature = getTemperature();
        if(temperature>80){
            PORTD |= (1<<1);
        }
        else{
            PORTD &= ~(1<<1);
        }
        //remotely add and remove logic

        if (temperature > 80) {
            temp_high = 1;  // Set the temperature alert flag
            // lcd_moveto(1, 0);
            // lcd_stringout("Temp above 80F!");
        } else {
            temp_high = 0;  // Clear the flag
        }
        

        if(response_complete){ 
            response_complete=0; // clear the flag 
            
            if(received_message[0]== '!'){ // haven't declear received_message // if remove
                
               
               
                if(strcmp(received_message+1, Butter.barcode) == 0){
                    
                    Butter.item_num--;
                    if(Butter.item_num < 0)
                    {
                        Butter.item_num = 0;
                    }
                }
                else if(strcmp(received_message+1, AZ_tea.barcode) == 0){
                    AZ_tea.item_num--;

                    if(AZ_tea.item_num < 0)
                    {
                        AZ_tea.item_num = 0;
                    }
                }
                else if(strcmp(received_message+1, Espresso.barcode) == 0){
                    Espresso.item_num--;
                    if(Espresso.item_num < 0)
                    {
                        Espresso.item_num = 0;
                    }
                }
                else if(strcmp(received_message+1, Biryani.barcode) == 0){
                    Biryani.item_num--;
                    if(Biryani.item_num < 0)
                    {
                        Biryani.item_num = 0;
                    }
                }
            }

            else{  // if add
                splitString(received_message,temp_barcode,temp_date);
                //testing purpose
                // lcd_moveto(1,0);
                // lcd_stringout(received_message);
                //lcd_moveto(2,0);
                //lcd_stringout(temp_barcode);
                //lcd_moveto(3,0);
                //lcd_stringout(temp_date);
                ////////////
                if(strcmp(temp_barcode, Butter.barcode) == 0){
                    Butter.item_num++;
                    strcpy(Butter.exp_date, temp_date);
                }
                else if(strcmp(temp_barcode, AZ_tea.barcode) == 0){
                    AZ_tea.item_num++;
                    strcpy(AZ_tea.exp_date, temp_date);
                }
                else if(strcmp(temp_barcode, Espresso.barcode) == 0){
                    Espresso.item_num++;
                    strcpy(Espresso.exp_date, temp_date);
                }
                else if(strcmp(temp_barcode, Biryani.barcode) == 0){
                    Biryani.item_num++;
                    strcpy(Biryani.exp_date, temp_date);
                }
            }
        }

        // state machine
        if(state==Add){
            //functional logic
            if(flag_print==0){ 
            flag_print=1;
            lcd_writecommand(1);
            lcd_writecommand(2);
            lcd_stringout("Storing: ");
            }
        // //for testing 
        //     if (response_complete) {
        //         //lcd_stringout("A");
        //         lcd_moveto(1,0);
        //         lcd_stringout(response_buffer);  // Display response on LCD
        //         response_complete = 0;  // Clear the flag
        //         splitString(response_buffer, temp_barcode, temp_date);
        //         lcd_moveto(2,0);
        //         lcd_stringout(temp_barcode);
        //         lcd_moveto(3,0);
        //         lcd_stringout(temp_date);
        //     }

            // if(message_ready){
            //     lcd_stringout(Send_message);
            //     message_ready=0;
            // }
            if(barcode_print){
                // comparing the scanning barcode with our databse and add quantity accordingly 
                if(strcmp(barcode, Butter.barcode) == 0){
                    Butter.item_num++;
                    item_processing = 1;
                }
                else if(strcmp(barcode, AZ_tea.barcode) ==0 ){
                    AZ_tea.item_num++;
                    item_processing = 2;
                }
                else if(strcmp(barcode, Espresso.barcode) == 0){
                    Espresso.item_num++;
                    item_processing = 3;
                }
                else if(strcmp(barcode, Biryani.barcode) == 0){
                    Biryani.item_num++;
                    item_processing = 4;
                }
                //print out the barcode
                lcd_moveto(1,0);
                lcd_stringout(barcode);
                barcode_print=0;
            }

            //transition logic
            if(barcode_date==1){
                _delay_ms(200);
                state = Exp_date;
                flag_print = 0;
                key = 'A';
            }
            
            key = keypad_scan();
            if (key=='2'){
                state = Delete; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='3'){
                state = Temperature_Display; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='4'){
                state = Item_Info; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            } 
            if (key=='5'){
                state = Date_disp; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='6'){
                state = Location; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
        }
/////////////////////////////////////////////////////////////
        if(state==Delete){
            //functional logic
            if(flag_print==0){ 
            flag_print=1;
            lcd_writecommand(1);
            lcd_writecommand(2);
            lcd_stringout("Removing: ");
            }
            if(barcode_print){
                // comparing the scanning barcode with our databse
                if(strcmp(barcode, Butter.barcode) == 0){
                    Butter.item_num--;
                }
                else if(strcmp(barcode, AZ_tea.barcode) == 0){
                    AZ_tea.item_num--;
                }
                else if(strcmp(barcode, Espresso.barcode) == 0){
                    Espresso.item_num--;
                }
                else if(strcmp(barcode, Biryani.barcode) == 0){
                    Biryani.item_num--;
                }
                //print out the barcode
                lcd_moveto(1,0);
                lcd_stringout(barcode);
                barcode_print=0;
            }
            //transition logic
            key = keypad_scan();
            if (key=='1'){
                state = Add; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                barcode_date = 0;
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='3'){
                state = Temperature_Display; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='4'){
                state = Item_Info; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='5'){
                state = Date_disp; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='6'){
                state = Location; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
        }
//////////////////////////////////////////////////////////
        if (state==Temperature_Display){
            //functional logic
            if(flag_print==0){
                flag_print=1;
                lcd_writecommand(1);
                lcd_writecommand(2);
                displayTemperature();
            }
            else{
                if(disp_count>=20){  // prevent temperature from flashing 
                    lcd_writecommand(1);
                    lcd_writecommand(2);
                    displayTemperature();
                    disp_count=0;
                }
                else{
                disp_count++;
                }
            }
            //transition logic
            key = keypad_scan();
            if (key=='1'){
                state = Add; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                barcode_date = 0;
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='2'){
                state = Delete; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='4'){
                state = Item_Info; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='5'){
                state = Date_disp; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='6'){
                state = Location; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
        }
///////////////////////////////////////////////////////////////////////////////////////
        if (state==Item_Info){
            //functional logic
            if(flag_print==0){
                flag_print=1;
                lcd_writecommand(1);
                lcd_writecommand(2);
            }
            row = 0;
            col = 0;
            char Item_num[2];
            if(disp_count>=20){
                if(Butter.item_num>0){ // printing out the item information if there is no item in stock then skip it 
                    lcd_moveto(row,col);
                    sprintf(Item_num, "%d", Butter.item_num);
                    lcd_stringout(Item_num);
                    lcd_stringout(" ");
                    lcd_stringout(Butter.info);
                    row++;
                }
                if(AZ_tea.item_num>0){
                    lcd_moveto(row,col);
                    sprintf(Item_num, "%d", AZ_tea.item_num);
                    lcd_stringout(Item_num);
                    lcd_stringout(" ");
                    lcd_stringout(AZ_tea.info);
                    row++;
                }
                if(Espresso.item_num>0){
                    lcd_moveto(row,col);
                    sprintf(Item_num, "%d", Espresso.item_num);
                    lcd_stringout(Item_num);
                    lcd_stringout(" ");
                    lcd_stringout(Espresso.info);
                    row++;
                }
                if(Biryani.item_num>0){
                    lcd_moveto(row,col);
                    sprintf(Item_num, "%d", Biryani.item_num);
                    lcd_stringout(Item_num);
                    lcd_stringout(" ");
                    lcd_stringout(Biryani.info);
                    row++;
                }
                while(row<4){
                    lcd_moveto(row,col);
                    lcd_stringout("                    ");
                    row++;
                }
                disp_count=0;
            }
            else{
                disp_count++;
            }
            //transition logic
            key = keypad_scan();
            if (key=='1'){
                state = Add; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                barcode_date = 0;
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='2'){
                state = Delete; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='3'){
                state = Temperature_Display; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='5'){
                state = Date_disp; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='6'){
                state = Location; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
        }
//////////////////////////////////////////////////////////////////////////////
        if (state==Exp_date){
            //functional logic 
            if(flag_print==0){
                enter_track=0;
                flag_print=1;
                barcode_date=0;
                lcd_writecommand(1);
                lcd_writecommand(2);
                lcd_stringout("Year: ");
                lcd_moveto(1,0);
                lcd_stringout("Month: ");
                lcd_moveto(2,0);
                lcd_stringout("Day: ");
                lcd_moveto(0,6);
            }
             
            while(enter_track<10){
                key = keypad_scan();
                _delay_ms(200);
                
                if (key >= '0' && key <= '9'){
                    //lcd_stringout("1");
                    // adjust the cursor for the formatting 
                    if(enter_track==4){
                        enter_track++;
                        lcd_moveto(1,7);
                        lcd_writedata(key);
                    }
                    else if(enter_track==7){
                        enter_track++;
                        lcd_moveto(2,6); 
                        lcd_writedata(key);      
                    }
                    else{
                        lcd_writedata(key); 
                    }
                    
                    // compare the barcode so the program knows which exp.date to enter 
                    if(strcmp(barcode, Butter.barcode) == 0){
                        Butter.exp_date[enter_track] = key;
                        
                    }
                    else if(strcmp(barcode, AZ_tea.barcode) == 0){
                            AZ_tea.exp_date[enter_track] = key;
                    }
                    else if(strcmp(barcode, Espresso.barcode) == 0){
                            Espresso.exp_date[enter_track] = key;
                    }
                    else if(strcmp(barcode, Biryani.barcode) == 0){
                            Biryani.exp_date[enter_track] = key;
                    }
                    enter_track++;
                }
                
            }
            //transition logic 
            key = keypad_scan();
            if (key=='*'){
                state = Add; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                barcode_date = 0;
                enter_track=0; // clear the enter_track here so that the program would not enter the while loop above
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
                if(item_processing==1){  // combining the message 
                    combineBuffers(barcode,Butter.exp_date, Send_message);
                    item_processing=0;
                }
                else if(item_processing==2){
                    combineBuffers(barcode,AZ_tea.exp_date, Send_message);
                    item_processing=0;
                }
                else if(item_processing==3){
                    combineBuffers(barcode,Espresso.exp_date, Send_message);
                    item_processing=0;
                }
                else if(item_processing==4){
                    combineBuffers(barcode,Biryani.exp_date, Send_message);
                    item_processing=0;
                }
                message_ready=1;
                barcode_ready=0;
            }
        }
/////////////////////////////////////////////////////////////
        if (state==Date_disp){
            //functional logic
            if(flag_print==0){
                flag_print=1;
                lcd_writecommand(1);
                lcd_writecommand(2);
                row = 0;
                col = 0;
                
                if(Butter.item_num>0){ // printing out the item information if there is no item in stock then skip it 
                    lcd_moveto(row,col);
                    lcd_stringout("Butter: ");
                    lcd_stringout(Butter.exp_date);
                    row++;
                }
                if(AZ_tea.item_num>0){
                    lcd_moveto(row,col);
                    lcd_stringout("TEA: ");
                    lcd_stringout(AZ_tea.exp_date);
                    row++;
                }
                if(Espresso.item_num>0){
                    lcd_moveto(row,col);
                    lcd_stringout("Coffee: ");
                    lcd_stringout(Espresso.exp_date);
                    row++;
                }
                if(Biryani.item_num>0){
                    lcd_moveto(row,col);
                    lcd_stringout("Biryani: ");
                    lcd_stringout(Biryani.exp_date);
                    row++;
                }
                row = 0;
            }

            //transitional logic
            key = keypad_scan();
            if (key=='1'){
                state = Add; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                barcode_date = 0;
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='2'){
                state = Delete; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='3'){
                state = Temperature_Display; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='4'){
                state = Item_Info; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='6'){
                state = Location; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
        }

        if (state==Location){
            //functional logic
            if(flag_print==0){
                flag_print=1;
                lcd_writecommand(1);
                lcd_writecommand(2);
                row = 0;
                col = 0;
                
                if(Butter.item_num>0){ // printing out the item information if there is no item in stock then skip it 
                    lcd_moveto(row,col);
                    lcd_stringout(Butter.loc_fridge);
                    row++;
                }
                if(AZ_tea.item_num>0){
                    lcd_moveto(row,col);
                    lcd_stringout(AZ_tea.loc_fridge);
                    row++;
                }
                if(Espresso.item_num>0){
                    lcd_moveto(row,col);
                    lcd_stringout(Espresso.loc_fridge);
                    row++;
                }
                if(Biryani.item_num>0){
                    lcd_moveto(row,col);
                    lcd_stringout(Biryani.loc_fridge);
                    row++;
                }
                row = 0;
            }

            //transitional logic
            key = keypad_scan();
            if (key=='1'){
                state = Add; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                barcode_date = 0;
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='2'){
                state = Delete; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='3'){
                state = Temperature_Display; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='4'){
                state = Item_Info; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
            if (key=='5'){
                state = Date_disp; 
                flag_print = 0; // clear the flag_print so the next state could print out the format
                key = 'A'; // the value of the key should not be '1' to '9' after pressing the button
            }
        }

    }
    return 0;


}





