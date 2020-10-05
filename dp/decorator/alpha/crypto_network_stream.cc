#include "crypto_network_stream.h"
#include <iostream>
using namespace std;

namespace alpha {
char CryptoNetowrkStream::Read(int number) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoNetworkStream Read" << endl;
  return NetworkStream::Read(number);
}
void CryptoNetowrkStream::Seek(int position) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoNetworkStream Seek to " << position << endl;
  NetworkStream::Seek(position);
  cout << "Extra Encodoing" << endl;
  return;
}
void CryptoNetowrkStream::Write(char data) {
  cout << "Extra Encodoing" << endl;
  cout << "CryptoNetworkStream Write " << data << endl;
  NetworkStream::Write(data);
  cout << "Extra Encodoing" << endl;
  return;
}
}  // namespace alpha
