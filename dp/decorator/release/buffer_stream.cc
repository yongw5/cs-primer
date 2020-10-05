#include "buffer_stream.h"
#include <iostream>
#include <typeinfo>
using namespace std;

namespace release {
char BufferStream::Read(int number) {
  cout << "Extra Encodoing" << endl;
  cout << "Buffer" << typeid(*stream_).name() << " Read" << endl;
  return stream_->Read(number);
}
void BufferStream::Seek(int position) {
  cout << "Extra Encodoing" << endl;
  cout << "Buffer" << typeid(*stream_).name() << " to " << position << endl;
  stream_->Seek(position);
  cout << "Extra Encodoing" << endl;
  return;
}
void BufferStream::Write(char data) {
  cout << "Extra Encodoing" << endl;
  cout << "Buffer" << typeid(*stream_).name() << " Write " << data << endl;
  stream_->Write(data);
  cout << "Extra Encodoing" << endl;
  return;
}
}  // namespace release
