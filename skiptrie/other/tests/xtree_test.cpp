/*
 * SkipList concurrent perf test
 */
#include <cstdio>
#include "data/load_data.h"
#include "../xtree/xtree.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
using namespace std;

XTree *xtree;

int total = 0;
int nthreads;
const int NREP = 2;
//extern const int NQUERY; //from load_data
//extern const int NINSERT;

void* runOneTest(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  //printf("starting thread = %d\n", myThreadIndex);

  //bool ret = false;
  int count = 0;
  int p = myThreadIndex * (NQUERY/nthreads); //starting index for the queries
  int queryLen = NQUERY/nthreads;

  int nrep = NREP;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  count  += xtree->find(queryEl[p]) != NULL;
	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }
  total += count;
  //printf("count = %d\n", count);
  return NULL;
}

int main(int argc, char **argv)
{
  if (argc != 2)
    {
      printf("missing arg\n");
      return 1;
    }
  nthreads = 1;
  sscanf(argv[1], "%d", &nthreads);
  printf("nthreads = %d\n", nthreads);

  loadData();
  xtree = new XTree(NINSERT + 10);

  Node<unsigned> foo(1,5);
  for (int i = 0; i < NINSERT; i++)
    {
      xtree->insert(insertEl[i], &foo);
     }

  //initialize nthreads threads
  pthread_t *threads = new pthread_t[nthreads];
  

   struct timeval startt, endt;
   gettimeofday(&startt, NULL);
   
   int tmpid[500];
   for (int thr = 0; thr < nthreads; thr++)
     {
       tmpid[thr] = thr;
       pthread_create(&threads[thr], NULL, &runOneTest, (void *)&tmpid[thr]);
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
