#include "multistorage.hh"

using namespace std;

namespace Formaline {

multistorage::multistorage() {}

multistorage::~multistorage() { close(); }

void multistorage::close() {
  for (list<storage *>::const_iterator it = stores.begin(); it != stores.end();
       ++it) {
    delete *it;
  }
  stores.clear();
}

void multistorage::add_storage(storage *const s) { stores.push_front(s); }

storage *multistorage::remove_storage() {
  storage *const s = stores.front();
  stores.pop_front();
  return s;
}

int multistorage::num_storages() const { return stores.size(); }

void multistorage::open_group(multistorage &ms, char const *const name) {
  for (list<storage *>::const_iterator it = stores.begin(); it != stores.end();
       ++it) {
    ms.add_storage((*it)->open_group(name));
  }
}

void multistorage::close_group(multistorage &ms) {
  for (list<storage *>::const_iterator it = stores.begin(); it != stores.end();
       ++it) {
    (*it)->close_group(ms.remove_storage());
  }
}

void multistorage::store(char const *const key, bool const value) const {
  for (list<storage *>::const_iterator it = stores.begin(); it != stores.end();
       ++it) {
    (*it)->store(key, value);
  }
}

void multistorage::store(char const *const key, CCTK_INT const value) const {
  for (list<storage *>::const_iterator it = stores.begin(); it != stores.end();
       ++it) {
    (*it)->store(key, value);
  }
}

void multistorage::store(char const *const key, CCTK_REAL const value) const {
  for (list<storage *>::const_iterator it = stores.begin(); it != stores.end();
       ++it) {
    (*it)->store(key, value);
  }
}

void multistorage::store(char const *const key, char const *const value) const {
  // Ignore null strings
  if (value == 0)
    return;

  for (list<storage *>::const_iterator it = stores.begin(); it != stores.end();
       ++it) {
    (*it)->store(key, value);
  }
}

} // namespace Formaline
