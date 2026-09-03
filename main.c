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
    DDRC  = DDRC | 0xF7;     // Set LCD pins (and PC3) as output
    PORTC = PORTC & 0x08;   // Set pins to low; retain buzzer line state
}

void lcd_set_4bit(void)
{
    _delay_ms(1);
    cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = (lcd_port & 0x0F) | 0x30; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
    _delay_ms(1);
    cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = (lcd_port & 0x0F) | 0x30; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
    _delay_ms(1);
    cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = (lcd_port & 0x0F) | 0x30; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
    _delay_ms(1);
    cbit(lcd_port,RS); cbit(lcd_port,RW); lcd_port = (lcd_port & 0x0F) | 0x20; sbit(lcd_port,EN); _delay_ms(5); cbit(lcd_port,EN);
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

// Buzzer Configuration (PC3 on Firebird V)
void buzzer_pin_config(void)
{
    DDRC  |= (1 << PC3);  // Set PC3 as output
    PORTC &= ~(1 << PC3); // Turn off buzzer initially
}

void buzzer_on(void)
{
    PORTC |= (1 << PC3);
}

void buzzer_off(void)
{
    PORTC &= ~(1 << PC3);
}

void beep_3_times(void)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        buzzer_on();
        _delay_ms(100);
        buzzer_off();
        _delay_ms(100);
    }
}

// Boot Switch Configuration (PE7 on Firebird V)
void boot_switch_pin_config(void)
{
    DDRE  &= ~(1 << PE7); // Set PE7 as input
    PORTE |= (1 << PE7);  // Enable pull-up
}

void port_init(void)
{
    lcd_port_config();
    buzzer_pin_config();
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
        // When boot switch is pressed (Active-Low)
        if (!(PINE & (1 << PE7)))
        {
            lcd_string(1, 4, "Daku_Maharaj     ");
            beep_3_times();

            // Wait for switch release to avoid continuous repeating beeps while holding
            while (!(PINE & (1 << PE7)))
            {
                _delay_ms(10);
            }
        }
        else
        {
            lcd_string(1, 4, "Embedded Lab      ");
        }
        _delay_ms(50);
    }
    return 0;
}
