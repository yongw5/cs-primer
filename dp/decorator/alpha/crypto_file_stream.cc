#include "crypto_file_stream.h"
#include <iostream>
using namespace std;

namespace alpha {
char CryptoFileStream::Read(int number) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoFileStream Read" << endl;
  return FileStream::Read(number);
}
void CryptoFileStream::Seek(int position) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoFileStream Seek to " << position << endl;
  FileStream::Seek(position);
  cout << "Extra Encodoing" << endl;
  return;
}
void CryptoFileStream::Write(char data) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoFileStream Write " << data << endl;
  FileStream::Write(data);
  cout << "Extra Encodoing" << endl;
  return;
}
}