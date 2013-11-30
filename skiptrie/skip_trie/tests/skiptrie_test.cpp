#include <cstdio>
#include "../skip_trie.h"
#include <set>
#include <cstdlib>
using namespace std;

int main()
{
  //cut off depth, should be set to log log u ~ 6
  //max 10M elements
  SkipTrie *l = new SkipTrie(6);
  set<unsigned long long> truth;
  
  Node<unsigned long long> *preds[40], *succs[40];

  int p =0;
  while (true)
    {
      p++;
      unsigned long long x = 0;

      do
	{
	  x = ((unsigned long long) rand() << 29) | rand() | 1;
	}
      while (truth.find(x) != truth.end());

      //printf("inserting %llu\n", x);
      truth.insert(x);
      l->insert(x);

      int topHits, rightMoves, bottomMoves;
      int r = l->find(x, preds, succs, &topHits, &rightMoves, &bottomMoves);
      
      if (!r)
	{
	  printf("FAILURE for key = %llu\n", x);
	  return 1;
	}

      unsigned long long y = x + 1;
      r = l->find(y, preds, succs, &topHits, &rightMoves, &bottomMoves);
      int r2 = truth.find(y) != truth.end();
      if ((bool)r != (bool)r2)
	{
	  printf("failure for y = %llu\n", y);
	  return 1;
	}
	

      if (p % 1000 == 0)
	printf("%d\n", p);
    }

  l->printStats();

  return 0;
}
