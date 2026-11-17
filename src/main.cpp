#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#define BAUD 9600
#define UBRR_VAL (F_CPU / 16 / BAUD - 1)

#define STEP_MS 300
#define MUX_REPEATS (STEP_MS / 10)

#define DHT_BIT  PB2          // czujnik DHT11 (DATA)
#define BTN1_BIT PB4          // przycisk 1: pauza (wcisniety = stoi)
#define BTN2_BIT PE3          // przycisk 2: reset

#define DHT_THRESHOLD 25      // prog: ponizej = bit 0, powyzej = bit 1

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

/* ---------------- UART (USART1, nadawanie na PB3) ---------------- */

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

void uart_put_u8(uint8_t n)
{
    uart_putc('0' + n / 100);
    uart_putc('0' + (n % 100) / 10);
    uart_putc('0' + n % 10);
}

/* ---------------- DHT11 ---------------- */

void dht_init(void)
{
    DDRB  &= ~(1 << DHT_BIT);
    PORTB |=  (1 << DHT_BIT);
}

uint8_t dht_pin_read(void)
{
    return (PINB >> DHT_BIT) & 1;
}

// czeka az pin przestanie miec stan 'level'; 0 = timeout, inaczej czas (>=1)
uint8_t dht_wait_while(uint8_t level)
{
    uint8_t countDHT = 0;

    while (level == dht_pin_read())
    {
        _delay_us(1);
        countDHT++;

        if (countDHT > 100)
        {
            return 0;
        }
    }

    return countDHT ? countDHT : 1;
}

void dht_start(void)
{
    DDRB  |=  (1 << DHT_BIT);
    PORTB &= ~(1 << DHT_BIT);
    _delay_ms(20);
    PORTB |=  (1 << DHT_BIT);
    _delay_us(30);
    DDRB  &= ~(1 << DHT_BIT);
}

// czyta jeden bajt (8 bitow); zwraca 0 = OK, 1 = timeout
uint8_t dht_read_byte(uint8_t *out)
{
    uint8_t value = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        if (!dht_wait_while(0)) return 1;     // ok. 50 us stanu niskiego
        uint8_t w = dht_wait_while(1);        // czas stanu wysokiego
        if (!w) return 1;

        value <<= 1;
        if (w >= DHT_THRESHOLD)
        {
            value |= 1;
        }
    }

    *out = value;
    return 0;
}

// pelny odczyt; zwraca 0 = OK, 1..5 = kod bledu
uint8_t dht_read(uint8_t *hum, uint8_t *temp)
{
    uint8_t data[5];

    dht_start();

    if (!dht_wait_while(1)) return 1;         // brak reakcji na start
    if (!dht_wait_while(0)) return 2;         // ~80 us niski
    if (!dht_wait_while(1)) return 3;         // ~80 us wysoki

    for (uint8_t i = 0; i < 5; i++)
    {
        if (dht_read_byte(&data[i])) return 4;
    }

    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) return 5;

    *hum  = data[0];
    *temp = data[2];
    return 0;
}

void dht_report(void)
{
    uint8_t hum, temp;
    uint8_t err = dht_read(&hum, &temp);

    if (err == 0)
    {
        uart_puts("HUMIDITY:");
        uart_put_u8(hum);
        uart_puts(" %\r\n");
        uart_puts("TEMPERATURE:");
        uart_put_u8(temp);
        uart_puts(" C\r\n");
    }
    else
    {
        uart_puts("DHT ERROR:");
        uart_put_u8(err);
        uart_puts("\r\n");
    }
}

/* ---------------- Przyciski (aktywne stanem niskim) ---------------- */

uint8_t button1_pressed(void)
{
    return (PINB & (1 << BTN1_BIT)) == 0;
}

uint8_t button2_pressed(void)
{
    return (PINE & (1 << BTN2_BIT)) == 0;
}

/* ---------------- Wyswietlacz i diody ---------------- */

// jeden pelny obieg multipleksowania: obie cyfry po 5 ms
void display_cycle(uint8_t tens, uint8_t ones)
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

/* ---------------- main ---------------- */

int main(void)
{
    DDRB = 0xFF & ~(1 << DHT_BIT) & ~(1 << BTN1_BIT);
    DDRC = 0xFF;
    DDRD = 0xFF;
    DDRE = 0xFF & ~(1 << BTN2_BIT);

    PORTB |= (1 << PB0) | (1 << PB1);        // cyfry na starcie wygaszone
    PORTB |= (1 << BTN1_BIT);                // pull-up przycisku 1
    PORTE |= (1 << BTN2_BIT);                // pull-up przycisku 2
    PORTC = 0x00;

    uart_init();
    dht_init();

    uint8_t leds = 0, tens = 0, ones = 0;

    while (1)
    {
        // status w terminalu
        uart_puts("LED:");
        uart_putc('0' + count_leds(PORTC));
        uart_puts(" COUNTER:");
        uart_putc('0' + tens);
        uart_putc('0' + ones);
        uart_puts("\r\n");

        // wyswietlanie przez STEP_MS; czas liczy sie tylko gdy NIE ma pauzy
        uint8_t t = 0;
        uint8_t reset = 0;

        while (t < MUX_REPEATS)
        {
            display_cycle(tens, ones);

            if (button2_pressed())
            {
                reset = 1;
                break;
            }
            if (!button1_pressed())
            {
                t++;
            }
        }

        if (reset)
        {
            leds = 0;
            tens = 0;
            ones = 0;
            PORTC = 0x00;

            // przy przytrzymanym resecie stoi na 00 (bez zalewania terminala)
            while (button2_pressed())
            {
                display_cycle(0, 0);
            }
            continue;
        }

        // przeniesienie: jednosci -> dziesiatki -> diody
        ones++;
        if (ones == 10)
        {
            ones = 0;
            tens++;
        }

        if (tens == 10)
        {
            tens = 0;

            PORTB |= (1 << PB0) | (1 << PB1);   // wygas cyfry na czas odczytu
            dht_report();

            if (leds < 6)
                PORTC = (PORTC << 1) | 0x01;
            else
                PORTC = PORTC >> 1;

            leds++;
            if (leds == 12)
            {
                leds = 0;
            }
        }
    }
}