#include <iostream>
#include "buffer_stream.h"
#include "crypto_stream.h"
#include "file_stream.h"
#include "memory_stream.h"
#include "network_stream.h"
using namespace std;

void Test(release::Stream& s, const char* name) {
  cout << "Testing " << name << endl;
  s.Read(1);
  s.Seek(2);
  s.Write('3');
  cout << endl;
}

int main() {
  release::FileStream fs;
  Test(fs, "FileStream");
  release::NetworkStream ns;
  Test(ns, "NetworkStream");
  release::MemoryStream ms;
  Test(ms, "MemoryStream");
  release::CryptoStream cfs(&fs);
  Test(cfs, "CryptoFileStream");
  release::BufferStream bfs(&fs);
  Test(bfs, "BufferFileStream");
  return 0;
}
