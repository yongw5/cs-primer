#ifndef ABSTRACT_FACTORY_RELEASE_EMPLOYEE_DAO_H
#define ABSTRACT_FACTORY_RELEASE_EMPLOYEE_DAO_H

#include <vector>
#include "db_command_base.h"
#include "db_connection_base.h"
#include "db_data_reader_base.h"
#include "db_factory.h"
#include "employee_do.h"

namespace release {
class EmployeeDAO {
 public:
  EmployeeDAO(DBFactory* factory) : factory_(factory) {}

  std::vector<EmployeeDO> GetEmployees() {
    std::cout << "EmployeeDAO::GetEmployees\n";
    DBConnectionBase* connection = factory_->CreateDBConnnection();
    connection->ConnectionString("connection string");
    DBCommandBase* command = factory_->CreateDBCommand();
    command->CommandText("connand text");
    command->SetReaderCreator(
        std::bind(&DBFactory::CreateDBDataReader, factory_));
    DBDataReaderBase* reader = command->ExecuteReader();
    reader->Read();

    delete connection;
    delete command;
    delete reader;

    return {};
  }

 private:
  DBFactory* factory_;
};
}  // namespace release
#endif