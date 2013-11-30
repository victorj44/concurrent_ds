#ifndef __NODE_H__
#define __NODE_H__

#include "../inc.h"
#include <pthread.h>

template<typename T>
class Node
{
 private:
  //pthread_mutex_t nodeLock;
 public:
  T val;
  Node<T> *next[MAX_LEVEL + 1]; //+1 or not?
  int topLevel;
  bool marked;
  bool fullyLinked;

  Node(T val, int height = MAX_LEVEL);
  ~Node();
  void lock();
  void unlock();
};

#include "node.cpp"

#endif //__NODE_H__
