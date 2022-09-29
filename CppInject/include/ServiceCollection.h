#pragma once

#include <any>
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "IServiceProvider.h"
#include "TypeTraits.h"

namespace CppInject {

class ServiceCollection {
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

 private:
  enum class ServiceType {
    Singleton,
    Scoped,
    Transient,
  };
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

  struct ConcurrentService {
    std::atomic<bool> initialized{false};
    std::promise<std::any> servicePromise;
    std::shared_future<std::any> serviceFuture{servicePromise.get_future()};
  };

  class ServiceProvider : public IServiceProviderRoot {
    const std::unordered_map<std::type_index, FactoryFunctionCollection>
        _factories;
    mutable std::mutex _initializationOrderMutex;
    std::vector<std::any> _initializationOrder;
    mutable std::mutex _instancesMutex;
    std::unordered_map<std::type_index, std::unique_ptr<ConcurrentService[]>>
        _instances;

    class ScopedServiceProvider : public IServiceProvider {
      ServiceProvider& _parent;
      mutable std::mutex _initializationOrderMutex;
      std::vector<std::any> _initializationOrder;
      mutable std::mutex _instancesMutex;
      std::unordered_map<std::type_index, std::unique_ptr<ConcurrentService[]>>
          _instances;

     public:
      ScopedServiceProvider(ServiceProvider& parent);
      ~ScopedServiceProvider();
      std::any getService(std::type_index type) override;
      std::vector<std::any> getServices(std::type_index type) override;

      template <class TServiceProvider>
    static std::any getService(
          const std::pair<std::type_index, FactoryFunctionCollection>&
              factories,
          TServiceProvider& serviceProviderForThisService,
          IServiceProvider& serviceProviderForDependentServices, size_t index);
    };

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
    inline
        typename std::enable_if_t<TypeTraits::IsVector<T>::value &&
                                      TypeTraits::IsSharedPointer<std::decay_t<
                                          typename T::value_type>>::value,
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
    inline
        typename std::enable_if_t<TypeTraits::IsVector<T>::value &&
                                      !TypeTraits::IsSharedPointer<std::decay_t<
                                          typename T::value_type>>::value,
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
    inline std::enable_if_t<TypeTraits::IsSharedPointer<T>::value, T>
    getService(IServiceProvider& serviceProvider) {
      return std::any_cast<T>(
          serviceProvider.getService(std::type_index(typeid(T))));
    }

    template <class T>
    inline std::enable_if_t<!TypeTraits::IsSharedPointer<T>::value &&
                                !TypeTraits::IsVector<T>::value,
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
};

template <class TService, class TImplementation,
          ServiceCollection::ServiceType serviceType, typename>
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

template <class TServiceProvider>
std::any ServiceCollection::ServiceProvider::ScopedServiceProvider::getService(
    const std::pair<std::type_index, FactoryFunctionCollection>& factories,
    TServiceProvider& serviceProviderForThisService,
    IServiceProvider& serviceProviderForDependentSerices, size_t index) {
  const auto& desc = factories.second.at(index);
  if (desc.type == ServiceType::Singleton || desc.type == ServiceType::Scoped) {
    std::unique_lock instancesLock(
        serviceProviderForThisService._instancesMutex);
    auto instancesPair = serviceProviderForThisService._instances.emplace(
        factories.first, std::unique_ptr<ConcurrentService[]>{});
    if (instancesPair.second)
      instancesPair.first->second =
          std::make_unique<ConcurrentService[]>(factories.second.size());
    ConcurrentService* instances = instancesPair.first->second.get();
    instancesLock.unlock();
    bool expected = false;
    if (instances[index].initialized.compare_exchange_strong(expected, true)) {
      instances[index].servicePromise.set_value(
          desc.create(serviceProviderForDependentSerices));
      std::scoped_lock lock{
          serviceProviderForThisService._initializationOrderMutex};
      serviceProviderForThisService._initializationOrder.emplace_back(
          instances[index].serviceFuture.get());
    }
    return desc.convert(instances[index].serviceFuture.get());
  } else  // ServiceType::Transient
  {
    return desc.convert(desc.create(serviceProviderForDependentSerices));
  }
}

std::any ServiceCollection::ServiceProvider::getService(std::type_index type) {
  auto factoryIt = _factories.find(type);
  if (factoryIt == _factories.end()) return std::any();
  return ScopedServiceProvider::getService(*factoryIt, *this, *this,
                                           factoryIt->second.size() - 1);
}

std::vector<std::any> ServiceCollection::ServiceProvider::getServices(
    std::type_index type) {
  std::vector<std::any> res;
  auto factoryIt = _factories.find(type);
  if (factoryIt == _factories.end()) return res;
  for (size_t i = 0; i < factoryIt->second.size(); ++i)
    res.emplace_back(
        ScopedServiceProvider::getService(*factoryIt, *this, *this, i));
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
  return desc.type == ServiceType::Scoped
             ? getService(*factoryIt, *this, *this,
                          factoryIt->second.size() - 1)
             : getService(*factoryIt, _parent, *this,
                          factoryIt->second.size() - 1);
  ;
}

std::vector<std::any>
ServiceCollection::ServiceProvider::ScopedServiceProvider::getServices(
    std::type_index type) {
  std::vector<std::any> res;
  auto factoryIt = _parent._factories.find(type);
  if (factoryIt == _parent._factories.end()) return {};
  for (size_t i = 0; i < factoryIt->second.size(); ++i) {
    const auto& desc = factoryIt->second[i];
    res.emplace_back(desc.type == ServiceType::Scoped
                         ? getService(*factoryIt, *this, *this, i)
                         : getService(*factoryIt, _parent, *this, i));
  }
  return res;
}
}  // namespace CppInject