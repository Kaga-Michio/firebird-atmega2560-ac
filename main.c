#ifndef F_CPU
#define F_CPU 14745600UL
#endif

// Using double quotes to prevent GitHub Actions copy-paste bugs
#include "avr/io.h"
#include "avr/interrupt.h"
#include "util/delay.h"
#include "math.h"

// ==========================================
// THRESHOLDS & ADC CHANNELS
// ==========================================
#define WHITE_LINE_THRESHOLD 40   
#define OBSTACLE_THRESHOLD 20     

#define LEFT_WL_SENSOR 3          // ADC3 - Left White Line Sensor
#define CENTER_WL_SENSOR 2        // ADC2 - Center White Line Sensor
#define RIGHT_WL_SENSOR 1         // ADC1 - Right White Line Sensor
#define FRONT_SHARP_SENSOR 11     // ADC11 - Front Sharp IR Sensor

// Global Variables
unsigned char Left_white_line = 0;    
unsigned char Center_white_line = 0;  
unsigned char Right_white_line = 0;   
unsigned char Front_Sharp_Sensor = 0; 

// Track whether the robot is currently blocked to manage LCD updates cleanly
static unsigned char obstacle_state = 0; 

// ==========================================
// RAW LCD DRIVER FUNCTIONS (4-BIT MODE)
// ==========================================
void lcd_port_config(void) {
    DDRC = DDRC | 0xF7; 
    PORTC = PORTC & 0x80; 
}

void lcd_cmd(unsigned char cmd) {
    PORTC = (PORTC & 0x0F) | (cmd & 0xF0); 
    PORTC &= ~(1 << PC0); 
    PORTC &= ~(1 << PC1); 
    PORTC |= (1 << PC2);  
    _delay_ms(1);
    PORTC &= ~(1 << PC2); 
    _delay_ms(1);
    PORTC = (PORTC & 0x0F) | ((cmd << 4) & 0xF0); 
    PORTC |= (1 << PC2);  
    _delay_ms(1);
    PORTC &= ~(1 << PC2); 
    _delay_ms(2);
}

void lcd_wr_char(char data) {
    PORTC = (PORTC & 0x0F) | (data & 0xF0); 
    PORTC |= (1 << PC0);  
    PORTC &= ~(1 << PC1); 
    PORTC |= (1 << PC2);  
    _delay_ms(1);
    PORTC &= ~(1 << PC2); 
    _delay_ms(1);
    PORTC = (PORTC & 0x0F) | ((data << 4) & 0xF0); 
    PORTC |= (1 << PC2);  
    _delay_ms(1);
    PORTC &= ~(1 << PC2); 
    _delay_ms(2);
}

void lcd_string(char *str) {
    while (*str) {
        lcd_wr_char(*str++);
    }
}

void lcd_cursor(char row, char column) {
    if (row == 1) { lcd_cmd(0x80 + column - 1); }
    if (row == 2) { lcd_cmd(0xC0 + column - 1); }
}

// Function to convert an integer to text characters on the LCD
void lcd_print_number(unsigned int num) {
    unsigned char hundreds, tens, ones;
    if (num > 999) num = 999; 
    hundreds = (num / 100);
    tens = (num / 10) % 10;
    ones = num % 10;
    lcd_wr_char(hundreds + '0'); 
    lcd_wr_char(tens + '0');
    lcd_wr_char(ones + '0');
}

void lcd_init(void) {
    lcd_port_config();
    _delay_ms(20);
    lcd_cmd(0x02); 
    lcd_cmd(0x28); 
    lcd_cmd(0x0C); 
    lcd_cmd(0x06); 
    lcd_cmd(0x01); 
    _delay_ms(2);
}

// ==========================================
// CONFIGURATION FUNCTIONS
// ==========================================
void adc_pin_config(void) {
    DDRF = 0x00;   
    PORTF = 0x00;  
    DDRK = 0x00;   
    PORTK = 0x00;  
}

void motion_pin_config(void) {
    DDRA = DDRA | 0x0F;   
    PORTA = PORTA & 0xF0; 
    DDRL = DDRL | 0x18;   
    PORTL = PORTL | 0x18; 
}

void buzzer_pin_config(void) {
    DDRC = DDRC | 0x08;   // Set PC3 as output for buzzer
    PORTC = PORTC & 0xF7; // PC3 low to turn off buzzer initially
}

void port_init(void) {
    lcd_port_config();
    adc_pin_config();
    motion_pin_config();
    buzzer_pin_config(); 
}

void timer5_init(void) {
    TCCR5B = 0x00; 
    TCNT5H = 0xFF; 
    TCNT5L = 0x01; 
    OCR5AH = 0x00; 
    OCR5AL = 0xFF; 
    OCR5BH = 0x00; 
    OCR5BL = 0xFF; 
    ICR5H = 0x00;  
    ICR5L = 0xFF;  
    TCCR5A = 0xA9; 
    TCCR5C = 0x00; 
    TCCR5B = 0x0B; 
}

