#ifndef __TEST_TOOLS_H__
#define __TEST_TOOLS_H__

#include <cstdio>

//creates and array and iterates over it multiple time
//to flush the cache
int FlushCache()
{
  //40 * 4 = 160MB, should easily flush the cache
  int sz = 40 * 1000 * 1000;
  int *array = new int [sz];
  int sum = 0;
  //iterate 5 times over the entire array
  for (int iter = 0; iter < 5; iter++)
    {
      for (int i = 0; i < sz; i++)
	array[i] = (i - 4 + iter ) * 2;
      for (int i = 0; i < sz; i++)
	sum += array[i];
    }
  
  delete[] array;
  return sum;
}

//returns mean of 3 median elements of out 5 item array
//essentially excludes min & max values
long long meanMedian3(long long *arr)
{
  long long ret = 0;
  long long minv = arr[0], maxv = arr[0];
    
  for (int i = 0; i < 5; i++)
    {
      ret += arr[i];
      if (arr[i] < minv)
	minv = arr[i];
      if (arr[i] > maxv)
	maxv = arr[i];
    }
  ret -= maxv + minv;
  ret /= 3;
  return ret;
}

double meanMedian3(double *arr)
{
  double ret = 0;
  double minv = arr[0], maxv = arr[0];
    
  for (int i = 0; i < 5; i++)
    {
      ret += arr[i];
      if (arr[i] < minv)
	minv = arr[i];
      if (arr[i] > maxv)
	maxv = arr[i];
    }
  ret -= maxv + minv;
  ret /= 3.0;
  return ret;
}

//returns the median of 3 integers
long long median(long long a, long long b, long long c)
{
  //printf("median of %ld %ld %ld\n", a, b, c);
  long long minv = min(a, min(b,c));
  long long maxv = max(a, max(b,c));
  if (minv <= a && a <= maxv)
    {
      //printf("returning %ld\n", a);
      return a;
    }
  if (minv <= b && b <= maxv)
    {
      //printf("returning %ld\n", b);
      return b;
    }
  if (minv <= c && c <= maxv)
    {
      //printf("returning %ld\n", c);
      return c;
    }
  return 0;
}

double median(double a, double b, double c)
{
  //printf("median of %lf %lf %lf\n", a, b, c);
  double minv = min(a, min(b,c));
  double maxv = max(a, max(b,c));
  if (minv <= a && a <= maxv)
    return a;
  if (minv <= b && b <= maxv)
    return b;
  if (minv <= c && c <= maxv)
    return c;
  return 0;
}

void APPEND_TO_FILE(double runTime, long long l3a, long long l3m, long long l2a, long long l2m, long long l1m)
{
  FILE *f = fopen("output.txt", "a");
  if (f == NULL)
    {
      printf("CRITICAL: can't append to the file!\n");
      return;
    }

  fprintf(f, "%lf, %lld, %lld, %lld, %lld, %lld\n", runTime, l3a, l3m, l2a, l2m, l1m);

  fclose(f);
}

#endif //__TEST_TOOLS_H__
