#ifndef ABSTRACT_FACTORY_BETA_SQL_COMMAND_FACTORY_H
#define ABSTRACT_FACTORY_BETA_SQL_COMMAND_FACTORY_H
#include "db_command_factory.h"
#include "sql_command.h"
namespace beta {
class DBCommandBase;
class SQLCommandFactory : public DBCommandFactory {
 public:
  DBCommandBase* CreateDBCommand() override { return new SQLCommand(); }
  ~SQLCommandFactory() = default;
};
}  // namespace beta
#endif