#pragma once

#include <any>
#include <memory>
#include <typeindex>
#include <vector>

namespace CppInject {

template <class TService>
using ServiceVector = std::vector<std::reference_wrapper<TService>>;

class IServiceProvider {
 public:
  virtual ~IServiceProvider() = default;

  virtual std::any getService(std::type_index type) = 0;
  virtual std::vector<std::any> getServices(std::type_index type) = 0;

  template <typename TService>
  TService* getService() {
    auto res = getService(std::type_index(typeid(TService)));
    if (res.has_value())
      return &std::any_cast<std::reference_wrapper<TService>>(res).get();
    return nullptr;
  }

  template <typename TService>
  TService& getRequiredService() {
    auto res = getService<TService>();
    if (res != nullptr) return *res;
    throw std::logic_error(
        typeid(TService).name() +
        std::string{
            " has not been registered as a singleton or scoped service."});
  }

  template <typename TService>
  std::shared_ptr<TService> getTransientService() {
    auto res = getService(std::type_index(typeid(std::shared_ptr<TService>)));
    if (res.has_value())
      return std::any_cast<std::shared_ptr<TService>>(std::move(res));
    return nullptr;
  }

  template <typename TService>
  std::shared_ptr<TService> getRequiredTransientService() {
    auto res = getTransientService<TService>();
    if (res != nullptr) return res;
    throw std::logic_error(
        typeid(TService).name() +
        std::string{" has not been registered as a transient service."});
  }

  template <typename TService>
  ServiceVector<TService> getServices() {
    ServiceVector<TService> res;
    auto servicesAny = getServices(std::type_index(typeid(TService)));
    for (auto& service : servicesAny)
      res.emplace_back(
          std::any_cast<std::reference_wrapper<TService>>(service));
    return res;
  }

  template <typename TService>
  std::vector<std::shared_ptr<TService>> getTransientServices() {
    std::vector<std::shared_ptr<TService>> res;
    auto servicesAny =
        getServices(std::type_index(typeid(std::shared_ptr<TService>)));
    for (auto&& service : std::move(servicesAny))
      res.emplace_back(
          std::any_cast<std::shared_ptr<TService>&&>(std::move(service)));
    return res;
  }
};

class IServiceProviderRoot : public IServiceProvider {
 public:
  virtual std::unique_ptr<IServiceProvider> createScope() = 0;
};
}  // namespace CppInject