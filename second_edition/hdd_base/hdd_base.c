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

ISR(TIMER2_OVF_vect)
{
	//PORTA = (0x01&~PORTA)+(PORTA&0xFE);
	//TCNT1 = 0x0000;
}


ISR(TIMER2_COMP_vect)
{
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
ADMUX = 0x07;
ADCSRA = 0xDE;



//T1 PWM - DAC
TCCR1A = 0x40;
TCCR1B = 0x03;//CK/64
OCR1A  = 100; //3125 Hz

//T2 PWM 3khz 50% - Power
TCCR2 = 0x1A;
OCR2  =	84;

//Enable timer interrupts
TIMSK = 0xC0;
//PORTD |= 0x05;

sei();
}

int main(void)
{

Init();

while(1);
return 0;
}
