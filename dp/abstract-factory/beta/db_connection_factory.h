#ifndef ABSTRACT_FACTORY_BETA_DB_CONNECTION_FACTORY_H
#define ABSTRACT_FACTORY_BETA_DB_CONNECTION_FACTORY_H

namespace beta {
class DBConnectionBase;
class DBConnectionFactory {
 public:
  virtual DBConnectionBase* CreateDBConnnection() = 0;
  virtual ~DBConnectionFactory() = default;
};
}  // namespace beta
#endif