#include "us_tax.h"
namespace release {
double USTax::Calculate() { std::cout << "Calculate US Tax" << std::endl; }
REGISTER_TAX_CLASS(US);
}  // namespace release