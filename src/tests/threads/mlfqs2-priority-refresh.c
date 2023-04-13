/*
  Creates a 2 threads that run and moves down the queues,
  Check that the priority refresh every 50 ticks is working as expected
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
    int id;
  };

#define THREAD_CNT 2

static void test_cpubound(void *info_);
void test_mlfqs2_priority_refresh (void) ;

void
test_mlfqs2_priority_refresh (void) 
{
  struct thread_info info[THREAD_CNT];
  int i, j;
  int64_t start_time;

  ASSERT (thread_mlfqs);
  start_time = timer_ticks ();
  
  msg ("Starting 2 cpu-bound threads that run2 to completion.");
  msg ("Should move down one queue each time, before resetting back to PRI_MAX after 50 ticks");
  msg ("NOTE: Given that refresh duration is after 50 ticks, we can expect to see priority go down from 19 - 10 before a priority refresh. Setting MLFQ_PRIORITY_REFRESH to 210 ticks might allow priority to drop all the way down to 0");

  for (i = 0; i < THREAD_CNT; i++) 
    {
      char name[7];
      struct thread_info *ti = &info[i];
      snprintf (name, sizeof name, "proc %d", i);
      ti->id = i;
      ti->tick_count = 0;
      ti->start_time = start_time;
      ti->refresh_count = 0;
      for (j = 0; j < NUM_MLFQS; j++) ti->qtimes[j] = 0;
      thread_create (name, PRI_MAX, test_cpubound, ti);
    }
  
  msg ("Starting threads took %"PRId64" ticks.",
       timer_elapsed (start_time));

  thread_set_priority (PRI_MIN); // main at min priority
 
  msg("Sleeping %d ticks to let other threads run.",30*TIMER_FREQ);
  
  timer_sleep(30 * TIMER_FREQ);
  
  /* All the other threads now run to termination here. */
  for (i = 0; i < THREAD_CNT; i++) {
    int sum = 0;
    msg ("\n\nThread %d received %d ticks (from tick_count).", i, info[i].tick_count);
    for (j = PRI_MAX; j >= PRI_MIN; j--) {
      msg("Q %3d %6d",j,info[i].qtimes[j]);
      sum += info[i].qtimes[j];
    }
    msg ("Thread %d received %d ticks in total (cumulatively counted).", i, sum);
    msg ("Total number of detected priority refreshes = %d (Should be %d)",info[i].refresh_count,sum/MLFQS_PRIORITY_REFRESH);
  }
}

static void 
test_cpubound (void *info_) 
{
  
  struct thread_info *ti = info_;
  int64_t spin_time = 30  * TIMER_FREQ;
  int64_t last_time = 0;

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
       
      if ((ti->tick_count % MLFQS_PRIORITY_REFRESH == 0) && (ti->tick_count > 0)){
	ti->refresh_count++;
      }  
    }
  }
}
