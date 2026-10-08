#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include "avr/io.h"
#include "util/delay.h"
#include "math.h" 

// ==========================================
// PORT CONFIGURATION (LCD + Buzzer)
// ==========================================
void port_config(void)
{
    DDRC = 0xFF;   // Set all Port C pins as outputs (LCD + Buzzer on PC3)
    PORTC = 0x00;  // Initial state LOW
}

// ==========================================
// BUZZER BEEP FUNCTION
// ==========================================
void buzzer_beep(void)
{
    PORTC |= (1 << PC3);  // Turn Buzzer ON
    _delay_ms(100);       // Short 100ms beep
    PORTC &= ~(1 << PC3); // Turn Buzzer OFF
}

// ==========================================
// RAW LCD DRIVER FUNCTIONS (4-BIT MODE)
// ==========================================
void lcd_cmd(unsigned char cmd)
{
    PORTC = (PORTC & 0x0F) | (cmd & 0xF0); 
    PORTC &= ~(1 << PC0); // RS = 0 
    PORTC &= ~(1 << PC1); // RW = 0 
    PORTC |= (1 << PC2);  // EN = 1
    _delay_ms(1);
    PORTC &= ~(1 << PC2); // EN = 0
    _delay_ms(1);

    PORTC = (PORTC & 0x0F) | ((cmd << 4) & 0xF0); 
    PORTC |= (1 << PC2);  // EN = 1
    _delay_ms(1);
    PORTC &= ~(1 << PC2); // EN = 0
    _delay_ms(2);
}

void lcd_char(unsigned char data)
{
    PORTC = (PORTC & 0x0F) | (data & 0xF0); 
    PORTC |= (1 << PC0);  // RS = 1 
    PORTC &= ~(1 << PC1); // RW = 0 
    PORTC |= (1 << PC2);  // EN = 1
    _delay_ms(1);
    PORTC &= ~(1 << PC2); // EN = 0
    _delay_ms(1);

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

void lcd_cursor(char row, char column)
{
    if (row == 1) { lcd_cmd(0x80 + column - 1); }
    if (row == 2) { lcd_cmd(0xC0 + column - 1); }
}

// Custom function to extract and print a 3-digit number to the LCD
void lcd_print_number(unsigned int num)
{
    unsigned char hundreds, tens, ones;
    
    if (num > 999) num = 999; // Cap at 999 to prevent overflow
    
    hundreds = (num / 100);
    tens = (num / 10) % 10;
    ones = num % 10;
    
    lcd_char(hundreds + '0'); // Convert to ASCII character
    lcd_char(tens + '0');
    lcd_char(ones + '0');
}

void lcd_init(void)
{
    port_config();
    _delay_ms(20);
    lcd_cmd(0x02); 
    lcd_cmd(0x28); 
    lcd_cmd(0x0C); 
    lcd_cmd(0x06); 
    lcd_cmd(0x01); 
    _delay_ms(2);
}

// ==========================================
// ADC CONFIGURATION
// ==========================================
void adc_pin_config(void)
{
    DDRK = 0x00;   // Set PORTK direction as input (ADC channels 8-15)
    PORTK = 0x00;  // Set PORTK pins floating
}

void adc_init(void)
{
    ADCSRA = 0x00;
    ADCSRB = 0x00;
    ADMUX = 0x20; 
    ACSR = 0x80;
    ADCSRA = 0x86; 
}

unsigned char ADC_Conversion(unsigned char Ch)
{
    unsigned char a;
    if(Ch > 7)
    {
        ADCSRB = 0x08; 
    }
    Ch = Ch & 0x07;
    ADMUX = 0x20 | Ch;      
    ADCSRA = ADCSRA | 0x40; 
    
    while((ADCSRA & 0x10) == 0); 
    
    a = ADCH;               
    ADCSRA = ADCSRA | 0x10; 
    ADCSRB = 0x00;
    return a;
}

// ==========================================
// SHARP IR SENSOR FUNCTIONS
// ==========================================
unsigned char Sharp_sensor_data(void)
{
    // The Front Sharp IR range sensor is connected to ADC channel 11 (PK.3)
    return ADC_Conversion(11); 
}

unsigned int Sharp_GP2D12_estimation(unsigned char adc_reading)
{
    float distance;
    unsigned int distanceInt;
    distance = (int)(10.00 * (2799.6 * (1.00 / (pow(adc_reading, 1.1546)))));
    distanceInt = (unsigned int)distance;
    if(distanceInt > 800)
    {
        distanceInt = 800;
    }
    return distanceInt;
}

// ==========================================
// MAIN EXECUTABLE
// ==========================================
int main(void)
{
    unsigned char sharp_adc_val;
    unsigned int sharp_dist_mm;
    
    unsigned char current_state = 0;
    unsigned char previous_state = 0;

    adc_pin_config();
    adc_init();
    lcd_init();

    while (1)
    {
        sharp_adc_val = Sharp_sensor_data();
        sharp_dist_mm = Sharp_GP2D12_estimation(sharp_adc_val);

        // Determine the current distance zone
        if (sharp_dist_mm >= 100 && sharp_dist_mm <= 200) 
        {
            current_state = 1;
        }
        else if (sharp_dist_mm > 200 && sharp_dist_mm <= 500) 
        {
            current_state = 2;
        }
        else if (sharp_dist_mm > 500 && sharp_dist_mm <= 800) 
        {
            current_state = 3;
        }
        else
        {
            current_state = 0; // Out of range
        }

        // --- STEP 1: UPDATE TEXT & BEEP ONLY ON ZONE CHANGE ---
        if (current_state != previous_state)
        {
            if (current_state != 0) 
            {
                buzzer_beep();
            }

            if (current_state == 1)
            {
                lcd_cursor(1, 1);
                lcd_string("Obstacle is Near"); //[cite: 14]
                lcd_cursor(2, 1);
                lcd_string("Dist:     mm    "); 
            }
            else if (current_state == 2)
            {
                lcd_cursor(1, 1);
                lcd_string("Obstacle is far "); //[cite: 14]
                lcd_cursor(2, 1);
                lcd_string("Dist:     mm    "); 
            }
            else if (current_state == 3)
            {
                lcd_cursor(1, 1);
                lcd_string("Obstacle is very"); //[cite: 14]
                lcd_cursor(2, 1);
                lcd_string("far! D:   mm    "); // Adjusted spacing to fit row 2
            }
            else
            {
                lcd_cursor(1, 1);
                lcd_string("Out of Range    ");
                lcd_cursor(2, 1);
                lcd_string("Dist:     mm    ");
            }

            previous_state = current_state; 
        }

        // --- STEP 2: CONTINUOUSLY UPDATE DISTANCE VALUE ---
        // Write the 3-digit number directly into the blank spaces we left above
        if (current_state == 1 || current_state == 2 || current_state == 0)
        {
            lcd_cursor(2, 7); // Position cursor after "Dist: "
            lcd_print_number(sharp_dist_mm);
        }
        else if (current_state == 3)
        {
            lcd_cursor(2, 8); // Position cursor after "far! D:"
            lcd_print_number(sharp_dist_mm);
        }

        _delay_ms(50); // Small polling delay
    }

    return 0;
}
