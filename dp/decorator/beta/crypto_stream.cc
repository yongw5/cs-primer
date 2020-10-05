#include "crypto_stream.h"
#include <iostream>
#include <typeinfo>
using namespace std;

namespace beta {
char CryptoStream::Read(int number) {
  cout << "Extra Encodoing" << endl;
  cout << "Crypto" << typeid(*stream_).name() << " Read" << endl;
  return stream_->Read(number);
}
void CryptoStream::Seek(int position) {
  cout << "Extra Encodoing" << endl;
  cout << "Crypto" << typeid(*stream_).name() << " to " << position << endl;
  stream_->Seek(position);
  cout << "Extra Encodoing" << endl;
  return;
}
void CryptoStream::Write(char data) {
  cout << "Extra Encodoing" << endl;
  cout << "Crypto" << typeid(*stream_).name() << " Write " << data << endl;
  stream_->Write(data);
  cout << "Extra Encodoing" << endl;
  return;
}
}  // namespace beta
