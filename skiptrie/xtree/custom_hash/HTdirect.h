#ifndef __HT_DIRECT__
#define __HT_DIRECT__

#include <cstdio>
#include <cstring>
#include "../hash_node.h"
#include "../../skip_list/node.h"

HNode **ht;
unsigned long long htsize;

unsigned long long HT_READ, HT_WRITE;

void HT_ZERO_STATS()
{
  HT_READ = HT_WRITE = 0;
}

void HT_PRINT_STATS()
{
  printf("HT reads: %llu, writes %llu\n", HT_READ, HT_WRITE);
}

void HT_INIT(int size)
{
  printf("direct HT\n");
  htsize = size;
  ht = new HNode*[size + 1];
  memset(ht, 0, sizeof(ht));
  HT_ZERO_STATS();
}

void HT_INSERT(unsigned long long key, HNode *val)
{
  HT_WRITE++;
  if (key > htsize)
    {
      printf("key too big\n");
      return;
    }
  ht[key] = val;
}

bool HT_CONTAINS(unsigned long long key)
{
  HT_READ++;
  if (key > htsize)
    {
      printf("key too big\n");
      return false;
    }
  return ht[key] != NULL;
}

HNode *HT_GET(unsigned long long key)
{
  HT_READ++;
  if (key > htsize)
    {
      printf("key too big\n");
      return NULL;
    }
  return ht[key];
}

void HT_DELETE()
{
  delete[] ht;
}

int HT_SIZE()
{
  return htsize;
}

#endif //__HT_GCC__
