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
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ConstructorFinder.h"
#include "IServiceProvider.h"
#include "TypeTraits.h"

namespace CppInject {

/// <summary>
/// A builder for a service provider. This class is used to collect descriptions
/// for the creation and life time of services. The create function builds a
/// service provider.
/// </summary>
class ServiceCollection {
 public:
  /// <summary>
  /// Register a singleton service
  /// </summary>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <typeparam name="TImplementation">The type of the implementation -
  /// TService must be a base of TImplementation</typeparam>
  template <class TService, class TImplementation = TService,
            typename = typename std::enable_if_t<
                std::is_base_of_v<TService, TImplementation>>>
  inline void addSingleton();

  /// <summary>
  /// Register a singleton service using a factory
  /// </summary>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <typeparam name="F">
  /// The type of the factory function:
  /// (IServiceProvider&amp;) -> std::unique_ptr &lt; TImplementation &gt;
  /// </typeparam>
  /// <param name="factory">The factory function that creates the service
  /// instance</param>
  template <class TService, typename F,
            typename = typename std::enable_if_t<std::is_base_of_v<
                TService, typename std::invoke_result_t<
                              F, IServiceProvider&>::element_type>>>
  inline void addSingleton(F&& factory);

  /// <summary>
  /// Register a singleton service using a factory
  /// </summary>
  /// <typeparam name="F">
  /// The type of the factory function:
  /// (IServiceProvider&amp;) -> std::unique_ptr &lt; TImplementation &gt;
  /// </typeparam>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <param name="factory">The factory function that creates the service
  /// instance</param>
  template <typename F, typename = typename std::invoke_result_t<
                            F, IServiceProvider&>::element_type>
  inline void addSingleton(F&& factory);

  /// <summary>
  /// Add an existing service to the ServiceCollection.
  /// <para/>
  /// The ServiceCollection and the ServiceProvider keep a shared_ptr to the
  /// instance
  /// </summary>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <typeparam name="TImplementation">The type of the implementation -
  /// TService must be a base class of TImplementation</typeparam>
  /// <param name="existingService">An existing service.</param>
  template <class TService, typename TImplementation = TService,
            typename = typename std::enable_if_t<
                std::is_base_of_v<TService, TImplementation>>>
  inline void addSingleton(std::shared_ptr<TImplementation> existingService);

  /// <summary>
  /// Register a scoped service
  /// </summary>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <typeparam name="TImplementation">The type of the implementation -
  /// TService must be a base of TImplementation</typeparam>
  template <class TService, class TImplementation = TService,
            typename = typename std::enable_if_t<
                std::is_base_of_v<TService, TImplementation>>>
  inline void addScoped();

  /// <summary>
  /// Register a scoped service using a factory
  /// </summary>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <typeparam name="F">
  /// The type of the factory function:
  /// (IServiceProvider&amp;) -> std::unique_ptr &lt; TImplementation &gt;
  /// </typeparam>
  /// <param name="factory">The factory function that creates the service
  /// instance</param>
  template <class TService, typename F,
            typename = typename std::enable_if_t<std::is_base_of_v<
                TService, typename std::invoke_result_t<
                              F, IServiceProvider&>::element_type>>>
  inline void addScoped(F&& factory);

  /// <summary>
  /// Register a scoped service using a factory
  /// </summary>
  /// <typeparam name="F">
  /// The type of the factory function:
  /// (IServiceProvider&amp;) -> std::unique_ptr &lt; TImplementation &gt;
  /// </typeparam>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <param name="factory">The factory function that creates the service
  /// instance</param>
  template <typename F, typename = typename std::invoke_result_t<
                            F, IServiceProvider&>::element_type>
  inline void addScoped(F&& factory);

  /// <summary>
  /// Register a transient service
  /// </summary>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <typeparam name="TImplementation">The type of the implementation -
  /// TService must be a base of TImplementation</typeparam>
  template <class TService, class TImplementation = TService,
            typename = typename std::enable_if_t<
                std::is_base_of_v<TService, TImplementation>>>
  inline void addTransient();

