#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h" 

void buzzer_pin_config(void)
{
    DDRC |= (1 << PC3);   // Set Port C Pin 3 as output for the Buzzer
    PORTC &= ~(1 << PC3); // Ensure the buzzer is initially OFF
}

void boot_switch_pin_config(void)
{
    DDRE &= ~(1 << PE7); // Set Port E Pin 7 as input for the switch
    PORTE |= (1 << PE7); // Enable internal pull-up resistor on PE7
}

void port_init(void)
{
    lcd_port_config();
    buzzer_pin_config();
    boot_switch_pin_config();
}

int main(void)
{
    port_init();
    lcd_init();

    while (1)
    {
        // Check if boot switch is pressed (Active-Low logic reads 0 when pressed)
        if (!(PINE & (1 << PE7)))
        {
            // --- SWITCH IS PRESSED ---
            PORTC |= (1 << PC3); // Turn buzzer ON
            
            // Padded with 3 spaces to fully overwrite the 12 characters of "Mid Lab Exam"
            lcd_string(1, 1, "240102059   "); 
        }
        else
        {
            // --- SWITCH IS NOT PRESSED ---
            PORTC &= ~(1 << PC3); // Turn buzzer OFF
            
            lcd_string(1, 1, "Mid Lab Exam");
        }
        
        _delay_ms(50); // Debounce and refresh delay
    }

    return 0;
}
