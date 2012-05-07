/*
 * hdd_base.c
 *
 * Created: 08.05.2012 1:07:13
 *  Author: matveich
 */ 

#include <avr/interrupt.h>

#define F_CPU 8000000UL
#define SDI1 PB0
#define SDI2 PB1
#define SDI3 PB2
#define CLK PA1
#define LE PA2
#define OE PA0

ISR(TIMER1_COMPA_vect)
{
	PORTA = (0x01&~PORTA)+(PORTA&0xFE);
	TCNT1 = 0x0000;
}


void Init()
{
cli();

//Configure ports
//Input ports use pull-up
//NC pins - input

			 //   7      6      5      4      3      2      1      0
DDRA = 0x00; // | FG(i)|      |      |      |      |      |      |      |
DDRB = 0x01; // |      |      |      |      |      |      |      |LED(o)|
DDRC = 0x00; // |      |      |      |      |      |      |      |      |
DDRD = 0xFC; // |Pwr(o)|T_R(o)|DAC(o)|Brk(o)|Dir(o)| RC(i)|      |      |


PORTA = 0xFF;
PORTB = 0xFE;
PORTA = 0xFF;
PORTB = 0x03;

//Configure ADC
ADMUX = 0x07
ADCSRA = 0xDE

//For ATmega8535
TIMSK = 0x10; //COMPA interrupt
TCCR1A = 0x40;
TCCR1B = 0x03;//CK/64
OCR1A = 20; //3125 Hz

sei();
}

int main(void)
{

Init();

while(1);
return 0;
}