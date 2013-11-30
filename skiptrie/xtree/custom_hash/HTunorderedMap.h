#ifndef __HT_UNORDERED_MAP__
#define __HT_UNORDERED_MAP__

#include <cstdio>
#include "../hash_node.h"
#include "../../skip_list/node.h"
#include <unordered_map>

typedef std::unordered_map<unsigned long long, HNode*> HMAP;
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
  printf("regulard HT");
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

#endif //__HT_UNORDERED_MAP__
