#ifndef ABSTRACT_FACTORY_ALPHA_SQL_CONNECTION_H
#define ABSTRACT_FACTORY_ALPHA_SQL_CONNECTION_H

#include <iostream>
#include <string>
namespace alpha {
class SQLConnection {
 public:
  void ConnectionString(std::string str) {
    std::cout << "SQLConnection::ConnectionString\n";
  }
};
}  // namespace alpha

#endif