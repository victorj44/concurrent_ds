
template<typename T>
Node<T>::Node(T val, int height) :
  val(val),
  topLevel(height)
{
  //pthread_mutex_init(&nodeLock, NULL);
  marked = false;
  fullyLinked = false;
}

template<typename T>
Node<T>::~Node()
{
  
}

template<typename T>
void Node<T>::lock()
{
  //pthread_mutex_lock(&nodeLock);
}

template<typename T>
void Node<T>::unlock()
{
  //pthread_mutex_unlock(&nodeLock);
}
