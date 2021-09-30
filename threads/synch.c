/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

static bool
sort_priority(const struct list_elem *first, const struct list_elem *second, void *aux UNUSED)
{

 const struct thread *first_thread = list_entry(first, const struct thread, elem);
 const struct thread *second_thread = list_entry(second, const struct thread, elem);
 return first_thread -> priority > second_thread -> priority; 
}

/* Comparator that returns whether or not the lock is greater 
   than the second lock */
static bool
sort_lock_priority (const struct list_elem *first, const struct list_elem *second, void *aux UNUSED)
{
  return list_entry(first, struct lock, lock_elem)->priority > 
         list_entry(second, struct lock, lock_elem)->priority;
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_insert_ordered (&sema->waiters, &thread_current ()->elem,
                    &sort_priority, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;
  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) 
  {
    /* Sort the waiters list in case any have been donated priority to */
    list_sort(&sema->waiters, sort_priority, NULL);
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));
  }
  sema->value++;
  if (!intr_context())
    thread_yield();
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
  lock->priority = PRI_DEFAULT;
  lock->is_not_null = 0;
  lock->isntChain = 1;
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock)); 

  enum intr_level old_level;
  old_level = intr_disable ();
  struct thread *t_curr = thread_current ();
  
  /*
     ORIGINAL CODE:
      if (lock->holder != NULL) 
      {
        struct thread *low_lock = lock->holder;
        low_lock->priority = thread_get_priority();
        thread_yield();
      }
  */

  /* If the lock doesn't have a holder, then this thread will be
     its holder, so set the lock's priority to reflect this. */
  if (!lock->holder)
    lock->priority = t_curr->priority;

  // DanThy and Emily are driving here
  // If the lock has a holder and that holder's priority is less than curr...
  if (lock->holder && lock->holder->priority < t_curr->priority) {
    /* Set our needed lock for current thread & donate priority along to
       the lower lock holders */
    t_curr->neededLock = *lock;
    t_curr->neededLock.is_not_null = 1;
    lock -> isntChain = 0;
    next_lock_needed (lock);
    /* If the curr thread priority has increased, set it to reflect change */
    if (lock->priority < t_curr->priority)
      lock->priority = t_curr->priority;
  }

  /* I think you *would* want to yield here, since after potentially 
     having donated the thread's priority, you want to move on to the next one */
  thread_yield (); // original code
  sema_down (&lock->semaphore); // original code
  lock->holder = t_curr; //original code

  /* Add lock to thread_current's list in lock priority order */
  list_insert_ordered(&(t_curr->all_locks_held), &(lock->lock_elem), sort_lock_priority, NULL);

  t_curr->neededLock.is_not_null = 0;
  intr_set_level (old_level);  //original code
}

/* Finds out which thread holds lock needed by current thread 
   and possibly the holder's needed lock as well, passing along 
   the current thread's priority along to donate. */
static void
next_lock_needed(struct lock *lock) {
    /* Go down the rabbithole and get to the thread that is not 
       waiting on any lock. */
    if (lock->holder->neededLock.is_not_null == 1) {
      next_lock_needed (&lock->holder->neededLock);
    }
    // This should be the next lowest lock holder.
    struct thread *low_lock = lock->holder;
    // Set the donation status of the lower locks to true
    low_lock->current_dono = 1;
    // Set this lock holder's priority to curr thread's priority.
    low_lock->priority = thread_get_priority();
    return;
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
  /*
  check if lock->holder is an existing thread
  */
    lock->holder = thread_current ();
    //list_push_front (&thread_current() -> all_locks_held, &lock->lock_elem);
    /*
    add lock to thread_current's list
    */
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));
  struct thread *t_curr = thread_current ();
  lock->holder = NULL;
  // Remove the lock from list of locks held from thread.
  list_remove (&lock->lock_elem);
  
  // If the curr thread has been donated priority...
  if (t_curr->orig_priority != t_curr->priority) {
    // Check if there were other donations made.
    if (list_empty (&(t_curr->all_locks_held))) {
      t_curr->priority = t_curr->orig_priority;
      // Turn off donation mode.
      t_curr->current_dono = 0;
    }
    else {
      /* It needs to return to its previous donated priority, set it to
         the next highest lock holder priority */
      list_sort(&(t_curr->all_locks_held), sort_lock_priority, NULL);

      // if the lock is part of a chain, don't set priority
      if(lock -> isntChain == 0)
      {
      struct lock *next_highest_lock = list_entry(list_front(&(t_curr->all_locks_held)), struct lock, lock_elem);
      t_curr->priority = next_highest_lock->priority;
      }
  
    }
  }
  lock -> isntChain = 1;
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
{
  struct list_elem elem;              /* List element. */
  struct semaphore semaphore;         /* This semaphore. */
};

static bool 
sort_sem_priority (const struct list_elem *first, const struct list_elem *second, void *aux UNUSED) 
{
  struct semaphore_elem *first_s = list_entry(first, struct semaphore_elem, elem);
  struct semaphore_elem *second_s = list_entry(second, struct semaphore_elem, elem);

  if (list_empty (&second_s->semaphore.waiters))
  {
    return true;
  }

  if (list_empty (&first_s->semaphore.waiters)) 
  {
    return false;
  }

  list_sort(&first_s->semaphore.waiters, sort_priority, NULL);

  /* sort the order with waiters queue, make sure the priority is descending order */
  list_sort(&second_s->semaphore.waiters, sort_priority, NULL);

  struct thread *first_thread = list_entry(list_front (&first_s->semaphore.waiters), struct thread, elem);

  struct thread *second_thread = list_entry(list_front (&second_s->semaphore.waiters), struct thread, elem);

  if ((first_thread -> priority) > (second_thread -> priority))
    return true;
  
  
  return false;
}

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  list_insert_ordered(&cond -> waiters, &waiter.elem, sort_sem_priority, NULL);
  //list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) {
    list_sort(&cond -> waiters, (list_less_func *) &sort_sem_priority, NULL);
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                      struct semaphore_elem, elem)->semaphore);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
