#pragma once

#include <any>
#include <memory>
#include <typeindex>
#include <vector>

namespace CppInject {

/// <summary>
/// The type of the collection of services that is returned when requesting
/// multiple implementations.
/// </summary>
/// <typeparam name="TService">The type of the services</typeparam>
template <class TService>
using ServiceVector = std::vector<std::reference_wrapper<TService>>;

/// <summary>
/// Holds singleton and scoped service instances and allows creation of
/// services and access to existing services.
/// </summary>
class IServiceProvider {
 public:
  virtual ~IServiceProvider() = default;

  /// <summary>
  /// Get a singleton or scoped service.
  /// <para/>
  /// If multiple implementations have been registered for this service, the
  /// last added service is provided.
  /// </summary>
  /// <param name="type">The type of the service to get</param>
  /// <returns>An any, containing a pointer to the service, or a nullptr if the
  /// service is not available.</returns>
  virtual std::any getService(std::type_index type) = 0;

  /// <summary>
  /// Get singleton and scoped services of the requested type.
  /// </summary>
  /// <param name="type">The type of the services</param>
  /// <returns>An any containing a vector of reference_wrappers to the
  /// services - see ServiceVector.</returns>
  virtual std::vector<std::any> getServices(std::type_index type) = 0;

  /// <summary>
  /// Get a singleton or scoped service.
  /// <para/>
  /// If multiple implementations have been registered for this service, the
  /// last added service is provided.
  /// </summary>
  /// <typeparam name="TService">The type of the requested service</typeparam>
  /// <returns>A pointer to the service, or a nullptr if the service is not
  /// available</returns>
  template <typename TService>
  TService* getService() {
    auto res = getService(std::type_index(typeid(TService)));
    if (res.has_value())
      return &std::any_cast<std::reference_wrapper<TService>>(res).get();
    return nullptr;
  }

  /// <summary>
  /// Get a singleton or scoped service and fail if the service is not
  /// available.
  /// <para/>
  /// If multiple implementations have been registered for
  /// this service, the last added service is provided.
  /// </summary>
  /// <typeparam name="TService">The type of the requested service</typeparam>
  /// <returns>A reference to the service</returns>
  template <typename TService>
  TService& getRequiredService() {
    auto res = getService<TService>();
    if (res != nullptr) return *res;
    throw std::logic_error(
        typeid(TService).name() +
        std::string{
            " has not been registered as a singleton or scoped service."});
  }

  /// <summary>
  /// Create a transient service.
  /// <para/>
  /// If multiple implementations have been registered for this service, the
  /// last added service is provided.
  /// </summary>
  /// <typeparam name="TService">The type of the requested service</typeparam>
  /// <returns>A shared_ptr to the service, or a nullptr if the service is not
  /// available</returns>
  template <typename TService>
  std::shared_ptr<TService> getTransientService() {
    auto res = getService(std::type_index(typeid(std::shared_ptr<TService>)));
    if (res.has_value())
      return std::any_cast<std::shared_ptr<TService>>(std::move(res));
    return nullptr;
  }

  /// <summary>
  /// Create a transient service and fail if the service is not available.
  /// <para/>
  /// If multiple implementations have been registered for this service, the
  /// last added service is provided.
  /// </summary>
  /// <typeparam name="TService">The type of the requested service</typeparam>
  /// <returns>A shared_ptr to the service, guaranteed to not be null.</returns>
  template <typename TService>
  std::shared_ptr<TService> getRequiredTransientService() {
    auto res = getTransientService<TService>();
    if (res != nullptr) return res;
    throw std::logic_error(
        typeid(TService).name() +
        std::string{" has not been registered as a transient service."});
  }

  /// <summary>
  /// Get singleton and scoped services of the requested type.
  /// </summary>
  /// <param name="type">The type of the services</param>
  /// <returns>a vector of reference_wrappers to the services.</returns>
  template <typename TService>
  ServiceVector<TService> getServices() {
    ServiceVector<TService> res;
    auto servicesAny = getServices(std::type_index(typeid(TService)));
    for (auto& service : servicesAny)
      res.emplace_back(
          std::any_cast<std::reference_wrapper<TService>>(service));
    return res;
  }

  /// <summary>
  /// Get transient services of the requested type.
  /// </summary>
  /// <param name="type">The type of the services</param>
  /// <returns>a vector of shared_ptrs to the services.</returns>
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

/// <summary>
/// The root service provider that can create service scopes.
/// </summary>
class IServiceProviderRoot : public IServiceProvider {
 public:
  /// <summary>
  /// Create a service scope
  /// </summary>
  /// <returns>The scoped service provider</returns>
  virtual std::unique_ptr<IServiceProvider> createScope() = 0;
};
}  // namespace CppInject