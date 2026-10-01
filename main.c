#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include 
#include 

// ==========================================
// MOTOR CONFIGURATION (Port A and Port L)
// ==========================================
void motion_pin_config(void)
{
    DDRA |= 0x0F;  // PA0-PA3 as output (Direction control pins)
    PORTA &= 0xF0; // Initial value set to 0
    DDRL |= 0x18;  // PL3 and PL4 as output for Channel Enable/PWM
    PORTL |= 0x18; // Set initial value of PL3 and PL4 to logic 1 to enable motors
}

// ==========================================
// L293D DIRECTION FUNCTIONS
// ==========================================
void forward(void)
{
    PORTA &= 0xF0; 
    PORTA |= 0x06; // PA1 (LF) and PA2 (RF) HIGH
}

void left(void)
{
    PORTA &= 0xF0; 
    PORTA |= 0x05; // PA0 (LB) and PA2 (RF) HIGH (Pivot Left)
}

void right(void)
{
    PORTA &= 0xF0; 
    PORTA |= 0x0A; // PA1 (LF) and PA3 (RB) HIGH (Pivot Right)
}

void stop(void)
{
    PORTA &= 0xF0; // All direction pins LOW
}

// ==========================================
// FIGURE-8 TRAVERSAL LOGIC
// ==========================================
void traverse_8_shape(void)
{
    unsigned char i; // Declared outside the loop to fix GitHub compiler errors

    // Loop 1: Draw the first half of the '8' (Counter-Clockwise Square)
    for (i = 0; i < 4; i++)
    {
        forward();
        _delay_ms(1500); 
        
        stop();
        _delay_ms(300);  
        
        left();
        _delay_ms(700);  
        
        stop();
        _delay_ms(300);  
    }

    // Loop 2: Draw the second half of the '8' (Clockwise Square)
    for (i = 0; i < 4; i++)
    {
        forward();
        _delay_ms(1500); 
        
        stop();
        _delay_ms(300);  
        
        right();
        _delay_ms(700);  
        
        stop();
        _delay_ms(300);  
    }
}

// ==========================================
// MAIN EXECUTABLE
// ==========================================
int main(void)
{
    motion_pin_config();
    
    // Initial delay before starting
    _delay_ms(1000);
    
    traverse_8_shape();

    // Lock the robot in place when finished
    while (1)
    {
        stop();
    }

    return 0;
}
