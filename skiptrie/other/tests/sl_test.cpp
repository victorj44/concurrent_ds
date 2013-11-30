/*
 * SkipList concurrent perf test
 */
#include <cstdio>
#include "../LazySkipList/LazySkipList.h"
#include "data/load_data.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
using namespace std;

//constant amount of work for the entire test
//ie. decrease of work per thread
LazySkipList<unsigned> *l;
int nthreads;
const int NREP = 2;
extern const int NQUERY; //from load_data
extern const int NINSERT;
int globalRightMoves = 0, globalBottomMoves = 0;
int totalFound = 0;

void* runOneTest(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  //printf("starting thread = %d\n", myThreadIndex);

  int p = myThreadIndex * NQUERY/nthreads; //starting index for the queries
  int queryLen = NQUERY/nthreads;
  Node<unsigned> **preds = new Node<unsigned>*[MAX_LEVEL+1];
  Node<unsigned> **succs = new Node<unsigned>*[MAX_LEVEL+1];
  int count = 0;

  int localStats[] = {0,0,0};
  int localRights[] = {0,0};
  int localTops[] = {0,0};
  int rightMoves = 0, bottomMoves = 0;

  int nrep = NREP;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  int lFound = l->find(queryEl[p], preds, succs, NULL, -1, &rightMoves, &bottomMoves);
	  globalRightMoves += rightMoves;
	  globalBottomMoves += bottomMoves;
	  if (lFound > -1)
	    {
	      //globalRightMoves += rightMoves;
	      //globalBottomMoves += bottomMoves;
	      totalFound++;
	    }

	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }
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
  //printf("nthreads = %d\n", nthreads);

  loadData();
  //min value, max value
  l = new LazySkipList<unsigned>(0, 1<<28);
  
  for (int i = 0; i < NINSERT; i++)
    {
      //if (i % 100 == 0)
      //printf("%d\n", i);
      l->insert(insertEl[i], insertLvl[i]);
      //truth.insert(insertEl[i]);
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
   printf("%lf\n", elapsed);

   printf("total found = %d\nglobal Bottom moves = %d (av = %lf) \nglobal right moves = %d (av = %lf)\n", 
	  totalFound, globalBottomMoves, 
	  globalBottomMoves/(1.0*NQUERY*NREP),
	  globalRightMoves,
	  globalRightMoves/(1.0*NQUERY*NREP));

   printf("***** stats ****\n");
   l->printStats();
   
  return 0;
}
