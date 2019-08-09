//******************************************************************************
// Metronome for MSP430G2xx1
//
// Description:
// Simple demonstration on how to use Port1 and interrupts to drive LED/speaker
// and to trigger a change in behavior. It is NOT optimzed towards power
// considerations. The Metronome can be build up with 5(6) LEDs, 2 buttons,
// a speaker (i.e. from a old mobile phone, other a driver might be require6d),
// a 3V battery, and some resistors in case the LEDs cannot be driven by 3V.
//
//                   MSP430G2xx1
//                 -----------------
//                |                 |                     Vcc
// Beeper/LED0 <--| P1.0            |                      ^
// |--R1--LED1 <--| P1.1            |                     R47k
// | -R2--LED2 <--| P1.2            |                      |
// | |... LED3 <--| P1.3        RST |--> --/Reset switch/---
// | | .. LED4 <--| P1.4       P1.7 |--> --/Faster Button/------ => Pullup mode
// | |  . LED5 <--| P1.5       P1.6 |--> --/Slower Button/ ---  | => Pullup mode
// v v            |                 |                        |  |
// GND                                                       v  v
//                                                            GND
//   
// DCO ~ 1MHz
//  
// T. Pham
// October 2011
// Built with Code Composer Studio Core Edition Version: 4.2.4.00033
//******************************************************************************

#include  <msp430g2231.h>
#include <signal.h>

/*
#define LED1 BIT1			// Toggling LED 1-5
#define LED2 BIT2
#define LED3 BIT3
#define LED4 BIT4
#define LED5 BIT5

*/
#define LED0 BIT0 // led0
#define LED6 BIT6 // led6
//#define LED2 BIT3			// Different order according
//#define LED3 BIT1			// to wiring on final board
//#define LED4 BIT4
//#define LED5 BIT2


#define SLOWERBUTTON BIT4
#define FASTERBUTTON BIT3
#define CALIB_TIME 56		// Calibration constant for exact beat frequency
#define NUM_BEATS 39		// Number of speeds 39
#define BEEPER BIT5			// Port to connect speaker
#define BEEPERTONE 180		// Beep tone/frequency 180
#define BEEPERDURATION 75	// Duration of beep 100

// Timing values in milliseconds correspond to 40, 42, 44, 46, 48, 50, 52, 54, 56,
// 58, 60, 63, 66, 69, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120,
// 126, 132, 138, 144, 152, 160, 168, 176, 184, 192, 200, 208 beats per minute
// Those are the beats that are on my mechanical metronome. You could add your owns. ej:60/1.5=40

const unsigned int timings[NUM_BEATS] = {1500, 1429, 1364, 1304, 1250, 1200, 1154, 1111,
1071, 1034, 1000, 952, 909, 870, 833, 789, 750, 714, 682, 652, 625, 600, 577,
556, 536, 517, 500, 476, 455, 435, 417, 395, 375, 357, 341, 326, 313, 300, 288};

/*
const unsigned int timings[NUM_BEATS] = {1500000, 1428571, 1363636, 1304348, 1250000, 1200000, 1153846, 1111111, 1071429,
1034483, 1000000, 952381, 909091, 869565, 833333, 789474, 750000, 714286, 681818, 652174, 625000, 600000, 576923, 555556,
535714, 517241, 500000, 476190, 454545, 434783, 416667, 394737, 375000, 357143, 340909, 326087, 312500, 300000, 288462};
*/


static unsigned int index = 0;	// Start at middle speed 13

void delay_ms(unsigned int ms);
void delay_us(unsigned int us);
void delay_cal(unsigned int cycles);
void beep(unsigned int note, long duration);
void ticktack(int index);



int main( void )
{
    WDTCTL = WDTPW + WDTHOLD; // Disable Watchdog Timer
    BCSCTL1 = CALBC1_1MHZ;    // Set range
    DCOCTL = CALDCO_1MHZ;     // Set DCO step + modulation
    
    P1DIR |= (BIT0+BIT5+BIT6);	// P1.0-P1.5 output

   	P1OUT = FASTERBUTTON + SLOWERBUTTON;		// P1.6, P1.7 set, else reset
	P1REN |= FASTERBUTTON + SLOWERBUTTON;		// P1.6, P1.7 internal pullup resistor
    P1IE |= FASTERBUTTON + SLOWERBUTTON;		// P1.6, P1.7 interrupt enabled
    P1IES |= FASTERBUTTON + SLOWERBUTTON;		// P1.6, P1.7 Hi/Lo edge
    P1IFG &= ~FASTERBUTTON + SLOWERBUTTON;		// P1.6, P1.7 IFG cleared
    
    __enable_interrupt();     // enable all interrupts
	
    while(1)
    {
		ticktack(index);	// Do LED0-LED4 tick-tack routine using timing index
    }
}



