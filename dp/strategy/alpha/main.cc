#include <stdlib.h>
#include <iostream>
#include "sales_order.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "Usage " << argv[0] << " Country" << std::endl;
    return 0;
  } else if (atoi(argv[1]) >= alpha::END_Tax) {
    std::cout << "Invalid Country Number" << std::endl;
  }
  alpha::TaxBase base = static_cast<alpha::TaxBase>(atoi(argv[1]));
  alpha::SalesOrder sales(base);
  sales.CalculateTax();
  return 0;
}
