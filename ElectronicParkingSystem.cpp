/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "TextLCD_I2C.h"
#include <chrono>

#define Button1 PB_4
#define Button2 PB_5

DigitalOut Rows[4] = {DigitalOut(PA_5), DigitalOut(PA_6), DigitalOut(PA_7), DigitalOut(PB_6)};

DigitalIn Cols[4] = {DigitalIn(PC_7), DigitalIn(PA_9), DigitalIn(PA_8), DigitalIn(PB_10)};

Timer ButtonTimer;
Timer ParkingTimer;

TextLCD_I2C lcd(PB_9, PB_8);

BusOut Segments1(PC_10, PC_12, PA_13, PA_14, PA_15, PC_13, PB_7);
BusOut Segments2(PC_11, PC_2, PC_3, PA_0, PA_1, PC_1, PC_0);

InterruptIn Button_1(Button1);
InterruptIn Button_2(Button2);

DigitalOut LEDG(PB_15);
DigitalOut LEDY(PB_14);
DigitalOut LEDR(PB_13);

char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

using namespace std::chrono; 
int LEDS[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
int Count = 0;
int Digit1 = 0;
int Digit2 = 0;
int ParkingSpace = 0;

bool CarEntering = false;
bool CarExiting = false;

typedef struct{
    int TicketNumber;
    uint32_t time;
    bool isParked; 
}ParkingSpot;

ParkingSpot parkingLot[30];

void clearLCD() {
    lcd.locate(0, 0);
    lcd.printf("                "); 
    lcd.locate(0, 1);
    lcd.printf("                "); 

    lcd.locate(0, 0);
}

void wait(float x)
{
  wait_us(x*1000000);
}

int readKeypad() {
    for (int r = 0; r < 4; r++) {
        Rows[r] = 0;
        
        for (int c = 0; c < 4; c++) {
            if (Cols[c] == 0) { 
                wait(0.2); 
                Rows[r] = 1; 
                if (keyMap[r][c] == 'D')
                    return -3; 
            
                if(keyMap[r][c] == 'A'||keyMap[r][c] == 'B'||keyMap[r][c] == 'C')
                    return -1;

                if(keyMap[r][c] == '#')
                    return keyMap[r][c];

                return keyMap[r][c] - '0'; 
            }
        }
        Rows[r] = 1; 
    }
    return -2;
}

int key = readKeypad();

void Entering() {
    for (int i = 0; i < 30; i++) {
        if (parkingLot[i].isParked == false) {
            
            parkingLot[i].isParked = true;
            parkingLot[i].time = duration_cast<milliseconds>(ParkingTimer.elapsed_time()).count();

            parkingLot[i].TicketNumber = i + 1;
            CarEntering = false;
            lcd.locate(0, 0); 
            clearLCD(); 
            lcd.printf("Parking Ticket:\n%d", i + 1); 
            wait(1);
            clearLCD();
            return; 
        }
    }
}

void Exiting(){
    CarExiting = false;
    bool firstRound = true;

    clearLCD();
    wait(1);
    int enteredTicket = 0; 

while(1) {
    int key = readKeypad(); 
    if (firstRound) {
        clearLCD();
        lcd.printf("Ticket Number:");
        firstRound = false;
    }
    if (key != -2) { 
        if (key >= 0 && key <= 9 ) {
            if(enteredTicket < 10){
            enteredTicket = (enteredTicket * 10) + key;
            lcd.locate(0, 1);
            lcd.printf("Ticket: %d    ", enteredTicket);
            }
        }

        else if (key == -3){
            enteredTicket = enteredTicket / 10;
            lcd.locate(0, 1);
            lcd.printf("Ticket: %d    ", enteredTicket);
        }

        else if (key == 35) { 
            bool found = false;
            for (int i = 0; i < 30; i++) {
                if (parkingLot[i].isParked == true && parkingLot[i].TicketNumber == enteredTicket) {
                    
                    uint32_t timetaken = (duration_cast<milliseconds>(ParkingTimer.elapsed_time()).count() - parkingLot[i].time)/1000;
                    int parkingfee = 0;
                    parkingLot[i].isParked = false;
                    clearLCD();
                    lcd.printf("Ticket Valid!");
                    wait(1);
                    clearLCD();
                    if(timetaken > 10)
                        parkingfee = 2 + ((timetaken - 10)/5)*1;
                    lcd.printf("Fee is RM%d.00",parkingfee);
                    while(1){
                        key = readKeypad();
                        if (key != -2 ) { 
                            if (key == 35) {
                                clearLCD();
                                lcd.printf("Payment Success");
                                wait(1);
                                break;
                            }
                        }
                    }
                    
                    found = true;
                    enteredTicket = 0;
                    
                    return; 
                }                       
            
            }
            
            
            if (!found) {
                clearLCD();
                ThisThread::sleep_for(50ms); 

                lcd.printf("Invalid Ticket");
                enteredTicket = 0;
                wait(1);
                firstRound = true;
                continue;
                }
            
            }
        }
    
    ThisThread::sleep_for(10ms);
    }
}

void ISR()
{
    if (duration_cast<milliseconds>(ButtonTimer.elapsed_time()).count() > 1200) {
        Count++;
        CarEntering = true;
        ButtonTimer.reset(); 
    }
}

void ISR2()
{
    if (duration_cast<milliseconds>(ButtonTimer.elapsed_time()).count() > 1200) {
        Count--;
        CarExiting = true;
        ButtonTimer.reset(); 
    }
}

void CountDisplay()
{
    if (Count < 0) Count = 0;
        
        if (Count >30) Count = 30;
        
        ParkingSpace = 30 - Count;

        if (ParkingSpace < 10){
            Digit1 = ParkingSpace;
            Digit2 = 0;
        }
        else if(ParkingSpace < 20){
            Digit1 = ParkingSpace - 10;
            Digit2 = 1;
        }
        else if(ParkingSpace < 30){
            Digit1 = ParkingSpace - 20;
            Digit2 = 2;
        }
        else {
            Digit1 = 0;
            Digit2 = 3;
        }

        if(ParkingSpace == 0){
            LEDR = 1;
            LEDY = 0;
            LEDG = 0;
        }

        else if(ParkingSpace < 15){
            LEDR = 0;
            LEDY = 1;
            LEDG = 0;
        }
        
        else{
            LEDR = 0;
            LEDY = 0;
            LEDG = 1;
        }
        

        Segments1 = LEDS[Digit2]; 
        Segments2 = LEDS[Digit1];
        clearLCD();
        ThisThread::sleep_for(50ms); 
        if(ParkingSpace == 0)
            lcd.printf("Parking Full!"); 
        else
            lcd.printf("Capacity:%d/30   ", Count); 
}


int main()
{

    ButtonTimer.start();
    ParkingTimer.start();

    ThisThread::sleep_for(50ms); 

    for(int i = 0; i < 4; i++) {
        Cols[i].mode(PullUp);
        Rows[i] = 1; 
        }

    if (lcd.init()) 
        {
        lcd.display(TextLCD_I2C::DISPLAY_ON);
        lcd.setBacklight(true);
    }
    
    Button_1.fall(&ISR); 
    Button_2.fall(&ISR2); 
    lcd.locate(0, 0);
    clearLCD();
    ThisThread::sleep_for(50ms); 
    CountDisplay();

    while(1) 
    {
        if(CarEntering){
            CarEntering = false;
            Entering();             
            CountDisplay();
            }

        if(CarExiting){
            CarExiting = false;
                Exiting();

            CountDisplay();
            }  
    }
}

