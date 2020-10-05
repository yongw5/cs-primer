#ifndef STRATEGY_SALES_ALPHA_ORDER_H
#define STRATEGY_SALES_ALPHA_ORDER_H

namespace alpha {
enum TaxBase {
  CN_Tax,
  US_Tax,
  DE_Tax,
  END_Tax
};
class SalesOrder {
 public:
  SalesOrder(TaxBase tax = CN_Tax);
  double CalculateTax();
  ~SalesOrder();

 private:
  TaxBase tax_;
};
}  // namespace alpha
#endif
