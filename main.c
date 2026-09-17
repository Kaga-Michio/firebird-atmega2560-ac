#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include <avr/io.h>
#include <util/delay.h>

/* =======================================================
   RAW LCD DRIVER FUNCTIONS (4-BIT MODE)
   RS = PC0, RW = PC1, EN = PC2, Buzzer = PC3, D4-D7 = PC4-PC7
======================================================= */

void lcd_cmd(unsigned char cmd)
{
    // Send upper nibble (Keep PC0-PC3 intact)
    PORTC = (PORTC & 0x0F) | (cmd & 0xF0); 
    PORTC &= ~(1 << PC0); // RS = 0 (Command register)
    PORTC &= ~(1 << PC1); // RW = 0 (Write operation)
    PORTC |= (1 << PC2);  // EN = 1
    _delay_ms(1);
    PORTC &= ~(1 << PC2); // EN = 0
    _delay_ms(1);

    // Send lower nibble
    PORTC = (PORTC & 0x0F) | ((cmd << 4) & 0xF0); 
    PORTC |= (1 << PC2);  // EN = 1
    _delay_ms(1);
    PORTC &= ~(1 << PC2); // EN = 0
    _delay_ms(2);
}

void lcd_char(unsigned char data)
{
    // Send upper nibble (Keep PC0-PC3 intact)
    PORTC = (PORTC & 0x0F) | (data & 0xF0); 
    PORTC |= (1 << PC0);  // RS = 1 (Data register)
    PORTC &= ~(1 << PC1); // RW = 0 (Write operation)
    PORTC |= (1 << PC2);  // EN = 1
    _delay_ms(1);
    PORTC &= ~(1 << PC2); // EN = 0
    _delay_ms(1);

    // Send lower nibble
    PORTC = (PORTC & 0x0F) | ((data << 4) & 0xF0); 
    PORTC |= (1 << PC2);  // EN = 1
    _delay_ms(1);
    PORTC &= ~(1 << PC2); // EN = 0
    _delay_ms(2);
}

void lcd_string(char *str)
{
    while (*str)
    {
        lcd_char(*str++);
    }
}

void lcd_init(void)
{
    DDRC = 0xFF; // Set all PORTC pins as output (LCD + Buzzer)
    _delay_ms(20);
    
    // Standard 4-bit initialization sequence
    lcd_cmd(0x02); // Return Home (Initialize 4-bit mode)
    lcd_cmd(0x28); // 4-bit mode, 2 lines, 5x8 font
    lcd_cmd(0x0C); // Display ON, Cursor OFF
    lcd_cmd(0x06); // Auto-increment cursor
    lcd_cmd(0x01); // Clear display
    _delay_ms(2);
}


/* =======================================================
   MAIN EXECUTABLE
======================================================= */

int main(void)
{
    // Configure Boot Switch on PE7
    DDRE &= ~(1 << PE7); // Set as input
    PORTE |= (1 << PE7); // Enable internal pull-up resistor

    lcd_init();

    while (1)
    {
        // Check if boot switch is pressed (Active-Low logic)
        if (!(PINE & (1 << PE7)))
        {
            // --- SWITCH PRESSED ---
            PORTC |= (1 << PC3); // Turn Buzzer ON
            
            lcd_cmd(0x80); // Move cursor to Row 1, Column 1
            // Padded with spaces to fully overwrite "Mid Lab Exam"
            lcd_string("240102059   "); 
        }
        else
        {
            // --- SWITCH NOT PRESSED ---
            PORTC &= ~(1 << PC3); // Turn Buzzer OFF
            
            lcd_cmd(0x80); // Move cursor to Row 1, Column 1
            lcd_string("Mid Lab Exam");
        }
        
        _delay_ms(50); // Small debounce delay
    }

    return 0;
}
