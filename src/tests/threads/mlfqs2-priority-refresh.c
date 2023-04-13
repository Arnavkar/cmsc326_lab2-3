/*
  Creates a single threads that runs and moves down the queues, before being pushed
  back up to pri_max. Text that the priority refresh every 50 ticks is working as expected
  @Author: Arnav Shirodkar
 */


#include <stdio.h>
#include <inttypes.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

#define NUM_MLFQS 20  // Number of queues

struct thread_info 
  {
    int64_t start_time;             
    struct lock *lock;          /* Lock on output. */
    int tick_count;
    int refresh_count;
    int qtimes[NUM_MLFQS];
  };

#define ITER_CNT 1
#define THREAD_CNT 1

static void test_priority_refresh(void *info_);
void test_mlfqs2_priority_refresh (void) ;

void
test_mlfqs2_priority_refresh (void) 
{
  struct thread_info info[THREAD_CNT];
  struct lock lock;
  int i, j, cnt;
  int64_t start_time;

  ASSERT (thread_mlfqs);
  start_time = timer_ticks ();
  
  msg ("Starting 1 cpu-bound thread that runs to completion.");
  msg ("Should move down one queue each time, before resetting back to PRI_MAX after 210 ticks");
  msg ("NOTE: Total number of ticks spent traversing all queues based on quantums for each queue is 210 ticks. Ensure that the refresh time is set to 210 ticks instead of 50 ticks before running this test");

  lock_init (&lock);
  lock_acquire(&lock); // main holds lock
  lock_release(&lock); //release lock
  
  for (i = 0; i < THREAD_CNT; i++) 
    {
      char name[16];
      struct thread_info *ti = &info[i];
      snprintf (name, sizeof name, "lproc %d", i);
      ti->lock = &lock; // all share one lock
      ti->tick_count = 0;
      ti->start_time = start_time;
      ti->refresh_count = 0;
      for (j = 0; j < NUM_MLFQS; j++) ti->qtimes[j] = 0;
      thread_create (name, PRI_MAX, test_priority_refresh, ti);
    }
  msg ("Starting threads took %"PRId64" ticks.",
       timer_elapsed (start_time));

  thread_set_priority (PRI_MIN); // main at min priority
 
  msg("Sleeping %d ticks to let other threads run.",30*TIMER_FREQ);
  timer_sleep(30 * TIMER_FREQ);
  
  /* All the other threads now run to termination here. */
  msg("\n\nOutput from Main-------------");
  for (i = 0; i < THREAD_CNT; i++) {
    int sum = 0;
    msg ("Thread %d received %d ticks.", i, info[i].tick_count);
    for (j = PRI_MAX; j >= PRI_MIN; j--) {
      msg("Q %3d %6d",j,info[i].qtimes[j]);
      sum += info[i].qtimes[j];
    }
    msg ("thread %d received %d ticks in total.", i, sum);
    msg ("Total number of priority refreshes = %d, Should be %d",info[i].refresh_count,sum/MLFQS_PRIORITY_REFRESH);
  }
}

static void 
test_priority_refresh (void *info_) 
{
  struct thread_info *ti = info_;
  int64_t sleep_time = 5 * TIMER_FREQ;
  int64_t spin_time =  30 * TIMER_FREQ;
  int64_t last_time = 0;
  int last_priority = PRI_MAX;
  int num_priority_changes = 0;

  while (timer_elapsed (ti->start_time) < spin_time) {
    int64_t cur_time = timer_ticks ();
    if (cur_time != last_time) {
      /*
	If a new tick is recorded, increment ticks and store tick_count and qtimes value
	If the priority has changed, print the current priority
      */
      int cur_priority = thread_get_priority();
      ti->tick_count++;
      ti->qtimes[thread_get_priority()] += 1;
      last_time = cur_time;
      
      if(cur_priority != last_priority){
	//msg("%3d | ",cur_priority);
	last_priority = cur_priority;
	num_priority_changes++;
      }
       
      if ((ti->tick_count % 50 == 0) && (ti->tick_count > 0)){
	ti->refresh_count++;
      }  
    }
  }
  msg("%3d",num_priority_changes);
}
