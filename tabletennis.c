/***************************************************************************
 * Author: Brendan Moriarty												   * 
 * Date : July 2012														   *
 * Organization: Rose-Hulman Institute of Technology: Operation Catapult   *
 * Title: Smart Ping Pong Trainer Turret								   *
 * Description: This program controls the Ping Turret which utilizes	   *
 *			    four PWM controlled motors, one PWM servo, and four 	   *
 *              switches.  This code utilizes Timer interrupt and Port B   *
 *			    interrupt as well as ATD (Analog to Digital)			   *
 ***************************************************************************/


/*******************************************************************************************************
 * TIP: To look up register values look at a datasheet.  For this particular PIC chip the datasheet is:*
 *	http://ww1.microchip.com/downloads/en/DeviceDoc/41291b.pdf										   *
 *******************************************************************************************************/

/******************************************************************************************
 * These includes are to add basic libraries to allow use of certain PIC related commands *


 ******************************************************************************************/

#include <pic.h>  		// This header file contains symbolic register addresses

//#include <math.h>		

                     	// and to access other resources on the PIC 16F877 microcontroller

/*******************************************************************************************
 * These statements are added by default through the Project wizard and configure          *   
 * the coding for the PIC																   *
 *******************************************************************************************/
#if defined(_16F883) |  defined(_16F877A)	//configuration bits
	__CONFIG(CP_OFF &  FOSC_INTRC_NOCLKOUT  & FCMEN_OFF & WDTE_OFF & PWRTE_ON & LVP_OFF);

    		 /* NOTE: You can view what these configurations mean in Configure -> Configuration Bits  */
	
	
#endif       //#if_defined and #endif enable these configurations only if a specific chip is used (in this case PIC16F883 or PIC16F877A



/*******************************************************************************************
 *	These statements (defines) set a recognition within the software for a phrase that 	   *
 *	essentially just replaces the phrase with whatever it is defined as.				   *
 *	In this case, it is defining specific pins used for the motors and servos and some 	   *
 *	configuration statements to make them work			   								   *
 *******************************************************************************************/



/* These four defines (PWMxPin) x:{1,4} are the four motors used.  The statements allow "PWMxPin to be used in place of "RCx" when toggling power from the pin    */
#define PWM4Pin	RC0				//pin 11 					
#define PWM1Pin	RC1				//pin 12 of 16F883 PDIP		
#define PWM2Pin	RC2				//pin 13 of 16F883 PDIP
#define PWM3Pin	RC3				//pin 14
#define ServoControlPin RA1  	//pin 3



/*  These statements are used to set pins RC0-RC3 to outputs.  This means power will be sent from the pin as opposed to recieving it.
	Note that the statement TRISXx is in the format where Xx is the second and third digits of the pin address (TRISC1 ~ RC1)        */

#define PWM1PinDirection  TRISC1=0;
#define PWM2PinDirection  TRISC2=0;
#define PWMPin3Configuration TRISC3=0;
#define PWMPin4Configuration TRISC0=0;




#define INITIAL_COUNT 200			// 128 microseconds per interrupt at 8MHz || This is used to set how long the interrupt will run for

#define SERVO_PERIOD 200			// 11.5ms period  || This determines the period of the Servo PWM.  Period can be between 1 - 512 and sets how long a pulse runs for
#define PULSE_PERIOD 200			//11.5ms period	  || This is the period for the motor.
unsigned int PULSELENGTH = 14;		// The pulselength value sent to the servo.  This value requires some calibration so mess with this number to change the speed of 
									// the servo



void InterruptInitialization(void); 			 // This function is used to initialize necessary registers in the hardware in order for the Interrupt to work
void interrupt MotorDriver(void);				 // This is the Interrupt.  This is the statement that will control the PWM and the Port B switches
												 // Note that the Port B interrupt for switches must also be within the same interrupt as the Timer interrupts
												 // You cannot have more than one interrupt statements because the compiler will get confused and give you an error.
