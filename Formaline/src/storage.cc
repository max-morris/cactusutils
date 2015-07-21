#include <sstream>

#include "storage.hh"

namespace Formaline {

storage::storage(enum state const st) : m_state(st) {}

storage::~storage() {}

enum storage::state storage::get_state() const { return m_state; }

} // namespace Formaline
