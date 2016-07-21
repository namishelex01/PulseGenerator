#include "msp430G2553.h"
#include "stdint.h"

void main( void )
{
  WDTCTL = (WDTPW | WDTHOLD);                    // Stop watchdog timer

  BCSCTL1 = CALBC1_1MHZ;                         // Set range
  DCOCTL  = CALDCO_1MHZ;                         // Set DCO step and modulation

  P2OUT |= (BIT0 | BIT1 | BIT2 | BIT4);          // P2.0/1/2 high
  P2DIR |= (BIT0 | BIT1 | BIT2);                 // P2.0/1/2 to output direction

  TA0CCR0  = 2000;                               // Timer value for 2ms interrupt
  TA0CCTL0 = CCIE;                               // Enable compare interrupt
  TA0CTL   = (TASSEL_2 | ID_0 | MC_2 | TACLR);   // SMCLK, divider 1, continuous mode, clear

  __bis_SR_register( GIE );                      // Enable global interrupts

  while( 1 )                                     // Main program
  {

  }
}

// Timer 0 A0 interrupt service routine
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR( void )
{
  static uint8_t state = 0;                      // Variable holds actual state

  TA0CCR0 += 2000;                               // Timer value for next interrupt in 2ms

  switch( state )                                // Determine actual state
  {
    case 0:                                      // State 0
    {
      P2OUT |= BIT2;                             // Set P2.2 high
      P2OUT &= ~BIT0;
      break;
    }
    case 1:                                      // State 1
    {
      P2OUT |= BIT0;                             // Set P2.0 high
      P2OUT &= ~BIT1;                            // Set P2.1 low
      break;
    }
    case 2:                                      // State 2
    {
      P2OUT |= BIT1;                             // Set P2.1 high
      P2OUT &= ~BIT2;                            // Set P2.2 low
      break;
    }
  }

  if( ++state >= 3 )                             // Increment state and test if >= 3
  {
    state = 0;                                   // Reset state to 0
  }
}
