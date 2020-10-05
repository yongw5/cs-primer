#include <string>
#include "binary_splitter.h"
#include "console.h"
#include "picture_splitter.h"
#include "text_splitter.h"
#include "video_splitter.h"
using namespace release;
std::string get_text(const char* ptr);

#define TEST(classname)                                                      \
  do {                                                                       \
    std::string file_path = get_text("input file path to split: ");          \
    int file_num = atoi(get_text("input numbers of small files: ").c_str()); \
    SplitterBase* ptr = new classname(file_path, file_num);                  \
    Console console(ptr);                                                    \
    console.ButtonClick();                                                   \
    delete ptr;                                                              \
  } while (0)

int main() {
  TEST(BinarySplitter);
  TEST(TextSplitter);
  TEST(PictureSplitter);
  TEST(VideoSplitter);
}

std::string get_text(const char* ptr) {
  if (ptr) std::cout << ptr;
  std::string tmp;
  std::cin >> tmp;
  return tmp;
}