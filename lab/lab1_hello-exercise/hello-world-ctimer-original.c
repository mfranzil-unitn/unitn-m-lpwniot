#include "contiki.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h> /* For printf() */

#define PERIOD CLOCK_SECOND

static struct ctimer timer;  // timer object

// timer callback function (called by the system when the time comes)
static void timer_cb(void* ptr) {
  printf("Hello, world! I am 0x%04x (callback timer)\n", node_id);
  leds_toggle(LEDS_RED|LEDS_GREEN);
  ctimer_set(&timer, PERIOD, timer_cb, ptr); // set the same timer again
}

PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);

PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  
  // set the callback timer
  ctimer_set(&timer, PERIOD, timer_cb, NULL);
  
  PROCESS_END();
}

