#include "contiki.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h> /* For printf() */

#define PERIOD_ON (CLOCK_SECOND/10)
#define PERIOD_OFF (CLOCK_SECOND*9/10)

static struct ctimer timer;  // timer object

// declare functions in advance
static void timer_cb_on(void* ptr);
static void timer_cb(void* ptr);

// timer callback functions (called by the system when the time comes)
static void timer_cb(void* ptr) {
  leds_off(LEDS_RED|LEDS_GREEN);
  ctimer_set(&timer, PERIOD_OFF, timer_cb_on, ptr); // set the same timer again
}
static void timer_cb_on(void* ptr) {
  leds_on(LEDS_RED|LEDS_GREEN);
  ctimer_set(&timer, PERIOD_ON, timer_cb, ptr); // set the same timer again
}

PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);

PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  
  leds_on(LEDS_RED|LEDS_GREEN);

  // set the callback timer
  ctimer_set(&timer, PERIOD_ON, timer_cb, NULL);
  
  PROCESS_END();
}

