#ifndef __HIGH_ENUMERTOR_H__
#define __HIGH_ENUMERTOR_H__

#include "Index.h"
#include "HBaseDesignItem.h"

template <class Item, int disambiguation = 0>
class HEnumerator : public Item
{
  IDType m_start;
  IDType m_end;

public:
  HEnumerator(IDType aStart, IDType aEnd)
    : Item(aStart-1),
    m_end(aEnd),
    m_start(aStart)
  {
  }

  void Reset()
  {
    FromID(*this, m_start - 1);
  }

  bool MoveNext()
  {
    return ++__GetID(*this) < m_end;
  }
};

template <class Item, IDType base, int disambiguation = 0>
class HConstEnumerator : public Item
{
protected:
  IDType m_end;

public:
  HConstEnumerator(IDType aEnd)
    : Item(base - 1),
    m_end(aEnd)
  {
  }

  void Reset()
  {
    FromID(*this, base - 1);
  }

  bool MoveNext()
  {
    return ++__GetID(*this) < m_end;
  }
};

template <class Item, class Collection, int disambiguation = 0>
class HEnumeratorW : public Item
{
protected:
  IDType m_start;
  IDType m_end;

public:
  HEnumeratorW(Collection* data, IDType aStart, IDType aEnd)
    : Item(data, aStart-1),
    m_end(aEnd),
    m_start(aStart)
  {
  }

  void Reset()
  {
    FromID(*this, m_start - 1);
  }

  bool MoveNext()
  {
    return ++__GetID(*this) < m_end;
  }
};

template <class Item, int disambiguation = 0>
class HIndexEnumerator : public Item
{
protected:
  HIndex& m_Index;
  int m_curPos;

public:
  HIndexEnumerator(HIndex& index)
    : Item(0), m_Index(index), m_curPos(-1)
  {
  }

  void Reset()
  {
    m_curPos = -1;
  }

  bool MoveNext()
  {
    if(++m_curPos < m_Index.Size())
    {
      FromID(*this, m_Index[m_curPos]);
      return true;
    }
    return false;
  }
};

template <class Item, class Collection, int disambiguation = 0>
class HIndexEnumeratorW : public Item
{
  HIndex& m_Index;
  int m_curPos;

public:
  HIndexEnumeratorW(Collection* data, HIndex& index)
    : Item(data, 0), m_Index(index), m_curPos(-1)
  {
  }

  void Reset()
  {
    m_curPos = -1;
  }

  bool MoveNext()
  {
    if(++m_curPos < m_Index.Size())
    {
      FromID(*this, m_Index[m_curPos]);
      return true;
    }
    return false;
  }
};

#endif //__HIGH_ENUMERTOR_H__
