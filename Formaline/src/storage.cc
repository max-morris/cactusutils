// $Header$

#include <sstream>

#include "storage.hh"

using namespace std;



storage::
storage (enum state const st)
  : m_state (st)
{
}



storage::
~ storage ()
{
}



enum storage::state storage::
get_state ()
  const
{
  return m_state;
}

