/*
 * provides resource usage measurement, in particular timer, functionality.
 * 2003 Rajiv Wickremesinghe
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* #include "rtimer_papi.h" */
/* #include "rtimer_common.h" */
#include "rtimer.h"

#include <pthread.h>
#include "papiStdEventDefs.h"
#include "papi.h"

/* warning: assumes appropriate locks are already held when calling these functions */

#ifdef RTIMER_PAPI


#define RT_MAGIC 0xe251a
#define CHECK_VALID(rt) assert((rt)->valid == RT_MAGIC)

#define NUM_EVENTS 2
static int Events[NUM_EVENTS] = { PAPI_TOT_INS, PAPI_TOT_CYC };

static const PAPI_hw_info_t *hwinfo = NULL;


static void
test_fail(char *file, int line, char *msg, int err) {
  fprintf(stderr, "ERROR %d: %s\n", err, msg);
}

static void
rt_papi_global_init() {
  static int inited = 0;
  int err;
  pthread_attr_t attr;

  if(inited) {
    return;
  }
  inited = 1;

  if( (err = PAPI_library_init(PAPI_VER_CURRENT)) !=PAPI_VER_CURRENT) {
    test_fail(__FILE__,__LINE__,"PAPI_library_init",err);
  }

  if((hwinfo = PAPI_get_hardware_info()) == NULL) {
    test_fail(__FILE__,__LINE__,"PAPI_get_hardware_info",0);
  }


  /* thread init */

  err = PAPI_thread_init((unsigned long (*)(void))(pthread_self), 0);
  if ( err != PAPI_OK ) {
    test_fail(__FILE__, __LINE__, "PAPI_thread_init", err);
  }

  /* pthread init */

  pthread_attr_init(&attr);
#ifdef PTHREAD_CREATE_UNDETACHED
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_UNDETACHED);
#endif
#ifdef PTHREAD_SCOPE_SYSTEM
  err = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  if (err != 0)
    test_fail(__FILE__, __LINE__, "pthread_attr_setscope", err);    
#endif
}


void
rt_papi_init(rtimer_papi_t *rt)
{
  int err;

  rt_papi_global_init();

  if( (err = PAPI_create_eventset(&rt->EventSet)) != PAPI_OK ) {
    test_fail(__FILE__,__LINE__,"PAPI_create_eventset",err);
  }

  if( (err = PAPI_add_events(&rt->EventSet, Events,NUM_EVENTS))!=PAPI_OK) {
    test_fail(__FILE__,__LINE__,"PAPI_add_events",err);
  }

  rt->valid = RT_MAGIC;
}


void
rt_papi_start(rtimer_papi_t *rt)
{
  int err;

  CHECK_VALID(rt);

  if( (err = PAPI_start(rt->EventSet)) != PAPI_OK ) {
    test_fail(__FILE__,__LINE__,"PAPI_start",err);
  }
}

void
rt_papi_stop(rtimer_papi_t *rt)
{
  int err;
  long_long values[NUM_EVENTS];

  CHECK_VALID(rt);

  if ( (err = PAPI_read(rt->EventSet, values)) != PAPI_OK ) {
    test_fail(__FILE__,__LINE__,"PAPI_read",err);
  }
  rt->cycles = values[1];
}



u_int64_t
rt_papi_nanos(rtimer_papi_t *rt)
{
  u_int64_t nanos = 0;

  CHECK_VALID(rt);

  //printf(TWO12, values[0], values[1], "(INS/CYC)\n");
  nanos = (double)1000.0 * rt->cycles / hwinfo->mhz;

  return nanos;
}

#endif