#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>

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

int main(void)
{
  DDRB = 0xFF;
  DDRD = 0xFF;
  DDRC = 0xFF;
  PORTC = 0x00;
  while(1)
  {
      for (int leds=0; leds<12; leds++)
      {
          
          //_delay_ms(200);
          for(int tens=0; tens<10;tens++)
          {
              for(int ones=0;ones<10;ones++)
              {
                  for (int t = 0; t < 30; t++)
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
          if(leds<6)
          {
            PORTC = (PORTC << 1) | 0x01;
          }
          else
          {
            PORTC  = PORTC >> 1;
          } 
      }

  }
}