#ifndef STRATEGY_RELEASE_SALES_ORDER_H
#define STRATEGY_RELEASE_SALES_ORDER_H

#include "tax_strategy.h"
#include <string>

namespace release {
class SalesOrder {
 public:
  SalesOrder(const std::string& name);
  ~SalesOrder();
  double CalculateTax();

 private:
  TaxStrategy *strategy_;
};
}  // namespace release
#endif
