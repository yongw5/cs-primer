#include "memento.h"
#include "originator.h"
using namespace release;

int main() {
  Originator originator;
  /// 存储到备忘录
  Memento m = originator.CreateMemento();
  /// 改变 originator 状态
  /// 从备忘录中恢复
  originator.SetMemento(m);
  return 0;
}