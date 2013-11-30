#include "xtree.h"

/* INVARIANT: highest bit (left-most) of a key always set to 1
 * implies keys in range [0, 2^31-1]
 * public functions take "external keys", that is, keys in range [0, 2^31-1]
 * private functions and all internals operate on "internal keys", that is keys with the 
 * highest bit marked with 1 -> range [0. 2^32-1]
 */

//root has key = 1; [no prefix];
//children: 10 and 11 (for values 0 and 1)
XTree::XTree(int maxSize)
{
  root = new XNode();
  root->key = XTREE_ROOT_KEY;
  this->maxSize = maxSize;
  //NEED TO FIX THIS!!!
  memBlock = new XNode[maxSize * 20]; //this could theoretically backfire, but probably not for the tests that we have
  memset(memBlock, 0, sizeof(memBlock));
  memTop = 0;
  treeSize = 0;
}

XTree::~XTree()
{
  delete[] memBlock;
  //deleteAll(root);
}

void XTree::deleteAll(XNode *node)
{
  //make sure slNode is deleted in skipList
  if (node == NULL)
    return;
  if (node->hasLeft)
    deleteAll(node->left);
  if (node->hasRight)
    deleteAll(node->right);
  delete node;
}

/* getNewNode()
 * returns a new XNode item; goal: from a continuous block of memory
 */
XNode* XTree::getNewNode(unsigned key, Node<unsigned> *slPtr)
{
  XNode *ret;// = new XNode();
  ret = &memBlock[memTop++]; //read from a continuous block of memory
  ret->key = key;
  ret->slNode = slPtr;
  return ret;
}

/*unsigned XTree::externalKeyToInternal(unsigned value)
{

}*/

//NAIVE WAY - FIX THIS!!!
 /*unsigned XTree::internalKeyToExternal(unsigned key)
{
  //remove highest 1-bit
  for (int i = 31; i >= 0; i--)
    if ((key >> i) & 1)
      return key ^= (1 << i);
  return 0; //fail - impossible;
  }*/

/* keyLen:
 * return the index of the most significant bit of its argument
 */
unsigned XTree::keyLen(unsigned key)
{
  for (int i = 31; i >= 0; i--)
    if ((key >> i) & 1)
      return i;
  return 0;
}

/* insert():
 * inserts a key to the XTree
 * assumes keys are unique
 */
XNode* XTree::insert(unsigned newKey, Node<unsigned> *slPtr)
{
  treeSize++;
  XNode *cur = root;
  XNode *finalNode = getNewNode(newKey, slPtr);

  XNode *neighbor = NULL;

  //1 bit wasted for root?
  for (int len = 29; len >= 0; len--)
    {
      //cur = current node, "parent" of curKey
      //i.e. curKey needs to be inserted under cur
      //curkey is the high (30-len) bits shifted left
      //unsigned curkey = newKey & (~((1 << (len+1)) - 1));
      
      if ((newKey >> len) & 1) //right child
	{
	  //setting up successor
	  if (!cur->hasRight && neighbor == NULL) //has to create a branch for the first time
	    {
	      neighbor = cur->right;
	    }
	    
	  if (cur->left == NULL)
	    cur->left = finalNode;

	  if (!cur->hasLeft) //cur doesn't have left children
	    if(cur->left->key > newKey) //set successor to finalNode if newKey is smaller
	      cur->left = finalNode;
	  //done setting up successor

	  if (!cur->hasRight)
	    {
	      if (len > 0)
		cur->right = getNewNode(0, NULL); //curkey?
	      else
		cur->right = finalNode;
	      cur->hasRight = true;
	    }
	  cur = cur->right;

	  //insert a modified key (with high-bit on) to HT
	}
      else //left child
	{
	  //setting up successor
	  if (!cur->hasLeft && neighbor == NULL)
	    {
	      neighbor = cur->left;
	    }

	  if (cur->right == NULL)
	    cur->right = finalNode;

	  if (!cur->hasRight)
	    if (cur->right->key < newKey) //set predecessor to finalNode if newKey's bigger
	      cur->right = finalNode;
	  //done setting up successor

	  if (!cur->hasLeft)
	    {
	      if (len > 0)
		cur->left = getNewNode(0, NULL);//curkey?
	      else
		cur->left = finalNode;
	      cur->hasLeft = true;
	    }
	  cur = cur->left;
	  //insert a modified key (with high-bit on) to HT
	}
    }

  //insert cur to the leaves' linked list
  insertToLL(finalNode, neighbor);

  return cur;
}

