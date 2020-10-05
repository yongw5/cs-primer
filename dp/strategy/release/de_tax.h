#ifndef STRATEGY_RELEASE_DETAX_H
#define STRATEGY_RELEASE_DETAX_H

#include <iostream>
#include "tax_strategy.h"

namespace release {
class DETax : public TaxStrategy {
 public:
  double Calculate() override;
};
}  // namespace release
#endif