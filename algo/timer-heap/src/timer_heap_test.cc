#include "timer_heap.hpp"
#include <assert.h>
#include <stdio.h>
#include <climits>
#include <random>
using std::default_random_engine;

void AddTest() {
  const int kTotal = 1000;
  Timespan interval(0);
  TimerHeap::TimerID id;
  TimerHeap timer_heap;
  TimerHeap::Callback cb;
  default_random_engine e;
  for (int i = 0; i < kTotal; ++i) {
    Timestamp deadline(static_cast<uint64_t>(e()));
    timer_heap.AddTimer(cb, deadline, interval, id);
  }
  auto all =
      timer_heap.GetExpiredTimers(Timestamp(static_cast<uint64_t>(INT_MAX)));
  assert(all.size() == kTotal);
  TimerHeap::TimerObjSptr pre = all.front();
  for (int i = 1; i < all.size(); ++i) {
    assert(pre->deadline() <= all[i]->deadline());
    pre = all[i];
  }
}

void CancelTest() {
  const int kTotal = 1000;
  Timespan interval(0);
  TimerHeap::TimerID tmp, id;
  TimerHeap timer_heap;
  TimerHeap::Callback cb;
  default_random_engine e;
  int target = e() % kTotal;
  for (int i = 0; i < kTotal; ++i) {
    Timestamp deadline(static_cast<uint64_t>(e()));
    timer_heap.AddTimer(cb, deadline, interval, tmp);
    if (i == target) {
      id = tmp;
    }
  }

  timer_heap.CancelTimer(id);
  auto all =
      timer_heap.GetExpiredTimers(Timestamp(static_cast<uint64_t>(INT_MAX)));
  for (auto& p : all) {
    assert(p->id() != id.id());
  }
}

int main() {
  AddTest();
  CancelTest();
  return 0;
}
