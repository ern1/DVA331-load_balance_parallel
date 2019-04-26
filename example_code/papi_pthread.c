
//https://ptools-perfapi.eecs.utk.narkive.com/bYzDbhEf/using-papi-with-pthreads
// Tror inte det nedan är värst relevant för oss

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <papi.h>

#define NUM_THREADS 4

void *BusyWork(void *t)
{
  int i;
  long tid;
  double result;

  int MB_EventSet;
  long_long Value;
  Value = 0;
  MB_EventSet = PAPI_NULL;
  PAPI_create_eventset(&MB_EventSet);
  PAPI_add_event(MB_EventSet, PAPI_TOT_CYC);
  PAPI_start(MB_EventSet);

  result = 0.0;
  tid = (long)t;
  printf("Thread %ld starting...\n", tid);
  for (i=0; i<1000000; i++)
    result = result + sin(i) * tan(i);
  printf("Thread %ld done. Result = %e\n", tid, result);

  PAPI_stop(MB_EventSet, &Value);
  printf("Thread %ld Value: %lli\n", tid, Value);
  pthread_exit((void*) t);
}

int main (int argc, char *argv[])
{
  pthread_t thread[NUM_THREADS];
  pthread_attr_t attr;
  void *status;

  int MB_EventSet;
  long_long Value;
  MB_EventSet = PAPI_NULL;
  PAPI_library_init(PAPI_VER_CURRENT);
  PAPI_thread_init(pthread_self);
  PAPI_create_eventset(&MB_EventSet);
  PAPI_add_event(MB_EventSet, PAPI_TOT_CYC);
  PAPI_start(MB_EventSet);

  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  pthread_create(&thread[0], &attr, BusyWork, (void *)0);
  pthread_create(&thread[1], &attr, BusyWork, (void *)1);
  pthread_create(&thread[2], &attr, BusyWork, (void *)2);
  pthread_create(&thread[3], &attr, BusyWork, (void *)3);

  /* Free attribute and wait for the other threads */
  pthread_attr_destroy(&attr);
  pthread_join(thread[0], &status);
  pthread_join(thread[1], &status);
  pthread_join(thread[2], &status);
  pthread_join(thread[3], &status);
  printf("Main: program completed. Exiting.\n");

  Value = 0;
  PAPI_stop(MB_EventSet, &Value);
  printf("Benchmark Value: %lli\n", Value);
  pthread_exit(NULL);
}