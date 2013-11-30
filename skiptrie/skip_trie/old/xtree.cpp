
template<typename T>
XTree<T>::XTree()
{
  ht[1] = NULL; //insert the dummy one
}

template<typename T>
XTree<T>::~XTree()
{

}

/* find (key)
 * returns a pointer to the value
 * or NULL if not found
 * assumes key is a !! 31-bit !!  wide unsigned integer
 */
template<typename T>
T* XTree<T>::find(unsigned key)
{
  if ( (key >> 31) & 1)
    return false;
  key |= (1 << 31); //set the high bit

  return ht.find(key);
}

/* predecessor(key)
 * returns a predecessor of key 
 */
template<typename T>
unsigned predecessor(unsigned key)
{
  if ( (key >> 31) & 1)
    return false;
  key |= (1 << 31); //set the high bit

  return 0;
}

/* successor(key)
 * returns a successor of key
 */
template<typename T>
unsigned successor(unsigned key)
{
  return 0;
}

/* insert (key, value)
 * returns true if inserted (keys must be unique)
 * false if failed (ie. duplicate key)
 */
template<typename T>
bool XTree<T>::insert(unsigned key, T* val)
{
  if ( (key >> 31) & 1)
    return false;
  key |= (1 << 31); //set the high bit

  if (ht.find(key) != ht.end()) //key already in
    return false;

  //!!!!!!!!!!!!!! need to update left/right pointers!!!

  ht[key] = val;
  key >>= 1;
  for (int i = 29; i >= 0; i--) //don't re-insert the dummy one
    {
      if (ht.find(key) != ht.end())
	break;
      ht[key] = NULL;
      key >>= 1;
    }

  return true;
}

/* remove (key)
 * returns true if successful
 * false o/w (ie. key does not exist)
 */
template<typename T>
bool XTree<T>::remove(unsigned key)
{
  if ( (key >> 31) & 1)
    return false;
  key |= (1 << 31); //set the high bit

  if (ht.find(key) == ht.end()) //no key at all;
    return false;

  //!!!!!!!!!!!! need to update left/right pointers

  ht.erase(key);
  key >>= 1;
  for (int i = 29; i >= 0; i--)
    {
      //key has (exactly) one son left
      if (ht.find( (key << 1) + 1 ) != ht.end() || ht.find( (key << 1) ) != ht.end())
	break;
      ht.erase(key);
      key >>= 1;
    }

  return true;
}
