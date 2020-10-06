#include "timer_heap.hpp"
#include <atomic>

uint64_t TimerHeap::TimerID::UniqueID() {
  static std::atomic<uint64_t> counter(0);  // c++11 ensure init only once
  return ++counter;
}
TimerHeap::TimerObj::TimerObj(Callback cb, Timestamp deadline,
                              Timespan interval, std::size_t index)
    : callback_(std::move(cb)),
      deadline_(deadline),
      interval_(interval),
      id_(TimerID::UniqueID()),
      heap_index_(index),
      periodic_(interval > Timespan(0)) {}
TimerHeap::TimerObj::~TimerObj() {}
void TimerHeap::TimerObj::Restart(Timestamp now) {
  if (periodic_) {
    deadline_ = now.AddTimespan(interval_);
  } else {
    deadline_ = Timestamp(0);
  }
}
TimerHeap::TimerHeap() : heap_(), active_timers_() {}
TimerHeap::~TimerHeap() {}
bool TimerHeap::AddTimer(Callback cb, Timestamp deadline, Timespan interval,
                         TimerID& id) {
  bool ret = false;
  if (!heap_.empty() && deadline < heap_.front()->deadline()) {
    ret = true;  // added timer is first to expire
  }
  std::size_t index = heap_.size();
  heap_.emplace_back(new TimerObj(std::move(cb), deadline, interval, index));
  id = TimerID(heap_.back(), heap_.back()->id_);
  active_timers_.insert(id.id_);
  UpHeap(heap_.size() - 1);
  return ret;
}
void TimerHeap::CancelTimer(TimerHeap::TimerID& id) {
  if (!heap_.empty() && active_timers_.find(id.id_) != active_timers_.end()) {
    RemoveTimer(id.timer_);
    id.id_ = 0;
    id.timer_.reset();
  }
}
std::vector<TimerHeap::TimerObjSptr> TimerHeap::GetExpiredTimers(
    Timestamp now) {
  std::vector<TimerObjSptr> expired;
  while (!heap_.empty() && now >= heap_.front()->deadline_) {
    expired.emplace_back(heap_.front());
    RemoveTimer(heap_.front());
  }
  return expired;
}
Timestamp TimerHeap::GetNextExpire() const {
  if (!heap_.empty()) {
    return Timestamp(0);
  }
  return heap_.front()->deadline_;
}
void TimerHeap::UpHeap(std::size_t index) {
  while (index > 0) {
    std::size_t parent = (index - 1) / 2;
    if (heap_[index]->deadline_ >= heap_[parent]->deadline_) {
      break;
    }
    SwapHeap(index, parent);
    index = parent;
  }
}
void TimerHeap::DownHeap(std::size_t index) {
  std::size_t child = index * 2 + 1;
  while (child < heap_.size()) {
    std::size_t min_child =
        (child + 1 == heap_.size() ||
         heap_[child]->deadline_ < heap_[child + 1]->deadline_)
            ? child
            : child + 1;
    if (heap_[index]->deadline_ < heap_[min_child]->deadline_) {
      break;
    }
    SwapHeap(index, min_child);
    index = min_child;
    child = index * 2 + 1;
  }
}
void TimerHeap::SwapHeap(std::size_t index1, std::size_t index2) {
  std::swap(heap_[index1], heap_[index2]);
  heap_[index1]->heap_index_ = index1;
  heap_[index2]->heap_index_ = index2;
}
void TimerHeap::RemoveTimer(const TimerObjWptr& wptimer) {
  if (auto sptimer = wptimer.lock()) {
    active_timers_.erase(sptimer->id_);
    std::size_t index = sptimer->heap_index_;
    if (index == heap_.size() - 1) {
      heap_.pop_back();
    } else {
      SwapHeap(index, heap_.size() - 1);
      heap_.pop_back();
      std::size_t parent = (index - 1) / 2;
      if (index > 0 && heap_[index]->deadline_ < heap_[parent]->deadline_) {
        UpHeap(index);
      } else {
        DownHeap(index);
      }
    }
  }
}