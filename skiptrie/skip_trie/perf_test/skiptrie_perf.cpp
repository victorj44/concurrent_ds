#include <cstdio>
#include "../skip_trie.h"
#include "../../inc.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
#ifdef USE_PAPI
#include <papi.h>
#endif
using namespace std;

SkipTrie *trie;

//cache misses, total cachce accesses
const int TOTAL_REP = 5;
#ifdef USE_PAPI
const int n_papi_events = 5; 
int papi_events[] = { PAPI_L3_TCA, PAPI_L3_TCM, PAPI_L2_TCA, PAPI_L2_TCM, PAPI_L1_TCM };
long long papi_values[TOTAL_REP][n_papi_events] = {0,};
#endif

int nthreads;
int maxn, NREP;
extern unsigned long long *insertTable, *queryTable;
int localHtHits[100], localFound[100], localQueries[100];

void* runOneTest(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  int p = myThreadIndex * (maxn/nthreads); //starting index for the queries
  int queryLen = maxn/nthreads;

  Node<unsigned long long> *preds[40], *succs[40];

  int nrep = NREP;
  int sumht = 0, sumfound = 0, sumqueries = 0;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  int htHits = 0;
	  int rightMoves, bottomMoves, topHits;
	  bool found = trie->find(queryTable[p], preds, succs, &topHits, &rightMoves, &bottomMoves);
	  //HNode *found = trie->queryTop(queryTable[p], &topHits);

	  sumqueries++;
	  sumht += htHits;
	  sumfound += (int)((bool)found);

	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }

  localHtHits[myThreadIndex] = sumht;
  localFound[myThreadIndex] = sumfound;
  localQueries[myThreadIndex] = sumqueries;
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

  LoadDataRandom(maxn);
  NREP = (2 * 163840000ULL / maxn);
  //NREP = (2*40960000 / maxn);
  printf("maxn = %d, nthreads = %d, NREP = %d, querylen = %d\n", maxn, nthreads, NREP, maxn/nthreads);
  //maybe set it to 7?
  //super shallow skiptrie
  trie = new SkipTrie(1);

  for (int i = 0; i < maxn; i++)
    {
      //printf("inserting %u\n", insertTable[i]);
      trie->insert(insertTable[i]);
     }

  HT_ZERO_STATS();
  //initialize nthreads threads
  double allElapsed[TOTAL_REP];
  for (int REP = 0; REP < TOTAL_REP; REP++)
    {
      FlushCache();

      struct timeval startt, endt;
      gettimeofday(&startt, NULL);

      //maybe must do cache reads on per-thread basis?
#ifdef USE_PAPI
      if (PAPI_start_counters(papi_events, n_papi_events) != PAPI_OK)
	{
	  printf("failed to start papi\n");
	  return 1;
	}
#endif

      int footmp = 0;
      runOneTest(&footmp);
	
      gettimeofday(&endt, NULL);
      double elapsed = (endt.tv_sec - startt.tv_sec) + (endt.tv_usec - startt.tv_usec) / 1000000.0;
      allElapsed[REP] = elapsed;
      printf("query time = %lf\n", elapsed);
      trie->printStats();

      int totalfound = 0, totalqueries = 0;
      for (int i = 0; i < nthreads; i++)
	{
	  totalfound += localFound[i];
	  totalqueries += localQueries[i];
	}

      printf("total found = %d, queries = %d\n", totalfound, totalqueries);

#ifdef USE_PAPI
      if (PAPI_stop_counters(papi_values[REP], n_papi_events) != PAPI_OK)
	{
	  printf("failed to read countess\n");
	  return 1;
	}
      printf("counters' values: L3A = %llu, L3M = %llu, L2A = %llu, L2M = %llu, L1M = %llu\n",
	     papi_values[REP][0], papi_values[REP][1], papi_values[REP][2], papi_values[REP][3], papi_values[REP][4]);
#endif
    }

#ifdef USE_PAPI
  long long papi_trans[n_papi_events][5];
  for (int i = 0; i < n_papi_events; i++)
    for (int j = 0; j < 5; j++)
      papi_trans[i][j] = papi_values[j][i];
  
  printf("**** median counters: %lf s L3A = %llu, L3M = %llu, L2A = %llu, L2M = %llu, L1M = %llu\n",
	 meanMedian3(allElapsed),
	 meanMedian3(papi_trans[0]),
	 meanMedian3(papi_trans[1]),
	 meanMedian3(papi_trans[2]),
	 meanMedian3(papi_trans[3]),
	 meanMedian3(papi_trans[4]));

  APPEND_TO_FILE(meanMedian3(allElapsed),
		 meanMedian3(papi_trans[0]),
		 meanMedian3(papi_trans[1]),
		 meanMedian3(papi_trans[2]),
		 meanMedian3(papi_trans[3]),
		 meanMedian3(papi_trans[4]));
#endif


   
    return 0;
}
