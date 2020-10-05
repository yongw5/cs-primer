#ifndef STRATEGY_RELEASE_USTAX_H
#define STRATEGY_RELEASE_USTAX_H

#include <iostream>
#include "tax_strategy.h"

namespace release {
class USTax : public TaxStrategy {
 public:
  double Calculate() override;
};

}  // namespace release
#endif