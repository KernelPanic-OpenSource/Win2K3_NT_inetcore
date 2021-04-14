/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hashtable.cxx

Abstract:

    Simple hash table implementation.


Author:

    Paul M Midgen (pmidge) 14-August-2000


Revision History:

    14-August-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include <common.h>

/*++===========================================================================

    An array is separated into N buckets that     +---+
    each contain a pointer to a binary search     | 0 |------> O
    tree. The tree's nodes are indexed by a       +---+       / \
    DWORD identifier to enable speedy traversals. | 1 |      O   O
    The array buckets are indexed by the values   +---+     / \   \
    generated by the hashing function supplied    | 2 |    O   O   O
    by a derived class.                           +---+        
                                                  | N |

    Clients derive a class from the hashtable ADT and specialize it for a 
    given data type. Any data type can be used. The only function the client
    must implement is the GetHashAndBucket function, and their class must
    provide the number of buckets the ADT needs to support. This is usually
    some number that is modulo'd against the generated hashes to yield the
    bucket number.

===========================================================================--*/

#define HT_COMPARE_LARGER  0x00000001
#define HT_COMPARE_SMALLER 0x00000002
#define HT_COMPARE_EQUAL   0x00000003

#define HT_TREE_ROOT       0x00000004
#define HT_TREE_RHSUBTREE  0x00000005
#define HT_TREE_LHSUBTREE  0x00000006

typedef struct _NODE
{
  DWORD  hash;
  DWORD  bucket;
  LPVOID data;
  _NODE* parent;
  _NODE* rh_child;
  _NODE* lh_child;
  BOOL   isLeft;
}
NODE, *PNODE;

typedef VOID (*PFNCLEARFUNC)(LPVOID* ppv);

