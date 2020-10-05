#include "cn_tax.h"

namespace release {
double CNTax::Calculate() { std::cout << "Calculate CN Tax" << std::endl; }
REGISTER_TAX_CLASS(CN);
}  // namespace release