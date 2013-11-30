#include <cstdio>
#include "../skip_trie.h"
#include "load_data.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
using namespace std;

SkipTrie *trie;

int nthreads;
int maxn, NREP;
extern unsigned *insertTable, *queryTable;
HNode **topNodeTable;
int localHtHits[100], localFound[100], localQueries[100];

void* runTestTop(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  int p = myThreadIndex * (maxn/nthreads); //starting index for the queries
  int queryLen = maxn/nthreads;

  Node<unsigned> *preds[40], *succs[40];

  int nrep = NREP;
  int sumht = 0, sumfound = 0, sumqueries = 0;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  int htHits = 0;
	  topNodeTable[p] = trie->queryTop(queryTable[p], &htHits);

	  sumqueries++;
	  sumht += htHits;
	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }

  localHtHits[myThreadIndex] = sumht;
  localQueries[myThreadIndex] = sumqueries;
  return NULL;
}

void* runTestBottom(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  int p = myThreadIndex * (maxn/nthreads); //starting index for the queries
  int queryLen = maxn/nthreads;

  Node<unsigned> *preds[40], *succs[40];

  int nrep = NREP;
  int sumht = 0, sumfound = 0, sumqueries = 0;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  unsigned result;
	  int htHits = 0;
	  int topHits, rightMoves, bottomMoves;
	  bool found = trie->queryBottom(queryTable[p], topNodeTable[p], preds, succs, 
					 &rightMoves, &bottomMoves);
	  sumfound += (int)((bool)found);

	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }

  localFound[myThreadIndex] = sumfound;
  return NULL;
}

void* runTestBoth(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  int p = myThreadIndex * (maxn/nthreads); //starting index for the queries
  int queryLen = maxn/nthreads;

  Node<unsigned> *preds[40], *succs[40];

  int nrep = NREP;
  int sumht = 0, sumfound = 0, sumqueries = 0;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  unsigned result;
	  int htHits = 0;
	  int topHits, rightMoves, bottomMoves;
	  bool found = trie->find(queryTable[p], preds, succs, &topHits, &rightMoves, &bottomMoves);

	  sumqueries++;
	  sumht += htHits;
	  sumfound += (int)((bool)found);

	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }

  return NULL;
}


int main(int argc, char **argv)
{
  if (argc != 3)
    {
      printf("missing arg: n_threads test_size\n");
      return 1;
    }

  nthreads = 1;
  sscanf(argv[1], "%d", &nthreads);
  sscanf(argv[2], "%d", &maxn);
  printf("nthreads = %d\n", nthreads);

  //LoadData(maxn);
  LoadDataRandom(maxn);
  topNodeTable = new HNode* [2*maxn+1]; //just in case
  NREP = (128*1000*1000 / maxn);
  printf("maxn = %d, nthreads = %d, NREP = %d, querylen = %d\n", maxn, nthreads, NREP, maxn/nthreads);
  trie = new SkipTrie(6, maxn / (1 << 6));

  for (int i = 0; i < maxn; i++)
    {
      //printf("inserting %u\n", insertTable[i]);
      trie->insert(insertTable[i]);
     }


  //
  // ******** TOP QUERIES *************
  //
  
  //initialize nthreads threads
  pthread_t *threads = new pthread_t[nthreads];
  struct timeval startt, endt;
  gettimeofday(&startt, NULL);
   
  int tmpid[500];
  for (int thr = 0; thr < nthreads; thr++)
    {
      tmpid[thr] = thr;
      pthread_create(&threads[thr], NULL, &runTestTop, (void *)&tmpid[thr]);
    }

  for (int thr = 0; thr < nthreads; thr++)
    {
      pthread_join(threads[thr], NULL);
    }

   gettimeofday(&endt, NULL);
   double elapsed = (endt.tv_sec - startt.tv_sec) + (endt.tv_usec - startt.tv_usec) / 1000000.0;
   printf("top query time = %lf\n", elapsed);

   //
   // *********** BOTTOM QUERIES ************
   //
   delete[] threads;
   threads = new pthread_t[nthreads];
   gettimeofday(&startt, NULL);
   
  for (int thr = 0; thr < nthreads; thr++)
    {
      tmpid[thr] = thr;
      pthread_create(&threads[thr], NULL, &runTestBottom, (void *)&tmpid[thr]);
    }

  for (int thr = 0; thr < nthreads; thr++)
    {
      pthread_join(threads[thr], NULL);
    }

   gettimeofday(&endt, NULL);
   elapsed = (endt.tv_sec - startt.tv_sec) + (endt.tv_usec - startt.tv_usec) / 1000000.0;
   printf("bottom query time = %lf\n", elapsed);


   //
   // *********** COMBINED QUERIES *************
   //

   delete[] threads;
   threads = new pthread_t[nthreads];
   gettimeofday(&startt, NULL);
   
  for (int thr = 0; thr < nthreads; thr++)
    {
      tmpid[thr] = thr;
      pthread_create(&threads[thr], NULL, &runTestBoth, (void *)&tmpid[thr]);
    }

  for (int thr = 0; thr < nthreads; thr++)
    {
      pthread_join(threads[thr], NULL);
    }

   gettimeofday(&endt, NULL);
   elapsed = (endt.tv_sec - startt.tv_sec) + (endt.tv_usec - startt.tv_usec) / 1000000.0;
   printf("skiptrie full query time = %lf\n", elapsed);
   

   //
   // *********** END OF QUERIES ***************
   // 
   
   

   //
   // ************ PRINT STATS ****************
   //

   trie->printStats();
   int hthits = 0, found = 0, queries = 0;
   for (int i = 0; i < nthreads; i++)
     {
       hthits += localHtHits[i];
       found += localFound[i];
       queries += localQueries[i];
     }
   
   printf("total hthits, found, queries: %d %d %d\n", hthits, found, queries);

  return 0;
}
