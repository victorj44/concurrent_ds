/*
 * SkipList concurrent perf test
 */
#include <cstdio>
#include "data/load_data.h"
#include "../xtree/xtree_hash.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
using namespace std;

XTree *xtree;

int totalHits = 0;
int total = 0;
int nthreads;
const int NREP = 2;
extern const int NQUERY; //from load_data
extern const int NINSERT;
int totalRights = 0, totalBottoms = 0;

void* runOneTest(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  //printf("starting thread = %d\n", myThreadIndex);

  //bool ret = false;
  int count = 0;
  int p = myThreadIndex * (NQUERY/nthreads); //starting index for the queries
  int queryLen = NQUERY/nthreads;

  int nrep = NREP;
  int outputHits = 0;
  int rightMoves = 0;
  int bottomMoves = 0;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  unsigned output;
	  outputHits = 0;
	  count  += xtree->predecessor(queryEl[p], &output, &outputHits);
	  totalHits += outputHits;
	  totalRights += rightMoves;
	  totalBottoms += bottomMoves;
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
  xtree = new XTree();

  for (int i = 0; i < NINSERT; i++)
    {
      xtree->insert(insertEl[i]);
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

   printf("total #elements: %d\n", NINSERT);
   printf("total # of queries: %d\n", NQUERY);
   printf("total # of hash table hits: %d\n", totalHits);
   printf("total # of rightMoves: %d\n", totalRights);
   printf("total # of bottomMoves: %d\n", totalBottoms);

  return 0;
}
