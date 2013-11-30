#ifndef __HT_GCC__
#define __HT_GCC__

#include "../hash_node.h"
#include "../../skip_list/node.h"
#include <ext/hash_map>

typedef __gnu_cxx::hash_map<unsigned long long, HNode*> HMAP;
HMAP ht;

unsigned long long HT_READ, HT_WRITE;

inline void HT_ZERO_STATS()
{
  HT_READ = HT_WRITE = 0;
}

inline void HT_PRINT_STATS()
{
  printf("HT reads: %llu writes %llu\n", HT_READ, HT_WRITE);
}

inline void HT_INIT(int keyBits)
{
  HT_ZERO_STATS();
  return;
}

inline void HT_INSERT(unsigned long long key, HNode *val)
{
  HT_WRITE++;
  ht[key] = val;
}

inline bool HT_CONTAINS(unsigned long long key)
{
  HT_READ++;
  return ht.find(key) != ht.end();
}

inline HNode *HT_GET(unsigned long long key)
{
  HT_READ++;
  return ht[key];
}

inline void HT_DELETE()
{
  for (HMAP::iterator it = ht.begin(); it != ht.end(); it++)
    delete it->second;
  ht.clear();
}

inline int HT_SIZE()
{
  return ht.size();
}

#endif //__HT_GCC__
