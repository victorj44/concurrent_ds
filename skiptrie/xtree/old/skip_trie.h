#ifndef __SKIP_TRIE_H__
#define __SKIP_TRIE_H__

#include "xtree.h"
#include "../LazySkipList/LazySkipList.h"
#include <stdlib.h>
#include <cmath>

class SkipTrie
{
 private:
  LazySkipList<unsigned> *bottom;
  XTree *top;
  int scalpSize;
  //Node<unsigned> **preds, **succs;

 public:
  SkipTrie(int maxSize, int scalpSize = 1) 
    {
      bottom = new LazySkipList<unsigned>(0, 1<<28); //keys between 0 and 1<<28
      top = new XTree(maxSize);
      top->insert(0, bottom->getHead());
      top->insert(1<<28, bottom->getTail());
      /*preds = new Node<unsigned>*[MAX_LEVEL + 1];
	succs = new Node<unsigned>*[MAX_LEVEL + 1];*/
      this->scalpSize = scalpSize;
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
	top->insert(key, bottomNode);
      }
  }

  int compareKeys(unsigned result, unsigned query)
  {
    for (int i = 29; i >= 0; i--)
      if ( ((result >>i) & 1) != ((query >> i)&1))
	return 2*log2(29-i);
    return 2*log2(29);
  }

  //returns >=1 if element found
  //return 1 if directly in xtree
  //return 0 if not found
  int find(unsigned key, Node<unsigned> **preds, Node<unsigned> **succs, 
	   int *topHits, int *rightMoves)
  {
    //find pred/successor of key in top
    //move to predecessor (if successor) -> guarantees it's predecessor
    //find in skiplist & return true on success; false o/w
    XNode *topNode = top->findOrPredecessor(key);
    *topHits = compareKeys(topNode->key, key);
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
    int lFound = bottom->find(key, preds, succs, topNode->slNode, -1, rightMoves);
    //printf("prev = %d, found = %d\n", topNode->key, lFound);
    bool ret = lFound != -1 && succs[lFound]->fullyLinked && !succs[lFound]->marked;
    if (ret)
      return 2;
    return 0;
  }

  unsigned getTopSize()
  {
    return top->getSize();
  }
};

#endif __SKIP_TRIE_H__

