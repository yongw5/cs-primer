#include "sales_order.h"
#include <iostream>

namespace alpha {
SalesOrder::SalesOrder(TaxBase tax) : tax_(tax) {
  std::cout << "SalesOrder Construction" << std::endl;
}

SalesOrder::~SalesOrder() {
  std::cout << "SalesOrder Deconstruction" << std::endl;
}

double SalesOrder::CalculateTax() {
  double ans = 0;
  switch (tax_) { // 算法选择
    case CN_Tax:
      std::cout << "Calculate CN Tax" << std::endl;
      break;
    case US_Tax:
      std::cout << "Calculate US Tax" << std::endl;
      break;
    case DE_Tax:
      std::cout << "Calculate DE Tax" << std::endl;
  }
  return ans;
}
}  // namespace alpha