#ifndef FORMALINE_FILE_HH
#define FORMALINE_FILE_HH

#include <fstream>
#include <string>

#include "storage.hh"

namespace Formaline {

class file : public storage {
  std::ofstream fil;
  std::string const path;
  file *const parent;

public:
  file(char const *id, enum state st, char const *p = "", file *const par = 0);

  virtual ~file();

  virtual file *open_group(char const *name);

  virtual void close_group(storage *group);

  virtual void store(char const *key, bool value);

  virtual void store(char const *key, CCTK_INT value);

  virtual void store(char const *key, CCTK_REAL value);

  virtual void store(char const *key, char const *value);

private:
  void write(std::string const &msg);

  std::string clean(std::string const &txt) const;
};

} // namespace Formaline

#endif // ifndef FORMALINE_FILE_HH
