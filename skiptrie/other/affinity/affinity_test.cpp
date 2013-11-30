#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>

pthread_mutex_t printLock;
volatile int blah;

void* myFn(void *args)
{
  int foo = 123;
  for (int i = 0; i < 10*1000*1000; i++)
    foo += i;
  blah = foo;
  
  int myThreadId = *(int *)args;

  unsigned int pid = getpid();
  unsigned long tid = (unsigned long)pthread_self();

  cpu_set_t mycpuid;
  //sched_getaffinity called with pid=0 returns affinity of the current thread... hopefully!
  int ret = sched_getaffinity(0, sizeof(mycpuid), &mycpuid);
  if (ret != 0)
    {
      printf("sched_getaffinity failed!\n");
      return NULL;
    }
  
  int cpuCount = CPU_COUNT(&mycpuid);
  

  //print & flush under a lock so nothing gets messed up
  pthread_mutex_lock(&printLock);
  printf("myThreadId: %d\n", myThreadId);
  printf("pid = %d\n", pid);
  printf("cpu count = %d\nSet CPUs:", cpuCount);
  for (int i = 0; i < cpuCount; i++)
    if (CPU_ISSET(i, &mycpuid))
      printf("%d ", i);
  printf("\nthread id = %ld\n\n", tid);
  fflush(stdout);
  pthread_mutex_unlock(&printLock);

}

int main()
{
  pthread_mutex_init(&printLock, NULL);
  printf("hi\n");
  
  pthread_t threads[10];
  int tmpid[10];
  for (int i = 0; i < 10; i++)
    tmpid[i] = i;

  for (int i = 0; i < 5; i++)
    pthread_create(&threads[i], NULL, &myFn, (void *)&tmpid[i]);

  for (int i = 0; i < 5; i++)
    pthread_join(threads[i], NULL);

  printf("blah = %d\n", blah);

  return 0;
}
