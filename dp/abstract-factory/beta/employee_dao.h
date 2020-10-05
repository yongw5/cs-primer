#ifndef ABSTRACT_FACTORY_BETA_EMPLOYEE_DAO_H
#define ABSTRACT_FACTORY_BETA_EMPLOYEE_DAO_H

#include <vector>
#include "db_command_base.h"
#include "db_command_factory.h"
#include "db_connection_base.h"
#include "db_connection_factory.h"
#include "db_data_reader_base.h"
#include "db_data_reader_factory.h"
#include "employee_do.h"

namespace beta {
class EmployeeDAO {
 public:
  EmployeeDAO(DBConnectionFactory* connection_factory,
              DBCommandFactory* command_factory,
              DBDataReaderFactory* reader_factory)
      : connection_factory_(connection_factory),
        command_factory_(command_factory),
        reader_factory_(reader_factory) {}

  std::vector<EmployeeDO> GetEmployees() {
    std::cout << "EmployeeDAO::GetEmployees\n";
    DBConnectionBase* connection = connection_factory_->CreateDBConnnection();
    connection->ConnectionString("connection string");
    DBCommandBase* command = command_factory_->CreateDBCommand();
    command->CommandText("connand text");
    command->SetReaderFactory(reader_factory_);
    DBDataReaderBase* reader = command->ExecuteReader();
    reader->Read();

    delete connection;
    delete command;
    delete reader;

    return {};
  }

 private:
  DBConnectionFactory* connection_factory_;
  DBCommandFactory* command_factory_;
  DBDataReaderFactory* reader_factory_;
};
}  // namespace beta
#endif