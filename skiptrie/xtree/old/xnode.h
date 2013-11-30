#ifndef __X_NODE_H__
#define __X_NODE_H__

#include "../LazySkipList/node.h"

class XNode
{
 public:
  XNode() 
    { 
      key = 0; 
      hasLeft = hasRight = false; 
      left = right = NULL; 
      slNode = NULL; 
      leftNeighbor = rightNeighbor = NULL;
    }
  ~XNode() { }
  
  unsigned key;
  bool hasLeft, hasRight;
  XNode *left, *right; //children in tree
  Node<unsigned> *slNode; //ptr to the corresponding node in skip list

  XNode *leftNeighbor, *rightNeighbor; //used for the linked list of end nodes ("leaves")
};

#endif //__X_NODE_H__
