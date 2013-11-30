#include "xtree_hash.h"
#include <stdio.h>

XTree::XTree(unsigned long long maxSize)
{
  printf("key_bits = %d, HT size = %d\n", KEY_BITS, 1<<(KEY_BITS+1));
  HT_INIT(1ULL << (KEY_BITS + 1));
  root = new HNode(0, NULL, NULL);
  HT_INSERT(1, root);
  treeSize = 0;
}

XTree::~XTree()
{
  
}

//len in [1..KEY_BITS]
inline unsigned long long XTree::toHashKey(unsigned long long key, int len)
{
  return maskKey(key, len) | (1ULL << len);
}

//len in [1..KEY_BITS]
inline unsigned long long XTree::maskKey(unsigned long long key, int len)
{
  return key >> (KEY_BITS - len);
}

//returns true if key successfully inserted
//false o/w (ie. key duplicate, out of range, out of mem etc.)
bool XTree::insert(unsigned long long key, Node<unsigned long long> *bottomNode)
{
  //printf("INSERT KEY = %d\n", key);
  //key out of range?
  //if ((key >> KEY_BITS) & 1)
  if ((key >> KEY_BITS) > (unsigned long long)0)
    {
      printf("key out of range (key_bits = %d, key = %llu, valu = %llu!\n", KEY_BITS, key, key >> KEY_BITS);
      return false;
    }
  
  //duplicated key?
  if (HT_CONTAINS(toHashKey(key, KEY_BITS)))
    {
      printf("duplicate key %llu!\n", key);
      return false;
    }
  
  treeSize++;
  HNode *leaf = new HNode(key, NULL, NULL);
  leaf->bottomNode = bottomNode;

  HNode *cur = leaf;
  unsigned long long curKey = maskKey(key, KEY_BITS);
  unsigned long long curHKey = toHashKey(key, KEY_BITS);

  HT_INSERT(curHKey, leaf); //insert leaf

  //insert bottom-up
  for (int len = KEY_BITS - 1; len >= 0; len--)
    {
      unsigned long long parentKey = curKey >> 1;
      unsigned long long parentHKey = curHKey >> 1;
      HNode *parent = NULL;

      //hkey not in the xtrie
      if (!HT_CONTAINS(parentHKey))
	{
	  parent = new HNode(parentKey, leaf, leaf);
	  /*printf("inserting new key:\n");
	  printBinary(parentHKey); 
	  printf("\n");*/
	  HT_INSERT(parentHKey, parent);
	}
      else
	parent = HT_GET(parentHKey);

      //set successor & predecessor of the leaf on the linked list
      //when joining the path *the first time only* into the main tree
      if (leaf->left == NULL && leaf->right == NULL && 
	  parent->left != leaf && parent->right != leaf)
	{
	  if (curKey & 1) //attaching as the right child
	    {
	      leaf->left = parent->right;
	      if (leaf->left != NULL) //insert onto the linked list
		{
		  leaf->right = leaf->left->right;
		  leaf->left->right = leaf;
		  if (leaf->right != NULL)
		    leaf->right->left = leaf;
		} //NULL only when the first element is inserted
	    }
	  else //attaching as the left child
	    {
	      leaf->right = parent->left;
	      if (leaf->right != NULL) //insert onto the linked list
		{
		  leaf->left = leaf->right->left;
		  leaf->right->left = leaf;
		  if (leaf->left != NULL)
		    leaf->left->right = leaf;
		}//NULL only when the first element is inserted
	    }
	}

      if (parent->left == NULL || leaf->key < parent->left->key)
	parent->left = leaf;

      if (parent->right == NULL || leaf->key > parent->right->key)
	parent->right = leaf;      
      
      //right child?
      if (curKey & 1) 
	{
	  //if parent->left is not a direct son
	  //parent->left points to the left most leaf in the subtree (or NULL)

	  parent->right = cur;
	}
      else //left child
	{

	  parent->left = cur;
	}

      curKey = parentKey;
      curHKey = parentHKey;
      cur = parent;
    }
  return true;
}

//returns true if key is in the xtrie
bool XTree::find(unsigned long long key)
{
  //key out of range?
  if ((key >> KEY_BITS) & 1)
    return false;

  unsigned long long hkey = toHashKey(key, KEY_BITS);
  return HT_CONTAINS(hkey);
}

