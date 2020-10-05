#include <iostream>
#include "buffer_stream.h"
#include "crypto_stream.h"
#include "file_stream.h"
#include "memory_stream.h"
#include "network_stream.h"
using namespace std;

void Test(beta::Stream& s, const char* name) {
  cout << "Testing " << name << endl;
  s.Read(1);
  s.Seek(2);
  s.Write('3');
  cout << endl;
}

int main() {
  beta::FileStream fs;
  Test(fs, "FileStream");
  beta::NetworkStream ns;
  Test(ns, "NetworkStream");
  beta::MemoryStream ms;
  Test(ms, "MemoryStream");
  beta::CryptoStream cfs(&fs);
  Test(cfs, "CryptoFileStream");
  beta::BufferStream bfs(&fs);
  Test(bfs, "BufferFileStream");
  return 0;
}
