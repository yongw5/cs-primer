#ifndef ABSTRACT_FACTORY_ALPHA_SQL_COMMAND_H
#define ABSTRACT_FACTORY_ALPHA_SQL_COMMAND_H

#include <iostream>
#include <string>
#include "sql_data_reader.h"

namespace alpha {
class SQLConnection;
class SQLCommand {
 public:
  void CommandText(std::string text) {
    std::cout << "SQLCommand::CommandText\n";
  }
  void SetConnection(SQLConnection* connection) {
    std::cout << "SQLCommand::SetConnection\n";
  }
  SQLDataReader* ExecuteReader() { return new SQLDataReader(); }
};
}  // namespace alpha

#endif