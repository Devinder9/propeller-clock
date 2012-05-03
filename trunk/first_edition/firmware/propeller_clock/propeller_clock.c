/*
 * propeller_clock.c
 *
 * Created: 04.10.2011 12:02:09
 *  Author: danisimo
 */ 

#define F_CPU 8000000UL // Stub
// high and low ADC values to turn on/off power dumper
#define Dhigh 702
#define Dlow  526

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdint.h>

/* Declarations */
void init();
void led_output( int16_t line);

/* Timings for pwm */
#define SMALL_INT 125 // 10ms
#define BIG_INT 12500 // 100ms

/**
 * Output line to led ports.
 * Led connections:
 * PORTB |   |   | A | 8 | 6 | 4 | 2 | 3 |
 * PORTC |   |   |   | B | 9 | G | E | C |
 * PORTD | 5 | 7 | 1 |   |   |   | D | F |
 */

uint16_t type[4] = {0x0000, 0xFFFF, 0xAAAA, 0x5555};
uint8_t iter = 0 ;
uint8_t counter1 = 0;

void led_output( int16_t line)
{
	/* Read current port status and clean output bits */
	int8_t port_b = PORTB & 0xc0;
	int8_t port_c = PORTC & 0xe0;
	int8_t port_d = PORTD & 0x1c;
	port_d |= ( ( ( line >> 0) & 0x01) << PD5); // 1 - PD5
	port_b |= ( ( ( line >> 1) & 0x01) << PB1); // 2 - PB1
	port_b |= ( ( ( line >> 2) & 0x01) << PB0); // 3 - PB0
	port_b |= ( ( ( line >> 3) & 0x01) << PB2); // 4 - PB2
	port_d |= ( ( ( line >> 4) & 0x01) << PD7); // 5 - PD7
	port_b |= ( ( ( line >> 5) & 0x01) << PB3); // 6 - PB3
	port_d |= ( ( ( line >> 6) & 0x01) << PD6); // 7 - PD6
	port_b |= ( ( ( line >> 7) & 0x01) << PB4); // 8 - PB4
	port_c |= ( ( ( line >> 8) & 0x01) << PC3); // 9 - PC3
	port_b |= ( ( ( line >> 9) & 0x01) << PB5); // A - PB5
	port_c |= ( ( ( line >> 10) & 0x01) << PC4); // B - PC4
	port_c |= ( ( ( line >> 11) & 0x01) << PC0); // C - PC0
	port_d |= ( ( ( line >> 12) & 0x01) << PD1); // D - PD1
	port_c |= ( ( ( line >> 13) & 0x01) << PC1); // E - PC1
	port_d |= ( ( ( line >> 14) & 0x01) << PD0); // F - PD0
	port_c |= ( ( ( line >> 15) & 0x01) << PC2); // G - PC2
	PORTB = port_b;
	PORTC = port_c;
	PORTD = port_d;
}

/* Small timer interrupt. Switch off leds */
ISR( TIMER1_COMPA_vect)
{
	led_output( 0x0000);
}

/* Big timer interrupt. Switch on leds */
ISR( TIMER1_COMPB_vect)
{
	counter1 += 1;
	if (!counter1)
		iter += 1;
	if (iter > 3)
		iter = 0;
	led_output( type[iter]);
	TCNT1 = 0x00;
}

ISR( ADC_vect)
{
	// This section is important
	// Disable global interrupts
	cli(); 	
	
	uint16_t ADC_value;
	
	ADC_value = ADCL+((uint16_t) ADCH << 8);
	
	if ( ADC_value > Dhigh)
		PORTD = PORTD | (_BV(PIND4));
	if ( ADC_value < Dlow)
		PORTD = PORTD & (~(_BV(PIND4)));
	sei();
}

/* Initialization */
void init()
{
	cli();
	
	/* Set output registers */
	DDRB = 0x1F;
	DDRC = 0x1F;
	DDRD = 0xF3;
	
	PORTC = 0x20; //Pull up for PC5
	PORTD = 0x0C; //Pull up for PD3,PD2
		
	/** Init 16bit counter **/
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TIMSK = 0x00;
	/* Set small and big match registers */
	OCR1A = SMALL_INT;
	OCR1B = BIG_INT;
	/* Set prescaler value to clk/64 */
	TCCR1B |= ( ( 1 << CS11) | ( 0 << CS10));
	/* Enable counter compare interrupts */
	TIMSK |= ((1 << OCIE1A) | ( 1 << OCIE1B));
	
	// ADC initialization
	ADMUX = 0x05; //AREF as source, ADC5(PC5) as input source
	ADCSRA = 0xFF; // Enable ADC interrupt, set clock multiplexer to 128
	
	/* Enable global interrupts */
	sei();
	
}

int main( void)
{
	init();
	
    while ( 1)
    {
		/* Do nothing. Wait for interrupts */		
    }
}