#include <iostream>
#include "crypto_file_stream.h"
#include "crypto_memory_stream.h"
#include "crypto_network_stream.h"
#include "file_stream.h"
#include "memory_stream.h"
#include "network_stream.h"
using namespace std;

void Test(alpha::Stream& s, const char* name) {
  cout << "Testing " << name << endl;
  cout << s.Read(1) << endl;;
  s.Seek(2);
  s.Write('3');
  cout << endl;
}

int main() {
  alpha::FileStream fs;
  Test(fs, "FileStream");
  alpha::NetworkStream ns;
  Test(ns, "NetworkStream");
  alpha::MemoryStream ms;
  Test(ms, "MemoryStream");
  alpha::CryptoFileStream cfs;
  Test(cfs, "CryptoFileStream");
  return 0;
}
