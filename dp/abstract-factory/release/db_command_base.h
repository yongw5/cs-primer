#ifndef ABSTRACT_FACTORY_RELEASE_DB_COMMAND_BASE_H
#define ABSTRACT_FACTORY_RELEASE_DB_COMMAND_BASE_H

#include <functional>
#include <iostream>
#include <string>
#include "db_data_reader_base.h"

namespace release {
class DBConnectionBase;
class DBDataReaderBase;
using ReaderCreator = std::function<DBDataReaderBase*()>;
class DBCommandBase {
 public:
  virtual void CommandText(std::string text) = 0;
  virtual void SetConnection(DBConnectionBase* connection) = 0;
  virtual void SetReaderCreator(ReaderCreator creator) = 0;
  virtual DBDataReaderBase* ExecuteReader() = 0;
  virtual ~DBCommandBase() = default;

 protected:
  DBConnectionBase* connect_;
  ReaderCreator creator_;
};
}  // namespace release

#endif