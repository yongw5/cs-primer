#include "sales_order.hpp"
#include <assert.h>
#include <iostream>
#include "tax_factory.h"

namespace release {
SalesOrder::SalesOrder(const std::string& name) {
  strategy_ = TaxFactory::CreateTax(name);
  std::cout << "SalesOrder Construction" << std::endl;
  assert(strategy_ != nullptr);
}

SalesOrder::~SalesOrder() {
  delete strategy_;
  std::cout << "SalesOrder Deconstruction" << std::endl;
}

double SalesOrder::CalculateTax() { return strategy_->Calculate(); }
}  // namespace release
