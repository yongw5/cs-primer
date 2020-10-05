#include "file_stream.h"
#include <iostream>
using namespace std;

namespace alpha {
char FileStream::Read(int number) {
  cout << "FileStream Read" << endl;
  int idx = rand() % 26;
  return 'a' + idx;
}
void FileStream::Seek(int position) {
  cout << "FileStream Seek to " << position << endl;
  return;
}
void FileStream::Write(char data) {
  cout << "FileStream Write " << data << endl;
  return;
}
}  // namespace alpha
