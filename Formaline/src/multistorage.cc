// $Header$

#include "multistorage.hh"

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
store (char const * const key, int const value)
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
store (char const * const key, double const value)
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
  for (list<storage *>::const_iterator it = stores.begin();
       it != stores.end();
       ++ it)
  {
    (* it)->store (key, value);
  }
}