void XTree::insertToLL(XNode *finalNode, XNode *neighbor)
{
  if (finalNode == NULL)
    return;

  if (neighbor == NULL) //list must be empty
    {
      leavesList = finalNode;
      return;
    }
    
  XNode *prev = NULL, *next = NULL;
  //it's a successor
  if (neighbor->key > finalNode->key)
    {
      next = neighbor;
      prev = next->leftNeighbor;
    }
  else if (neighbor->key < finalNode->key)
    {
      prev = neighbor;
      next = prev->rightNeighbor;
    }

  finalNode->leftNeighbor = prev;
  finalNode->rightNeighbor = next;
  if (prev)
    prev->rightNeighbor = finalNode;
  if (next)
    next->leftNeighbor = finalNode;
  
  if (prev == NULL) //left most, make it head of the list
    leavesList = finalNode;    
}

/* find():
 * returns a node with this key
 * or NULL if not found
 * naive 
 */
XNode* XTree::find(unsigned searchKey)
{
  XNode *cur = root;

  for (int i = 29; i >= 0; i--)
    if (((searchKey >> i) & 1) && cur->hasRight) //go right
      cur = cur->right;
    else if (!((searchKey >> i) & 1) && cur->hasLeft)
      cur = cur->left;
    else
      return NULL;

  return cur;
}

/* findOrNeighbor():
 * returns the search key, if found 
 * otherwise:
 * returns its immediate predecessor or successor
 */
XNode* XTree::findOrNeighbor(unsigned searchKey)
{
  XNode *cur = root;
  for (int i = 29; i >= 0; i--)
    if ((searchKey >> i) & 1) //need to go to the right child
      {
	if (cur->hasRight)
	  cur = cur->right;
	else
	  {
	    cur = cur->right;
	    break; //return immediate predecessor
	  }
      }
    else //key should be in the left child
      {
	if (cur->hasLeft)
	  cur = cur->left;
	else
	  {
	    cur = cur->left;
	    break; //return immediate successor
	  }
      }
  return cur;
}

XNode* XTree::findOrPredecessor(unsigned searchKey)
{
  XNode *ret = NULL;
  ret = findOrNeighbor(searchKey);
  if (ret && ret->key <= searchKey)
    return ret;
  else
    return ret->leftNeighbor;
}

XNode* XTree::predecessor(unsigned searchKey)
{
  XNode *ret = findOrNeighbor(searchKey);
  if (ret == NULL)
    return NULL;
  if (ret->key < searchKey)
    return ret;
  else if (ret->key == searchKey)
    return ret->leftNeighbor;
  else if (ret->leftNeighbor != NULL)
    {
      if (ret->leftNeighbor->key < searchKey)
	return ret->leftNeighbor;
      return ret->leftNeighbor->leftNeighbor;
    }
  return NULL;
}

XNode* XTree::successor(unsigned searchKey)
{
  XNode *ret = findOrNeighbor(searchKey);
  if (ret == NULL)
    return NULL;
  if (ret->key > searchKey)
    return ret;
  else if (ret->key == searchKey)
    return ret->rightNeighbor;
  else if (ret->rightNeighbor != NULL)
    {
      if (ret->rightNeighbor->key > searchKey)
	return ret->rightNeighbor;
      //might be equal
      return ret->rightNeighbor->rightNeighbor;
    }
  return NULL;
}

void XTree::printTreeState(XNode *ptr, int lvl)
{
  if (ptr == NULL)
    ptr = root;

  if (ptr == NULL)
    {
      printf("Tree empty\n");
      return;
    }

  if (!ptr->hasLeft && !ptr->hasRight)
    {
      printf("\t key = (%d)\n", ptr->key);
      return;
    }

  if (ptr->hasLeft)
    printTreeState(ptr->left, lvl + 1);
  
  if (ptr->hasRight)
    printTreeState(ptr->right, lvl + 1);

}

void XTree::printBinary(unsigned key)
{
  int klen = keyLen(key);
  for (int i = klen; i >= 0; i--)
    printf("%d", (key >> i) & 1);
}

void XTree::printNodesSorted()
{
  XNode *cur = leavesList;
  while (cur)
    {
      printf("%d ", cur->key);
      cur = cur->rightNeighbor;
    }
  printf("\n");
}

/*XNode* XTree::remove(unsigned oldKey)
{
  unsigned key = externalKeyToInternal(newKey);
  }*/

