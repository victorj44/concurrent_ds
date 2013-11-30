#ifndef __X_FAST_TRIE__
#define __X_FAST_TRIE__

#include <cstddef>

//max size in # of keys 16M
//make this variable/read in test
int KEY_BITS = 16;

#ifdef USE_REGULAR_HT
#include "custom_hash/HTunorderedMap.h"
#endif

#ifdef USE_DIRECT_HT
#include "custom_hash/HTdirect.h"
#endif

#include "hash_node.h"
#include "../skip_list/node.h"
#include <cstring>

using namespace std;

/* XTree implementation
 * keys must be in range [0, 2^KEY_BITS - 1] inclusive
 */
class XTree
{
 public:
  XTree(unsigned long long maxSize = 0);
  ~XTree();

  bool insert(unsigned long long key, Node<unsigned long long> *bottomNode = NULL);
  bool find(unsigned long long key);
  bool predecessor(unsigned long long key, unsigned long long *output, int *outputHits = NULL);
  HNode* findOrPredecessor(unsigned long long key, int *outputHits = NULL);

  //in public for debugging
  //boost::unordered_map<unsigned, HNode* > ht;
  /*HashFunction *hashFn;
    HashMapBase<HNode*> *ht;*/


  HNode *root;
  void printTree(unsigned long long startHKey, int depth = 0);
  void printLinkedList(unsigned long long minKey);
  int getSize() { return 0; }//return ht.size(); }
  void printTreeStats();
 private:
  
  inline unsigned long long toHashKey(unsigned long long, int);
  inline unsigned long long maskKey(unsigned long long, int);
  void printBinary(unsigned long long key);
  long long treeSize;
};

#include "xtree_hash.cpp"

#endif //__X_FAST_TRIE__
