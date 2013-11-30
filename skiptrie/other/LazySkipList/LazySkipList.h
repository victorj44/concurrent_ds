#ifndef __LAZY_SKIP_LIST_H__
#define __LAZY_SKIP_LIST_H__

#include <cstdlib>

#include "../inc.h"
#include "../SkipList.h"
#include "node.h"

template<typename T>
class LazySkipList //: public SkipList<T>
{
 private:
  Node<T> *head, *tail;
  T negInf, posInf;
  int randomLevel();

  //Node<T> **contains_preds;
  //Node<T> **contains_succs;
 public:
  LazySkipList(T negInf, T posInf); //min and max values for keys
  //LazySkipList(int maxLevel);
  ~LazySkipList();

  int find(T x, Node<T> **preds, Node<T> **succs, Node<T> *startNode = NULL, 
	   int suggestedStartLevel = -1, int *rightMoves = NULL,
	   int *bottomMoves = NULL);

  Node<T>* insert(T x, int randomLvl = -1);
  bool remove(T x);
  bool contains(T x, Node<T> **contains_preds, Node<T> **contains_succs);

  void printStats();

  Node<T> *getHead() { return head; }
  Node<T> *getTail() { return tail; }

};

#include "LazySkipList.cpp"

#endif __LAZY_SKIP_LIST_H__