void adc_init(void) {
    ADCSRA = 0x00; 
    ADCSRB = 0x00; 
    ADMUX = 0x20;  
    ADCSRA = 0x86; 
    ACSR = 0x80;   
}

// ==========================================
// DATA ACQUISITION & ESTIMATION
// ==========================================
unsigned char ADC_Conversion(unsigned char Ch) {
    unsigned char a;
    if(Ch > 7) {
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

unsigned char Sharp_sensor_data(unsigned char channel) {
    return ADC_Conversion(channel); 
}

unsigned char White_Line_sensor_data(unsigned char channel) {
    return ADC_Conversion(channel); 
}

// Convert raw ADC value to distance in millimeters
unsigned int Sharp_GP2D12_estimation(unsigned char adc_reading) {
    float distance;
    unsigned int distanceInt;
    distance = (int)(10.00 * (2799.6 * (1.00 / (pow(adc_reading, 1.1546)))));
    distanceInt = (unsigned int)distance;
    if(distanceInt > 800) {
        distanceInt = 800;
    }
    return distanceInt;
}

// ==========================================
// MOTION CONTROL
// ==========================================
void motion_set(unsigned char Direction) {
    unsigned char PortARestore = 0;
    Direction &= 0x0F; 
    PortARestore = PORTA; 
    PortARestore &= 0xF0; 
    PortARestore |= Direction; 
    PORTA = PortARestore; 
}

void forward(void) { motion_set(0x06); } 
void back(void) { motion_set(0x09); }    
void left(void) { motion_set(0x05); }    
void right(void) { motion_set(0x0A); }   
void soft_left(void) { motion_set(0x04); }  
void soft_right(void) { motion_set(0x02); } 
void stop(void) { motion_set(0x00); }    

void velocity(unsigned char left_motor, unsigned char right_motor) {
    OCR5AL = left_motor;  
    OCR5BL = right_motor; 
}

// ==========================================
// BEHAVIOR LOGIC
// ==========================================
void white_line_follow(void) {
    // Read all sensor values
    Left_white_line = White_Line_sensor_data(LEFT_WL_SENSOR); 
    Center_white_line = White_Line_sensor_data(CENTER_WL_SENSOR); 
    Right_white_line = White_Line_sensor_data(RIGHT_WL_SENSOR); 
    Front_Sharp_Sensor = Sharp_sensor_data(FRONT_SHARP_SENSOR); 

    // 1. Check for obstacle first
    if(Front_Sharp_Sensor > OBSTACLE_THRESHOLD) { 
        stop(); 
        
        PORTC |= (1 << PC3); // Turn Buzzer ON
        
        // Calculate the physical distance in mm
        unsigned int dist_mm = Sharp_GP2D12_estimation(Front_Sharp_Sensor);
        
        lcd_cursor(1, 1); 
        lcd_string("OBSTACLE DETECTED"); 
        lcd_cursor(2, 1); 
        lcd_string("Dist: "); 
        lcd_print_number(dist_mm);
        lcd_string(" mm      "); 
        
        obstacle_state = 1; // Mark that we are currently blocked
        return; 
    }

    // 2. Clear obstacle warnings if path is clear
    if (obstacle_state == 1) {
        PORTC &= ~(1 << PC3); // Turn Buzzer OFF
        lcd_cursor(1, 1);
        lcd_string("FOLLOWING LINE   ");
        lcd_cursor(2, 1);
        lcd_string("                 "); // Clear distance readout
        obstacle_state = 0; 
    }

    // 3. White line following logic
    if(Center_white_line < WHITE_LINE_THRESHOLD) { 
        forward(); 
        velocity(200, 200); 
    }
    else if(Left_white_line < WHITE_LINE_THRESHOLD) { 
        soft_left(); 
        velocity(150, 200); 
    }
    else if(Right_white_line < WHITE_LINE_THRESHOLD) { 
        soft_right(); 
        velocity(200, 150); 
    }
    else {
        stop(); 
        lcd_cursor(1, 1); 
        lcd_string("LINE NOT FOUND   "); 
        lcd_cursor(2, 1); 
        lcd_string("                 ");
    }
}

// ==========================================
// MAIN EXECUTABLE
// ==========================================
int main(void) {
    port_init(); 
    adc_init();  
    timer5_init(); 
    lcd_init();  

    velocity(200, 200); 

    lcd_cursor(1, 1); 
    lcd_string("WHITE LINE ROBOT"); 
    lcd_cursor(2, 1); 
    lcd_string("INITIALIZING... "); 
    _delay_ms(2000); 
    
    lcd_cursor(1, 1); 
    lcd_string("FOLLOWING LINE  "); 
    lcd_cursor(2, 1); 
    lcd_string("                "); 
    _delay_ms(1000); 

    while(1) {
        white_line_follow(); 
        _delay_ms(100); 
    }
    return 0; 
}