  /// <summary>
  /// Register a transient service using a factory
  /// </summary>
  /// <typeparam name="TService">The type of the service</typeparam>
  /// <typeparam name="F">
  /// The type of the factory function:
  /// (IServiceProvider&amp;) -> std::unique_ptr &lt; TImplementation &gt;
  /// </typeparam>
  /// <param name="factory">The factory function that creates the service
  /// instance</param>
  template <class TService, typename F,
            typename = typename std::enable_if_t<std::is_base_of_v<
                TService, typename std::invoke_result_t<
                              F, IServiceProvider&>::element_type>>>
  inline void addTransient(F&& factory);

  /// <summary>
  /// Create a service provider from the service collection
  /// </summary>
  /// <returns>A unique_ptr to the service provider instance</returns>
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

  class ServiceProvider final : public IServiceProviderRoot {
    const std::unordered_map<std::type_index, FactoryFunctionCollection>
        _factories;
    mutable std::mutex _initializationOrderMutex;
    std::vector<std::any> _initializationOrder;
    mutable std::mutex _instancesMutex;
    std::unordered_map<std::type_index, std::unique_ptr<ConcurrentService[]>>
        _instances;

    class ScopedServiceProvider final : public IServiceProvider {
      ServiceProvider& _parent;
      mutable std::mutex _initializationOrderMutex;
      std::vector<std::any> _initializationOrder;
      mutable std::mutex _instancesMutex;
      std::unordered_map<std::type_index, std::unique_ptr<ConcurrentService[]>>
          _instances;

     public:
      ScopedServiceProvider(ServiceProvider& parent);
      inline ~ScopedServiceProvider();
      inline std::any getService(std::type_index type) final;
      inline std::vector<std::any> getServices(std::type_index type) final;

      template <class TServiceProvider>
      inline static std::any getService(
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

    inline ~ServiceProvider();

    inline std::any getService(std::type_index type) final;
    inline std::vector<std::any> getServices(std::type_index type) final;
    inline std::unique_ptr<IServiceProvider> createScope() final;
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
          getService<std::remove_reference_t<Type>>(serviceProvider));
    }

   public:
    inline std::shared_ptr<TService> create(IServiceProvider& serviceProvider) {
      return createInternal<ConstructorArgsAsTuple<TService>>(serviceProvider);
    }
  };

  template <class TService, class TImplementation, ServiceType serviceType,
            typename = typename std::enable_if_t<serviceType !=
                                                 ServiceType::Transient>>
  inline void addService();

