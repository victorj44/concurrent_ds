//#define COUNT_MOVES
//#define BREAK_EARLY_LEVEL

#ifdef COUNT_MOVES
const int SPLIT_LEVEL = 6;
unsigned long long N_RIGHTS, N_TOP_RIGHTS;
unsigned long long TOTAL_QUERIES;
#endif

#ifdef BREAK_EARLY_LEVEL
const int BREAK_LEVEL = 6; //stop when reach depth 4
bool INSERTING = false;
#endif


template<typename T>
SkipList<T>::SkipList(T negInf, T posInf, int maxDepth) :
  negInf(negInf),
  posInf(posInf),
  maxDepth(maxDepth)
{
  head = new Node<T>(negInf, maxDepth);
  tail = new Node<T>(posInf, maxDepth);
  for (int i = 0; i <= maxDepth; i++)
    {
      head->next[i] = tail;
      tail->next[i] = NULL;
    }
  totalItems = 0;

#ifdef COUNT_MOVES
  N_RIGHTS = N_TOP_RIGHTS = TOTAL_QUERIES = 0;
#endif
}

template<typename T>
SkipList<T>::~SkipList()
{ }




/* find() - returns -1 if not found
 * otw. returns the highest 'level' at which an element is found
 * and populates indices MAX_LEVEL .. 'level' of preds and succs
 * with pointers to respective elements
 */
template<typename T>
int SkipList<T>::find(T x, Node<T> **preds, Node<T> **succs, Node<T> *startNode, 
		      int *rightMoves, int *bottomMoves)
{
  int lFound = -1;
  Node<T> *pred = head;
  int startLevel = pred->topLevel;
  if (startNode != NULL)
    {
      pred = startNode;
      startLevel = pred->topLevel;
    }
  //printf("starting with pred = %p, %u\n", pred, pred->val);
  //for (int i = pred->topLevel; i >= 0; i--)
  //printf("%d: %p %u\n", i, pred->next[i], pred->next[i]->val);

#ifdef COUNT_MOVES
  TOTAL_QUERIES++;  
#endif

  if (rightMoves != NULL)
    *rightMoves = 0;
  for (int level = startLevel; level >= 0; level--)
    {
      Node<T> *curr = pred->next[level];
      //printf("curr = %p, %u\n", curr, curr->val);
      while (x > curr->val)
	{
	  if (rightMoves != NULL)
	    (*rightMoves)++;
	  pred = curr;
	  curr = pred->next[level];
#ifdef COUNT_MOVES
	  N_RIGHTS++;
	  if (level > SPLIT_LEVEL)
	    N_TOP_RIGHTS++;
#endif
	}

      preds[level] = pred;
      succs[level] = curr;

      //for prodecessor just return this node
      if (lFound == -1 && x == curr->val)
	{
	  lFound = level;
	  //if we're doing an equality test -> can break here
	  //if looking for predecessor/successor -> must go all the way down
	  break;
	}
#ifdef BREAK_EARLY_LEVEL
      //breaky early only if NOT insert!
      if (!INSERTING && level <= BREAK_LEVEL)
	  break;
#endif
    }

  if (bottomMoves != NULL)
    *bottomMoves = startLevel - lFound;
  return lFound;
}

/* add - adds elements x to the skip lists
 * returns false if fails (ie. key already in the list)
 * true on success
 * sequential insert
 */
template<typename T>
Node<T>* SkipList<T>::insert(T x, Node<T> *predecessor)
{
#ifdef BREAK_EARLY_LEVEL
  INSERTING = true;
#endif
  Node<T> *ret = NULL;
  int topLevel = randomLevel();
  totalItems += topLevel + 1;
  //printf("inserting toplevel = %d\n", topLevel);
  //Node<T> **preds = new Node<T>*[topLevel + 1];
  //Node<T> **succs = new Node<T>*[topLevel + 1];
  Node<T> *preds[40];
  Node<T> *succs[40];
  bool done = false;

  while (!done)
    {
      find(x, preds, succs, predecessor);

      //Node<T> *pred, *succ;
      bool valid = true;
      for (int level = 0; valid && (level <= topLevel); level++)
	{
	  //pred = preds[level];
	  //succ = succs[level];
	}
      Node<T> *newNode = new Node<T>(x, topLevel);
      ret = newNode;
      newNode->topLevel = topLevel;
      for (int level = 0; level <= topLevel; level++)
	{
	  newNode->next[level] = succs[level];
	  preds[level]->next[level] = newNode;
	}
      done = true;
    }

#ifdef BREAK_EARLY_LEVEL
  INSERTING = false;
#endif

  return ret;
}

template<typename T>
bool SkipList<T>::remove(T x)
{
  return false;
}

template<typename T>
bool SkipList<T>::contains(T x, Node<T> **contains_preds, Node<T> **contains_succs)
{
  int lFound = find(x, contains_preds, contains_succs);
  bool ret = lFound != -1;

  return ret;
}

/* randomLevel() - returns a random level 
 * in range 0 .. max depth - 1, inclusive.
 */
template<typename T>
int SkipList<T>::randomLevel()
{
  return maxDepth - (log2(1 + rand() % (1 << maxDepth)));
}

template<typename T>
void SkipList<T>::CLEAR_STATS()
{
#ifdef COUNT_MOVES

#endif
}

/* 
 *
 */
template<typename T>
void SkipList<T>::printStats()
{
  printf("total items = %llu\n", totalItems);
#ifdef COUNT_MOVES
  printf("queries = %llu, total rights = %llu, top rights = %llu\n", 
	 TOTAL_QUERIES, N_RIGHTS, N_TOP_RIGHTS);
  printf("per query: top = %lf, total = %lf\n", 
	 (double)N_TOP_RIGHTS/(double)TOTAL_QUERIES,
	 (double)N_RIGHTS/(double)TOTAL_QUERIES);	 
  TOTAL_QUERIES = N_RIGHTS = N_TOP_RIGHTS = 0;
#endif
}


