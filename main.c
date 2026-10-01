#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include 
#include 

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

void right(void)
{
    PORTA &= 0xF0; 
    PORTA |= 0x0A; 
}

void stop(void)
{
    PORTA &= 0xF0; 
}

// ==========================================
// FIGURE-8 TRAVERSAL LOGIC
// ==========================================
void traverse_8_shape(void)
{
    unsigned char i; 

    // Loop 1: Draw the first half of the '8' (Counter-Clockwise Square)
    for (i = 0; i < 4; i++)
    {
        forward();
        safe_delay_ms(500); 
        
        stop();
        safe_delay_ms(100); 
        
        left();
        safe_delay_ms(400); 
        
        stop();
        safe_delay_ms(100);  
    }

    // Loop 2: Draw the second half of the '8' (Clockwise Square)
    for (i = 0; i < 4; i++)
    {
        forward();
        safe_delay_ms(500); 
        
        stop();
        safe_delay_ms(100);  
        
        right();
        safe_delay_ms(400); 
        
        stop();
        safe_delay_ms(100);  
    }
}

// ==========================================
// MAIN EXECUTABLE
// ==========================================
int main(void)
{
    motion_pin_config();
    safe_delay_ms(500);
    
    traverse_8_shape();

    while (1)
    {
        stop();
    }

    return 0;
}
