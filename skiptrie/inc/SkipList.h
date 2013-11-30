#ifndef __SKIP_LIST_H__
#define __SKIP_LIST_H__

//assumes that keys are unique?
template<typename T>
class SkipList
{
 public:
  virtual bool add(T x) = 0;
  virtual bool remove(T x) = 0;
  virtual bool contains(T x) = 0;  

  virtual void printStats() = 0;
};

#endif
