#include "console.h"
#include "progress_bar.h"

int main() {
  gamma::ProgressBar bar;
  gamma::Console console(&bar);
  console.ButtonClick();
  return 0;
}
