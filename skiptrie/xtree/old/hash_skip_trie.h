#ifndef __SKIP_TRIE_H__
#define __SKIP_TRIE_H__

#include "xtree_hash.h"
#include "../LazySkipList/LazySkipList.h"
#include <stdlib.h>
#include <cmath>
#include <map>
using namespace std;

class SkipTrie
{
 private:
  LazySkipList<unsigned> *bottom;
  XTree *top;
  int scalpSize;

 public:
  SkipTrie(int scalpSize = 1) 
    {
      bottom = new LazySkipList<unsigned>(0, 1<<28); //keys between 0 and 1<<28
      top = new XTree();
      top->insert(0, bottom->getHead());
      top->insert(1<<28, bottom->getTail());
      insertToLevelStats(bottom->getHead());
      insertToLevelStats(bottom->getTail());
      this->scalpSize = scalpSize;

      positionSum = 0;
      nupdates = 0;
    }
  ~SkipTrie() { }

  void insert(unsigned key, int randomLvl = -1)
  {
    //insert to the bottom first;
    //if ends up in the top line, "re-insert" to the top
    Node<unsigned>* bottomNode = bottom->insert(key, randomLvl);
    
    if (bottomNode == NULL) //failed to insert!
      {
	printf("Failed to insert %d (probably repeated key)\n", key);
	exit(1);
      }

    //printf("MAX_Level = %d, scalpSize = %d, topLevel = %d\n", MAX_LEVEL, scalpSize, bottomNode->topLevel);
    if (bottomNode->topLevel >= MAX_LEVEL - scalpSize + 1)
      {
	//printf("inserting %d %d\n", key, randomLvl);
	//insert to the top
	insertToLevelStats(bottomNode);
	top->insert(key, bottomNode);
      }
  }

  //returns >=1 if element found
  //return 1 if directly in xtree
  //return 0 if not found
  int find(unsigned key, Node<unsigned> **preds, Node<unsigned> **succs, 
	   int *topHits, int *rightMoves, int *bottomMoves)
  {
    //find pred/successor of key in top
    //move to predecessor (if successor) -> guarantees it's predecessor
    //find in skiplist & return true on success; false o/w
    HNode *topNode = top->findOrPredecessor(key, topHits);
    if (topNode->key == key)
      {
	//printf("(1) query key = %d, topnode key = %d\n", key, topNode->key);
	return 1;
      }
    else
      {
	//printf("(2) query key = %d, topnode key = %d\n", key, topNode->key);
      }
    //find in the bottom starting search @ topNode
    //possibly +0 here!
    //printf("landed on level = %d\n", MAX_LEVEL - scalpSize);

    updateTopLevelStats(topNode->bottomNode);
    int lFound = bottom->find(key, preds, succs, topNode->bottomNode, 
			      MAX_LEVEL - scalpSize, rightMoves, bottomMoves);
    //printf("prev = %d, found = %d\n", topNode->key, lFound);
    bool ret = lFound != -1 && succs[lFound]->fullyLinked && !succs[lFound]->marked;
    if (ret)
      return 2;
    return 0;
  }

  unsigned getTopSize()
  {
    HT_SIZE();
  }

  map< Node<unsigned>*, int > position;
  long long positionSum, nupdates;
  void insertToLevelStats(Node<unsigned> *node)
  {
    position[node] = 0;
  }

  void makeLevelStats()
  {
    int p = 0;
    for (map< Node<unsigned>*, int >::iterator it = position.begin(); it != position.end(); it++)
      {
	it->second = p++;
      }
  }

  void updateTopLevelStats(Node<unsigned> *node)
  {
    if (position.find(node) == position.end())
      {
	printf("Error - failed to find for update top level stats\n");
	exit(1);
      }
    nupdates++;
    positionSum += position[node];
  }

  void printTopLevelStats()
  {
    bottom->printStats();
    printf("total # of items on this level = %d\n", position.size());
    printf("positionSum = %lld, nupdates = %lld, avg. landing position = %lf\n",
	   positionSum, nupdates, positionSum/((double)1.0*nupdates));
  }

};

#endif __SKIP_TRIE_H__

