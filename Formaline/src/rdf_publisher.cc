#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include "util_Table.h"
#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include "Publish.h"

#include "rdf.hh"

namespace Formaline {

// buffer of published RDF metadata since the last Update() call
std::vector<rdfPublishItem> rdfPublishList;

static rdfPublishItem *CreateNewItem(CCTK_POINTER_TO_CONST cctkGH,
                                     CCTK_POINTER data, CCTK_INT level,
                                     CCTK_STRING name, CCTK_STRING key,
                                     bool isTable) {
  DECLARE_CCTK_PARAMETERS;

  // prevent compiler warnings about unused parameters
  data = &data;

  assert(key and *key);
  assert(level >= 0);
  rdfPublishItem *item = NULL;
  if (level <= publish_level) {
    rdfPublishItem newItem;
    rdfPublishList.push_back(newItem);
    item = &rdfPublishList.back();
    char *currentdate = Util_CurrentDateTime();
    item->datetime = clean(currentdate);
    free(currentdate);
    item->hasCCTKinfo = cctkGH != NULL;
    if (item->hasCCTKinfo) {
      const cGH *_cctkGH = (const cGH *)cctkGH;
      item->cctk_time = _cctkGH->cctk_time;
      item->cctk_iteration = _cctkGH->cctk_iteration;
    }
    if (name and *name)
      item->name = clean(name);
    item->key = clean(key);
    item->isTable = isTable;
  }
  return item;
}

static CCTK_INT PublishBooleanAsRDF(CCTK_POINTER_TO_CONST cctkGH,
                                    CCTK_POINTER data, CCTK_INT level,
                                    CCTK_INT value, CCTK_STRING key,
                                    CCTK_STRING name) {
  rdfPublishItem *item = CreateNewItem(cctkGH, data, level, name, key, false);
  if (item) {
    item->scalar.value = value ? "true" : "false";
    item->scalar.type = "boolean";
  }

  return (item ? 1 : 0);
}

static CCTK_INT PublishIntAsRDF(CCTK_POINTER_TO_CONST cctkGH, CCTK_POINTER data,
                                CCTK_INT level, CCTK_INT value, CCTK_STRING key,
                                CCTK_STRING name) {
  rdfPublishItem *item = CreateNewItem(cctkGH, data, level, name, key, false);
  if (item) {
    std::ostringstream buf;
    buf << value;
    item->scalar.value = buf.str();
    item->scalar.type = "int";
  }

  return (item ? 1 : 0);
}

static CCTK_INT PublishRealAsRDF(CCTK_POINTER_TO_CONST cctkGH,
                                 CCTK_POINTER data, CCTK_INT level,
                                 CCTK_REAL value, CCTK_STRING key,
                                 CCTK_STRING name) {
  rdfPublishItem *item = CreateNewItem(cctkGH, data, level, name, key, false);
  if (item) {
    std::ostringstream buf;
    buf << value;
    item->scalar.value = buf.str();
    item->scalar.type = "double";
  }

  return (item ? 1 : 0);
}

static CCTK_INT PublishStringAsRDF(CCTK_POINTER_TO_CONST cctkGH,
                                   CCTK_POINTER data, CCTK_INT level,
                                   CCTK_STRING value, CCTK_STRING key,
                                   CCTK_STRING name) {
  rdfPublishItem *item = CreateNewItem(cctkGH, data, level, name, key, false);
  if (item) {
    item->scalar.value = clean(value);
    item->scalar.type = "string";
  }

  return (item ? 1 : 0);
}

static CCTK_INT PublishTableAsRDF(CCTK_POINTER_TO_CONST cctkGH,
                                  CCTK_POINTER data, CCTK_INT level,
                                  CCTK_INT table, CCTK_STRING key,
                                  CCTK_STRING name) {
  rdfPublishItem *item = CreateNewItem(cctkGH, data, level, name, key, true);
  if (not item)
    return 0;

  const int maxkeylen = Util_TableQueryMaxKeyLength(table);
  if (maxkeylen <= 0)
    return (-1);
  char *userkey = new char[maxkeylen + 1];

  int iterator;
  for (iterator = Util_TableItCreate(table);
       Util_TableItQueryIsNonNull(iterator) > 0;
       Util_TableItAdvance(iterator)) {
    CCTK_INT typecode, n_elements = -1;

    Util_TableItQueryKeyValueInfo(iterator, maxkeylen + 1, userkey, &typecode,
                                  &n_elements);
    if (n_elements <= 0) {
      CCTK_VWarn(3, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "No value provided for user key '%s' in table to be "
                 "published with key '%s'",
                 userkey, key);
      continue;
    }
    if (n_elements > 1 and
        (typecode == CCTK_VARIABLE_INT or typecode == CCTK_VARIABLE_REAL)) {
      CCTK_VWarn(3, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Array value not supported for user key '%s' in table to "
                 "be published with key '%s'",
                 userkey, key);
      continue;
    } else if (typecode != CCTK_VARIABLE_CHAR and
               typecode != CCTK_VARIABLE_INT and
               typecode != CCTK_VARIABLE_REAL) {
      CCTK_VWarn(3, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Unsupported datatype '%s' for value with user key '%s' "
                 "in table to be published with key '%s'",
                 CCTK_VarTypeName(typecode), userkey, key);
      continue;
    }

    rdfTableEntry newEntry;
    item->table.push_back(newEntry);
    rdfTableEntry *entry = &item->table.back();
    entry->key = clean(userkey);
    std::ostringstream buf;
    if (typecode == CCTK_VARIABLE_CHAR) {
      char *buffer = new char[n_elements + 1];
      Util_TableGetString(table, n_elements + 1, buffer, userkey);
      entry->value.value = buffer;
      delete[] buffer;
    } else if (typecode == CCTK_VARIABLE_INT) {
      CCTK_INT int_value;
      Util_TableGetInt(table, &int_value, userkey);
      buf << int_value;
      entry->value.value = buf.str();
      entry->value.type = "int";
    } else if (typecode == CCTK_VARIABLE_REAL) {
      CCTK_REAL real_value;
      Util_TableGetReal(table, &real_value, userkey);
      buf << real_value;
      entry->value.value = buf.str();
      entry->value.type = "double";
    } else {
      assert(0);
    }
  }
  Util_TableItDestroy(iterator);
  delete[] userkey;

  return 1;
}

static void ParameterSetNotify(void *, const char *thorn, const char *parameter,
                               const char *new_value) {
  DECLARE_CCTK_PARAMETERS;

  // should parameter changes be logged at all ?
  if (nr_of_parameter_changes_to_be_logged == 0)
    return;

  // reparse the "steered_parameters_log_exclusion_list" if it has changed
  int timesSet = CCTK_ParameterQueryTimesSet(
      "steered_parameters_log_exclusion_list", CCTK_THORNSTRING);
  static int lastTimesSet = -1;
  static std::set<std::string> exclusionList;
  if (lastTimesSet != timesSet) {
    exclusionList.clear();
    std::string parseString(steered_parameters_log_exclusion_list);
    const std::string whitespaces(" \t\n");
    while (1) {
      std::string::size_type start = parseString.find_first_not_of(whitespaces);
      if (start == std::string::npos)
        break;
      std::string::size_type end =
          parseString.find_first_of(whitespaces, start);
      if (end == std::string::npos)
        end = parseString.length();
      std::string parameter(parseString, start, end - start);
      std::transform(parameter.begin(), parameter.end(), parameter.begin(),
                     tolower);
      exclusionList.insert(parameter);
      parseString.erase(0, end);
    }
    lastTimesSet = timesSet;
  }

  const char *name = "Steer parameter";
  std::string key(thorn);
  key.append("::");
  key.append(parameter);

  // // do not log steering events for parameters in the log exclusion list
  std::string lcKey(key);
  std::transform(lcKey.begin(), lcKey.end(), lcKey.begin(), tolower);
  if (exclusionList.find(lcKey) != exclusionList.end())
    return;

  // check if the maximum number of parameter change logs has been reached
  static std::map<std::string, int> logList;
  logList[lcKey]++;
  if (nr_of_parameter_changes_to_be_logged > 0 and
      logList[lcKey] >= nr_of_parameter_changes_to_be_logged)
    return;

  int type;
  const void *data = CCTK_ParameterGet(parameter, thorn, &type);

  if (type == PARAMETER_KEYWORD || type == PARAMETER_STRING ||
      type == PARAMETER_SENTENCE) {
    if (CCTK_IsFunctionAliased("PublishString")) {
      PublishString(NULL, 0, *(const char *const *)data, key.c_str(), name);
    }
  } else if (type == PARAMETER_BOOLEAN) {
    if (CCTK_IsFunctionAliased("PublishBoolean")) {
      PublishBoolean(NULL, 0, *(const CCTK_INT *)data, key.c_str(), name);
    }
  } else if (type == PARAMETER_INT) {
    if (CCTK_IsFunctionAliased("PublishInt")) {
      PublishInt(NULL, 0, *(const CCTK_INT *)data, key.c_str(), name);
    }
  } else if (type == PARAMETER_REAL) {
    if (CCTK_IsFunctionAliased("PublishReal")) {
      PublishReal(NULL, 0, *(const CCTK_REAL *)data, key.c_str(), name);
    }
  } else {
    assert(0 and "invalid parameter type");
  }
}

extern "C" void Formaline_RegisterPublishRDF_Callbacks(CCTK_ARGUMENTS) {
  int registered = 0;

#define REGISTER_RDF_CALLBACKS(type)                                           \
  if (CCTK_IsFunctionAliased("Publish" #type "_Register")) {                   \
    if (Publish##type##_Register(Publish##type##AsRDF, NULL,                   \
                                 "Publish as RDF")) {                          \
      CCTK_WARN(0, "Failed to register Publish" #type " callback");            \
    }                                                                          \
    registered++;                                                              \
  }
  // we don't have a valid cctkGH yet
  if (CCTK_MyProc(NULL) == 0) {
    REGISTER_RDF_CALLBACKS(Boolean);
    REGISTER_RDF_CALLBACKS(Int);
    REGISTER_RDF_CALLBACKS(Real);
    REGISTER_RDF_CALLBACKS(String);
    REGISTER_RDF_CALLBACKS(Table);
    if (registered) {
      if (CCTK_ParameterSetNotifyRegister(ParameterSetNotify, NULL,
                                          CCTK_THORNSTRING, NULL, NULL)) {
        CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "Couldn't register parameter set notify callback");
      }
    } else {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Couldn't register Publish RDF callbacks because no thorn "
                 "provides Publish register API aliases functions");
    }
  }
}

extern "C" void Formaline_UnregisterPublishRDF_Callbacks(CCTK_ARGUMENTS) {
#define UNREGISTER_RDF_CALLBACKS(type)                                         \
  if (CCTK_IsFunctionAliased("Publish" #type "_Unregister")) {                 \
    Publish##type##_Unregister("Publish as RDF");                              \
  }
  if (CCTK_MyProc(cctkGH) == 0) {
    CCTK_ParameterSetNotifyUnregister(CCTK_THORNSTRING);
    UNREGISTER_RDF_CALLBACKS(Boolean);
    UNREGISTER_RDF_CALLBACKS(Int);
    UNREGISTER_RDF_CALLBACKS(Real);
    UNREGISTER_RDF_CALLBACKS(String);
    UNREGISTER_RDF_CALLBACKS(Table);
  }
}

} // namespace Formaline
