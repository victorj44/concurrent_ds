#ifndef __SKIP_LIST_H__
#define __SKIP_LIST_H__

#include <cstdlib>
#include "node.h"
#include "math.h"

template<typename T>
class SkipList
{
 private:
  Node<T> *head, *tail;
  T negInf, posInf;
  int maxDepth;
  int randomLevel();
  unsigned long long totalItems;


 public:
  SkipList(T negInf, T posInf, int maxDepth); //min and max values for keys
  ~SkipList();

  int find(T x, Node<T> **preds, Node<T> **succs, Node<T> *startNode = NULL, 
	   int *rightMoves = NULL, int *bottomMoves = NULL);

  //sequential insert
  Node<T>* insert(T x, Node<T> *predecessor = NULL);
  bool remove(T x);
  bool contains(T x, Node<T> **contains_preds, Node<T> **contains_succs);
  Node<T> *getHead() { return head; }
  Node<T> *getTail() { return tail; }
  void CLEAR_STATS();
  void printStats();
};

#include "SkipList.cpp"

#endif //__SKIP_LIST_H__

