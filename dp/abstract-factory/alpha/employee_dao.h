#ifndef ABSTRACT_FACTORY_ALPHA_EMPLOYEE_DAO_H
#define ABSTRACT_FACTORY_ALPHA_EMPLOYEE_DAO_H

#include <vector>
#include "employee_do.h"
#include "sql_command.h"
#include "sql_connection.h"
#include "sql_data_reader.h"

namespace alpha {
class EmployeeDAO {
 public:
  std::vector<EmployeeDO> GetEmployees() {
    std::cout << "EmployeeDAO::GetEmployees\n";
    SQLConnection* connection = new SQLConnection();
    connection->ConnectionString("connection string");
    SQLCommand* command = new SQLCommand();
    command->CommandText("connand text");
    command->SetConnection(connection);
    SQLDataReader* reader = command->ExecuteReader();
    reader->Read();

    delete connection;
    delete command;
    delete reader;

    return {};
  }
};
}  // namespace alpha
#endif