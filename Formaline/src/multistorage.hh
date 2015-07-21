#ifndef FORMALINE_MULTISTORAGE_HH
#define FORMALINE_MULTISTORAGE_HH

#include <list>

#include "cctk.h"

#include "storage.hh"

namespace Formaline {

class multistorage {
  std::list<storage *> stores;

  multistorage(multistorage const &);

  multistorage operator=(multistorage const &);

public:
  multistorage();

  ~multistorage();

  void close();

  void add_storage(storage *);

  storage *remove_storage();

  int num_storages() const;

  void open_group(multistorage &, char const *name);

  void close_group(multistorage &);

  void store(char const *key, bool value) const;

  void store(char const *key, CCTK_INT value) const;

  void store(char const *key, CCTK_REAL value) const;

  void store(char const *key, char const *value) const;

#ifdef HAVE_CCTK_INT1
#ifndef CCTK_INTEGER_PRECISION_1
  void store(char const *key, CCTK_INT1 value) const {
    store(key, static_cast<CCTK_INT>(value));
  }
#endif
#endif

#ifdef HAVE_CCTK_INT2
#ifndef CCTK_INTEGER_PRECISION_2
  void store(char const *key, CCTK_INT2 value) const {
    store(key, static_cast<CCTK_INT>(value));
  }
#endif
#endif

#ifdef HAVE_CCTK_INT4
#ifndef CCTK_INTEGER_PRECISION_4
  void store(char const *key, CCTK_INT4 value) const {
    store(key, static_cast<CCTK_INT>(value));
  }
#endif
#endif

#ifdef HAVE_CCTK_INT8
#ifndef CCTK_INTEGER_PRECISION_8
  void store(char const *key, CCTK_INT8 value) const {
    store(key, static_cast<CCTK_INT>(value));
  }
#endif
#endif

#ifdef HAVE_CCTK_REAL4
#ifndef CCTK_REAL_PRECISION_4
  void store(char const *key, CCTK_REAL4 value) const {
    store(key, static_cast<CCTK_REAL>(value));
  }
#endif
#endif

#ifdef HAVE_CCTK_REAL8
#ifndef CCTK_REAL_PRECISION_8
  void store(char const *key, CCTK_REAL8 value) const {
    store(key, static_cast<CCTK_REAL>(value));
  }
#endif
#endif

#ifdef HAVE_CCTK_REAL16
#ifndef CCTK_REAL_PRECISION_16
  void store(char const *key, CCTK_REAL16 value) const {
    store(key, static_cast<CCTK_REAL>(value));
  }
#endif
#endif
};

} // namespace Formaline

#endif // ifndef FORMALINE_MULTISTORAGE_HH
