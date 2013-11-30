#define USE_REGULAR_HT

#include <cstdio>
#include "../xtree_hash.h"
#include "../../inc.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
#ifdef USE_PAPI
#include <papi.h>
#endif
using namespace std;


//there's no L1_TCA !
const int MAX_THREADS = 80;
#ifdef USE_PAPI
const int n_papi_events = 5; 
int papi_events[] = { PAPI_L3_TCA, PAPI_L3_TCM, PAPI_L2_TCA, PAPI_L2_TCM, PAPI_L1_TCM };
long long papi_values[5][n_papi_events] = {0,};
#endif
int REP; //current run test rep
long long NREP;

XTree *xtree;

int nthreads;
int maxn;
extern unsigned long long *insertTable, *queryTable;
int localHtHits[100], localFound[100], localQueries[100];

void* runOneTest(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  int p = myThreadIndex * (maxn/nthreads); //starting index for the queries
  int queryLen = maxn/nthreads;

  //init PAPI for this thread here

  long long nrep = NREP;
  int sumht = 0, sumfound = 0, sumqueries = 0;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  unsigned long long result;
	  int htHits = 0;
	  bool found = xtree->predecessor(queryTable[p], &result, &htHits);
	  //bool found = xtree->find(queryTable[p]);

	  sumqueries++;
	  sumht += htHits;
	  sumfound += found;

	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }

  localHtHits[myThreadIndex] = sumht;
  localFound[myThreadIndex] = sumfound;
  localQueries[myThreadIndex] = sumqueries;

  //save PAPI counters here for this thread
  

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

  //LoadDataRandom(maxn);
  LoadDataRandomLimited(maxn);
  KEY_BITS = ceil(log2(maxn));
  //run the max test at least twice!
  //max test size = 650M
  NREP = (2 * 80*1000*1000ULL / maxn);
  printf("maxn = %d, nthreads = %d, NREP = %lld, querylen = %d\n", maxn, nthreads, NREP, maxn/nthreads);
  xtree = new XTree(maxn);

  for (int i = 0; i < maxn; i++)
    {
      //printf("inserting %u\n", insertTable[i]);
      xtree->insert(insertTable[i]);
     }


  //initialize nthreads threads
  //pthread_t *threads = new pthread_t[nthreads];

  double allElapsed[5];
  for (REP = 0; REP < 5; REP++)
    {
      FlushCache();

      HT_ZERO_STATS();
      struct timeval startt, endt;
      gettimeofday(&startt, NULL);

#ifdef USE_PAPI
      //maybe must do cache reads on per-thread basis?
      if (PAPI_start_counters(papi_events, n_papi_events) != PAPI_OK)
	{
	  printf("failed to start papi\n");
	  return 1;
	}
#endif
      
      //int tmpid[500];
      /*for (int thr = 0; thr < nthreads; thr++)
	{
	  tmpid[thr] = thr;
	  pthread_create(&threads[thr], NULL, &runOneTest, (void *)&tmpid[thr]);
	}

      for (int thr = 0; thr < nthreads; thr++)
	{
	  pthread_join(threads[thr], NULL);
	  }*/
      int footmp = 0;
      runOneTest(&footmp);

      gettimeofday(&endt, NULL);
      double elapsed = (endt.tv_sec - startt.tv_sec) + (endt.tv_usec - startt.tv_usec) / 1000000.0;
      allElapsed[REP] = elapsed;
      printf("query time = %lf\n", elapsed);
      xtree->printTreeStats();

      int hthits = 0, found = 0, queries = 0;
      for (int i = 0; i < nthreads; i++)
	{
	  hthits += localHtHits[i];
	  found += localFound[i];
	  queries += localQueries[i];
	}

      printf("total hthits, found, queries: %d %d %d\n", hthits, found, queries);
      HT_PRINT_STATS();

#ifdef USE_PAPI
      //papi_values[REP][0], papi_values[REP][1], papi_values[REP][2], papi_values[REP][3], papi_values[REP][4]);
      PAPI_stop_counters(papi_values[REP], n_papi_events);
      printf("counters' values: L3A = %ld, L3M = %ld, L2A = %ld, L2M = %ld, L1M = %ld\n",
	     papi_values[REP][0], papi_values[REP][1], papi_values[REP][2], papi_values[REP][3], papi_values[REP][4]);
#endif
    }

#ifdef USE_PAPI
  for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < n_papi_events; j++)
	printf("%ld ", papi_values[i][j]);
      printf("\n"); 
    }

  long long papi_trans[n_papi_events][5];
  for (int i = 0; i < n_papi_events; i++)
    for (int j = 0; j < 5; j++)
      papi_trans[i][j] = papi_values[j][i];

  printf("**** median counters: %lf s L3A = %ld, L3M = %ld, L2A = %ld, L2M = %ld, L1M = %ld\n",
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
