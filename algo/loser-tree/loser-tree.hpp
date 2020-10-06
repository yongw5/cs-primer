#include <climits>
#include <utility>
#include <vector>
using std::size_t;
using std::vector;

struct ListNode {
  int val;
  ListNode* next;
  ListNode(int x) : val(x), next(nullptr) {}
};

class LoserTree {
 public:
  LoserTree() {}
  ~LoserTree() {}

  ListNode* mergeKLists(vector<ListNode*>& lists);

 private:
  void Adjust(int s);
  void CreateLoserTree();
  void input(vector<ListNode*>& lists, int pos);
  void output(vector<ListNode*>& lists, ListNode*& pre);

 private:
  vector<int> loser_;
  vector<int> leaf_;
};

ListNode* LoserTree::mergeKLists(vector<ListNode*>& lists) {
  if (lists.empty()) {
    return nullptr;
  }
  const size_t kWays = lists.size();
  loser_.resize(kWays);
  leaf_.resize(kWays + 1);
  ListNode dummy(-1), *tail;
  tail = &dummy;

  for (int i = 0; i < kWays; ++i) {
    input(lists, i);
  }
  CreateLoserTree();
  while (leaf_[loser_[0]] != INT_MAX) {
    output(lists, tail);
    input(lists, loser_[0]);
    Adjust(loser_[0]);
  }
  return dummy.next;
}

void LoserTree::Adjust(int s) {
  const size_t kWays = loser_.size();
  for (size_t p = (s + kWays) / 2; p > 0; p /= 2) {
    if (leaf_[s] > leaf_[loser_[p]]) {
      std::swap(s, loser_[p]);
    }
  }
  loser_[0] = s;
}

void LoserTree::CreateLoserTree() {
  const int kWays = loser_.size();
  leaf_[kWays] = INT_MIN;
  for (int i = 0; i < kWays; ++i) {
    loser_[i] = kWays;
  }
  for (int i = kWays - 1; i >= 0; --i) {
    Adjust(i);
  }
}

void LoserTree::input(vector<ListNode*>& lists, int i) {
  if (lists[i]) {
    leaf_[i] = lists[i]->val;
  } else {
    leaf_[i] = INT_MAX;
  }
}

void LoserTree::output(vector<ListNode*>& lists, ListNode*& tail) {
  tail->next = lists[loser_[0]];
  tail = tail->next;
  lists[loser_[0]] = lists[loser_[0]]->next;
}
