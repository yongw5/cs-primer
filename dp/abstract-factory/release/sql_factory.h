#ifndef ABSTRACT_FACTORY_RELEASE_SQL_FACTORY_H
#define ABSTRACT_FACTORY_RELEASE_SQL_FACTORY_H
#include "db_factory.h"
#include "sql_command.h"
#include "sql_connection.h"
#include "sql_data_reader.h"

namespace release {
class DBCommandBase;
class SQLFactory : public DBFactory {
 public:
  DBCommandBase* CreateDBCommand() override { return new SQLCommand(); }
  DBConnectionBase* CreateDBConnnection() override {
    return new SQLConnection();
  }
  DBDataReaderBase* CreateDBDataReader() override {
    return new SQLDataReader();
  }
};
}  // namespace release
#endif