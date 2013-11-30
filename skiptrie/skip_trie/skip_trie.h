#ifndef __SKIP_TRIE_H__
#define __SKIP_TRIE_H__

#include "../xtree/xtree_hash.h"
#include "../skip_list/SkipList.h"
#include <stdlib.h>
#include <cmath>
#include <map>
using namespace std;

class SkipTrie
{
 private:
  SkipList<unsigned long long> *bottom;
  XTree *top;
  int scalpSize;

 public:
  SkipTrie(int scalpSize = 1)
    {
      unsigned long long maxKey = ((unsigned long long)-1) >> 1;
      bottom = new SkipList<unsigned long long>((unsigned long long)0, maxKey, scalpSize); //keys between 0 and 1<<300
      top = new XTree();
      top->insert(0, bottom->getHead());
      //this key is too big??
      top->insert(maxKey, bottom->getTail());
      insertToLevelStats(bottom->getHead());
      insertToLevelStats(bottom->getTail());
      this->scalpSize = scalpSize;

      positionSum = 0;
      nupdates = 0;
    }
  ~SkipTrie() { }

  void insert(unsigned long long key, int randomLvl = -1)
  {
    //insert to the bottom first;
    //if ends up in the top line, "re-insert" to the top
    Node<unsigned long long>* bottomNode = bottom->insert(key, 
							  top->findOrPredecessor(key)->bottomNode);
    
    if (bottomNode == NULL) //failed to insert!
      {
	printf("Failed to insert %llu (probably repeated key)\n", key);
	exit(1);
      }

    //printf("MAX_Level = %d, scalpSize = %d, topLevel = %d\n", MAX_LEVEL, scalpSize, bottomNode->topLevel);
    if (bottomNode->topLevel >= scalpSize)
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
  int find(unsigned long long key, Node<unsigned long long> **preds, Node<unsigned long long> **succs, 
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

    //updateTopLevelStats(topNode->bottomNode);
    //printf("landed at %d\n", topNode->bottomNode->val);
    int lFound = bottom->find(key, preds, succs, topNode->bottomNode, 
			      rightMoves, bottomMoves);
    //printf("prev = %d, found = %d\n", topNode->key, lFound);
    bool ret = lFound != -1;
    if (ret)
      return 2;
    return 0;
  }

  HNode *queryTop(unsigned long long key, int *topHits)
  {
    HNode *topNode = top->findOrPredecessor(key, topHits);
    return topNode;
  }

  int queryBottom(unsigned long long key, HNode *topNode, Node<unsigned long long> **preds, 
		  Node<unsigned long long> **succs, int *rightMoves, int *bottomMoves)
  {
    int lFound = bottom->find(key, preds, succs, topNode->bottomNode, 
			      rightMoves, bottomMoves);
    bool ret = lFound != -1;
    if (ret)
      return 2;
    return 0;
  }

  map< Node<unsigned long long>*, int > position;
  long long positionSum, nupdates;
  void insertToLevelStats(Node<unsigned long long> *node)
  {
    position[node] = 0;
  }

  void makeLevelStats()
  {
    int p = 0;
    for (map< Node<unsigned long long>*, int >::iterator it = position.begin(); it != position.end(); it++)
      {
	it->second = p++;
      }
  }

  void updateTopLevelStats(Node<unsigned long long> *node)
  {
    if (position.find(node) == position.end())
      {
	printf("Error - failed to find for update top level stats\n");
	exit(1);
      }
    nupdates++;
    positionSum += position[node];
  }

  void printStats()
  {
    bottom->printStats();
    printf("total # of items on this level = %lld\n", (long long)position.size());
    printf("positionSum = %lld, nupdates = %lld, avg. landing position = %lf\n",
	   positionSum, nupdates, positionSum/((double)1.0*nupdates));
  }

};

#endif //__SKIP_TRIE_H__

