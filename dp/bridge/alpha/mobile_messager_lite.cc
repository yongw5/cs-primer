#include "mobile_messager_lite.h"
#include <iostream>

namespace alpha {
void MobileMessagerLite::Login(const std::string name,
                               const std::string password) {
  MobileMessagerBase::Connect();
  std::cout << "MobileMessagerLite::Login\n";
}
void MobileMessagerLite::SendMessage(const std::string message) {
  MobileMessagerBase::WriteText();
  std::cout << "MobileMessagerLite::SendMessage\n";
}
void MobileMessagerLite::SendPicture(const std::string img_name) {
  MobileMessagerBase::DrawShape();
  std::cout << "MobileMessagerLite::SendPicture\n";
}
}  // namespace alpha