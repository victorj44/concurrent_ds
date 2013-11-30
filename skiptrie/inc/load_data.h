#ifndef __LOAD_DATA_H__
#define __LOAD_DATA_H__

#include <cstdio>
#include <algorithm>
#include <cstdlib>
using namespace std;

unsigned long long *insertTable, *queryTable;

void LoadDataRandom(unsigned long long maxn)
{
  insertTable = new unsigned long long[maxn];
  unsigned long long prev = 2;
  for (unsigned long long i = 0; i < maxn; i++)
    {
      insertTable[i] = prev + (1 + rand()%4);
      prev = insertTable[i];
    }

  queryTable = new unsigned long long[maxn * 2];
  for (unsigned long long i = 0; i < maxn; i++)
    {
      queryTable[2*i] = queryTable[i] + 1;
      queryTable[2*i + 1] = queryTable[i] + 2;
    }

  random_shuffle(insertTable, insertTable + maxn);
  random_shuffle(queryTable, queryTable + 2*maxn);
}

//essentially load small values
//generates 1..maxn, same for queries
void LoadDataRandomLimited(unsigned long long maxn)
{
  insertTable = new unsigned long long[maxn];
  queryTable = new unsigned long long[maxn*2];
  for (unsigned long long i = 0; i < maxn; i++)
    insertTable[i] = i+1;
  random_shuffle(insertTable, insertTable + maxn);
  for (unsigned long long i = 0; i < maxn; i++)
    queryTable[i] = insertTable[i];
  random_shuffle(insertTable, insertTable + maxn);
  for (unsigned long long i = 0; i < maxn; i++)
    queryTable[maxn + i] = insertTable[i];
  random_shuffle(insertTable, insertTable + maxn);
}

void LoadDataSmallNumbers(unsigned long long maxn)
{
  insertTable = new unsigned long long[maxn];
  queryTable = new unsigned long long[maxn*2];
  for (unsigned long long i = 0; i < maxn; i++)
    insertTable[i] = 2*i + 1;
  for (unsigned long long i = 0; i < 2*maxn; i++)
    queryTable[i] = i;
  random_shuffle(insertTable, insertTable + maxn);
  random_shuffle(queryTable, queryTable + 2*maxn);
}

#endif //__LOAD_DATA_H__
