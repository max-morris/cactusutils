#include <cassert>
#include <iomanip>
#include <ios>
#include <limits>
#include <sstream>

#include "cctk_Parameters.h"

#include "file.hh"

using namespace std;

namespace Formaline {

file::file(char const *const id, enum state const st, char const *const p,
           file *const par)
    : storage(st), path(p), parent(par) {
  DECLARE_CCTK_PARAMETERS;

  if (parent)
    return;

  ostringstream filenamebuf;
  filenamebuf << out_dir << "/" << storage_filename;
  string const filenamestring = filenamebuf.str();

  ios::openmode const mode = get_state() == initial ? ios::trunc : ios::app;
  fil.open(filenamestring.c_str(), mode);

  if (get_state() == initial) {
    store("jobid", id);
  }
}

file::~file() {
  if (parent)
    return;

  if (get_state()) {
    store("simulation", "done");
  }
  fil.close();
}

file *file::open_group(char const *const name) {
  assert(name);
  string name1(name);
  if (not name1.empty() and name1[name1.length() - 1] != '/') {
    name1 = name1 + "/";
  }
  return new file(0, get_state(), name1.c_str(), this);
}

void file::close_group(storage *group) { delete group; }

void file::store(char const *const key, bool const value) {
  assert(key);

  ostringstream keybuf;
  keybuf << path << key;
  ostringstream valuebuf;
  valuebuf << (value ? "yes" : "no");

  ostringstream buf;
  buf << clean(keybuf.str()) << "=" << clean(valuebuf.str()) << "\n";

  write(buf.str());
}

void file::store(char const *const key, CCTK_INT const value) {
  assert(key);

  ostringstream keybuf;
  keybuf << path << key;
  ostringstream valuebuf;
  valuebuf << value;

  ostringstream buf;
  buf << clean(keybuf.str()) << "=" << clean(valuebuf.str()) << "\n";

  write(buf.str());
}

void file::store(char const *const key, CCTK_REAL const value) {
  assert(key);

  int const prec = numeric_limits<CCTK_REAL>::digits10;

  ostringstream keybuf;
  keybuf << path << key;
  ostringstream valuebuf;
  valuebuf << setprecision(prec) << value;

  ostringstream buf;
  buf << clean(keybuf.str()) << "=" << clean(valuebuf.str()) << "\n";

  write(buf.str());
}

void file::store(char const *const key, char const *const value) {
  assert(key);

  ostringstream keybuf;
  keybuf << path << key;
  ostringstream valuebuf;
  valuebuf << value;

  ostringstream buf;
  buf << clean(keybuf.str()) << "="
      << "\"" << clean(valuebuf.str()) << "\"\n";

  write(buf.str());
}

void file::write(string const &msg) {
  if (parent) {
    parent->write(msg);
  } else {
    fil << msg;
  }
}

string file::clean(string const &txt) const {
  ostringstream buf;

  for (string::const_iterator p = txt.begin(); p != txt.end(); ++p) {
    switch (*p) {
    case '=':
      buf << "\\=";
      break;
    case '"':
      buf << "\\\"";
      break;
    case '\\':
      buf << "\\\\";
      break;
    default:
      buf << *p;
    }
  }

  return buf.str();
}

} // namespace Formaline
