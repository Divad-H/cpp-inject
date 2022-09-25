#pragma once

#include <any>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "TypeTraits.h"

namespace CppInject {

enum class ServiceType {
  Singleton,
  Scoped,
  Transient,
};

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

class ServiceCollection {
  struct ServiceDescription {
    using FactoryFunction = std::function<std::any(IServiceProvider& sp)>;
    using ConversionFunction = std::function<std::any(std::any managedData)>;
    FactoryFunction create;
    ConversionFunction convert;
    ServiceType type;

    ServiceDescription(FactoryFunction&& createFunc,
                       ConversionFunction&& conversionFunc, ServiceType type)
        : create(std::move(createFunc)),
          convert(std::move(conversionFunc)),
          type(type) {}
  };
  using FactoryFunctionCollection = std::vector<ServiceDescription>;

  std::unordered_map<std::type_index, FactoryFunctionCollection> _factories;

  class ServiceProvider : public IServiceProviderRoot {
    const std::unordered_map<std::type_index, FactoryFunctionCollection>
        _factories;
    std::vector<std::any> _initializationOrder;
    std::unordered_map<std::type_index, std::vector<std::any>> _instances;

    class ScopedServiceProvider : public IServiceProvider {
      ServiceProvider& _parent;
      std::vector<std::any> _initializationOrder;
      std::unordered_map<std::type_index, std::vector<std::any>> _instances;

     public:
      ScopedServiceProvider(ServiceProvider& parent);
      ~ScopedServiceProvider();
      std::any getService(std::type_index type) override;
      std::vector<std::any> getServices(std::type_index type) override;
    };

    static std::any getService(
        const std::pair<std::type_index, FactoryFunctionCollection>& factories,
        std::unordered_map<std::type_index, std::vector<std::any>>&
            providerInstances,
        std::vector<std::any>& initializationOrder,
        IServiceProvider& serviceProvider, size_t index);

   public:
    ServiceProvider(
        std::unordered_map<std::type_index, FactoryFunctionCollection>
            factories)
        : _factories(std::move(factories)) {}

    ~ServiceProvider();

    std::any getService(std::type_index type) override;
    std::vector<std::any> getServices(std::type_index type) override;
    std::unique_ptr<IServiceProvider> createScope() override;
  };

  template <class TService>
  class ServiceFactory {
    template <class T>
    inline typename std::enable_if_t<
        IsVector<T>::value &&
            IsSharedPointer<std::decay_t<typename T::value_type>>::value,
        T>
    getService(IServiceProvider& serviceProvider) {
      T services{};
      auto servicesAny = serviceProvider.getServices(
          std::type_index(typeid(std::decay_t<typename T::value_type>)));
      for (auto&& service : std::move(servicesAny))
        services.emplace_back(std::any_cast<T::value_type>(std::move(service)));
      return services;
    }

    template <class T>
    inline typename std::enable_if_t<
        IsVector<T>::value &&
            !IsSharedPointer<std::decay_t<typename T::value_type>>::value,
        T>
    getService(IServiceProvider& serviceProvider) {
      T services{};
      auto servicesAny = serviceProvider.getServices(
          std::type_index(typeid(std::decay_t<typename T::value_type::type>)));
      for (auto& service : servicesAny)
        services.emplace_back(std::any_cast<T::value_type>(service));
      return services;
    }

    template <class T>
    inline std::enable_if_t<IsSharedPointer<T>::value, T> getService(
        IServiceProvider& serviceProvider) {
      return std::any_cast<T>(
          serviceProvider.getService(std::type_index(typeid(T))));
    }

    template <class T>
    inline std::enable_if_t<!IsSharedPointer<T>::value && !IsVector<T>::value,
                            T&>
    getService(IServiceProvider& serviceProvider) {
      return std::any_cast<std::reference_wrapper<T>>(
          serviceProvider.getService(std::type_index(typeid(T))));
    }

    template <class Tuple, std::size_t I = 0, class... Args>
    inline typename std::enable_if_t<I == std::tuple_size_v<Tuple>,
                                     std::shared_ptr<TService>>
    createInternal(IServiceProvider& serviceProvider, Args&&... args) {
      return std::make_shared<TService>(std::forward<Args>(args)...);
    }

    template <class Tuple, std::size_t I = 0, class... Args>
    inline typename std::enable_if_t<(I < std::tuple_size_v<Tuple>),
                                     std::shared_ptr<TService>>
    createInternal(IServiceProvider& serviceProvider, Args&&... args) {
      using Type = std::tuple_element_t<I, Tuple>;
      return createInternal<Tuple, I + 1, Args...>(
          serviceProvider, std::forward<Args>(args)...,
          getService<Type>(serviceProvider));
    }

   public:
    std::shared_ptr<TService> create(IServiceProvider& serviceProvider) {
      return createInternal<ConstructorArgsAsTuple<TService>>(serviceProvider);
    }
  };

  template <class TService, class TImplementation, ServiceType serviceType,
            typename = typename std::enable_if_t<serviceType !=
                                                 ServiceType::Transient>>
  void addService();

 public:
  template <class TService, class TImplementation = TService,
            typename = typename std::enable_if_t<
                std::is_base_of_v<TService, TImplementation>>>
  void addSingleton();

  template <class TService, class TImplementation = TService,
            typename = typename std::enable_if_t<
                std::is_base_of_v<TService, TImplementation>>>
  void addScoped();

  template <class TService, class TImplementation = TService,
            typename = typename std::enable_if_t<
                std::is_base_of_v<TService, TImplementation>>>
  void addTransient();

