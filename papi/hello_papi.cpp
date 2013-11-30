#include <cstdio>
#include <papi.h>

const int MAXSZ = 100*1000*1000;

int foo[MAXSZ];
const int n_papi_events = 2;
//cache misses, total cachce accesses
int papi_events[] = { PAPI_L3_TCM, PAPI_L3_TCA };
long long papi_values[1][n_papi_events] = {0,};


int doWork(int k)
{
  int sum = 0;
  for (int bar = 0; bar < 20; bar++)
    {
      for (int i = 0; i < MAXSZ; i++)
	foo[i] = i * k;
      for (int i = 0; i < MAXSZ; i++)
	sum += foo[i];
    }
  return sum;
}

int main()
{
  //this will fail if some counters can't be accessed
  if (PAPI_start_counters(papi_events, n_papi_events) != PAPI_OK)
    {
      printf("failed to start papi\n");
      return 1;
    }

  doWork(123);

  if (PAPI_read_counters(papi_values[0], n_papi_events) != PAPI_OK)
    {
      printf("failed to read countess\n");
      return 1;
    }

  printf("counters' values: misses = %d, accesses = %d\n", papi_values[0][0], papi_values[0][1]);

  return 0;
}
