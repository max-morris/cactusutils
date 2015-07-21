#include <cassert>
#include <cctype>
#include <iomanip>
#include <ios>
#include <limits>
#include <sstream>

#include "cctk_Parameters.h"

#include "json_file.hh"

using namespace std;

namespace Formaline {

json_file::json_file(char const *const id, enum state const st,
                     json_file *const par)
    : storage(st), parent(par), need_comma(false), indent_level(0) {
  DECLARE_CCTK_PARAMETERS;

  if (parent)
    return;

  ostringstream filenamebuf;
  filenamebuf << out_dir << "/" << storage_json_filename;
  string const filenamestring = filenamebuf.str();

  ios::openmode const mode = get_state() == initial ? ios::trunc : ios::app;
  fil.open(filenamestring.c_str(), mode);
  fil << "{";
  increase_indent_level();

  if (get_state() == initial) {
    store("jobid", id);
  }
}

json_file::~json_file() {
  if (parent)
    return;

  if (get_state()) {
    store("simulation", "done");
  }

  decrease_indent_level();
  set_need_comma(false);
  write("}");

  fil << "\n";
  fil.close();
  assert(indent_level == 0);
}

json_file *json_file::open_group(char const *const name) {
  assert(name);
  ostringstream buf;
  buf << stringify(name) << ": {";
  write(buf.str());
  increase_indent_level();
  set_need_comma(false);
  return new json_file(0, get_state(), this);
}

void json_file::close_group(storage *group) {
  delete group;
  decrease_indent_level();
  set_need_comma(false);
  write("}");
}

void json_file::store(char const *const key, bool const value) {
  assert(key);

  ostringstream buf;
  buf << stringify(key) << ": " << boolalpha << value << noboolalpha;

  write(buf.str());
}

void json_file::store(char const *const key, CCTK_INT const value) {
  assert(key);

  ostringstream buf;
  buf << stringify(key) << ": " << value;

  write(buf.str());
}

void json_file::store(char const *const key, CCTK_REAL const value) {
  assert(key);

  int const prec = numeric_limits<CCTK_REAL>::digits10;

  // TODO: ensure that there is always a decimal dot
  ostringstream buf;
  buf << stringify(key) << ": " << setprecision(prec) << value;

  write(buf.str());
}

void json_file::store(char const *const key, char const *const value) {
  assert(key);

  ostringstream buf;
  buf << stringify(key) << ": " << stringify(value);

  write(buf.str());
}

void json_file::set_need_comma(bool flag) { need_comma = flag; }

void json_file::increase_indent_level() { ++indent_level; }

void json_file::decrease_indent_level() {
  assert(indent_level > 0);
  --indent_level;
}

void json_file::write(string const &msg) {
  if (parent) {
    parent->write(msg);
    return;
  }
  if (need_comma) {
    fil << ",";
  }
  fil << "\n";
  for (int i = 0; i < indent_level; ++i) {
    fil << "    ";
  }
  fil << msg;
  need_comma = true;
}

string json_file::stringify(string const &txt) const {
  ostringstream buf;

  buf << "\"";
  for (string::const_iterator p = txt.begin(); p != txt.end(); ++p) {
    switch (*p) {
    case '"':
      buf << "\\\"";
      break;
    case '\\':
      buf << "\\\\";
      break;
    case '\b':
      buf << "\\b";
      break;
    case '\f':
      buf << "\\f";
      break;
    case '\n':
      buf << "\\n";
      break;
    case '\r':
      buf << "\\r";
      break;
    case '\t':
      buf << "\\t";
      break;
    default:
      if (!isprint(*p)) {
        buf << "\\u" << hex << setw(4) << setfill('0') << int(*p) << dec
            << setw(0) << setfill(' ');
      } else {
        buf << *p;
      }
    }
  }
  buf << "\"";

  return buf.str();
}

} // namespace Formaline
