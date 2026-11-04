#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#define BAUD 9600
#define UBRR_VAL (F_CPU / 16 / BAUD - 1)

#define STEP_MS 300 
#define MUX_REPEATS (STEP_MS / 10)

const uint8_t numbers[10] = {
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111  //9
};

void uart_init(void)
{
    UBRR1H = (UBRR_VAL >> 8);
    UBRR1L = (UBRR_VAL & 0xFF);
    UCSR1B = (1 << TXEN1);
    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
}

void uart_putc(char c)
{
    while (!(UCSR1A & (1 << UDRE1)));
    UDR1 = c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

uint8_t count_leds(uint8_t v)
{
    uint8_t n = 0;
    while (v)
    {
        n += v & 1;
        v >>= 1;
    }
    return n;
}

int main(void)
{
    DDRB = 0xFF & ~(1 << PB4);
    DDRD = 0xFF;
    DDRC = 0xFF;
    PORTC = 0x00;

    uart_init();

    while (1)
    {
        for (int leds = 0; leds < 12; leds++)
        {
            for (int tens = 0; tens < 10; tens++)
            {
                for (int ones = 0; ones < 10; ones++)
                {
                    uart_puts("LED:");
                    uart_putc('0' + count_leds(PORTC));
                    uart_puts(" COUNTER:");
                    uart_putc('0' + tens);
                    uart_putc('0' + ones);
                    uart_puts("\r\n");

                    for (int t = 0; t < MUX_REPEATS; t++)
                    {
                        PORTB |= (1 << PB1);
                        PORTD = ~numbers[tens];
                        PORTB &= ~(1 << PB0);
                        _delay_ms(5);

                        PORTB |= (1 << PB0);
                        PORTD = ~numbers[ones];
                        PORTB &= ~(1 << PB1);
                        _delay_ms(5);
                    }
                }
            }

            PORTB |= (1 << PB0) | (1 << PB1);

            if (leds < 6)
                PORTC = (PORTC << 1) | 0x01;
            else
                PORTC = PORTC >> 1;
        }
    }
}