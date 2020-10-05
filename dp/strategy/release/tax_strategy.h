#ifndef STRATEGY_RELEASE_TAX_STRATEGY_H
#define STRATEGY_RELEASE_TAX_STRATEGY_H
#include "tax_factory.h"
namespace release {
class TaxStrategy {
 public:
  virtual double Calculate() = 0;
  virtual ~TaxStrategy() = default;
};
}  // namespace release
#endif