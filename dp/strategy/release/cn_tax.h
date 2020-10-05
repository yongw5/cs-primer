#ifndef STRATEGY_RELEASE_CNTAX_H
#define STRATEGY_RELEASE_CNTAX_H

#include <iostream>
#include "tax_strategy.h"

namespace release {
class CNTax : public TaxStrategy {
 public:
  double Calculate() override;
};

}  // namespace release
#endif