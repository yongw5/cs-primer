#pragma once

#include <functional>
#include <limits>
#include <memory>
#include <unordered_set>
#include <vector>
#include "copyalbe_tag.hpp"
#include "timespan.hpp"
#include "timestamp.hpp"

class TimerHeap: private NotCopyable {
 public:
  class TimerObj;
  typedef std::function<void()> Callback;
  typedef std::shared_ptr<TimerObj> TimerObjSptr;
  typedef std::weak_ptr<TimerObj> TimerObjWptr;

  class TimerID : private Copyable {
   public:
    TimerID() : timer_(), id_(0) {}
    TimerID(TimerObjWptr timer, uint64_t id) : timer_(timer), id_(id) {}
    TimerObjWptr timer() const { return timer_; }
    uint64_t id() const { return id_; }

   private:
    static uint64_t UniqueID();
    friend class TimerHeap;

    TimerObjWptr timer_;
    uint64_t id_;
  };

  class TimerObj : private NotCopyable {
   public:
    TimerObj(Callback cb, Timestamp deadline, Timespan interval,
             std::size_t index = std::numeric_limits<std::size_t>::max());
    ~TimerObj();
    void Start() const { callback_(); }
    void Restart(Timestamp now);
    Timestamp deadline() const { return deadline_; }
    uint64_t id() const { return id_; }
    bool periodic() const { return periodic_; }

   private:
    friend class TimerHeap;
    Callback callback_;
    Timestamp deadline_;
    Timespan interval_;
    uint64_t id_;
    bool periodic_;
    std::size_t heap_index_;
  };

 public:
  TimerHeap();
  ~TimerHeap();
  bool AddTimer(Callback cb, Timestamp deadline, Timespan interval,
                TimerID& id);
  void CancelTimer(TimerID& id);
  std::vector<TimerObjSptr> GetExpiredTimers(Timestamp now);
  Timestamp GetNextExpire() const;
  bool empty() const { return heap_.empty(); }

 private:
  void UpHeap(std::size_t index);
  void DownHeap(std::size_t index);
  void SwapHeap(std::size_t index1, std::size_t index2);
  void RemoveTimer(const TimerObjWptr& timer);

 private:
  std::vector<TimerObjSptr> heap_;
  std::unordered_set<uint64_t> active_timers_;
};