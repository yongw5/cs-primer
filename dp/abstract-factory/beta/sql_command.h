#ifndef ABSTRACT_FACTORY_BETA_SQL_COMMAND_H
#define ABSTRACT_FACTORY_BETA_SQL_COMMAND_H

#include <iostream>
#include <string>
#include "db_command_base.h"
#include "sql_data_reader_factory.h"
namespace beta {
class SQLCommand : public DBCommandBase {
 public:
  void CommandText(std::string text) override {
    std::cout << "SQLCommand::CommandText\n";
  }
  void SetConnection(DBConnectionBase* connection) override {
    std::cout << "SQLCommand::SetConnection\n";
    connect_ = connection;
  }
  void SetReaderFactory(DBDataReaderFactory* factory) override {
    std::cout << "SQLCommand::SetReaderFactory\n";
    reader_factory_ = factory;
  }
  DBDataReaderBase* ExecuteReader() override {
    std::cout << "SQLCommand::ExecuteReader\n";
    reader_factory_->CreateDBDataReader();
  }
};
}  // namespace beta

#endif