  std::unique_ptr<IServiceProviderRoot> build();
};

template <class TService, class TImplementation, ServiceType serviceType,
          typename>
void ServiceCollection::addService() {
  const auto typeIndex = std::type_index(typeid(TService));
  auto& serviceFactories =
      _factories.emplace(typeIndex, FactoryFunctionCollection{}).first->second;
  serviceFactories.emplace_back(
      [](IServiceProvider& sp) -> std::any {
        ServiceFactory<TImplementation> sf;
        return sf.create(sp);
      },
      [](std::any managedData) -> std::any {
        return std::ref(static_cast<TService&>(
            *std::any_cast<std::shared_ptr<TImplementation>&>(managedData)));
      },
      serviceType);
}

template <class TService, class TImplementation, typename>
void ServiceCollection::addSingleton() {
  addService<TService, TImplementation, ServiceType::Singleton>();
}

template <class TService, class TImplementation, typename>
void ServiceCollection::addScoped() {
  addService<TService, TImplementation, ServiceType::Scoped>();
}

template <class TService, class TImplementation, typename>
void ServiceCollection::addTransient() {
  const auto typeIndex = std::type_index(typeid(std::shared_ptr<TService>));
  auto& factories =
      _factories.emplace(typeIndex, FactoryFunctionCollection{}).first->second;
  factories.emplace_back(
      [](IServiceProvider& sp) -> std::any {
        ServiceFactory<TImplementation> sf;
        return sf.create(sp);
      },
      [](std::any managedData) -> std::any {
        return std::static_pointer_cast<TService>(
            std::any_cast<std::shared_ptr<TImplementation>>(
                std::move(managedData)));
      },
      ServiceType::Transient);
}

std::unique_ptr<IServiceProviderRoot> ServiceCollection::build() {
  return std::make_unique<ServiceProvider>(_factories);
}

std::any ServiceCollection::ServiceProvider::getService(
    const std::pair<std::type_index, FactoryFunctionCollection>& factories,
    std::unordered_map<std::type_index, std::vector<std::any>>&
        providerInstances,
    std::vector<std::any>& initializationOrder,
    IServiceProvider& serviceProvider, size_t index) {
  const auto& desc = factories.second.at(index);
  if (desc.type == ServiceType::Singleton || desc.type == ServiceType::Scoped) {
    auto instances =
        providerInstances.emplace(factories.first, std::vector<std::any>{});
    if (instances.second)
      instances.first->second.resize(factories.second.size());
    if (!instances.first->second.at(index).has_value()) {
      auto it = instances.first->second.emplace(
          std::next(instances.first->second.begin(), index),
          desc.create(serviceProvider));
      initializationOrder.emplace_back(*it);
    }
    return desc.convert(instances.first->second.at(index));
  } else  // ServiceType::Transient
  {
    return desc.convert(desc.create(serviceProvider));
  }
}

std::any ServiceCollection::ServiceProvider::getService(std::type_index type) {
  auto factoryIt = _factories.find(type);
  if (factoryIt == _factories.end()) return std::any();
  return getService(*factoryIt, _instances, _initializationOrder, *this, 0);
}

std::vector<std::any> ServiceCollection::ServiceProvider::getServices(
    std::type_index type) {
  std::vector<std::any> res;
  auto factoryIt = _factories.find(type);
  if (factoryIt == _factories.end()) return res;
  for (size_t i = 0; i < factoryIt->second.size(); ++i)
    res.emplace_back(
        getService(*factoryIt, _instances, _initializationOrder, *this, i));
  return res;
}

std::unique_ptr<IServiceProvider>
ServiceCollection::ServiceProvider::createScope() {
  return std::make_unique<ScopedServiceProvider>(*this);
}

ServiceCollection::ServiceProvider::~ServiceProvider() {
  _instances.clear();
  while (!_initializationOrder.empty()) _initializationOrder.pop_back();
}

ServiceCollection::ServiceProvider::ScopedServiceProvider::
    ScopedServiceProvider(ServiceProvider& parent)
    : _parent(parent) {}

ServiceCollection::ServiceProvider::ScopedServiceProvider::
    ~ScopedServiceProvider() {
  _instances.clear();
  while (!_initializationOrder.empty()) _initializationOrder.pop_back();
}

std::any ServiceCollection::ServiceProvider::ScopedServiceProvider::getService(
    std::type_index type) {
  auto factoryIt = _parent._factories.find(type);
  if (factoryIt == _parent._factories.end()) return std::any();
  const auto& desc = factoryIt->second.front();
  return ServiceProvider::getService(
      *factoryIt,
      desc.type == ServiceType::Scoped ? _instances : _parent._instances,
      desc.type == ServiceType::Scoped ? _initializationOrder
                                       : _parent._initializationOrder,
      *this, 0);
}

std::vector<std::any>
ServiceCollection::ServiceProvider::ScopedServiceProvider::getServices(
    std::type_index type) {
  std::vector<std::any> res;
  auto factoryIt = _parent._factories.find(type);
  if (factoryIt == _parent._factories.end()) return {};
  for (size_t i = 0; i < factoryIt->second.size(); ++i) {
    const auto& desc = factoryIt->second[i];
    res.emplace_back(ServiceProvider::getService(
        *factoryIt,
        desc.type == ServiceType::Scoped ? _instances : _parent._instances,
        desc.type == ServiceType::Scoped ? _initializationOrder
                                         : _parent._initializationOrder,
        *this, 0));
  }
  return res;
}
}  // namespace CppInject