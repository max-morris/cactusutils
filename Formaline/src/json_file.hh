#ifndef FORMALINE_JSON_FILE_HH
#define FORMALINE_JSON_FILE_HH

#include <fstream>
#include <string>

#include "storage.hh"

namespace Formaline {

class json_file : public storage {
  std::ofstream fil;
  json_file *const parent;

  bool need_comma;
  int indent_level;

public:
  json_file(char const *id, enum state st, json_file *const par = 0);

  virtual ~json_file();

  virtual json_file *open_group(char const *name);

  virtual void close_group(storage *group);

  virtual void store(char const *key, bool value);

  virtual void store(char const *key, CCTK_INT value);

  virtual void store(char const *key, CCTK_REAL value);

  virtual void store(char const *key, char const *value);

private:
  void set_need_comma(bool flag);

  void increase_indent_level();

  void decrease_indent_level();

  void write(std::string const &msg);

  std::string stringify(std::string const &txt) const;
};

} // namespace Formaline

#endif // ifndef FORMALINE_JSON_FILE_HH
