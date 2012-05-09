/*
 * propeller_clock.c
 *
 * Created: 04.05.2012 19:54:37
 * Author: Denis Anisimov
 * Device: ATmega32
 *
 * Firmware for propeller clock.
 *
 */ 
#define F_CPU 8000000UL
#define SDI1 PB0
#define SDI2 PB1
#define SDI3 PB2
#define CLK PA1
#define LE PA2
#define OE PA0

/* ADC defines */
#define Dhigh 702
#define Dlow  526

/* ratio = PWM / 256. 
 * PWM = 0..256 */
#define PWM 25

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdint.h>
#include <avr/io.h>

/* Declarations */
void led_on();
void led_off();
void output_to_sr( uint32_t led_line);
void init();

/* Switch on leds */
inline void led_on()
{
	PORTA &= ~(1 << OE);
}

/* Switch off leds */
inline void led_off()
{
    PORTA |= 1 << OE;	
}

/* Output led line using shift register based drivers */
void output_to_sr(uint32_t led_line)
{
	/* Short solution */
	uint8_t i = 0;
	for (i = 0; i < 8; i++)
	{
		/* Set SDI */
		PORTB |= ((led_line & (1 << i)) >> i) << SDI1;
		PORTB |= ((led_line & (1 << (i + 8))) >> (i + 8)) << SDI2;
		PORTB |= ((led_line & (1 << (i + 16))) >> (i + 16)) << SDI3;
		/* CLK output */
		PORTA |= (1 << CLK);	
		PORTA &= ~(1 << CLK);
		/* Reset SDI */
		PORTB &= ~((1 << SDI1) | (1 << SDI2) | (1 << SDI3));
	}
	/* CLK and LE output */
    PORTA |= (1 << LE);
	PORTA |= (1 << CLK);
	PORTA &= ~(1 << CLK);
    PORTA &= ~(1 << LE);		
}

/*
 *   0   OCR0         0
 *    ____             ____
 * __|    |___________|    |____....
 * 
 */

/* PWM duty cycle */
ISR( TIMER0_COMP_vect)
{
	led_off();
}

/* PWM on cycle */
ISR( TIMER0_OVF_vect)
{
	led_on();
}

struct time_t{
	uint8_t hour;
	uint8_t minute;
}time;

/* Draw analog clock */
uint32_t draw_clock(uint16_t pos)
{
	uint32_t out = 0x00000000;
	
	/* Draw 5-minutes bars */
	if ((pos % 30) == 0)
	{
		out |= (1 << 23) | (1 << 22);
	}
	
	/* Draw 15-minutes bars */
	if ((pos % 90) == 0)
	{
		out |= (1 << 21) | (1 << 21);
	}
	
	/* Draw hour arrow */
	if ((pos / 6) == time.hour)
	{
		out |= 0x000000ff;
	}
	
	/* Draw minute arrow */
	if ((pos / 6) == time.minute)
	{
		out |= 0x0000ffff;
	}
	
	return out;
}

/* Duration in counts of one grad 
 * Requires tuning depending on initial rotation speed */
uint16_t grad_dur = 100;

/* Current position in grad */
uint16_t pos_grad = 0;

/* Temp stuff */
uint32_t cnt = 0;
ISR( TIMER1_COMPA_vect)
{
	pos_grad++;
	output_to_sr(grad_dur);
}


/* Position detector interrupt */
ISR( INT0_vect)
{
	/* Tune grad duration */
	if (pos_grad < 356)
	{
		grad_dur++;
	} else if (pos_grad > 364)
	{
		grad_dur--;
	}
	
	/* Reset position */
	pos_grad = 0;
}

/* Realtime counter interrupt 
 * Update current time */
ISR( TIMER2_COMP_vect)
{
	TCNT2 = 0x00;
	uint8_t carry = 0;
	carry = (time.minute == 59) ? 1 : 0;
	time.minute = (carry) ? 0 : (time.minute + 1);
	time.hour = ((time.hour + carry) == 12) ? 0 : time.hour + 1;
}

/* Software voltage damper */
ISR( ADC_vect)
{
	cli(); 	
	
	uint16_t ADC_value;
	
	ADC_value = ADCL+((uint16_t) ADCH << 8);
	
	if ( ADC_value > Dhigh)
		PORTD = PORTD | (_BV(PIND4));
	if ( ADC_value < Dlow)
		PORTD = PORTD & (~(_BV(PIND4)));
	sei();
}

/* Device initialization */
inline void init()
{
	cli();
	
	/* Output/input port init */
	             //   7   6   5   4   3   2   1   0
	DDRA = 0x07; // |   |   |   |   |   | OE|CLK| CE|
	DDRB = 0x07; // |   |   |   |   |   |SDI|SDI|SDI|
	DDRC = 0x00; // |   |   |   |   |   |   |   |   |
	DDRD = 0x00; // |   |   |   |   | RC|DET|   |   |
	/* Pull up */
	PORTD = 0x0C;
	
	/* ADC initialization */
	ADMUX = 0x03;  // AREF as source, ADC3(PA3) as input source
	ADCSRA = 0xFF; // Enable ADC interrupt, set clock multiplexer to 128
	
	/* Init 8-bit counter used for led PWM */
	TCCR0 = 0x04; // 
	OCR0 = PWM;   // Set PWM ratio
	TIMSK = 0x03;  // Enable compare and overflow interrupt
	
	/* Init 16-bit counter used for different stuff */
	TCCR1A = 0x00;
	TCCR1B = 0x05; // clk/1024
	OCR1A = 800;  // ~10hz interrupt
	TIMSK |= 0x10; // Enable compare interrupt
	
	/* Init 8-bit counter with external oscillator */
	TCCR2 = 0x01;          // No prescaling
	ASSR = (1 << AS2);     // Enable external oscillator
	TIMSK |= (1 << OCIE2); // Enable interrupt on compare match
	
	
	/* Turn on INT0 on falling edge */
	MCUCR = (1 << ISC01);
	GICR = (1 << INT0);
	
	/* Turn off leds by default by setting 1 to OE */
	led_off();
		
	sei();
}

int main(void)
{
	/* Device initialization */
	init();
	
	/* Main cycle */	
    while(1)
    {
        // Do nothing. Wait for interrupts.
    }
}