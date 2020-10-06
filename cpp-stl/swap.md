## std::swap() <utility>
通过移动类交换两个变量。它要求传入的变量是可以移动的（具有移动构造函数和重载移动赋值运算符）。宏定义 _GLIBCXX_MOVE 在 C++11 展开为 std::move()。
```
/// @file include/bits/move.h
174   template<typename _Tp>
175     inline void
176     swap(_Tp& __a, _Tp& __b)
177 #if __cplusplus >= 201103L
178     noexcept(__and_<is_nothrow_move_constructible<_Tp>,
179                 is_nothrow_move_assignable<_Tp>>::value)
180 #endif
181     {
182       // concept requirements
183       __glibcxx_function_requires(_SGIAssignableConcept<_Tp>)
184 
185       _Tp __tmp = _GLIBCXX_MOVE(__a); // 调用移动构造函数构建一个临时对象
186       __a = _GLIBCXX_MOVE(__b); // 移动赋值
187       __b = _GLIBCXX_MOVE(__tmp); // 移动赋值
188     }
189 
190   // _GLIBCXX_RESOLVE_LIB_DEFECTS
191   // DR 809. std::swap should be overloaded for array types.
192   /// Swap the contents of two arrays.
193   template<typename _Tp, size_t _Nm>
194     inline void
195     swap(_Tp (&__a)[_Nm], _Tp (&__b)[_Nm])
196 #if __cplusplus >= 201103L
197     noexcept(noexcept(swap(*__a, *__b)))
198 #endif
199     {
200       for (size_t __n = 0; __n < _Nm; ++__n)
201     swap(__a[__n], __b[__n]);
202     }
```
在移动的过程中，不会有资源的申请和释放。看下面的例子：
```
#define ADD_MOVE
class Test {
 public:
  explicit Test(size_t l): len_(l), data_(new unsigned char[l]) {
    printf("Test Ctor\n");
  }
  Test(const Test& other): len_(other.len_), data_(new unsigned char[len_]) {
    printf("Test Copy Ctor\n");
    memmove(data_, other.data_, len_);
  }
  Test& operator=(const Test& rhs) {
    printf("Test operator=\n");
    if(this != &rhs) {
	  len_ = rhs.len_;
	  data_ = new unsigned char[len_];
	  memmove(data_, rhs.data_, len_);
	  return *this;
    }
  }
#ifdef ADD_MOVE
  Test(Test&& other): len_(len_), data_(other.data_) {
    printf("Test Move Ctor\n");
    other.len_ = 0;
    other.data_ = nullptr;
  }
  Test&operator=(Test&& rhs) {
    printf("Test Move operator=\n");
    if(this != &rhs) {
      len_ = rhs.len_;
      data_ = rhs.data_;
      rhs.len_ = 0;
      rhs.data_ = nullptr;
    }
  }
#endif
  ~Test() {
    printf("Test Dtor\n");
    delete[] data_;
  }

 private:
  size_t len_;
  unsigned char* data_;
};
```
我们重载 operator new() 和 operator delete() 输出分配或释放内存操作
```
void* operator new(size_t size) {
  printf("allocate memory\n");
  return malloc(size);
}

void operator delete(void* ptr) {
  printf("free memory\n");
  free(ptr);
}
```
进行交换测试
```
int main() {
  Test a(1), b(1);
  printf("===========Swap==========\n");
  std::swap(a, b);
  printf("===========Done==========\n");
  return 0;
}
```
在对象“无法”移送的时候（没有显式移动构造函数和移动赋值运算符），交换需要拷贝资源
```
+Allocate Memory
Test Ctor
+Allocate Memory
Test Ctor
===========Swap==========
+Allocate Memory
Test Copy Ctor
Test operator=
+Allocate Memory
Test operator=
+Allocate Memory
Test Dtor
-Free Memory
===========Done==========
Test Dtor
-Free Memory
Test Dtor
-Free Memory
```
当对象可以移动时，交换没有资源的拷贝
```
+Allocate Memory
Test Ctor
+Allocate Memory
Test Ctor
===========Swap==========
Test Move Ctor
Test Move operator=
Test Move operator=
Test Dtor
===========Done==========
Test Dtor
-Free Memory
Test Dtor
-Free Memory
```