template <class T> class CHashTable
{
  public:
    CHashTable(DWORD buckets)
    {
      pfnClear  = NULL;
      cBuckets  = buckets;
      arBuckets = new PNODE[buckets];
      InitializeCriticalSection(&csTable);
    }

   ~CHashTable()
    {
      SAFEDELETEBUF(arBuckets);
      DeleteCriticalSection(&csTable);
    }

    virtual void GetHashAndBucket(T id, LPDWORD lpHash, LPDWORD lpBucket) =0;

    DWORD Insert(T id, LPVOID pv);
    DWORD Get(T id, LPVOID* ppv);
    DWORD Delete(T id, LPVOID* ppv);

    void  Clear(void);
    void  SetClearFunction(PFNCLEARFUNC pfn) { pfnClear = pfn; }

  private:
    void  _Get(DWORD hash, PNODE& proot, PNODE& pnode);
    DWORD _Insert(PNODE& proot, PNODE pnew);
    void  _Remove(DWORD hash, PNODE& proot, PNODE& pnode);

    PNODE _NewNode(T id, LPVOID pv);
    DWORD _CompareNodes(DWORD hash_target, DWORD hash_tree);

    BOOL  _HasChildren(PNODE pnode);
    void  _PostTraverseAndDelete(PNODE proot);
    void  _Lock(void)   { EnterCriticalSection(&csTable); }
    void  _Unlock(void) { LeaveCriticalSection(&csTable); }

    PNODE*       arBuckets;
    DWORD        cBuckets;
    CRITSEC      csTable;
    PFNCLEARFUNC pfnClear;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <class T> DWORD CHashTable<T>::Insert(T id, LPVOID pv)
{
  DWORD ret = ERROR_SUCCESS;
  PNODE pn  = _NewNode(id, pv);

  _Lock();

    if( pn )
    {
      ret = _Insert(arBuckets[pn->bucket], pn);
    }
    else
    {
      ret = ERROR_OUTOFMEMORY;
    }

  _Unlock();

  return ret;
}

template <class T> DWORD CHashTable<T>::Get(T id, LPVOID* ppv)
{
  DWORD ret    = ERROR_SUCCESS;
  DWORD hash   = 0L;
  DWORD bucket = 0L;
  PNODE pnode  = NULL;

  GetHashAndBucket(id, &hash, &bucket);

  _Lock();

    _Get(hash, arBuckets[bucket], pnode);

    if( pnode )
    {
      *ppv = (void*) pnode->data;
    }
    else
    {
      *ppv = NULL;
      ret  = ERROR_NOT_FOUND;
    }

  _Unlock();

  return ret;
}

template <class T> DWORD CHashTable<T>::Delete(T id, LPVOID* ppv)
{
  DWORD ret   = ERROR_SUCCESS;
  DWORD hash   = 0L;
  DWORD bucket = 0L;
  PNODE pnode = NULL;

  GetHashAndBucket(id, &hash, &bucket);

  _Lock();

    _Remove(hash, arBuckets[bucket], pnode);

    if( pnode )
    {
      if( ppv )
      {
        *ppv = pnode->data;
      }
      else
      {
        if( pfnClear )
        {
          pfnClear(&pnode->data);
        }
      }

      delete pnode;
    }
    else
    {
      ret = ERROR_NOT_FOUND;

      if( ppv )
      {
        *ppv = NULL;
      }
    }

  _Unlock();

  return ret;
}

template <class T> void CHashTable<T>::Clear(void)
{
  _Lock();

    for(DWORD n=0; n < cBuckets; n++)
    {
      if( arBuckets[n] )
      {
        _PostTraverseAndDelete(arBuckets[n]);
        arBuckets[n] = NULL;
      }
    }

  _Unlock();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <class T> DWORD CHashTable<T>::_Insert(PNODE& proot, PNODE pnew)
{
  DWORD ret = ERROR_SUCCESS;

  if( pnew )
  {
    if( !proot )
    {
      proot = pnew;
    }
    else
    {
      switch( _CompareNodes(pnew->hash, proot->hash) )
      {
        case HT_COMPARE_SMALLER :
          {
            pnew->isLeft = TRUE;
            pnew->parent = proot;
            ret = _Insert(proot->lh_child, pnew);
          }
          break;

        case HT_COMPARE_LARGER :
          {
            pnew->isLeft = FALSE;
            pnew->parent = proot;
            ret = _Insert(proot->rh_child, pnew);
          }
          break;

        case HT_COMPARE_EQUAL :
          {
            if( pfnClear )
            {
              pfnClear(&proot->data);
            }

            ret         = ERROR_DUP_NAME;
            proot->data = pnew->data;
            delete pnew;
          }
          break;
      }
    }
  }

  return ret;
}

template <class T> void CHashTable<T>::_Get(DWORD hash, PNODE& proot, PNODE& pnode)
{
  if( proot )
  {
    switch( _CompareNodes(hash, proot->hash) )
    {
      case HT_COMPARE_SMALLER :
        {
          _Get(hash, proot->lh_child, pnode);
        }
        break;

      case HT_COMPARE_LARGER :
        {
          _Get(hash, proot->rh_child, pnode);
        }
        break;

      case HT_COMPARE_EQUAL :
        {
          pnode = proot;
        }
        break;
    }
  }
  else
  {
    pnode = NULL;
  }
}

template <class T> void CHashTable<T>::_Remove(DWORD hash, PNODE& proot, PNODE& pnode)
{
  if( proot )
  {
    switch( _CompareNodes(hash, proot->hash) )
    {
      case HT_COMPARE_SMALLER :
        {
          _Remove(hash, proot->lh_child, pnode);
        }
        break;

      case HT_COMPARE_LARGER :
        {
          _Remove(hash, proot->rh_child, pnode);
        }
        break;

      case HT_COMPARE_EQUAL :
        {
          pnode = proot;

          //
          // if proot has no parent it is the tree's root node
          //
          //   - if it has children, promote the left child to root
          //     and insert the right child in the new tree. after
          //     inserting, make sure the new root has no parent.
          //
          //   - if it has no children the tree is empty, set the root
          //     to null
          //

          if( !proot->parent )
          {
            if( _HasChildren(proot) )
            {
              proot = proot->lh_child;
              _Insert(proot, pnode->rh_child);
              proot->parent = NULL;
            }
            else
            {
              proot = NULL;
            }
          }
          else
          {
            if( proot->isLeft )
            {
              proot->parent->lh_child = NULL;
            }
            else
            {
              proot->parent->rh_child = NULL;
            }

            _Insert(pnode->parent, pnode->lh_child);
            _Insert(pnode->parent, pnode->rh_child);
          }
        }
        break;
    }
  }
  else
  {
    pnode = NULL;
  }
}

template <class T> void CHashTable<T>::_PostTraverseAndDelete(PNODE proot)
{
  if( proot )
  {
    _PostTraverseAndDelete(proot->lh_child);
    _PostTraverseAndDelete(proot->rh_child);

    if( pfnClear )
    {
      pfnClear(&proot->data);
    }

    delete proot;
    proot = NULL;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <class T> DWORD CHashTable<T>::_CompareNodes(DWORD hash_target, DWORD hash_tree)
{
  if( hash_target == hash_tree )
  {
    return HT_COMPARE_EQUAL;
  }
  else if( hash_target < hash_tree )
  {
    return HT_COMPARE_SMALLER;
  }
  else
  {
    return HT_COMPARE_LARGER;
  }
}

template <class T> PNODE CHashTable<T>::_NewNode(T id, LPVOID pv)
{
  PNODE pn = new NODE;

  if( pn )
  {
    GetHashAndBucket(id, &pn->hash, &pn->bucket);
    pn->data = pv;
  }

  return pn;
}

template <class T> BOOL CHashTable<T>::_HasChildren(PNODE pnode)
{
  if( pnode )
  {
    return (pnode->lh_child || pnode->rh_child);
  }
  else
  {
    return FALSE;
  }
}
