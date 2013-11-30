/*
 * SkipList concurrent perf test
 */
#include <cstdio>
#include "load_data.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
using namespace std;

const int MAXTHREADS = 80;
int globalret = 0;

void* runOneTest(void *)
{
  int NREP = MAXTHREADS*10;
  bool ret = false;
  int p = rand() % (NQUERY - NQUERY/MAXTHREADS - 1);
  int val = 1;
  while (NREP > 0)
    {
      for (int i = 0; i < NQUERY/MAXTHREADS; i++)
	{
	  int x = i/3 + i+1;
	  val += x;
	  val /= 2;
	  val *= i+4;
	}
      p -= NQUERY/MAXTHREADS;
      NREP--;
    }
  globalret += val;
  return NULL;
}

int main(int argc, char **argv)
{
  if (argc != 2)
    {
      printf("missing arg\n");
      return 1;
    }
  int nthreads = 1;
  sscanf(argv[1], "%d", &nthreads);
  printf("nthreads = %d\n", nthreads);

  loadData();

  //initialize nthreads threads
  pthread_t *threads = new pthread_t[nthreads];
   struct timeval startt, endt;
   gettimeofday(&startt, NULL);
   
   for (int thr = 0; thr < nthreads; thr++)
     {
       pthread_create(&threads[thr], NULL, &runOneTest, NULL);
     }

   for (int thr = 0; thr < nthreads; thr++)
     {
       pthread_join(threads[thr], NULL);
     }

   gettimeofday(&endt, NULL);
   double elapsed = (endt.tv_sec - startt.tv_sec) + (endt.tv_usec - startt.tv_usec) / 1000000.0;
   printf("query time = %lf\n", elapsed);

  return 0;
}
