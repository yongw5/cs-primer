#ifndef ABSTRACT_FACTORY_RELEASE_DB_CONNECTION_BASE_H
#define ABSTRACT_FACTORY_RELEASE_DB_CONNECTION_BASE_H

#include <iostream>
#include <string>
namespace release {
class DBConnectionBase {
 public:
  virtual void ConnectionString(std::string str) = 0;
  virtual ~DBConnectionBase() = default;
};
}  // namespace release

#endif