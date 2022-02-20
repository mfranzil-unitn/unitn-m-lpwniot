#include "contiki.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h> /* For printf() */

#define PERIOD CLOCK_SECOND

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

  while (1) {
    // set a timer
    etimer_set(&timer, PERIOD);
    // wait for it to expire
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    // print the node ID
    printf("Hello, world! I am 0x%04x\n", node_id);
    // toggle red and green LEDs
    leds_toggle(LEDS_RED|LEDS_GREEN); // see also leds_on(), leds_off()
  }
  
  PROCESS_END();              // all processes should end with PROCESS_END()
}

