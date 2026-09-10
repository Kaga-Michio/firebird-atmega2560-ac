#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define RS 0
#define RW 1
#define EN 2
#define lcd_port PORTC
#define sbit(reg,bit) reg |= (1<<bit)
#define cbit(reg,bit) reg &= ~(1<<bit)

// Speed array as specified in the experiment
unsigned char speed_levels[] = {0, 63, 126, 189, 252, 189, 126, 63, 0, 63};
unsigned char speed_index = 0;

// ==========================================
// LCD DRIVER FUNCTIONS
// ==========================================

void lcd_port_config(void)
{
DDRC = DDRC | 0xF7; // All LCD pins set as output
PORTC = PORTC & 0x08; // All LCD pins set to logic 0 except PC3
}

void lcd_set_4bit(void)
{
_delay_ms(1);
cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = 0x30; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
_delay_ms(1);
cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = 0x30; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
_delay_ms(1);
cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = 0x30; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
_delay_ms(1);
cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = 0x20; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
}

void lcd_wr_command(unsigned char cmd)
{
unsigned char temp = cmd & 0xF0;
lcd_port &= 0x0F;
lcd_port |= temp;
cbit(lcd_port,RS); cbit(lcd_port,RW); sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);

cmd = (cmd & 0x0F) << 4;
lcd_port &= 0x0F;
lcd_port |= cmd;
cbit(lcd_port,RS); cbit(lcd_port,RW); sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
}

void lcd_init(void)
{
lcd_set_4bit();
_delay_ms(1);
lcd_wr_command(0x28); // 4-bit mode, 2 lines
lcd_wr_command(0x01); // Clear screen
lcd_wr_command(0x06); // Entry mode
lcd_wr_command(0x0E); // Display ON, cursor ON
lcd_wr_command(0x80); // Home position
}

void lcd_cursor(char row, char column)
{
switch (row)
{
case 1: lcd_wr_command(0x80 + column - 1); break;
case 2: lcd_wr_command(0xC0 + column - 1); break;
case 3: lcd_wr_command(0x94 + column - 1); break;
case 4: lcd_wr_command(0xD4 + column - 1); break;
default: break;
}
}

void lcd_wr_char(char row, char column, char alpha_num_char)
{
lcd_cursor(row, column);
char temp = alpha_num_char & 0xF0;
lcd_port &= 0x0F; lcd_port |= temp;
sbit(lcd_port,RS); cbit(lcd_port,RW); sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);

alpha_num_char = (alpha_num_char & 0x0F) << 4;
lcd_port &= 0x0F; lcd_port |= alpha_num_char;
sbit(lcd_port,RS); cbit(lcd_port,RW); sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
}

void lcd_string(char row, char column, char *str)
{
while (*str != '\0')
{
lcd_wr_char(row, column, *str);
str++;
column += 1;
}
}

void lcd_numeric_value(char row, char column, int val, int digits)
{
unsigned char flag = 0;
lcd_cursor(row, column);

if (digits == 3 || flag == 1)
{
unsigned char hundred = (val / 100) + 48;
lcd_wr_char(row, column, hundred);
column += 1;
flag = 1;
}
if (digits >= 2 || flag == 1)
{
unsigned char tens = ((val / 10) % 10) + 48;
lcd_wr_char(row, column, tens);
column += 1;
flag = 1;
}
if (digits >= 1 || flag == 1)
{
unsigned char unit = (val % 10) + 48;
lcd_wr_char(row, column, unit);
column += 1;
}
}

// ==========================================
// MOTOR & TIMER CONFIGURATION
// ==========================================

void motion_pin_config(void)
{
DDRA |= 0x0F; // PA0-PA3 as output (Motor direction control)
PORTA &= 0xF0; // Initial direction to 0
DDRL |= 0x18; // PL3 (OC5A) and PL4 (OC5B) as output for PWM
PORTL |= 0x18; // Set initial PWM pins high
}

void boot_switch_config(void)
{
DDRE &= ~(1 << PE7); // Set PE7 as input
PORTE |= (1 << PE7); // Enable internal pull-up resistor on PE7
}

// Timer 5 initialized in Fast PWM 8-bit mode (non-inverting)
void motion_timer5_init(void)
{
TCCR5A = (1 << WGM50) | (1 << COM5A1) | (1 << COM5B1);
TCCR5B = (1 << WGM52) | (1 << CS51) | (1 << CS50); // Prescaler = 64
OCR5AL = 0; // Initial speed Left = 0
OCR5BL = 0; // Initial speed Right = 0
}

void velocity(unsigned char left_motor, unsigned char right_motor)
{
OCR5AL = left_motor;
OCR5BL = right_motor;
}

void forward(void)
{
PORTA &= 0xF0;
PORTA |= 0x06; // PA1=1, PA2=1 -> Forward motion
}

void stop(void)
{
PORTA &= 0xF0; // PA0-PA3=0 -> Motors stopped
}

void port_init(void)
{
motion_pin_config();
boot_switch_config();
}

// ==========================================
// MAIN CONTROL LOOP
// ==========================================

void motion_control(void)
{
lcd_string(1, 3, "Speed: ");
lcd_numeric_value(1, 10, speed_levels[speed_index], 3);

while (1)
{
// Check if boot switch is pressed (Active-Low)
if (!(PINE & (1 << PE7)))
{
_delay_ms(300); // Debounce delay

speed_index = (speed_index + 1) % 10; // Cycle through 10 values

lcd_numeric_value(1, 10, speed_levels[speed_index], 3);

if (speed_levels[speed_index] == 0)
{
stop();
}
else
{
forward();
velocity(speed_levels[speed_index], speed_levels[speed_index]);
}

// Wait for switch release
while (!(PINE & (1 << PE7)))
{
_delay_ms(10);
}
}
}
}

int main(void)
{
lcd_port_config();
lcd_init();
port_init();
motion_timer5_init();
motion_control();

return 0;
}
