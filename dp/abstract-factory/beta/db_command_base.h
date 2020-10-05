#ifndef ABSTRACT_FACTORY_BETA_DB_COMMAND_BASE_H
#define ABSTRACT_FACTORY_BETA_DB_COMMAND_BASE_H

#include <iostream>
#include <string>
#include "db_data_reader_base.h"

namespace beta {
class DBConnectionBase;
class DBDataReaderFactory;
class DBCommandBase {
 public:
  virtual void CommandText(std::string text) = 0;
  virtual void SetConnection(DBConnectionBase* connection) = 0;
  virtual void SetReaderFactory(DBDataReaderFactory* factory) = 0;
  virtual DBDataReaderBase* ExecuteReader() = 0;
  virtual ~DBCommandBase() = default;

 protected:
  DBConnectionBase* connect_;
  DBDataReaderFactory* reader_factory_;
};
}  // namespace beta

#endif