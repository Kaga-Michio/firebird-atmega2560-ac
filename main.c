#ifndef F_CPU
#define F_CPU 14745600UL
#endif

// Using double quotes to prevent the GitHub text editor from stripping the headers
#include "avr/io.h"
#include "avr/interrupt.h"
#include "util/delay.h"
#include "math.h"

// ==========================================
// THRESHOLDS & ADC CHANNELS
// ==========================================
#define WHITE_LINE_THRESHOLD 40   //[cite: 44]
#define OBSTACLE_THRESHOLD 20     //[cite: 44]

#define LEFT_WL_SENSOR 3          // ADC3 - Left White Line Sensor[cite: 44]
#define CENTER_WL_SENSOR 2        // ADC2 - Center White Line Sensor[cite: 44]
#define RIGHT_WL_SENSOR 1         // ADC1 - Right White Line Sensor[cite: 44]
#define FRONT_SHARP_SENSOR 11     // ADC11 - Front Sharp IR Sensor[cite: 44]

// Global Variables
unsigned char Left_white_line = 0;    //[cite: 44]
unsigned char Center_white_line = 0;  //[cite: 44]
unsigned char Right_white_line = 0;   //[cite: 44]
unsigned char Front_Sharp_Sensor = 0; //[cite: 44]

// ==========================================
// RAW LCD DRIVER FUNCTIONS (4-BIT MODE)
// ==========================================
void lcd_port_config(void) {
    DDRC = DDRC | 0xF7; // All LCD pins as output except PC7[cite: 44]
    PORTC = PORTC & 0x80; // All LCD pins low except PC7[cite: 44]
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
    DDRF = 0x00;   // Set PORTF as input for ADC[cite: 45]
    PORTF = 0x00;  // Internal pull-up disabled[cite: 45]
    DDRK = 0x00;   // Set PORTK as input for ADC[cite: 45]
    PORTK = 0x00;  // Internal pull-up disabled[cite: 45]
}

void motion_pin_config(void) {
    DDRA = DDRA | 0x0F;   // Set PA0-PA3 as output for motor direction[cite: 45]
    PORTA = PORTA & 0xF0; // Initialize motor direction pins to 0[cite: 45]
    DDRL = DDRL | 0x18;   // Set PL3 and PL4 as output for PWM[cite: 45]
    PORTL = PORTL | 0x18; // PL3 and PL4 high for velocity control[cite: 45]
}

void buzzer_pin_config(void) {
    DDRC = DDRC | 0x08;   // Set PC3 as output for buzzer[cite: 45]
    PORTC = PORTC & 0xF7; // PC3 low to turn off buzzer[cite: 45]
}

void port_init(void) {
    lcd_port_config();
    adc_pin_config();
    motion_pin_config();
    buzzer_pin_config(); //[cite: 45]
}

void timer5_init(void) {
    TCCR5B = 0x00; // Stop timer[cite: 46]
    TCNT5H = 0xFF; // Counter high value[cite: 46]
    TCNT5L = 0x01; // Counter low value[cite: 46]
    OCR5AH = 0x00; // Output compare register high value[cite: 46]
    OCR5AL = 0xFF; // Output compare register low value[cite: 46]
    OCR5BH = 0x00; // Output compare register high value[cite: 46]
    OCR5BL = 0xFF; // Output compare register low value[cite: 46]
    ICR5H = 0x00;  // Input capture register high value[cite: 46]
    ICR5L = 0xFF;  // Input capture register low value[cite: 46]
    TCCR5A = 0xA9; // Timer control register A[cite: 46]
    TCCR5C = 0x00; // Timer control register C[cite: 46]
    TCCR5B = 0x0B; // Timer control register B[cite: 46]
}

void adc_init(void) {
    ADCSRA = 0x00; //[cite: 46]
    ADCSRB = 0x00; //[cite: 46]
    ADMUX = 0x20;  // Vref = 5V external, right adjusted value[cite: 46]
    ADCSRA = 0x86; // ADC enabled, prescaler = 64[cite: 46]
    ACSR = 0x80;   //[cite: 46]
}

// ==========================================
// DATA ACQUISITION
// ==========================================
unsigned char ADC_Conversion(unsigned char Ch) {
    unsigned char a;
    if(Ch > 7) {
        ADCSRB = 0x08; //[cite: 46]
    }
    Ch = Ch & 0x07; //[cite: 46]
    ADMUX = 0x20 | Ch; //[cite: 46]
    ADCSRA = ADCSRA | 0x40; // Set start conversion bit[cite: 46]
    while((ADCSRA & 0x10) == 0); // Wait for conversion to complete[cite: 46]
    a = ADCH; //[cite: 46]
    ADCSRA = ADCSRA | 0x10; // Clear ADIF bit[cite: 47]
    ADCSRB = 0x00; //[cite: 47]
    return a;
}

unsigned char Sharp_sensor_data(unsigned char channel) {
    return ADC_Conversion(channel); //[cite: 47]
}

unsigned char White_Line_sensor_data(unsigned char channel) {
    return ADC_Conversion(channel); //[cite: 47]
}

// ==========================================
// MOTION CONTROL
// ==========================================
void motion_set(unsigned char Direction) {
    unsigned char PortARestore = 0;
    Direction &= 0x0F; // Removing upper nibble as it's not needed[cite: 47]
    PortARestore = PORTA; // Reading the PORTA's original status[cite: 47]
    PortARestore &= 0xF0; // Setting lower direction nibble to 0[cite: 47]
    PortARestore |= Direction; // Adding lower nibble for direction command[cite: 47]
    PORTA = PortARestore; // Setting the command to PORTA[cite: 47]
}

