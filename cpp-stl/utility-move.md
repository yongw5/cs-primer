## std::move() <utility>
move() 获得一个右值引用
```
/// @file include/bits/move.h
 99   template<typename _Tp>
100     constexpr typename std::remove_reference<_Tp>::type&&
101     move(_Tp&& __t) noexcept
102     { return static_cast<typename std::remove_reference<_Tp>::type&&>(__t); }
```
move() 的函数参数 _Tp&& 是一个指向模板类型参数的右值引用。通过引用折叠，此参数可以与任何类型的实参匹配。特别是，我们既可以传递给 move() 一个左值，也可以传递给它一个右值。

首先看以下 std::remove_reference 的定义
```
/// @file include/std/type_traits
1574   template<typename _Tp>
1575     struct remove_reference
1576     { typedef _Tp   type; };
1577 
1578   template<typename _Tp>
1579     struct remove_reference<_Tp&>
1580     { typedef _Tp   type; };
1581 
1582   template<typename _Tp>
1583     struct remove_reference<_Tp&&>
1584     { typedef _Tp   type; };
```
std::remove_reference 是去除引用：如果类型 _Tp 是引用类型（_Tp& 或 _Tp&&），则提供成员 typedef 类型，该类型是由 _Tp 引用的类型。否则类型为 _Tp。

通常我们不能将一个右值引用绑定到一个左值上，但是C++在正常绑定规则之外定义了两个例外规则，允许这种绑定：
- 当我们将一个左值传递给函数的右值引用参数，且此右值引用指向模板类型参数时，编译器推断模板类型参数为实参的左值引用类型。
- 如果我们创建一个引用的引用，则这些引用形成了“折叠”（只能应用于间接创建的引用的引用，如类型别名或模板参数）
  - X& &、X& &&、X&& & 都会折叠成 X&
  - X&& && 折叠成 X&&

这个规则导致了两个重要的结果：
- 如果一个函数参数是一个指向模板类型参数的右值引用，则它可以被绑定到一个左值；且
- 如果实参是一个左值，则推断出的模板实参类型将是一个左值引用，且函数参数将被实例化为一个普通的左值引用参数（T&）

因此 std::move() 的工作如下：
1. 传入右值，如 auto s2 = std::move(string("bye!"));
   - 推断出的 _Tp 的类型为 string
   - 因此，remove_reference 用 string 进行实例化
   - remove_reference\<string> 的 type 成员是 string
   - move() 的返回类型是 string&&
   - move() 的函数参数 __t 的类型为 string&&
2. 传入左值，如 auto s1 = string("bye!"); auto s2 = std::move(s1);
   - 推断出的 _Tp 的类型为 string&
   - 因此，remove_reference 用 string& 进行实例化
   - remove_reference\<string> 的 type 成员是 string
   - move() 的返回类型是 string&
   - move() 的函数参数 __t 的类型为 string& &&，会折叠为 string&