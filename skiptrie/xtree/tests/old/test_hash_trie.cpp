#include "../hash_skip_trie.h"
#include <set>
using namespace std;

int main()
{
  SkipTrie stree(10);

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

  /*unsigned perm[20];
  for (int i = 0; i < 20; i++)
    perm[i] = i + 1;
  for (int i = 0; i < 20; i++)
    {
      int p = rand()%20;
      int tmp = perm[i];
      perm[i] = perm[p];
      perm[p] = tmp;
    }
  
  for (int i = 0; i < 20; i++)
    {
      stree.insert(perm[i]);
      printf("***** AFTER %d (insert %d) *****\n", i, perm[i]);
      for (int j =0; j <= i; j++)
	{
	  printf("%d", stree.find(perm[j]));
	}
      printf("\n");
      }*/

  set<int> truth;

  Node<unsigned> **preds = new Node<unsigned>*[MAX_LEVEL+1];
  Node<unsigned> **succs = new Node<unsigned>*[MAX_LEVEL+1];
  int topHits;
  int rightMoves;

  int iter = 0;
  while(true)
    {
      iter++;
      
      int x = 1 + (rand()) >> 5; //0..2^27

      if (truth.find(x) == truth.end()) //not in the set
	{
	  //	  printf("about to insert %d\n", x);
	  truth.insert(x);
	  stree.insert(x);
	}

      for (set<int>::iterator it = truth.begin(); it != truth.end(); it++)
	for (int k = 0; k < 5; k++)
	  {
	    int y = (*it) + k;
	    //	    printf("test for %d", y);
	    bool in = truth.find(y) != truth.end();
	    bool in2 = stree.find(y, preds, succs, &topHits, &rightMoves) >= 1;

	    if (in != in2)
	      {
		printf("!!! ERROR !!!! %d\n", y);
		return 1;

	      }
	  }
      if (iter % 100 == 0)
	{
	  
	  printf("%d OK (top size = %d)\n", iter, stree.getTopSize());
	}
    }

  return 0;
}
