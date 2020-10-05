#ifndef ABSTRACT_FACTORY_BETA_DB_FACTORY_H
#define ABSTRACT_FACTORY_BETA_DB_FACTORY_H

namespace release {
class DBCommandBase;
class DBConnectionBase;
class DBDataReaderBase;
class DBFactory {
 public:
  virtual DBCommandBase* CreateDBCommand() = 0;
  virtual DBConnectionBase* CreateDBConnnection() = 0;
  virtual DBDataReaderBase* CreateDBDataReader() = 0;
  virtual ~DBFactory() = default;
};
}  // namespace release
#endif