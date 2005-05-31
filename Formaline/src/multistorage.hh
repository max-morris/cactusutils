// $Header$

#ifndef FORMALINE_MULTISTORAGE_HH
#define FORMALINE_MULTISTORAGE_HH

#include <list>

#include "storage.hh"



namespace Formaline
{

  using namespace std;



  class multistorage
  {
    list<storage *> stores;
  
    multistorage (multistorage const &);
  
    multistorage
    operator= (multistorage const &);
  
  public:
  
    multistorage ();
  
    ~ multistorage ();
  
    void
    add_storage (storage *);
  
    int
    num_storages ()
      const;
  
    void
    store (char const * key, bool value)
      const;
  
    void
    store (char const * key, int value)
      const;
  
    void
    store (char const * key, double value)
      const;
  
    void
    store (char const * key, char const * value)
      const;
  };



} // namespace Formaline



#endif // ifndef FORMALINE_MULTISTORAGE_HH