  template <
      class TService, ServiceType serviceType, class F,
      typename = typename std::enable_if_t<
          serviceType != ServiceType::Transient &&
          std::is_base_of_v<TService, typename std::invoke_result_t<
                                          F, IServiceProvider&>::element_type>>>
  inline void addService(F&& factory);
};

template <class TService, class TImplementation,
          ServiceCollection::ServiceType serviceType, typename>
inline void ServiceCollection::addService() {
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
inline void ServiceCollection::addSingleton() {
  addService<TService, TImplementation, ServiceType::Singleton>();
}

template <class TService, class TImplementation, typename>
inline void ServiceCollection::addScoped() {
  addService<TService, TImplementation, ServiceType::Scoped>();
}

template <class TService, ServiceCollection::ServiceType serviceType, class F,
          typename>
inline void ServiceCollection::addService(F&& factory) {
  using SharedPtrType = std::shared_ptr<
      typename std::invoke_result_t<F, IServiceProvider&>::element_type>;
  const auto typeIndex = std::type_index(typeid(TService));
  auto& serviceFactories =
      _factories.emplace(typeIndex, FactoryFunctionCollection{}).first->second;
  serviceFactories.emplace_back(
      [f = std::move(factory)](IServiceProvider& sp) -> std::any {
        return SharedPtrType(f(sp));
      },
      [](std::any managedData) -> std::any {
        return std::ref(static_cast<TService&>(
            *std::any_cast<SharedPtrType&>(managedData)));
      },
      serviceType);
}

template <class TService, class F, typename>
inline void ServiceCollection::addSingleton(F&& factory) {
  addService<TService, ServiceType::Singleton>(std::forward<F>(factory));
}

template <typename F, typename>
inline void ServiceCollection::addSingleton(F&& factory) {
  addSingleton<
      typename std::invoke_result_t<F, IServiceProvider&>::element_type, F>(
      std::forward<F>(factory));
}

template <class TService, class F, typename>
inline void ServiceCollection::addScoped(F&& factory) {
  addService<TService, ServiceType::Scoped>(std::forward<F>(factory));
}

template <typename F, typename>
inline void ServiceCollection::addScoped(F&& factory) {
  addScoped<typename std::invoke_result_t<F, IServiceProvider&>::element_type,
            F>(std::forward<F>(factory));
}

template <class TService, typename TImplementation, typename>
inline void ServiceCollection::addSingleton(
    std::shared_ptr<TImplementation> existingService) {
  addService<TService, ServiceType::Singleton>(
      [service = std::move(existingService)](IServiceProvider&) {
        return service;
      });
}

template <class TService, class TImplementation, typename>
inline void ServiceCollection::addTransient() {
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

template <class TService, class F, typename>
inline void ServiceCollection::addTransient(F&& factory) {
  using SharedPtrType = std::shared_ptr<
      typename std::invoke_result_t<F, IServiceProvider&>::element_type>;
  const auto typeIndex = std::type_index(typeid(std::shared_ptr<TService>));
  auto& factories =
      _factories.emplace(typeIndex, FactoryFunctionCollection{}).first->second;
  factories.emplace_back(
      [f = std::move(factory)](IServiceProvider& sp) -> std::any {
        return SharedPtrType(f(sp));
      },
      [](std::any managedData) -> std::any {
        return std::static_pointer_cast<TService>(
            std::any_cast<SharedPtrType>(std::move(managedData)));
      },
      ServiceType::Transient);
}

inline std::unique_ptr<IServiceProviderRoot> ServiceCollection::build() {
  return std::make_unique<ServiceProvider>(_factories);
}

template <class TServiceProvider>
inline std::any
ServiceCollection::ServiceProvider::ScopedServiceProvider::getService(
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

inline std::any ServiceCollection::ServiceProvider::getService(
    std::type_index type) {
  auto factoryIt = _factories.find(type);
  if (factoryIt == _factories.end()) return std::any();
  return ScopedServiceProvider::getService(*factoryIt, *this, *this,
                                           factoryIt->second.size() - 1);
}

inline std::vector<std::any> ServiceCollection::ServiceProvider::getServices(
    std::type_index type) {
  std::vector<std::any> res;
  auto factoryIt = _factories.find(type);
  if (factoryIt == _factories.end()) return res;
  for (size_t i = 0; i < factoryIt->second.size(); ++i)
    res.emplace_back(
        ScopedServiceProvider::getService(*factoryIt, *this, *this, i));
  return res;
}

inline std::unique_ptr<IServiceProvider>
ServiceCollection::ServiceProvider::createScope() {
  return std::make_unique<ScopedServiceProvider>(*this);
}

inline ServiceCollection::ServiceProvider::~ServiceProvider() {
  _instances.clear();
  while (!_initializationOrder.empty()) _initializationOrder.pop_back();
}

inline ServiceCollection::ServiceProvider::ScopedServiceProvider::
    ScopedServiceProvider(ServiceProvider& parent)
    : _parent(parent) {}

inline ServiceCollection::ServiceProvider::ScopedServiceProvider::
    ~ScopedServiceProvider() {
  _instances.clear();
  while (!_initializationOrder.empty()) _initializationOrder.pop_back();
}

inline std::any
ServiceCollection::ServiceProvider::ScopedServiceProvider::getService(
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

inline std::vector<std::any>
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