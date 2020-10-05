#ifndef ABSTRACT_FACTORY_BETA_DB_CONNECTION_BASE_H
#define ABSTRACT_FACTORY_BETA_DB_CONNECTION_BASE_H

#include <iostream>
#include <string>
namespace beta {
class DBConnectionBase {
 public:
  virtual void ConnectionString(std::string str) = 0;
  virtual ~DBConnectionBase() = default;
};
}  // namespace beta

#endif