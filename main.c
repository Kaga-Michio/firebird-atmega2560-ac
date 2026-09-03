#ifndef F_CPU
#define F_CPU 14745600UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#define RS 0
#define RW 1
#define EN 2
#define lcd_port PORTC
#define sbit(reg,bit)   reg |= (1<<bit)
#define cbit(reg,bit)   reg &= ~(1<<bit)

// LCD Driver Functions
void lcd_port_config(void)
{
    DDRC = DDRC | 0xF7;     // All LCD pins as output
    PORTC = PORTC & 0x08;   // All LCD pins set to logic 0 except PC3
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
    lcd_wr_command(0x28);
    lcd_wr_command(0x01);
    lcd_wr_command(0x06);
    lcd_wr_command(0x0E);
    lcd_wr_command(0x80);
}

void lcd_cursor(char row, char column)
{
    switch (row)
    {
        case 1: lcd_wr_command(0x80 + column - 1); break;
        case 2: lcd_wr_command(0xC0 + column - 1); break;
        case 3: lcd_wr_command(0x94 + column - 1); break;
        case 4: lcd_wr_command(0xD4 + column - 1); break;
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
    while(*str != '\0')
    {
        lcd_wr_char(row, column, *str);
        str++;
        column += 1;
    }
}

// Firebird V Switch Configuration
void boot_switch_pin_config(void)
{
    DDRE  &= ~(1 << PE7); // PE7 as input
    PORTE |= (1 << PE7);  // Enable pull-up
}

void port_init(void)
{
    lcd_port_config();
    boot_switch_pin_config();
}

void init_devices(void)
{
    port_init();
    lcd_init();
}

int main(void)
{
    init_devices();
    
    while (1)
    {
        if (!(PINE & (1 << PE7)))
        {
            // Switch PRESSED: Starts at Row 1, Column 4
            // Trailing spaces clear characters from "NOT PRESSED"
            lcd_string(1, 4, "PRESSED    ");
        }
        else
        {
            // Switch NOT PRESSED: Starts at Row 1, Column 4
            lcd_string(1, 4, "NOT PRESSED");
        }
        _delay_ms(50);
    }
    return 0;
}
