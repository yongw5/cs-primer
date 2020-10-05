#ifndef ABSTRACT_FACTORY_BETA_DB_COMMAND_FACTORY_H
#define ABSTRACT_FACTORY_BETA_DB_COMMAND_FACTORY_H

namespace beta {
class DBCommandBase;
class DBCommandFactory {
 public:
  virtual DBCommandBase* CreateDBCommand() = 0;
  virtual ~DBCommandFactory() = default;
};
}  // namespace beta
#endif