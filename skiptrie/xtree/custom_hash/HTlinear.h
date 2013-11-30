#ifndef __HT_LINEAR__
#define __HT_LINEAR__

//init stuff
void HT_INIT()
{
  hashFn = new MultiplicationHash(maxSize * 2);
  ht = new LinearHash<HNode*>(maxSize * 2, hashFn);
}

#endif //__HT_LINEAR__
