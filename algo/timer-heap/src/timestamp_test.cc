#include "timestamp.hpp"
#include <stdio.h>
#include <vector>

void PassByReference(const Timestamp& timestamp) {
  printf("pass by reference: %lu\n", timestamp.EpochMicroseconds());
}
void PassByValue(const Timestamp timestamp) {
  printf("pass by value: %lu\n", timestamp.EpochMicroseconds());
}
void Benckmark() {
  const int kTotal = 1000 * 1000;
  std::vector<Timestamp> stamp_vec;
  stamp_vec.reserve(kTotal);
  for (int i = 0; i < kTotal; ++i) {
    stamp_vec.push_back(Timestamp());
  }
  printf("first timestamp: %lu\n", stamp_vec.front().EpochMicroseconds());
  printf("last timestamp: %lu\n", stamp_vec.back().EpochMicroseconds());
  printf("last - first: %ld\n",
         (stamp_vec.back() - stamp_vec.front()).ToMicroseconds());

  std::vector<int> increments(100, 0);
  Timestamp start = stamp_vec.front();
  for (int i = 0; i < kTotal; i += 1000) {
    Timestamp next = stamp_vec[i];
    int64_t inc = (next - start).ToMicroseconds();
    start = next;
    if (inc < 0) {
      printf("reverse!\n");
    } else if (inc < 100) {
      ++increments[inc];
    } else {
      printf("big gap %d\n", static_cast<int>(inc));
    }
  }
  for (int i = 0; i < 100; ++i) {
    printf("%2d: %d\n", i, increments[i]);
  }
}

int main() {
  Timestamp now;
  printf("now: %lu\n", now.EpochMicroseconds());
  PassByReference(now);
  PassByValue(now);
  Benckmark();
  return 0;
}