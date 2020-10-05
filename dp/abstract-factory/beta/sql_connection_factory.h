#ifndef ABSTRACT_FACTORY_BETA_SQL_CONNECTION_FACTORY_H
#define ABSTRACT_FACTORY_BETA_SQL_CONNECTION_FACTORY_H

#include "db_connection_factory.h"
#include "sql_connection.h"

namespace beta {
class SQLConnectionFactory : public DBConnectionFactory {
 public:
  DBConnectionBase* CreateDBConnnection() override {
    return new SQLConnection();
  }
  ~SQLConnectionFactory() = default;
};
}  // namespace beta
#endif