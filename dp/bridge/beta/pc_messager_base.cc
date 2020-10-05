#include "pc_messager_base.h"
#include <iostream>
namespace beta {
void PCMessagerBase::PlaySound() { std::cout << "PCMessagerBase::PlaySound\n"; }
void PCMessagerBase::DrawShape() { std::cout << "PCMessagerBase::DrawShape\n"; }
void PCMessagerBase::WriteText() { std::cout << "PCMessagerBase::WriteText\n"; }
void PCMessagerBase::Connect() { std::cout << "PCMessagerBase::Connect\n"; }
}  // namespace beta