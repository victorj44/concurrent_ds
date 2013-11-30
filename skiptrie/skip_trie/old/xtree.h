#ifndef __X_FAST_TRIE__
#define __X_FAST_TRIE__

#include "../inc.h"
#include <cstddef>
//uhm, this is probably (practically) deprecated
#include <ext/hash_map>

using namespace std;

template<typename T>
class XTree
{
 private:
  __gnu_cxx::hash_map<unsigned, T*> ht; //hash table

 public:
  XTree();
  ~XTree();

  T *find(unsigned key); 
  unsigned predecessor(unsigned key);
  unsigned successor(unsigned key);

  bool insert(unsigned key, T* val);
  bool remove(unsigned key);
};

#include "xtree.cpp"

#endif __X_FAST_TRIE__