//if predecessor exists, returns true and write its value to output
//o/w returns false
//if returned != NULL, sets outputHits to the # of hash table hits
HNode *XTree::findOrPredecessor(unsigned long long key, int *outputHits)
{
  //key out of range?
  if ((key >> KEY_BITS) & 1)
    return NULL;

  int hthits = 0; //stats for the # of hash table hits
  unsigned long long hkey = toHashKey(key, KEY_BITS);

  //binary search to find the longest prefix (MSBs) of the search key
  int left = 0, right = KEY_BITS;
  int maxFound = 0;
  while (left <= right)
    {
      int len = (left + right)/2;
      unsigned long long tmpKey = hkey >> (KEY_BITS - len);
      hthits++;
      if (!HT_CONTAINS(tmpKey))
	right = len - 1;
      else 
	{
	  maxFound = len;
	  left = len + 1;
	}
    }

  //printf("max found = %d\n", maxFound);
  HNode *leaf = NULL;
  hthits++;
  //longest prefix = maxFound
  if (maxFound == KEY_BITS) //the entire key
    leaf = HT_GET(hkey);
  else
    {
      HNode *parent = HT_GET(hkey >> (KEY_BITS - maxFound));

      /*printf("parent = %p, key = %u, left = %p (%u), right = %p (%u)\n", 
	     parent, parent->key, parent->left, parent->left->key, 
	     parent->right, parent->right->key);
	     printf("parent hkey: \n");*/
      //printBinary(toHashKey(parent->key, maxFound)); printf("\n");

      //would be in the right sub-tree, but there's nothing there
      //so leaf is a "predecessor" of search key
      if ((hkey >> (KEY_BITS - maxFound - 1)) & 1)
	{
	  //printf("leaf = right of parent\n");
	  leaf = parent->right;
	}
      else
	{
	  //printf("leaf = left of parent\n");
	  leaf = parent->left; //leaf is a successor of the search key
	}
    }
  if (leaf != NULL && leaf->key > key)
    leaf = leaf->left;
  //printf("returning key = %u, %p\n", leaf != NULL ? leaf->key : 0, (void *)leaf);
  //if (leaf)
  //printBinary(toHashKey(leaf->key, KEY_BITS));
  //printf("\n");
  
  if (outputHits != NULL)
    *outputHits = hthits;

  return leaf;
}

bool XTree::predecessor(unsigned long long key, unsigned long long *output, int *outputHits)
{
#ifdef RUN_DUMB_QUERIES
  int ret = 0;
  for (int i = 0; i < 7; i++)
    {
      ret += (int)HT_CONTAINS(key + i *100);
    }
  return (bool) ret;
#endif

  //printf("********** predecessor for %d\n", key);
  HNode *leaf = findOrPredecessor(key, outputHits);
  if (leaf == NULL)
    return false;
  if (leaf->key < key)
    {
      *output = leaf->key;
      return true;
    }
  //must be the case that leaf->key >= key
  leaf = leaf->left;
  if (leaf == NULL)
    return false;
  *output = leaf->key;
  return true;
}

void XTree::printBinary(unsigned long long key)
{
  for (int i = KEY_BITS; i >= 0; i--)
    printf("%llu", (key >> i) & 1);
}


void XTree::printTree(unsigned long long startHKey, int depth)
{
  if (depth >= KEY_BITS + 1)
    return;
  if (!HT_CONTAINS(startHKey))
    return;
  HNode *node = HT_GET(startHKey);
  
  printf("key = %llu, node = %p, left key = %llu (%p), right key = %llu (%p)\n", 
	 startHKey,
	 (void *)node, 
	 node->left != NULL ? node->left->key : 0,
	 node->left,
	 node->right != NULL ? node->right->key : 0,
	 node->right);
  printTree(startHKey << 1, depth+1); //left sub-tree
  printTree((startHKey << 1) | 1, depth+1); //right sub-tree
}

void XTree::printLinkedList(unsigned long long minKey)
{
  /*unsigned minHKey = minKey | (1<<KEY_BITS);
  if (ht.find(minHKey) == ht.end())
    {
      printf("start key not found\n");
      return;
    }
  HNode *p = ht[minHKey];
  while (p != NULL)
    {
      printf("p = %p, key = %u, left = %p, right = %p, key:\n", (void *)p, 
	     p->key, (void*)(p->left), (void*)(p->right));
      printBinary(p->key);
      printf("\n");
      p = p->right;
      }*/
}

void XTree::printTreeStats()
{
  printf("sizeof(HNODE) = %lu\n", sizeof(HNode));
  printf("total # items: %llu\n", treeSize);
  //printf("HT size: %d\n", (int)ht.size());
}