void forward(void) { motion_set(0x06); } // Left forward, Right forward[cite: 48]
void back(void) { motion_set(0x09); }    // Left backward, Right backward[cite: 48]
void left(void) { motion_set(0x05); }    // Left backward, Right forward[cite: 48]
void right(void) { motion_set(0x0A); }   // Left forward, Right backward[cite: 48]
void soft_left(void) { motion_set(0x04); }  // Left stop, Right forward[cite: 48]
void soft_right(void) { motion_set(0x02); } // Left forward, Right stop[cite: 48]
void stop(void) { motion_set(0x00); }    // Both motors stop[cite: 49]

void velocity(unsigned char left_motor, unsigned char right_motor) {
    OCR5AL = left_motor;  //[cite: 49]
    OCR5BL = right_motor; //[cite: 49]
}

// ==========================================
// BEHAVIOR LOGIC
// ==========================================
void print_sensor_values(void) {
    lcd_cursor(1, 1); //[cite: 49]
    lcd_string("L:"); //[cite: 49]
    lcd_wr_char((Left_white_line / 10) + 48); //[cite: 49]
    lcd_wr_char((Left_white_line % 10) + 48); //[cite: 49]
    
    lcd_string(" C:"); //[cite: 49]
    lcd_wr_char((Center_white_line / 10) + 48); //[cite: 49]
    lcd_wr_char((Center_white_line % 10) + 48); //[cite: 49]
    
    lcd_string(" R:"); //[cite: 49]
    lcd_wr_char((Right_white_line / 10) + 48); //[cite: 49]
    lcd_wr_char((Right_white_line % 10) + 48); //[cite: 50]
    
    lcd_cursor(2, 1); //[cite: 50]
    lcd_string("Sharp:"); //[cite: 50]
    lcd_wr_char((Front_Sharp_Sensor / 10) + 48); //[cite: 50]
    lcd_wr_char((Front_Sharp_Sensor % 10) + 48); //[cite: 50]
}

void white_line_follow(void) {
    // Read all sensor values
    Left_white_line = White_Line_sensor_data(LEFT_WL_SENSOR); //[cite: 50]
    Center_white_line = White_Line_sensor_data(CENTER_WL_SENSOR); //[cite: 50]
    Right_white_line = White_Line_sensor_data(RIGHT_WL_SENSOR); //[cite: 50]
    Front_Sharp_Sensor = Sharp_sensor_data(FRONT_SHARP_SENSOR); //[cite: 50]

    // Check for obstacle first
    if(Front_Sharp_Sensor > OBSTACLE_THRESHOLD) { //[cite: 50]
        stop(); //[cite: 50]
        lcd_cursor(1, 1); //[cite: 50]
        lcd_string("OBSTACLE DETECTED"); //[cite: 50]
        lcd_cursor(2, 1); //[cite: 50]
        lcd_string("ROBOT STOPPED    "); //[cite: 50]
        return; //[cite: 50]
    }

    // White line following logic
    if(Center_white_line < WHITE_LINE_THRESHOLD) { //[cite: 50]
        forward(); // Center sensor on white line - move forward[cite: 50]
        velocity(200, 200); // Medium speed[cite: 50]
    }
    else if(Left_white_line < WHITE_LINE_THRESHOLD) { //[cite: 50]
        soft_left(); // Left sensor on white line - turn left[cite: 50]
        velocity(150, 200); // Reduce left motor speed[cite: 50]
    }
    else if(Right_white_line < WHITE_LINE_THRESHOLD) { //[cite: 51]
        soft_right(); // Right sensor on white line - turn right[cite: 51]
        velocity(200, 150); // Reduce right motor speed[cite: 51]
    }
    else {
        stop(); // No sensor on white line - stop or search[cite: 51]
        lcd_cursor(1, 1); //[cite: 51]
        lcd_string("LINE NOT FOUND   "); //[cite: 51]
    }
}

// ==========================================
// MAIN EXECUTABLE
// ==========================================
int main(void) {
    // Initialize all components
    port_init(); //[cite: 51]
    adc_init();  //[cite: 51]
    timer5_init(); //[cite: 51]
    lcd_init();  //[cite: 51]

    // Set initial velocity
    velocity(200, 200); //[cite: 51]

    // Display startup message
    lcd_cursor(1, 1); //[cite: 51]
    lcd_string("WHITE LINE ROBOT"); //[cite: 51]
    lcd_cursor(2, 1); //[cite: 51]
    lcd_string("INITIALIZING... "); //[cite: 51]
    _delay_ms(2000); //[cite: 51]
    
    lcd_cursor(1, 1); //[cite: 51]
    lcd_string("ROBOT READY     "); //[cite: 51]
    lcd_cursor(2, 1); //[cite: 51]
    lcd_string("STARTING...     "); //[cite: 51]
    _delay_ms(1000); //[cite: 51]

    // Main loop
    while(1) {
        white_line_follow(); //[cite: 52]
        // print_sensor_values(); // For debugging (Uncomment if needed)[cite: 52]
        _delay_ms(100); // Small delay for stability[cite: 52]
    }
    return 0; //[cite: 52]
}
