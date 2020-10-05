#include "crypto_memory_stream.h"
#include <iostream>
using namespace std;

namespace alpha {
char CryptoMemoryStream::Read(int number) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoMemoryStream Read" << endl;
  return MemoryStream::Read(number);
}
void CryptoMemoryStream::Seek(int position) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoMemoryStream Seek to " << position << endl;
  MemoryStream::Seek(position);
  cout << "Extra Encodoing" << endl;
  return;
}
void CryptoMemoryStream::Write(char data) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoMemoryStream Write " << data << endl;
  MemoryStream::Write(data);
  cout << "Extra Encodoing" << endl;
  return;
}
}  // namespace alpha