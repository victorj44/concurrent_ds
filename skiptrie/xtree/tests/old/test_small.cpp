#include "../xtree_hash.h"
#include <set>
#include <cstdlib>
using namespace std;

int main()
{
  XTree xtree;
  set<int> truth;

  int iter = 0;
  while (true)
    {
      int x = rand()%(1<<20);
      if (truth.find(x) != truth.end())
	continue;
      //printf("insert %u\n", x);
      truth.insert(x);
      xtree.insert(x);
      
      /*set<int>::iterator it = truth.begin();
      for (it++; it != truth.end(); it++)
	{
	  set<int>::iterator prev = it;
	  prev--;
	  bool tr = 1;
	  unsigned output;
	  bool r = xtree.predecessor(*it, &output);
	  if (r != tr || (r == true && (*prev) != output))
	    {
	      printf("mismatch on %u\n", *it);
	      printf("r = %d, tr = %d\n", (int)r, (int)tr);
	      printf("output = %u, truth = %u\n", output, *prev);
	      return 1;
	    }
	    }*/

      for (int i = 0; i < 1000; i++)
	{
	  unsigned y = rand()%(1<<20);
	  unsigned output;
	  bool r = xtree.predecessor(y, &output);
	  set<int>::iterator it = truth.lower_bound(y); //returns it >= y
	  if (it == truth.begin() && r)
	    {
	      printf("Failure!\n");
	      return 1;
	    }
	  it--;

	  if (r && *it != output)
	    {
	      printf("failure 2 search = %u,  truth = %u, output = %u\n", y, *it, output);
	      return 1;
	    }	  
	}
      iter++;
      if (iter % 1000 == 0)
	printf("%d\n", iter);
    }
  
  
  return 0;
}
