#include "employee_dao.h"
#include "sql_command_factory.h"
#include "sql_connection_factory.h"
#include "sql_data_reader_factory.h"

int main() {
  beta::SQLCommandFactory command_factory;
  beta::SQLConnectionFactory conn_factory;
  beta::SQLDataReaderFactory reader_factory;
  beta::EmployeeDAO employee_dao(&conn_factory, &command_factory,
                                 &reader_factory);
  employee_dao.GetEmployees();
}