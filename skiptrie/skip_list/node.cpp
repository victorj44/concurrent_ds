
template<typename T>
Node<T>::Node(T val, int height) :
  val(val)
{
  next = new Node<T>*[height + 1];
  topLevel = height;
}

template<typename T>
Node<T>::~Node()
{
  //delete[] next;
}

