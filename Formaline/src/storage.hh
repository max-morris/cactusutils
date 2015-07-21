#ifndef FORMALINE_STORAGE_HH
#define FORMALINE_STORAGE_HH

#include "cctk.h"

namespace Formaline {

class storage {
public:
  enum state { initial, update, final };

private:
  enum state m_state;

public:
  storage(enum state);

  virtual ~storage();

  enum state get_state() const;

  virtual storage *open_group(char const *name) = 0;

  virtual void close_group(storage *group) = 0;

  virtual void store(char const *key, bool value) = 0;

  virtual void store(char const *key, CCTK_INT value) = 0;

  virtual void store(char const *key, CCTK_REAL value) = 0;

  virtual void store(char const *key, char const *value) = 0;
};

} // namespace Formaline

#endif // #ifndef FORMALINE_STORAGE_HH
