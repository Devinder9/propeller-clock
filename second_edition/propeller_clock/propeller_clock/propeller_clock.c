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
#define PWM 5

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdint.h>
#include <avr/sleep.h>

/* Declarations */
void led_on();
void led_off();
uint32_t mixer();
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

/* Mix output led line to display it correctly 
 * See led line schematic
 *
 * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13| 14| 15| 16| 17| 18| 19| 20| 21| 22| 23|
 *
 * | 0 | 1 | 2 | 3 | 7 | 6 | 5 | 4 | 8 | 9 | 10| 11| 15| 14| 13| 12| 16| 17| 18| 19| 23| 22| 21| 20|
 *
 */
uint32_t mixer(uint32_t input_line)
{
	uint32_t output_line = 0x00000000;
	const uint8_t bit_mix[24] = {0,1,2,3,7,6,5,4,8,9,10,11,15,14,13,12,16,17,18,19,23,22,21,20};
	uint8_t i = 0;
	for (i = 0; i < 24; i++)
	{
		output_line |= ((input_line & (1UL << i)) >> i) << bit_mix[i];
	}
	return output_line;
}

/* Output led line using shift register based drivers */
void output_to_sr(uint32_t led_line)
{
	uint32_t led_line_mixd = mixer(led_line);
	
	int8_t i = 0;
	for (i = 7; i >= 0; i--)
	{
		/* CLK output */
		PORTA |= (1 << CLK);	
		PORTA &= ~(1 << CLK);
		/* Reset SDI */
		PORTB &= ~((1 << SDI1) | (1 << SDI2) | (1 << SDI3));
		/* Set SDI */
		PORTB |= ((led_line_mixd & (1UL << i)) >> i) << SDI1;
		PORTB |= ((led_line_mixd & (1UL << (i + 8))) >> (i + 8)) << SDI2;
		PORTB |= ((led_line_mixd & (1UL << (i + 16))) >> (i + 16)) << SDI3;
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

/* Structure to store current time */
struct time_t{
	uint8_t sec;
	uint8_t min;
}time;

/* Draw analog clock */
uint32_t draw_clock(uint16_t pos)
{
	uint32_t out = 0x00000000;
	
	/* Draw 5-minutes bars */
	if ((pos % 30) == 0)
	{
		out |= (1UL << 23) | (1UL << 22);
	}
	
	/* Draw 15-minutes bars */
	if ((pos % 90) == 0)
	{
		out |= (1UL << 21) | (1UL << 21);
	}
	
	/* Draw minute arrow */
	if ((pos / 6) == time.min)
	{
		out |= 0x000000ff;
	}
	
	/* Draw second arrow */
	if ((pos / 6) == time.sec)
	{
		out |= 0x0000ffff;
	}
	
	return out;
}

/* Draw digital clock */
const uint8_t dig[10][5] = {
   {0b11111111, // 0
	0b10000001,
	0b10000001,
	0b10000001,
	0b11111111},
   {0b00000001, // 1
	0b01000001,
	0b11111111,
	0b00000001,
	0b00000001},
   {0b11110001, // 2
	0b10010001,
	0b10010001,
	0b10010001,
	0b10011111},
   {0b10000001, // 3
	0b10000001,
	0b10010001,
	0b10010001,
	0b11111111},
   {0b11110000, // 4
	0b00010000,
	0b00010000,
	0b00010000,
	0b11111111},
   {0b10011111, // 5
	0b10010001,
	0b10010001,
	0b10010001,
	0b11110001},
   {0b11111111, // 6
	0b10010001,
	0b10010001,
	0b10010001,
	0b10011111},
   {0b10000000, // 7
	0b10000000,
	0b10011111,
	0b10100000,
	0b11000000},
   {0b11111111, // 8
	0b10010001,
	0b10010001,
	0b10010001,
	0b11111111},
   {0b11110001, // 9
	0b10010001,
	0b10010001,
	0b10010001,
	0b11111111}
};
uint32_t draw_digital(uint16_t pos)
{
	uint32_t out = 0x00000000;
	if (pos < 30)
	{
	    if ((pos - 0) % 6 == 0)
	    {
		    out = dig[time.sec % 10][(pos - 0) / 6];
	    }
	} else if (pos > 330)
    {
        if ((pos - 330) % 6 == 0)
	    {
		    out = dig[(time.sec / 10) % 10][(pos - 330) / 6];
	    }        
    }
	return out << 10;
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
	/* DEBUG */
	output_to_sr(time.sec);
	TCNT2 = 0x00;
	time.sec++;
	if (time.sec == 60)
	{
		time.sec = 0;
		time.min++;
	}
	if (time.min == 60)
	{
		time.min = 0;
	}
}

/* Software voltage damper */
ISR( ADC_vect)
{
	cli(); 	
	
	uint16_t ADC_value;
	
	ADC_value = ADCL+((uint16_t) ADCH << 8);
	
	//if ( ADC_value > Dhigh)
		//PORTD = PORTD | (_BV(PIND4));
	//if ( ADC_value < Dlow)
		//PORTD = PORTD & (~(_BV(PIND4)));
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
	TCCR1B = 0x01; // clk/1
	OCR1A = 1111;  // ~7200hz interrupt
	TIMSK |= 0x10; // Enable compare interrupt
	
	/* Init 8-bit counter with external oscillator */
	TCCR2 = 0x07;          // clk/1024
	ASSR = (1 << AS2);     // Enable external oscillator
	OCR2 = 32;             // Interrupt every second
	TIMSK |= (1 << OCIE2); // Enable interrupt on compare match
	
	
	/* Turn on INT0 on falling edge */
	MCUCR = (1 << ISC01);
	GICR = (1 << INT0);
	
	/* Turn off leds by default by setting 1 to OE */
	led_off();
	
	/* Finally set desired sleep mode */
	set_sleep_mode(SLEEP_MODE_IDLE);
		
	sei();
}

int main(void)
{
	/* Device initialization */
	init();
	
	/* Main cycle */	
    while(1)
    {
        /* Do nothing. Sleep and wake on interrupts */
		sleep_mode();
    }
}