// Port 1 interrupt service routine
// LED blinking to indicate speed change and if min/max speed is reached
// OR beeper sound, depending which one is hooked up.
// Obviously, LED and speaker cannot be hooked up at the same time
// Might be better to route to different outputs

//#pragma vector = PORT1_VECTOR
//__interrupt void Port_1(void) 

interrupt(PORT1_VECTOR) Port_1 (void)
{
        delay_ms(500); //mejora debounce
	if(P1IFG & FASTERBUTTON && index < NUM_BEATS-1)
    {
    	index++;			// Increases beat freq.
    	P1OUT &= ~BIT0;		// Turn off LED @ P1.0 if max/min freq. reached
    }
    else if(P1IFG & SLOWERBUTTON && index > 0)
    {
    	index--;			// Decreases beat freq.
    	P1OUT &= ~BIT0;		// Turn off LED @ P1.0 if max/min freq. reached
    }
    
	if(index != 0 && index != NUM_BEATS-1)
	{
		delay_cal(1500);	//
		P1OUT |= BIT0;		// Turn on LED @ P1.0 if max/min freq. reached
	}
	else
	{
		P1OUT |= BIT0;
	}
	
	P1IFG &= ~(FASTERBUTTON+SLOWERBUTTON);	// P1.6, P1.7 IFG cleared
}


void delay_ms(unsigned int ms)
{
    unsigned int i;
    for (i = 0; i<= ms; i++)
    __delay_cycles(1000);
}

void delay_us(unsigned int us)
{
    unsigned int i;
    for (i = 0; i<= us; i++)
       __delay_cycles(1);
}
void delay_cal(unsigned int cycles)
{
    unsigned int i;
    for(i = 0; i <= cycles; i++)
    {
    	__delay_cycles(CALIB_TIME);	//Delay with built-in function
    }
}

// Beep Code original autor is kphlight
// http://www.instructables.com/id/
// Intro-to-Microcontroller-Debugging-and-a-Pomodoro/
// step9/Super-Plumber-Pomodoro-Timer-Code/

void beep(unsigned int note, long duration) {

    long delay = (long)(62500/note); //This is the semiperiod of each note 62500 102500
    long time = (long)((duration*100)/delay); //This is how much time we need to spend on the note 100 75
	long i;
    for (i = 0; i < time; i++) {
        P1OUT |= BEEPER; //Set buzzer on...
        delay_us(delay); //...for a semiperiod...
        P1OUT &= ~BEEPER; //...then reset it...
        delay_us(delay); //...for the other semiperiod.
    }
}

// 'Tick-tack routine'
// Argument: index of time duration in array timing[]
// Delays are accordingly picked to give a realistic,
// smoother blinking transition of the LEDs
// One could use another array with other timing values to achieve
// desired blinking effect, i.e. transitions[] = {2,5,2,2,1,1,1,2... etc.

void ticktack(int index)	 
{
	unsigned int beat = timings[index];	// Take next/previous timing

	P1OUT |= BIT6;
	beep(BEEPERTONE, BEEPERDURATION);
	P1OUT &= ~BIT6;	
delay_ms(beat);
/*
    delay_cal(beat*2);
  
    delay_cal(beat*5);

    delay_cal(beat*2);
  
    delay_cal(beat*2);

    delay_cal(beat*1); 
   
    delay_cal(beat*1);
  
    delay_cal(beat*1);
 
    delay_cal(beat*2);
*/
    P1OUT |= BIT6;
    beep(BEEPERTONE*2, BEEPERDURATION);
    P1OUT &= ~BIT6;
delay_ms(beat);
/*  
    delay_cal(beat*2);
    
    delay_cal(beat*5);
   
    delay_cal(beat*2);
    
    delay_cal(beat*2);
    
    delay_cal(beat*1);
   
    delay_cal(beat*1);
    
    delay_cal(beat*1);
  
    delay_cal(beat*2);
*/	
}


