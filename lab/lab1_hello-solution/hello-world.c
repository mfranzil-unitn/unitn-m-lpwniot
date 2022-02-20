#include "contiki.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h> /* For printf() */

#define PERIOD_ON (CLOCK_SECOND/10)
#define PERIOD_OFF (CLOCK_SECOND*9/10)

// declare a process
PROCESS(hello_world_process, "Hello world process");
// list processes to start at boot
AUTOSTART_PROCESSES(&hello_world_process);

// implement the process thread function
PROCESS_THREAD(hello_world_process, ev, data)
{
  // timer object
  static struct etimer timer; // ALWAYS use static variables in processes!

  PROCESS_BEGIN();            // all processes should start with PROCESS_BEGIN()

  printf("CLOCK_SECOND:%lu 100ms:%lu 900ms:%lu\n", CLOCK_SECOND, PERIOD_ON, PERIOD_OFF);

  while (1) {
    leds_on(LEDS_RED|LEDS_GREEN);

    // set a timer
    etimer_set(&timer, PERIOD_ON);
    // wait for it to expire
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    leds_off(LEDS_RED|LEDS_GREEN);

    // set (the same) timer
    etimer_set(&timer, PERIOD_OFF);
    // wait for it to expire
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  }
  
  PROCESS_END();              // all processes should end with PROCESS_END()
}

