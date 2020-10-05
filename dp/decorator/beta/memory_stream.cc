#include "memory_stream.h"
#include <iostream>
using namespace std;

namespace beta {
char MemoryStream::Read(int number) {
  cout << "MemoryStream Read" << endl;
  int idx = rand() % 26;
  return 'a' + idx;
}
void MemoryStream::Seek(int position) {
  cout << "MemoryStream Seek to " << position << endl;
  return;
}
void MemoryStream::Write(char data) {
  cout << "MemoryStream Write " << data << endl;
  return;
}
}
