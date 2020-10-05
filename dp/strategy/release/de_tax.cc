#include "de_tax.h"

namespace release {
double DETax::Calculate() { std::cout << "Calculate DE Tax" << std::endl; }
REGISTER_TAX_CLASS(DE);
}  // namespace release