void ATD_initialization(unsigned char channel);  // This is ATD or Analog-to-digital.  This is used to get a voltage reading from a pin and convert it to a 10 bit number (2^10 - 1)
												 // Note that you need Dr. Song's ATDroutines.c file included in your project for this to work.
												 
void Get_Voltage(unsigned int *Voltage);		 // This is the atual function that will retrieve the voltage from a pin.

// interrupt duration
	


unsigned int period, pulsewidth, count;    												// Defines some integers used in the PWM process.
unsigned int pulsewidth3, PULSELENGTH3, pulsewidth4, PULSELENGTH4;						// Defines the statements to be used by PWM.  These should be modified to change speeds
unsigned int pulsewidth1, PULSELENGTH1, pulsewidth2, PULSELENGTH2, count, PulsePeriod;
unsigned int Voltage; //0 to 1023														// This is the value returned by Get_Voltage(). 
																						// This value can be used in conditionals (see below) to adjust speed.



unsigned int pl =40,ph=200;  					   // defines two variables for easy calibration. 
												   // pl ~ Pulse low: the value of the motor running slower in order to add spin 
						       					   // ph ~ Pulse high: the speed the motor will run at when you want it running normally.







void main(void)
  {

	ATD_initialization(2);		// Allows use of Get_Voltage.  The value passed (2) means pin RA2. See ATDroutines for more explanation

	InterruptInitialization();	// Initializes flags in order for the interrupt to work on the hardware side of things.




#if defined(_16F883)
//	IRCF2=1;	IRCF1=0;	IRCF0=0;				//1 MHz, Internal Oscillator Frequency Select bits || This refers to the Internal Oscillator which
													//  determines how fast the clock goes and in turn affects the PWM speed.

	IRCF2=1;	IRCF1=1;	IRCF0=1;				//8 MHZ ||Sets the oscillator to 8MHz by setting all three bits to 1.  The settings vary from 000: 31kHz to 111 8Mhz
#endif
	
	PWM1PinDirection								//4x PWM Motors 
	PWM2PinDirection
	PWMPin3Configuration	
	PWMPin4Configuration
	TRISB5=0; ANS13=0; IOCB5=0; WPUB5=0, nRBPU = 0;	
	TRISB0=1; ANS12=0; IOCB0=1; WPUB0=1; 			//Switches for spin
	TRISB1=1; ANS10=0; IOCB1=1; WPUB1=1; 			//TRIS: 0/1:output/input
	TRISB2=1; ANS8=0;  IOCB2=1; WPUB2=1;			//ANSxx=0/1 toggles analog input on switch.  Analog is enabled by default. This disables it for digital use 
	TRISB3=1; ANS9=0;  IOCB3=1; WPUB3=1;  			//B1-B5 is used for switches (digital) 
	TRISB4=1; ANS11=0; IOCB4=1; WPUB4=1;			//IOCB: Must be enabled for Port B interrupt. 
	TRISB5=1; ANS13=0; IOCB5=1; WPUB5=1;			//WPUB: Weak Pull-ups Port B. Sets B1-B5 as pull-ups. Necessary for Port B interrupt.
	TRISA1 = 0;										//Servo: TRISA1 makes it an output
	TMR2ON=1;										//Turns on Timer2 
	TRISA0 = 0;
	RA0 = 1;
	
	
	InterruptInitialization();						//Calls the Initialization function
	PulsePeriod = ph;								//Sets the PulsePeriod to the same value as the length which sets the motors to full speed

	PWM1Pin=1;										// Turns on the motors by sending power to the pin the motor is connected to
	PWM2Pin=1;
	PWM3Pin=1;
	PWM4Pin=1;

	PULSELENGTH1=ph;								// Sets all motors to full speed 
	PULSELENGTH2=ph;
	PULSELENGTH3=ph;
	PULSELENGTH4=ph;
	

	while(1){
	Get_Voltage(&Voltage);					//Gets a reading of the voltage 

	if(Voltage<100)							//Conditionals to adjust the speed of the motors based on the current position of the potentiometer.
	PulsePeriod = 512;						//a greater period means that the pulse runs longer which means that the motors go slower
											//as the Voltage increases, the resistance from the potentiometer must be decreasing so as the knob turns the Pulse is 
											//decreased to change the speed as the knob is turned.

	if((Voltage<=200)&&(Voltage>=100))
	PulsePeriod = 450;	

	if((Voltage>200)&&(Voltage<=300))
	PulsePeriod = 400;

	if((Voltage>300)&&(Voltage<=400))
	PulsePeriod = 350;

	if((Voltage>400)&&(Voltage<=500))
	PulsePeriod = 300;

	if((Voltage>500)&&(Voltage<=600))
	PulsePeriod = 250;

	if((Voltage>700)&&(Voltage<=800))
	PulsePeriod = 200;

	if((Voltage>800)&&(Voltage<=900))
	PulsePeriod = 250;

	if((Voltage>900)&&(Voltage<1000))
	PulsePeriod = 200;

	if(Voltage>=1000)
	PulsePeriod = 100;

}

}
//Initialize Timer0 for interrupt to generate servo control signal
void InterruptInitialization(void) {
//Timer0 Interrupt initialization
	T0CS = 0;	// Set prescaler to Timer0: Prescalar affects the PWM by a ratio based on the PSx bits.
	PSA=0;		// interrupt source connected to prescaler
	PS2=0;		//Disables watchdog timer to avoid interference with Timer Interrupt.	(Watchdog Timer: Interrupt based on when something bad happens, could interfere.)
	PS1=0;		//PSx x:0/1/2 disables all three bits of the watchdog register
	PS0=0;
// Timer0 interrupt
	T0IF=0;		//T0IF Timer0 Interrupt flag.  (0/1) When enabled program executes the interrupt (MotorDriver)
	T0IE=1;		//Timer0 Interrupt Enabled.  Necessary to use Timer0
	TMR0=INITIAL_COUNT;	// interrupt occurs every 10 microseconds

//set up RB0 RB Change Interrupt
	RBIE=1;	//enable RB change interrupt
	RBIF=0;
	GIE = 1;	// turn on global interrupt flag
}//end of Timer0_Initialization()

