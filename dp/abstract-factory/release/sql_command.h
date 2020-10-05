#ifndef ABSTRACT_FACTORY_RELEASE_SQL_COMMAND_H
#define ABSTRACT_FACTORY_RELEASE_SQL_COMMAND_H

#include <iostream>
#include <string>
#include "db_command_base.h"

namespace release {
class SQLCommand : public DBCommandBase {
 public:
  void CommandText(std::string text) override {
    std::cout << "SQLCommand::CommandText\n";
  }
  void SetConnection(DBConnectionBase* connection) override {
    std::cout << "SQLCommand::SetConnection\n";
    connect_ = connection;
  }
  void SetReaderCreator(ReaderCreator creator) override {
    std::cout << "SQLCommand::SetReaderFactory\n";
    creator_ = creator;
  }
  DBDataReaderBase* ExecuteReader() override {
    std::cout << "SQLCommand::ExecuteReader\n";
    return creator_();
  }
};
}  // namespace release

#endif