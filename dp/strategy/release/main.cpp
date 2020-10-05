#include <iostream>
#include <string>
#include "sales_order.hpp"
int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "Usage " << argv[0] << " name (CN, DE, US)" << std::endl;
    return 0;
  }
  std::string name(argv[1]);
  release::SalesOrder sales(name);
  sales.CalculateTax();
  return 0;
}
