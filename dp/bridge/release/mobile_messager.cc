#include "mobile_messager.h"
#include <iostream>
namespace release {
void MobileMessager::PlaySound() { std::cout << "MobileMessager::PlaySound\n"; }
void MobileMessager::DrawShape() { std::cout << "MobileMessager::DrawShape\n"; }
void MobileMessager::WriteText() { std::cout << "MobileMessager::WriteText\n"; }
void MobileMessager::Connect() { std::cout << "MobileMessager::Connect\n"; }
}  // namespace release