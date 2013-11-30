#include <pthread.h>
#include <stdio.h>

int main()
{
  int s = sizeof(pthread_mutex_t);
  int i = sizeof(int);
  printf("mutex size = %d\n", s);
  printf("int size = %d\n", i);
  return 0;
}
