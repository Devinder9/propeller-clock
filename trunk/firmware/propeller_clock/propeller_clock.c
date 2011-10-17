/*
 * propeller_clock.c
 *
 * Created: 04.10.2011 12:02:09
 *  Author: danisimo
 */ 

#define F_CPU 8000000UL // Stub

#include <avr/io.h>
#include <avr/interrupt.h>

/* Declarations */
void init();
void led_output( int16_t line);

/* Timings for pwm */
#define SMALL_INT 1250 // 10ms
#define BIG_INT 12500 // 100ms

/**
 * Output line to led ports.
 * Led connections:
 * PORTB |   |   | A | 8 | 6 | 4 | 2 | 3 |
 * PORTC |   |   |   | B | 9 | G | E | C |
 * PORTD | 5 | 7 | 1 |   |   |   | D | F |
 */

void led_output( int16_t line)
{
	/* Read current port status and clean output bits */
	int8_t port_b = PORTB & 0xc0;
	int8_t port_c = PORTC & 0xd0;
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
	led_output( 0xffff);
	TCNT1 = 0x00;
}

/* Initialization */
void init()
{
	/* Set output registers */
	DDRB = 0x3f;
	DDRC = 0x1f;
	DDRD = 0xe3; 
	
	/* Enable global interrupts */
	sei();
	
	/** Init 16bit counter **/
	TCCR1A = 0x0;
	TCCR1B = 0x0;
	TIMSK = 0x0;
	/* Set small and big match registers */
	OCR1A = SMALL_INT;
	OCR1B = BIG_INT;
	/* Set prescaler value to clk/64 */
	TCCR1B |= ( ( 1 << CS11) | ( 1 << CS10));
	/* Enable counter compare interrupts */
	TIMSK |= ((1 << OCIE1A) | ( 1 << OCIE1B));
}

int main( void)
{
	init();
	
    while ( 1)
    {
		/* Do nothing. Wait for interrupts */		
    }
}