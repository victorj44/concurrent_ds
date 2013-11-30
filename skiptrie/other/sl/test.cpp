/*
 * SkipList concurrent perf test
 */
#include <cstdio>
#include "SkipList.h"
#include <set>
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
using namespace std;

//constant amount of work for the entire test
//ie. decrease of work per thread
SkipList<int> *l;
int nthreads;
const int NREP = 2;
//extern const int NQUERY; //from load_data
//extern const int NINSERT;
int NQUERY = 0;
int NINSERT = 0;

void* runOneTest(void *argIndex)
{
  int myThreadIndex = *(int*)argIndex; //my thread index
  //printf("starting thread = %d\n", myThreadIndex);

  int p = myThreadIndex * NQUERY/nthreads; //starting index for the queries
  int queryLen = NQUERY/nthreads;
  Node<int> **preds = new Node<int>*[MAX_LEVEL+1];
  Node<int> **succs = new Node<int>*[MAX_LEVEL+1];
  int count = 0;

  int nrep = NREP;
  while (nrep > 0)
    {
      for (int i = 0; i < queryLen; i++)
	{
	  //this queries the skip list
	  //notice that you have to pass preds and succes into skip list
	  //(prevents it from re-allocating for every call -> perf benefit
	  //count += l->contains(queryEl[p], preds, succs);
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

  //loadData();
  //min value, max value
  l = new SkipList<int>(0, 1<<28);
  
  //this inserts a key with level = level into SkipList
  //l->insert(key, level);

  //this was used to test it on 80 threads

  //initialize nthreads threads
  /*pthread_t *threads = new pthread_t[nthreads];
  

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
   printf("%lf\n", elapsed);*/

  return 0;
}
