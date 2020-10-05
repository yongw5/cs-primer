#include "console.h"
#include "progress_bar.h"

int main() {
  beta::ProgressBar bar;
  beta::Console console(&bar);
  console.ButtonClick();
  return 0;
}
