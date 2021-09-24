bulbasaur
#6968
tacOS

Cosmic — Today at 3:52 PM
Im prolly picking something up omw back to my dorm but I'll keep posted
PrinCe_LoGic — Today at 5:29 PM
Image
@Cosmic
What do I say
bulbasaur — Today at 5:37 PM
who is this?
Cosmic — Today at 5:46 PM
SAY YES
JUST DO IT
PrinCe_LoGic — Today at 5:58 PM
alright, done
so what's the move about food padilla
Cosmic — Today at 6:03 PM
I am at a food truck rn 6-6
PrinCe_LoGic — Today at 6:03 PM
oh ok nice
what you gettimg
bulbasaur — Today at 6:04 PM
oh i forgot to tell yall
but i got one more test working
PrinCe_LoGic — Today at 6:05 PM
whoa nice
you're gonna have to catch us up to speed
bulbasaur — Today at 6:05 PM
i did like one thing
which is change the while loop statement
Cosmic — Today at 6:23 PM
Yeye epic!
Which test does it pass?
Aside from alarm-zero
bulbasaur — Today at 6:23 PM
alarm negative
its a temporary solution tho
well kinda
actually its not
Cosmic — Today at 6:24 PM
By that you mean we'd most likely keep that change you made?
Bc I might head back go my dorm shortly
bulbasaur — Today at 6:25 PM
ahh okay yeah im just sitting at patton hall right now since its so nice outside
Cosmic — Today at 6:32 PM
WAIT DANTHY
PrinCe_LoGic — Today at 6:53 PM
I’m eating at kins now
Cosmic — Today at 6:57 PM
This might take longer than i expected I'll come over as soon as I'm ready
PrinCe_LoGic — Today at 6:57 PM
Wym this might take longer
You been eating
Cosmic — Today at 6:57 PM
Im talking w the club people about semester plans T_T 
PrinCe_LoGic — Today at 6:58 PM
Quantum collective?
bulbasaur — Today at 6:58 PM
yeah she was meeting with them
PrinCe_LoGic — Today at 7:00 PM
Yo they had pull pork today
Smacks 😫😋
Cosmic — Today at 7:03 PM
Omw!!
Sorry for the lateness ahh
PrinCe_LoGic — Today at 7:09 PM
Wya
bulbasaur — Today at 7:09 PM
we are walking together
PrinCe_LoGic — Today at 7:09 PM
Where at
bulbasaur — Today at 7:09 PM
Welch
PrinCe_LoGic — Today at 7:10 PM
Tell me when you’re approaching kins:)
bulbasaur — Today at 7:10 PM
okay!
bulbasaur — Today at 7:18 PM
were here!
bulbasaur — Today at 8:56 PM
struct intr_frame *curr_interrupt;
https://prod.liveshare.vsengsaas.visualstudio.com/join?E76E43B020C7C8D0CA6921B494FF88E6D30B
Join my Visual Studio Live Share session
Real-time collaborative development
Image
Cosmic — Today at 10:25 PM
https://prod.liveshare.vsengsaas.visualstudio.com/join?758DC39A1766BC7641B895554B05438BE9E6
Join my Visual Studio Live Share session
Real-time collaborative development
Image
Cosmic — Today at 10:33 PM
https://prod.liveshare.vsengsaas.visualstudio.com/join?758DC39A1766BC7641B895554B05438BE9E6
Join my Visual Studio Live Share session
Real-time collaborative development
Image
https://prod.liveshare.vsengsaas.visualstudio.com/join?279583D013F4ADE74718720B8A900A629801
Join my Visual Studio Live Share session
Real-time collaborative development
Image
Cosmic — Today at 11:01 PM
#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
Expand
message.txt
9 KB
﻿
#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"



/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

// struct semaphore


/* List of processes in THREAD_BLOCKED state, that is, processes
   that are waiting for an event/resource/interrupt to end.*/
static struct list wait_list;


static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void) 
{
  list_init(&wait_list);
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void) 
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1)) 
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    //if (!too_many_loops (high_bit | test_bit)) 20190206 ans
    if (!too_many_loops (loops_per_tick | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}

/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
// TODO: wait w/o busy waiting
// more specifically:
    // make sema not NULL before entering init
    //
void
timer_sleep (int64_t ticks) 
{ 
  /**
   * Suspends execution of the calling thread until time has advanced by at least x timer ticks. 
   * Unless the system is otherwise idle, the thread need not wake up after exactly x ticks. 
   * Just put it on the ready queue after they have waited for the right amount of time
   */ 

  // Initiate the thread
  struct thread *curr = thread_current ();
  //sema_down(&curr->isAwake);
  // Insert thread into wait list
  //struct thread *thr_e = list_entry (e, struct thread, elem);
  list_push_back(&wait_list, &curr->elem);
  //list_insert(list_tail(&wait_list), &curr->elem);
  // 0 is true and 1 is false 
  //printf("Downing isAwake...\n");
  curr->timer.value = ticks + timer_ticks();
  // Add thread to waiting list
  //sema_down(&curr->isAwake);
  //sema_down(&curr->isAwake);

  //while (curr->timer.value > -1) 
  //{
    //printf("Downing timer...\n");
    //sema_down (&curr->timer); //time progresses
   
  //}

 sema_down(&curr->timer);

  //sema_up(&curr->isAwake);

  //sema_up(list_head(&wait_list)->next);

  //printf("%s \n", curr -> tid);
  //sema_up(&curr -> isAwake);
  
   // probs not right
  //sema_up (&curr->timer); 
   //list_remove(&curr -> elem);
   //figure out what goes in here
  
  //run idle if the ready queue is empty!

  //READY Q is moved with sema up
  
}
    

/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void
timer_msleep (int64_t ms) 
{
  real_time_sleep (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void
timer_usleep (int64_t us) 
{
  real_time_sleep (us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void
timer_nsleep (int64_t ns) 
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void
timer_mdelay (int64_t ms) 
{
  real_time_delay (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void
timer_udelay (int64_t us) 
{
  real_time_delay (us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void
timer_ndelay (int64_t ns) 
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}


/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick();
  //DanThy is driving here
  if (!list_empty(&wait_list)) {
    struct list_elem *curr_elem = list_begin(&wait_list);
    while (list_next(curr_elem) != list_tail(&wait_list)) {
      struct thread *curr_thread = list_entry(curr_elem, struct thread, elem);
      if (curr_thread->timer.value <= timer_ticks()) { //??
        sema_up(&curr_thread->timer);
        list_remove(curr_elem);
      }
      curr_elem = list_next(curr_elem);
    }
  }
 
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops) 
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait (loops);

  /* If the tick count changed, we iterated too long. */
  barrier ();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops) 
{
  while (loops-- > 0)
    barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom) 
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.
          
        (NUM / DENOM) s          
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks. 
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         timer_sleep() because it will yield the CPU to other
         processes. */                
      timer_sleep (ticks); 
    }
  else 
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      real_time_delay (num, denom); 
    }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000)); 
}