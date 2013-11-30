#include "../xtree_hash.h"
#include <set>
#include <cstdlib>
using namespace std;

int main()
{
  KEY_BITS = 20;
  XTree xtree(0);
  set<unsigned long long> truth;
  int iter = 0;
  while (true)
    {
      //unsigned long long x = ((unsigned long long)rand() << 32) | rand();
      unsigned long long x = rand() % (1 << KEY_BITS);
      if (truth.find(x) != truth.end())
	continue;
      //printf("insert %llu\n", x);
      truth.insert(x);
      xtree.insert(x);
      
      set<unsigned long long>::iterator it = truth.begin();
      for (it++; it != truth.end(); it++)
	{
	  set<unsigned long long>::iterator prev = it;
	  prev--;
	  bool tr = 1;
	  unsigned long long output;
	  bool r = xtree.predecessor(*it, &output);
	  if (r != tr || (r == true && (*prev) != output))
	    {
	      printf("mismatch on %llu\n", *it);
	      printf("r = %d, tr = %d\n", (int)r, (int)tr);
	      printf("output = %llu, truth = %llu\n", output, *prev);
	      return 1;
	    }
	}

      for (int i = 0; i < 1000; i++)
	{
	  //unsigned long long y = ((unsigned long long)rand() << 32) | rand();
	  unsigned long long y = rand() % (1 << KEY_BITS);
	  unsigned long long output;
	  bool r = xtree.predecessor(y, &output);
	  set<unsigned long long>::iterator it = truth.lower_bound(y); //returns it >= y
	  if (it == truth.begin() && r)
	    {
	      printf("Failure!\n");
	      return 1;
	    }
	  it--;

	  if (r && *it != output)
	    {
	      printf("failure 2 search = %llu,  truth = %llu, output = %llu\n", y, *it, output);
	      return 1;
	    }	  
	}
      iter++;
      if (iter % 1000 == 0)
	printf("%d\n", iter);
    }
  
  
  return 0;
}
