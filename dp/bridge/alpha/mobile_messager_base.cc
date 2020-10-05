#include "mobile_messager_base.h"
#include <iostream>
namespace alpha {
void MobileMessagerBase::PlaySound() {
  std::cout << "MobileMessagerBase::PlaySound\n";
}
void MobileMessagerBase::DrawShape() {
  std::cout << "MobileMessagerBase::DrawShape\n";
}
void MobileMessagerBase::WriteText() {
  std::cout << "MobileMessagerBase::WriteText\n";
}
void MobileMessagerBase::Connect() {
  std::cout << "MobileMessagerBase::Connect\n";
}
}  // namespace alpha