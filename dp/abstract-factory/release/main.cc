#include "employee_dao.h"
#include "sql_factory.h"

int main() {
  release::SQLFactory factory;
  release::EmployeeDAO employee_dao(&factory);
  employee_dao.GetEmployees();
}