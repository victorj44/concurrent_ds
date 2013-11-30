/*
 * SkipList concurrent perf test
 */
#include <cstdio>
#include "../xtree/skip_trie.h"
#include "data/load_data.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
using namespace std;

SkipTrie *st;
int nthreads;
const int NREP = 2;
extern const int NQUERY; //from load_data
extern const int NINSERT;
int stats[100][3];
int allRights[100][2], allTops[100][2];

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

  int nrep = NREP;
  int topHits, rightMoves;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  topHits = rightMoves = 0;
	  int in = st->find(queryEl[p], preds, succs, &topHits, &rightMoves);
	  //printf("%d ", in);
	  localStats[in]++;
	  if (in != 1) //must have gone down
	    {
	      localRights[1]++;
	      localRights[0] += rightMoves;
	    }
	  localTops[0] += topHits;
	  localTops[1]++;

	  count += 1;
	  p += 1;
	}
      p -= queryLen;
      nrep--;
    }

  //printf("local stats = %d %d %d\n", localStats[0], localStats[1], localStats[2]);

  for (int i = 0; i < 3; i++)
    stats[myThreadIndex][i] = localStats[i];
  for (int i = 0; i < 2; i++)
    {
      allRights[myThreadIndex][i] += localRights[i];
      allTops[myThreadIndex][i] += localTops[i];
    }

  return NULL;
}

void printStats()
{
  printf("xtree size = %d\n", st->getTopSize());
  //for (int i = 0; i < 20; i++)
  //printf("%d %d %d\n", stats[i][0], stats[i][1], stats[i][2]);

  int totals[] = {0,0,0};
  int rights[] = {0,0};
  int tops[] = {0,0};
  for (int i = 0; i < 100;i++)
    for (int j = 0; j < 3;j++)
      totals[j] += stats[i][j];

  for (int i = 0; i < 100; i++)
    for (int j = 0; j < 2; j++)
      {
	tops[j] += allTops[i][j];
	rights[j] += allRights[i][j];
      }
  printf("not found, in top, in bottom\n%d %d %d\n", totals[0], totals[1], totals[2]);
  printf("tops sum = %d, count = %d, av = %f\n", tops[0], tops[1], tops[0]/(1.0*tops[1]));
  printf("rights sum = %d, count = %d, av = %f\n", rights[0], rights[1], rights[0]/(1.0*rights[1]));
}

int main(int argc, char **argv)
{
  if (argc != 3)
    {
      printf("missing arg\n");
      return 1;
    }
  memset(stats, 0, sizeof(stats));
  memset(allTops, 0, sizeof(allTops));
  memset(allRights, 0, sizeof(allRights));
  nthreads = 1;
  int scalpsize = 1;
  sscanf(argv[1], "%d", &nthreads);
  sscanf(argv[2], "%d", &scalpsize);
  //printf("scalpsize = %d\n", scalpsize);
  //printf("nthreads = %d\n", nthreads);

  loadData();
  st = new SkipTrie(NINSERT * 2, scalpsize);

  for (int i = 0; i < NINSERT; i++)
    {
      //if (i % 100 == 0)
      //printf("%d\n", i);
      st->insert(insertEl[i], insertLvl[i]);
      //truth.insert(insertEl[i]);
    }

  //printf("X-Tree size = %d\n", st->getTopSize());

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
   printStats();
   
  return 0;
}