// Interrupt service routine to generate servo control signal

void interrupt MotorDriver(void)
{   
	if(T0IF==1) {											// If the T0IF flag is triggered, enter the statement to send a pulse.
		if (period!=0) period = period --;					// This makes the if continue running until it equals 0, determining how long the pulse is
		else {												// If the period is not yet 0, initialize all the PWM values.
			period = PulsePeriod;							// initialize period
			pulsewidth1=PULSELENGTH1;						// initialize pulse
			pulsewidth2=PULSELENGTH2;						// initialize pulse
			pulsewidth3=PULSELENGTH3;						// initialize pulse
			pulsewidth4=PULSELENGTH4;						// initialize pulse
			PWM1Pin=1;	//start pulse
			PWM2Pin=1;	//start pulse
			PWM3Pin=1;	//start pulse
			PWM4Pin=1;	//start pulse
		}
	if (pulsewidth1!=1) pulsewidth1--;						// depending on how big the period is, the motor will have a certain amount of time turned on
		else 												// Example: the period is 200 and the length is 200.  If the length is 200, the motors will run full speed
			PWM1Pin=0;	//end pulse							//  because when the period decrements to 0, the width will have decremented to 0, meaning it does not turn off
															//  this makes it run at full speed.
	if (pulsewidth2!=1) pulsewidth2--;
		else 
			PWM2Pin=0;	//end pulse

	if (pulsewidth3!=1) pulsewidth3--;
		else 
			PWM3Pin=0;	//end pulse

	if (pulsewidth4!=1) pulsewidth4--;
		else 
			PWM4Pin=0;	//end pulse

		TMR0=INITIAL_COUNT;								  // Determine how often the timer is triggered.  The higher the count, the less often it checks for a change in 
														  // the Pulse width.
	




	if (period!=0) period = period --;				      // Same as for the motor, except for the Servo
		else {
			period = SERVO_PERIOD;	//initialize period
			pulsewidth=PULSELENGTH;	//initialize pulse
			ServoControlPin=1;	//start pulse
		}
		if (pulsewidth!=0) pulsewidth--;
		else 
			ServoControlPin=0;	//end pulse
	
		TMR0=INITIAL_COUNT;								   // same as above


		T0IF=0;											   // Toggles the interrupt to exit the interrupt.	


	}
if(RBIF == 1)											   // This is the "Port B" interrupt.  This is triggered when a voltage is deteceted on a RB pin
{ 
	if( RB2 == 0)      // Note that it is checking if it is zero.  This is because it is a pull-up switch.  If a reading is detected on RB2, slow down Motor 1. (topspin)
	PULSELENGTH1 = pl;

		
	if( RB1 == 0)
	PULSELENGTH2 = pl; //sidespin left

	
	if( RB4 == 0)		//sidespin right
	PULSELENGTH3 = pl;

	if( RB5 == 0)		//underspin
	PULSELENGTH4 = pl;
		
	if((RB2 == 1)&&(RB0 == 1))    //These statements were written to return the speed to full speed when the switch is off.  
	PULSELENGTH1 = ph;			  //Notice the RB0 is also checked to make sure that it does not interfere with the random spin case below.
	
	if((RB1 == 1)&&(RB0 == 1))
	PULSELENGTH2 = ph;

	if((RB4 == 1)&&(RB0 == 1))
	PULSELENGTH3 = ph;

	if((RB5 == 1)&&(RB0 == 1))
	PULSELENGTH4 = ph;


    if(RB0 == 0)
	{ 
	int spin = rand() %7;		// picks a random number between 0 and 7 and stores in spin. 
      
	switch(spin){				// picks a case based on the random number picked by spin 
		case 0: {
				PULSELENGTH1 = pl; //sets one motor to a slower speed to induce spin
				PULSELENGTH2=ph;   //sets the rest to high.
				PULSELENGTH3=ph;
				PULSELENGTH4=ph;

				break;}
		case 1: {
				PULSELENGTH2 = pl;
				PULSELENGTH2=ph;
				PULSELENGTH3=ph;
				PULSELENGTH4=ph;
				
				break;}
		case 2: {
				PULSELENGTH3 = pl;
				PULSELENGTH2 = ph;
				PULSELENGTH3 = ph;
				PULSELENGTH4 = ph;
				break;}
		case 3: {
				PULSELENGTH4 = pl;					
				PULSELENGTH2 = ph;
				PULSELENGTH3 = ph;
				PULSELENGTH1 = ph;
				break;}
		case 4: {
				PULSELENGTH1 = pl, PULSELENGTH2 = pl;
				PULSELENGTH4 = ph;
				PULSELENGTH3 = ph;
				break;}
		case 5: {
				PULSELENGTH1 = pl, PULSELENGTH4 = pl;				
				PULSELENGTH2 = ph;
				PULSELENGTH3 = ph;
				break;}
		case 6: {
				PULSELENGTH2 = pl, PULSELENGTH3 = pl;
				PULSELENGTH1 = ph;
				PULSELENGTH3 = ph;
				break;}
		case 7: {
				PULSELENGTH3 = pl, PULSELENGTH4 = pl;
				PULSELENGTH1 = ph;
				PULSELENGTH2 = ph;
				break;}
		
	   default:{
				PULSELENGTH1 = ph;
				PULSELENGTH2 = ph;
				PULSELENGTH3 = ph;
				PULSELENGTH4 = ph;
				break;}
	}	

	}			





RBIF = 0;	// resets the Port B interrupt flag.

}


}