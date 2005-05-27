// $Header$

#ifndef MULTISTORAGE_HH
#define MULTISTORAGE_HH

#include <list>

#include "storage.hh"

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
  store (char const * key, int value)
    const;
  
  void
  store (char const * key, char const * value)
    const;
};



#endif // ifndef MULTISTORAGE_HH
