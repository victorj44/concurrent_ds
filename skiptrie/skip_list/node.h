#ifndef __NODE_H__
#define __NODE_H__

#include <pthread.h>

template<typename T>
class Node
{
 public:
  T val;
  Node<T> **next;
  int topLevel;

  Node(T val, int height);
  ~Node();
};

#include "node.cpp"

#endif //__NODE_H__
