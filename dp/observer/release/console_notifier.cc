#include "console_notifier.h"
#include <iostream>
namespace release {
void ConsoleNotifier::DoProgress(float value) { std::cout << '.'; }
}  // namespace release