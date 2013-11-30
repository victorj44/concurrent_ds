
template<typename T>
SkipList<T>::SkipList(T negInf, T posInf) :
  negInf(negInf),
  posInf(posInf)
{
  head = new Node<T>(negInf);
  tail = new Node<T>(posInf);
  for (int i = 0; i <= MAX_LEVEL; i++)
    {
      head->next[i] = tail;
    }
  //contains_preds = new Node<T>*[MAX_LEVEL + 1];
  //contains_succs = new Node<T>*[MAX_LEVEL + 1];
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
int SkipList<T>::find(T x, Node<T> **preds, Node<T> **succs, Node<T> *startNode, int *rightMoves)
{
  int lFound = -1;
  Node<T> *pred = head;
  int startLevel = MAX_LEVEL;
  if (startNode != NULL)
    {
      pred = startNode;
      startLevel = startNode->topLevel;
    }

  if (rightMoves != NULL)
    *rightMoves = 0;
  for (int level = startLevel; level >= 0; level--)
    {
      Node<T> *curr = pred->next[level];
      while (x > curr->val)
	{
	  if (rightMoves != NULL)
	    (*rightMoves)++;
	  pred = curr;
	  curr = pred->next[level];
	}
      
      if (lFound == -1 && x == curr->val)
	{
	  lFound = level;
	}
      
      preds[level] = pred;
      succs[level] = curr;
    }

  return lFound;
}

/* add - adds elements x to the skip lists
 * returns false if fails (ie. key already in the list)
 * true on success
 */
template<typename T>
Node<T>* SkipList<T>::insert(T x, int randomLvl)
{
  Node<T> *ret = NULL;
  int topLevel = randomLvl;
  if (randomLvl == -1)
    topLevel = randomLevel();
  Node<T> **preds = new Node<T>*[MAX_LEVEL + 1];
  Node<T> **succs = new Node<T>*[MAX_LEVEL + 1];
  bool done = false;

  while (!done)
    {
      int lFound = find(x, preds, succs);

      if (lFound != -1)
	{
	  Node<T> *nodeFound = succs[lFound];
	  if (!nodeFound->marked)
	    {
	      while (!nodeFound->fullyLinked)
		{ } //keep spinning
	      return ret;
	    }
	  continue; //try again
	}

      int highestLocked = -1;
      
      //some block

      Node<T> *pred, *succ;
      bool valid = true;
      for (int level = 0; valid && (level <= topLevel); level++)
	{
	  pred = preds[level];
	  succ = succs[level];
	  if (level == 0 || preds[level] != preds[level - 1]) //don't try to lock same node twice
	    pred->lock();
	  highestLocked = level;
	  
	  //make sure nothing has changed in between
	  valid = !pred->marked && !succ->marked && pred->next[level] == succ;
	}
      
      if (valid)
	{
	  Node<T> *newNode = new Node<T>(x, topLevel);
	  ret = newNode;
	  newNode->topLevel = topLevel;
	  for (int level = 0; level <= topLevel; level++)
	    {
	      newNode->next[level] = succs[level];
	      preds[level]->next[level] = newNode;
	    }
	  newNode->fullyLinked = true;
	  done = true;
	}
	
      //unlock everything here
      for (int level = 0; level <= highestLocked; level++)
	{
	  if (level == 0 || preds[level] != preds[level - 1]) //don't try to unlock the same node twice
	    preds[level]->unlock();
	}
    }

  return ret;
}

template<typename T>
bool SkipList<T>::remove(T x)
{
  Node<T> *victim = NULL;
  bool isMarked = false;
  int topLevel = -1;
  Node<T> **preds = new Node<T>*[MAX_LEVEL + 1];
  Node<T> **succs = new Node<T>*[MAX_LEVEL + 1];
  bool done = false;
  bool ret = false;

  while (!done)
    {
      int lFound = find(x, preds, succs);
      if (lFound != -1)
	{
	  victim = succs[lFound];
	}
      else
	{
	  break; //nothing
	}

      //logical or binary or?
      if (!isMarked ||
	  (lFound != -1 && 
	   (victim->fullyLinked && victim->topLevel == lFound && !victim->marked)))
	{
	  if (!isMarked)
	    {
	      topLevel = victim->topLevel;
	      victim->lock();
	      if (victim->marked)
		{
		  victim->unlock();
		  //return false; //not existant exit; clear up memory and stuff?
		  ret = false;
		  done = true;
		}
	      if (!done)
		{
		  victim->marked = true;
		  isMarked = true;
		}
	    }

	  if (!done)
	    {
	      int highestLocked = -1;
	      Node<T> *pred;//, *succ;
	      bool valid = true;
	      
	      for (int level = 0; valid && (level <= topLevel); level++)
		{
		  pred = preds[level];
		  if (level == 0 || preds[level] != preds[level - 1]) //don't do twice
		    pred->lock();
		  highestLocked = level;
		  valid = !pred->marked && pred->next[level] == victim;
		}
	      
	      if (valid)
		{
		  for (int level = topLevel; level >= 0; level--)
		    {
		      preds[level]->next[level] = victim->next[level];
		    }
		  victim->unlock();
		  //return true;
		  done = true;
		  ret = true;
		}
	      
	      //unlock mutexes
	      for (int i = 0; i <= highestLocked; i++)
		{
		  if (i == 0 || preds[i] != preds[i - 1])
		    preds[i]->unlock();
		}
	    }
	} 
      else 
	{
	  done = true;
	  ret = false;
	}
    } //while;

  //make sure we only delete the array of pointers here, not the actual elements!
  delete[] preds; 
  delete[] succs;

  return ret;
}

template<typename T>
bool SkipList<T>::contains(T x, Node<T> **contains_preds, Node<T> **contains_succs)
{
  //Node<T> contains_preds[MAX_LEVEL + 1];
  //Node<T> contains_succs[MAX_LEVEL + 1];
  int lFound = find(x, contains_preds, contains_succs);
  bool ret = lFound != -1 && contains_succs[lFound]->fullyLinked && !contains_succs[lFound]->marked;
  
  //**** JUST POINTERS ****
  //delete[] preds;
  //delete[] succs;

  return ret;
}

/* randomLevel() - returns a random level 
 * in range 0 .. MAX_LEVEL, inclusive.
 */
template<typename T>
int SkipList<T>::randomLevel()
{
  return rand() % (MAX_LEVEL + 1);
}

/*
 *
 */
template<typename T>
void SkipList<T>::printStats()
{
  int total = 0;
  for (int level = MAX_LEVEL; level >= 0; level--)
    {
      int count = 0;

      Node<T> *ptr = head;
      while (ptr->next[level] != NULL)
	{
	  count++;
	  ptr = ptr->next[level];
	}

      printf("Level %d: %d\n", level, count);
      total += count;
    }
  printf("total = %d\n", total);
}


