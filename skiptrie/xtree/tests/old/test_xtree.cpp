#include "../xtree.h"
#include <set>
using namespace std;

int main()
{
  XTree xtree(10*1000*1000);
  //xtree.printTreeState();
  Node<unsigned> foo(1,5);

  /*xtree.insert(0, &foo);
  xtree.insert(9, &foo);
  xtree.insert(2, &foo);
  xtree.insert(15, &foo);
  xtree.insert(6, &foo);
  xtree.insert(11, &foo);
  xtree.insert(10, &foo);
  XNode *r = xtree.successor(9);
  if (r != NULL)
  printf("successor of 9: %d\n", r->key);*/

  /*int perm[20];
  for (int i = 0; i < 20; i++)
    perm[i] = i;
  for (int i = 0; i < 20; i++)
    {
      int p = rand()%20;
      int tmp = perm[i];
      perm[i] = perm[p];
      perm[p] = tmp;
    }
  
  for (int i = 0; i < 20; i++)
    {
      xtree.insert(perm[i], &foo);
      printf("***** AFTER %d (insert %d) *****\n", i, perm[i]);
      xtree.printTreeState();
      for (int j =0; j <= i; j++)
	{
	  int res = xtree.find(perm[j]) != NULL;
	  printf("%d", res);
	  printf("successor %d = ", j);
	  XNode *tmp = xtree.successor(perm[j]);
	  if (tmp == NULL)
	    printf("NULL\n");
	  else
	    printf("%d\n", tmp->key);

	  int realsucc = 100;
	  for (int k = 0; k <= i; k++)
	    if (perm[k] > perm[j] && perm[k] < realsucc)
	      realsucc = perm[k];
	  if (realsucc == 100 && tmp != NULL)
	    {
	      printf("successor error! query = %d; real = %d, got NULL!\n", perm[j], realsucc);
	      return 1;
	    }

	  if (realsucc < 20 && tmp->key != realsucc)
	    {
	      printf("successor error! query = %d; real = %d, got %d\n", perm[j], realsucc, tmp->key);
	      return 1;
	    }

	}
      printf("\n");
      xtree.printNodesSorted();
    }*/

  set<int> truth;

  int iter = 0;
  while(true)
    {
      iter++;
      
      int x = (rand()) >> 4; //0..2^28

      if (truth.find(x) == truth.end()) //not in the set
	{
	  //	  printf("about to insert %d\n", x);
	  truth.insert(x);
	  xtree.insert(x, &foo);
	}

      for (set<int>::iterator it = truth.begin(); it != truth.end(); it++)
	for (int k = 0; k < 5; k++)
	  {
	    int y = (*it) + k;
	    //	    printf("test for %d", y);
	    bool in = truth.find(y) != truth.end();
	    bool in2 = xtree.find(y) != NULL;

	    if (in)
	      {
		set<int>::iterator jt = it;
		jt++;

		XNode *suc = xtree.successor(*it);
		bool jin = jt != truth.end();
		bool sin = suc != NULL;
		if (jin != sin)
		  {
		    printf("Error in successor! %d\n", *it);
		    return 1;
		  }
		
		if (jin)
		  {
		    if ((unsigned)(*jt) != (unsigned)suc->key)
		      {
			printf("error in successor key! query = %d, res = %d %d\n", *it, (*jt), suc->key);
			return 1;
		      }
		  }
		
	      }
	    

	    if (in != in2)
	      {
		printf("!!! ERROR !!!! %d\n", y);
		return 1;

	      }
	  }
      if (iter % 100 == 0)
	printf("%d OK\n", iter);
    }

  return 0;
}
