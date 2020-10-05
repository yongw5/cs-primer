#ifndef STRATEGY_RELEASE_TAX_FACTIRY_H
#define STRATEGY_RELEASE_TAX_FACTIRY_H

#include <string>
#include <unordered_map>
namespace release {
class TaxStrategy;
class TaxFactory {
 public:
  typedef TaxStrategy* (*Creator)();
  typedef std::unordered_map<std::string, Creator> CreatorMap;
  TaxFactory(std::string name, Creator creator) { Register(name, creator); }
  static TaxStrategy* CreateTax(const std::string& name) {
    if (get_map().count(name)) {
      return get_map()[name]();
    } else {
      return nullptr;
    }
  }

 private:
  void Register(const std::string& name, Creator creator) {
    get_map()[name] = creator;
  }
  static CreatorMap& get_map() {
    static CreatorMap creator_map;
    return creator_map;
  }
};

#define REGISTER_TAX_CLASS(type)                                 \
  namespace {                                                    \
  TaxStrategy* Creator_##type##Tax() { return new type##Tax(); } \
  static TaxFactory g_register(#type, Creator_##type##Tax);                 \
  }
}  // namespace release

#endif