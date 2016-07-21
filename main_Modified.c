#include "msp430G2553.h"
#include "stdint.h"

volatile uint16_t down_counter;                  // Variable for counting down
volatile uint8_t  output_control = 0;            // Variable controls output activity
volatile uint8_t  input_state    = 0;            // Variable holds actual input state


void main( void )
{
  WDTCTL = (WDTPW | WDTHOLD);	                 // Stop watchdog timer

  BCSCTL1 = CALBC1_1MHZ;                         // Set range
  DCOCTL  = CALDCO_1MHZ;                         // Set DCO step and modulation
	
  P2REN |= BIT4;                                 // Enable resistor for P2.4
  P2OUT |= (BIT0 | BIT1 | BIT2 | BIT4);          // P2.0/1/2 high, pull-up resistor for P2.4
  P2DIR |= (BIT0 | BIT1 | BIT2);                 // P2.0/1/2 to output direction
  P2IES |= BIT4;                                 // Interrupt on high->low transition
  P2IFG &= ~BIT4;                                // Clear P2.4 IFG
  P2IE  |= BIT4;                                 // Enable interrupts for P2.4

  TA0CCR0  = 2000;                               // Timer value for 2ms interrupt
  TA0CCTL0 = CCIE;                               // Enable compare interrupt
  TA0CTL   = (TASSEL_2 | ID_0 | MC_2 | TACLR);   // SMCLK, divider 1, continuous mode, clear

  // Input emulation
  P1OUT |= BIT0;                                 // P1.0 high
  P1DIR |= BIT0;                                 // P1.0 output - emulates output of sensor

  TA1CCR0  = 62500;                              // Timer value for 500ms interrupt
  TA1CCTL0 = CCIE;                               // Enable compare interrupt
  TA1CTL   = (TASSEL_2 | ID_3 | MC_2 | TACLR);   // SMCLK, divider 8, continuous mode, clear
  // Input emulation

  __bis_SR_register( GIE );                      // Enable global interrupts

  while( 1 )                                     // Main program
  {

  }
}


// Input emulation
// Timer 1 A0 interrupt service routine
#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR( void )
{
  static uint8_t timer_state = 0;                // Variable holds actual timer state
  static uint8_t multiplier  = 0;                // Variable to extend timer interval

  switch( timer_state )                          // Determine actual timer state
  {
    case 0:                                      // Delay between pulses
    {
      if( ++multiplier < 2 )                     // Not 1s elapsed yet
      {
        TA1CCR0 += 62500;                        // Add 500ms
      }
      else                                       // 1s elapsed
      {
        TA1CCR0 += 12500;                        // Add 100ms
        P1OUT &= ~BIT0;                          // P1.0 low
        multiplier  = 0;                         // Reset multiplier
        timer_state++;                           // Increment timer state
      }

      break;
    }

    case 1:                                      // Signal was low for 100ms
    {
      TA1CCR0 += 50000;                          // Add 400ms
      P1OUT |= BIT0;                             // P1.0 high
      timer_state++;                             // Increment timer state

      break;
    }

    case 2:                                      // Signal was high for 400ms
    {
      TA1CCR0 += 12500;                          // Add 100ms
      P1OUT &= ~BIT0;                            // P1.0 low
      timer_state++;                             // Increment timer state

      break;
    }

    case 3:                                      // Signal was low for 100ms
    {
      TA1CCR0 += 62500;                          // Add 500ms
      P1OUT |= BIT0;                             // P1.0 high
      timer_state = 0;                           // Reset timer state

      break;
    }
  }
}
// Input emulation


// Timer 0 A0 interrupt service routine
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR( void )
{
  static uint8_t output_state = 0;               // Variable holds actual output state

  TA0CCR0 += 2000;                               // Timer value for next interrupt in 2ms

  if( down_counter )                             // Test if > 0
  {
    down_counter--;                              // Decrement counter

    if( !output_control )                        // Test if output is currently off
    {
      if( !down_counter )                        // Test if down counter became 0 now
      {
        input_state = 0;                         // Reset input state
        P2IE  &= ~BIT4;                          // Disable interrupts for P2.4
        P2IES |= BIT4;                           // Interrupt on high->low transition
        P2IFG &= ~BIT4;                          // Clear P2.4 IFG
        P2IE  |= BIT4;                           // Enable interrupts for P2.4
      }
    }
  }

  if( output_control )                           // Test if output is active
  {
    switch( output_state )                       // Determine actual output state
    {
      case 0:                                    // State 0
      {
        P2OUT &= ~BIT0;                          // Set P2.0 low
        output_state++;                          // Increment output state
        break;
      }

      case 1:                                    // State 1
      {
        P2OUT |= BIT0;                           // Set P2.0 high
        P2OUT &= ~BIT1;                          // Set P2.1 low
        output_state++;                          // Increment output state
        break;
      }

      case 2:                                    // State 2
      {
        P2OUT |= BIT1;                           // Set P2.1 high
        P2OUT &= ~BIT2;                          // Set P2.2 low
        output_state++;                          // Increment output state
        break;
      }

      case 3:                                    // State 3
      {
        P2OUT |= BIT2;                           // Set P2.2 high
        output_state = 0;                        // Reset output state
        output_control = 0;                      // Reset output control
        P2IFG &= ~BIT4;                          // Clear P2.4 IFG
        P2IE  |= BIT4;                           // Enable interrupts for P2.4
        break;
      }
    }
  }
}


// Port 2 interrupt service routine
#pragma vector = PORT2_VECTOR
__interrupt void Port2_ISR( void )
{
  switch( input_state )                          // Determine actual input state
  {
    case 0:                                      // State 0: High->low transition of first pulse
    {
      down_counter = 60;                         // Timeout of 120ms
      input_state++;                             // Increment input state
      P2IES &= ~BIT4;                            // Interrupt on low->high transition
      break;
    }

    case 1:                                      // State 1: Low->high transition of first pulse
    {
      down_counter = 225;                        // Timeout of 450ms
      input_state++;                             // Increment input state
      P2IES |= BIT4;                             // Interrupt on high->low transition
      break;
    }

    case 2:                                      // State 2: High->low transition of second pulse
    {
      down_counter = 60;                         // Timeout of 120ms
      input_state++;                             // Increment input state
      P2IES &= ~BIT4;                            // Interrupt on low->high transition
      break;
    }

    case 3:                                      // State 3: Low->high transition of second pulse
    {
      P2IES |= BIT4;                             // Interrupt on high->low transition
      output_control = 1;                        // Activate output
      P2IE &= ~BIT4;                             // Disable interrupts for P2.4
      input_state = 0;                           // Reset input state
      break;
    }
  }

  P2IFG &= ~BIT4;                                // Clear P2.4 IFG
}
