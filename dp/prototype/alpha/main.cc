#include "binary_splitter_factory.h"
#include "console.h"
#include "picture_splitter_factory.h"
#include "splitter_factory.h"
#include "text_splitter_factory.h"
#include "video_splitter_factory.h"
using namespace alpha;
#define TEST(factory)                     \
  do {                                    \
    SplitterFactory* ptr = new factory(); \
    Console console(ptr);                 \
    console.ButtonClick();                \
    delete ptr;                           \
  } while (0)

int main() {
  TEST(BinarySplitterFactory);
  TEST(TextSplitterFactory);
  TEST(PictureSplitterFactory);
  TEST(VideoSplitterFactory);
}