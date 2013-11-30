#include <cstdio>
#include "../SkipList.h"
#include "LazySkipList.h"
#include <set>
using namespace std;

int main()
{
  LazySkipList<int> *l = new LazySkipList<int>(0, 1<<28);
  //LazySkipList<int> *l = new LazySkipList<int>();
  //LazySkipList<int> *l;
  
  printf("starting tests...\n");
  printf("INSERT:\n");
  for (int i = 0; i < 5; i++)
    {
      Node<int>* ret = l->insert(i);
      printf("add %d: ret = %d\n", i, ret != NULL);
    }

  printf("CONTAINS?\n");
  for (int i = 0; i < 5; i++)
    {
      bool ret = l->contains(i);
      printf(" %d? %d\n", i, (int)ret);
    }

  printf("REMOVE:\n");
  for (int i = 0; i < 5; i++)
    {
      bool ret = l->remove(i);
      printf(" removed %d = %d\n", i, (int) ret);

      ret = l->remove(i);
      printf(" removed %d = %d\n", i, (int) ret);
      
      ret = l->contains(i);
      printf(" %d ? = %d\n", i, (int) ret);
    }

  //big random test
  set<int> truth;
  int nelem = 0;
  printf("Running 10k ops\n...\n");
  for (int i = 0; i < 10000; i++)
    {
      int action = rand()%10;
      int x = rand()%(1<<28);
      if (action <= 7)
	{
	  //printf("insert %d\n", x);
	  truth.insert(x);
	  l->insert(x);
	  nelem++;
	}
      else
	{
	  //printf("remove %d\n", x);
	  if (truth.find(x) != truth.end())
	    nelem--;

	  truth.erase(x);
	  l->remove(x);
	}
      
      bool intruth = truth.find(x) != truth.end();
      bool inskip = l->contains(x);

      if (intruth != inskip)
	{
	  printf("!!!!!!!! error!\n");
	  return 1;
	}
    }
  
  nelem = truth.size();
  while (!truth.empty())
    {
      int tr = *truth.begin();
      truth.erase(tr);
      bool inskip = l->contains(tr);
      if (!inskip)
	{
	  printf("error! %d not found!\n", tr);
	  return 1;
	}
      //printf("got %d\n", tr);
    }


  printf("Everything OK\n");
  printf("printing stats... (orig size = %d)\n", nelem);

  l->printStats();

  return 0;
}
