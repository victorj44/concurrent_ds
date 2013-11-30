#include <cstdio>
#include "../SkipList.h"
#include <set>
#include <cstdlib>
using namespace std;

int main()
{
  SkipList<unsigned long long> *l = new SkipList<unsigned long long>(0, (unsigned long long)-1, 20);
  set<unsigned long long> truth;
  
  Node<unsigned long long> *preds[40], *succs[40];

  int p =0;
  while (true)
    {
      p++;
      unsigned long long x = 0;
      
      do
	{
	  x = rand() << 30 | rand() | 1;
	}
      while (truth.find(x) != truth.end());

      //printf("inserting %u\n", x);
      truth.insert(x);
      l->insert(x);

      int r = l->contains(x, preds, succs);
      
      if (!r)
	{
	  printf("FAILURE for key = %llu\n", x);
	  return 1;
	}

      if (p % 1000 == 0)
	printf("%d\n", p);
    }

  l->printStats();

  return 0;
}
