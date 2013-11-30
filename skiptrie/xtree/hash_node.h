#ifndef __HASH_NODE_H__
#define __HASH_NODE_H__

#include "../skip_list/node.h"

class HNode
{
 public:
  HNode() 
    { }
 HNode(unsigned long long key, HNode *left, HNode *right) :
  key(key),
    left(left),
    right(right)
    { }
  ~HNode() { }
  
  unsigned long long key;
  HNode *left, *right;
  Node<unsigned long long> *bottomNode;
};


#endif //__HASH_NODE_H__
