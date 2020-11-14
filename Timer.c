
#include <cmsis_os.h>                                           // CMSIS RTOS header file
#include "stm32f4xx_gpio.h"

/*----------------------------------------------------------------------------
 *      Timer: Sample timer functions
 *---------------------------------------------------------------------------*/
extern osThreadId thread_id;
int timer2 = 1;
int timer1 = 0;

/*----- One-Shoot Timer Example -----*/
static void Timer1_Callback  (void const *arg);           // prototype for timer callback function
static osTimerId id1;                                     // timer id
static uint32_t  exec1;                                   // argument for the timer call back function
static osTimerDef (Timer1, Timer1_Callback);              // define timers

// One-Shoot Timer Function
static void Timer1_Callback  (void const *arg)  {
  // add user code here
	timer1 = 1;
}	
	

/*----- Periodic Timer Example -----*/
static void Timer2_Callback  (void const *arg);           // prototype for timer callback function
static osTimerId id2;                                     // timer id
static uint32_t  exec2;                                   // argument for the timer call back function
static osTimerDef (Timer2, Timer2_Callback);                                      
 
// Periodic Timer Example
static void Timer2_Callback  (void const *arg)  {
	osSignalSet (thread_id, 1);
	timer2 = 1;
}	


	
// Example: Create and Start timers
void Init_Timers (void) {
  osStatus  status;                                        // function return status
 
  // Create one-shoot timer
  exec1 = 1;
  id1 = osTimerCreate (osTimer(Timer1), osTimerPeriodic, &exec1);
  if (id1 != NULL)  {     // One-shot timer created
    // start timer with delay 100ms
		status = osTimerStart (id1,1000);            
    if (status != osOK)  {
      // Timer could not be started
    }
	}
 
  // Create periodic timer
  exec2 = 2;
  id2 = osTimerCreate (osTimer(Timer2), osTimerPeriodic, &exec2);
  if (id2 != NULL)  {     // Periodic timer created
    // start timer with periodic 1000ms interval
		status = osTimerStart (id2, 1000);          
    if (status != osOK)  {
      // Timer could not be started
    }
  }
}

