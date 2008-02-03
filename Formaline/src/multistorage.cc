#include "multistorage.hh"



namespace Formaline
{

  using namespace std;



  multistorage::
  multistorage ()
  {
  }



  multistorage::
  ~ multistorage ()
  {
    for (list<storage *>::const_iterator it = stores.begin();
         it != stores.end();
         ++ it)
    {
      delete * it;
    }
  }



  void multistorage::
  add_storage (storage * const s)
  {
    stores.push_front (s);
  }



  int multistorage::
  num_storages ()
    const
  {
    return stores.size();
  }



  void multistorage::
  store (char const * const key, bool const value)
    const
  {
    for (list<storage *>::const_iterator it = stores.begin();
         it != stores.end();
         ++ it)
    {
      (* it)->store (key, value);
    }
  }



  void multistorage::
  store (char const * const key, CCTK_INT const value)
    const
  {
    for (list<storage *>::const_iterator it = stores.begin();
         it != stores.end();
         ++ it)
    {
      (* it)->store (key, value);
    }
  }



  void multistorage::
  store (char const * const key, CCTK_REAL const value)
    const
  {
    for (list<storage *>::const_iterator it = stores.begin();
         it != stores.end();
         ++ it)
    {
      (* it)->store (key, value);
    }
  }



  void multistorage::
  store (char const * const key, char const * const value)
    const
  {
    // Ignore null strings
    if (value == 0) return;
    
    for (list<storage *>::const_iterator it = stores.begin();
         it != stores.end();
         ++ it)
    {
      (* it)->store (key, value);
    }
  }



} // namespace Formaline
