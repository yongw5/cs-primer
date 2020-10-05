#include "pc_messager_lite.h"
#include <iostream>

namespace alpha {
void PCMessagerLite::Login(const std::string name, const std::string password) {
  PCMessagerBase::Connect();
  std::cout << "PCMessagerBase::Login\n";
}
void PCMessagerLite::SendMessage(const std::string message) {
  PCMessagerBase::WriteText();
  std::cout << "PCMessagerBase::SendMessage\n";
}
void PCMessagerLite::SendPicture(const std::string img_name) {
  PCMessagerBase::DrawShape();
  std::cout << "PCMessagerBase::SendPicture\n";
}
}  // namespace alpha