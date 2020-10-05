#ifndef ABSTRACT_FACTORY_BETA_SQL_CONNECTION_H
#define ABSTRACT_FACTORY_BETA_SQL_CONNECTION_H

#include <iostream>
#include <string>
#include "db_connection_base.h"
namespace beta {
class SQLConnection: public DBConnectionBase {
 public:
  void ConnectionString(std::string str) override {
    std::cout << "SQLConnection::ConnectionString\n";
  }
};
}  // namespace beta

#endif