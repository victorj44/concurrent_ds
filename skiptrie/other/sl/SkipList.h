#ifndef __SKIP_LIST_H__
#define __SKIP_LIST_H__

#include <cstdlib>

#include "../inc.h"
#include "../SkipList.h"
#include "node.h"

template<typename T>
class SkipList //: public SkipList<T>
{
 private:
  Node<T> *head, *tail;
  T negInf, posInf;
  int randomLevel();

  //removed for perf; must be provided by the caller!
  //Node<T> **contains_preds;
  //Node<T> **contains_succs;
 public:
  SkipList(T negInf, T posInf); //min and max values for keys
  ~SkipList();

  int find(T x, Node<T> **preds, Node<T> **succs, Node<T> *startNode = NULL, int *rightMoves = NULL);

  Node<T>* insert(T x, int randomLvl = -1);
  bool remove(T x);
  bool contains(T x, Node<T> **contains_preds, Node<T> **contains_succs);

  void printStats();

  Node<T> *getHead() { return head; }
  Node<T> *getTail() { return tail; }

};

#include "SkipList.cpp"

#endif __SKIP_LIST_H__

