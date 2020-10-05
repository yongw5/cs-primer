#include "console.h"
#include "progress_bar.h"

int main() {
  release::ProgressBar bar;
  release::Console console(&bar);
  console.ButtonClick();
  return 0;
}
