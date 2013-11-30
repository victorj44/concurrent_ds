#include "../xtree_hash.h"
#include <set>
#include <cstdlib>
using namespace std;

int main()
{
  XTree xtree;
  //xtree.printTreeState();
  //Node<unsigned> foo(1,5);

  //printf("root left, right:\n%p %p\n", (void *)(xtree.root->left), (void *)(xtree.root->right));
  /*xtree.insert(5);
  xtree.insert(1);

  xtree.printLinkedList(1);

  unsigned out = 0;
  bool r = xtree.predecessor(0, &out);
  printf("pred of 0: %d %d\n", (int)r, out);
  r = xtree.predecessor(1, &out);
  printf("pred of 1: %d %d\n", (int)r, out);
  r = xtree.predecessor(3, &out);
  printf("pred of 3: %d %d\n", (int)r, out);
  r = xtree.predecessor(5, &out);
  printf("pred of 5: %d %d\n", (int)r, out);
  r = xtree.predecessor(6, &out);
  printf("pred of 6: %d %d\n", (int)r, out);
  r = xtree.predecessor(7, &out);
  printf("pred of 7: %d %d\n", (int)r, out);
  r = xtree.predecessor(132, &out);
  printf("pred of 32234234: %d %d\n", (int)r, out);*/
  //printf("root left, right:\n%p %p\n", (void *)(xtree.root->left), (void *)(xtree.root->right));
  //printf("root left, right:\n%p %p\n", (void *)(xtree.root->left), (void *)(xtree.root->right));
  //printf("root left, right:\n%p %p\n", (void *)(xtree.root->left), (void *)(xtree.root->right));


  //xtree.printTree(1);


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
