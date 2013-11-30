#ifndef __X_FAST_TRIE__
#define __X_FAST_TRIE__

#include "../inc.h"
#include <cstddef>
//uhm, this is probably (practically) deprecated
//#include <ext/hash_map>
#include "xnode.h"
#include <cstring>
#include <cstdio>

using namespace std;

class XTree
{
 private:
  static const unsigned XTREE_ROOT_KEY = 1;
  static const int LEFT = 0;
  static const int RIGHT = 1;
  int treeSize;
  int maxSize, memTop;
  XNode *memBlock;
  //__gnu_cxx::hash_map<unsigned, T*> ht; //hash table
  XNode *root;
  XNode *leavesList;

  void deleteAll(XNode *node);

  unsigned makeLeft(unsigned key) { return key << 1; }
  unsigned makeRight(unsigned key) { return (key << 1) | 1; }

  //NAIVE WAY - FIX THIS!!
  //unsigned externalKeyToInternal(unsigned value);
  //NAIVE WAY - FIX THIS!!!
  //unsigned internalKeyToExternal(unsigned key);
  void insertToLL(XNode *finalNode, XNode *neighbor);

  unsigned keyLen(unsigned key);
  XNode *getNewNode(unsigned key, Node<unsigned> *slPtr);

 public:
  XTree(int maxSize);
  ~XTree();

  XNode *predecessor(unsigned searchKey);
  XNode *successor(unsigned searchKey);
  XNode *findOrPredecessor(unsigned searchKey);
  XNode *find(unsigned key);
  XNode *findOrNeighbor(unsigned key);

  XNode *insert(unsigned key, Node<unsigned> *slPtr);
  //XNode *remove(unsigned key);
  void printNodesSorted();
  void printTreeState(XNode *ptr = NULL, int lvl = 0);
  void printBinary(unsigned key);
  unsigned getSize() { return treeSize; }
};

#include "xtree.cpp"

#endif __X_FAST_TRIE__
