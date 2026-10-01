#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include "avr/io.h"
#include "util/delay.h"

// ==========================================
// SAFE DELAY WRAPPER (Fixes compiler limits)
// ==========================================
void safe_delay_ms(unsigned int delay_time)
{
    unsigned int j;
    for (j = 0; j < delay_time; j++)
    {
        _delay_ms(1); 
    }
}

// ==========================================
// MOTOR CONFIGURATION (Port A and Port L)
// ==========================================
void motion_pin_config(void)
{
    DDRA |= 0x0F;  
    PORTA &= 0xF0; 
    DDRL |= 0x18;  
    PORTL |= 0x18; 
}

// ==========================================
// L293D DIRECTION FUNCTIONS
// ==========================================
void forward(void)
{
    PORTA &= 0xF0; 
    PORTA |= 0x06; 
}

void left(void)
{
    PORTA &= 0xF0; 
    PORTA |= 0x05; 
}

void stop(void)
{
    PORTA &= 0xF0; 
}

// ==========================================
// SQUARE TRAVERSAL LOGIC
// ==========================================
void traverse_square(void)
{
    unsigned char i; 

    // A square has 4 equal sides and 4 90-degree turns
    for (i = 0; i < 4; i++)
    {
        forward();
        safe_delay_ms(600);  // Short forward burst for the side of the square
        
        stop();
        safe_delay_ms(100);  // Brief pause to prevent skidding
        
        left();
        safe_delay_ms(400);  // 90-degree pivot left (Adjust this if it under/over turns)
        
        stop();
        safe_delay_ms(100);  // Brief pause to stabilize before the next side
    }
}

// ==========================================
// MAIN EXECUTABLE
// ==========================================
int main(void)
{
    motion_pin_config();
    
    // Initial delay before the robot starts moving
    safe_delay_ms(500);
    
    traverse_square();

    // Lock the robot in place when finished
    while (1)
    {
        stop();
    }

    return 0;
}
