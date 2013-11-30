#include <cstdio>
#include <cstdlib>
#include "../../inc.h"

const int NINSERT = 8 * 1000 *1000;
const int NQUERY = 16 * 1000 * 1000;

//const int NINSERT = 1000;
//const int NQUERY = 1000;

int insertEl[NINSERT], insertLvl[NINSERT];
int queryEl[NQUERY];

void loadData()
{
  FILE *fins = fopen("data/insert.txt", "r");
  FILE *fquery = fopen("data/query.txt", "r");
  if (fins == NULL || fquery == NULL)
    {
      printf("failed to open files");
      return;
    }
    
  for (int i = 0; i < NINSERT; i++)
    {
      fscanf(fins, "%d %d", &insertEl[i], &insertLvl[i]);
    }

  for (int i = 0; i < NQUERY; i++)
    {
      fscanf(fquery, "%d", &queryEl[i]);
    }
    

  fclose(fins);
  fclose(fquery);
}
