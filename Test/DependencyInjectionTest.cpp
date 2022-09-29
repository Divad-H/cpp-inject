#include <ConstructorFinder.h>
#include <ServiceCollection.h>

#include <bitset>

using namespace CppInject;

namespace DependencyInjectionTest {
struct LeafService1 {
  int value = 1;
};

TEST(ServiceProviderTest, CanCreateSimpleTransientService) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getTransientService<LeafService1>();
  ASSERT_NE(nullptr, service);
}

TEST(ServiceProviderTest, EachTransientServiceIsNewInstance) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service1 = serviceProvider->getTransientService<LeafService1>();
  auto service2 = serviceProvider->getTransientService<LeafService1>();
  ASSERT_NE(nullptr, service1);
  ASSERT_NE(nullptr, service2);
  ASSERT_NE(service1, service2);
}

TEST(ServiceProviderTest, CanCreateSimpleSingletonService) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<LeafService1>();
  ASSERT_NE(nullptr, service);
}

TEST(ServiceProviderTest, SingletonServiceIsSingleInstance) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service1 = serviceProvider->getService<LeafService1>();
  auto service2 = serviceProvider->getService<LeafService1>();
  ASSERT_NE(nullptr, service1);
  ASSERT_NE(nullptr, service2);
  ASSERT_EQ(service1, service2);
}

TEST(ServiceProviderTest, CanCreateSimpleScopedService) {
  ServiceCollection serviceCollection;
  serviceCollection.addScoped<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<LeafService1>();
  ASSERT_NE(nullptr, service);
}

TEST(ServiceProviderTest, ScopedServiceIsSingleInstance) {
  ServiceCollection serviceCollection;
  serviceCollection.addScoped<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service1 = serviceProvider->getService<LeafService1>();
  auto service2 = serviceProvider->getService<LeafService1>();
  ASSERT_NE(nullptr, service1);
  ASSERT_NE(nullptr, service2);
  ASSERT_EQ(service1, service2);
}

TEST(ServiceProviderTest, ServiceScopesProvideSameSingleton) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<LeafService1>();
  auto serviceScope1 = serviceProvider->createScope();
  auto serviceScope2 = serviceProvider->createScope();
  auto scopedService1 = serviceScope1->getService<LeafService1>();
  auto scopedService2 = serviceScope2->getService<LeafService1>();
  ASSERT_NE(nullptr, service);
  ASSERT_NE(nullptr, scopedService1);
  ASSERT_NE(nullptr, scopedService2);
  ASSERT_EQ(service, scopedService1);
  ASSERT_EQ(scopedService1, scopedService2);
}

TEST(ServiceProviderTest, EachServiceScopesProvidesOwnInstance) {
  ServiceCollection serviceCollection;
  serviceCollection.addScoped<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto service1 = serviceProvider->getService<LeafService1>();
  auto service2 = serviceProvider->getService<LeafService1>();
  auto serviceScope1 = serviceProvider->createScope();
  auto serviceScope2 = serviceProvider->createScope();
  auto scopedService1a = serviceScope1->getService<LeafService1>();
  auto scopedService1b = serviceScope1->getService<LeafService1>();
  auto scopedService2a = serviceScope2->getService<LeafService1>();
  auto scopedService2b = serviceScope2->getService<LeafService1>();
  ASSERT_NE(nullptr, service1);
  ASSERT_NE(nullptr, service2);
  ASSERT_NE(nullptr, scopedService1a);
  ASSERT_NE(nullptr, scopedService1b);
  ASSERT_NE(nullptr, scopedService2a);
  ASSERT_NE(nullptr, scopedService2b);
  ASSERT_EQ(service1, service2);
  ASSERT_EQ(scopedService1a, scopedService1b);
  ASSERT_EQ(scopedService2b, scopedService2b);
  ASSERT_NE(service1, scopedService1a);
  ASSERT_NE(service1, scopedService2a);
  ASSERT_NE(scopedService1a, scopedService2a);
}

TEST(ServiceProviderTest, CanCreateTransientServicesFromServiceScopes) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<LeafService1>();
  auto serviceProvider = serviceCollection.build();
  auto serviceScope1 = serviceProvider->createScope();
  auto serviceScope2 = serviceProvider->createScope();
  auto service1 = serviceProvider->getTransientService<LeafService1>();
  auto service2 = serviceProvider->getTransientService<LeafService1>();
  auto service3 = serviceScope1->getTransientService<LeafService1>();
  auto service4 = serviceScope1->getTransientService<LeafService1>();
  auto service5 = serviceScope2->getTransientService<LeafService1>();
  auto service6 = serviceScope2->getTransientService<LeafService1>();
  ASSERT_NE(nullptr, service1);
  ASSERT_NE(nullptr, service2);
  ASSERT_NE(nullptr, service3);
  ASSERT_NE(nullptr, service4);
  ASSERT_NE(nullptr, service5);
  ASSERT_NE(nullptr, service6);
  ASSERT_NE(service1, service2);
  ASSERT_NE(service1, service3);
  ASSERT_NE(service1, service4);
  ASSERT_NE(service1, service5);
  ASSERT_NE(service1, service6);
  ASSERT_NE(service2, service3);
  ASSERT_NE(service2, service4);
  ASSERT_NE(service2, service5);
  ASSERT_NE(service2, service6);
  ASSERT_NE(service3, service4);
  ASSERT_NE(service3, service5);
  ASSERT_NE(service3, service6);
  ASSERT_NE(service4, service5);
  ASSERT_NE(service4, service6);
  ASSERT_NE(service5, service6);
}

struct ServiceWithTransientDependency {
  std::shared_ptr<LeafService1> _leafService;
  ServiceWithTransientDependency(std::shared_ptr<LeafService1> leafService)
      : _leafService(std::move(leafService)) {}
};

TEST(ServiceProviderTest, CanInjectTransientServiceIntoTransientService) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<LeafService1>();
  serviceCollection.addTransient<ServiceWithTransientDependency>();
  auto serviceProvider = serviceCollection.build();
  auto service =
      serviceProvider->getTransientService<ServiceWithTransientDependency>();
  ASSERT_NE(nullptr, service);
  auto secondInstance =
      serviceProvider->getTransientService<ServiceWithTransientDependency>();
  ASSERT_NE(nullptr, secondInstance);
  ASSERT_NE(service, secondInstance);
  ASSERT_NE(service->_leafService, secondInstance->_leafService);
}

TEST(ServiceProviderTest, CanInjectTransientServiceIntoSingletonService) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<LeafService1>();
  serviceCollection.addSingleton<ServiceWithTransientDependency>();
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<ServiceWithTransientDependency>();
  ASSERT_NE(nullptr, service);
  auto secondPointer =
      serviceProvider->getService<ServiceWithTransientDependency>();
  ASSERT_EQ(service, secondPointer);
  ASSERT_EQ(service->_leafService, secondPointer->_leafService);
}

TEST(ServiceProviderTest, CanInjectTransientServiceIntoScopedService) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<LeafService1>();
  serviceCollection.addScoped<ServiceWithTransientDependency>();
  auto serviceProvider = serviceCollection.build();
  auto serviceScope = serviceProvider->createScope();
  auto service = serviceScope->getService<ServiceWithTransientDependency>();
  ASSERT_NE(nullptr, service);
  auto secondPointer =
      serviceScope->getService<ServiceWithTransientDependency>();
  ASSERT_EQ(service, secondPointer);
  ASSERT_EQ(service->_leafService, secondPointer->_leafService);
  auto serviceScope2 = serviceProvider->createScope();
  auto secondInstance =
      serviceScope2->getService<ServiceWithTransientDependency>();
  ASSERT_NE(nullptr, secondInstance);
  ASSERT_NE(secondInstance, service);
  ASSERT_NE(service->_leafService, secondInstance->_leafService);
}

struct ServiceWithDependency {
  LeafService1& _leafService;
  ServiceWithDependency(LeafService1& leafService)
      : _leafService(leafService) {}
};

TEST(ServiceProviderTest, CanInjectSingletonServiceIntoTransientService) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addTransient<ServiceWithDependency>();
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getTransientService<ServiceWithDependency>();
  ASSERT_NE(nullptr, service);
  auto secondInstance =
      serviceProvider->getTransientService<ServiceWithDependency>();
  ASSERT_NE(nullptr, secondInstance);
  ASSERT_NE(service, secondInstance);
  ASSERT_EQ(&service->_leafService, &secondInstance->_leafService);
}

TEST(ServiceProviderTest, CanInjectScopedServiceIntoTransientService) {
  ServiceCollection serviceCollection;
  serviceCollection.addScoped<LeafService1>();
  serviceCollection.addTransient<ServiceWithDependency>();
  auto serviceProvider = serviceCollection.build();
  auto serviceScope = serviceProvider->createScope();
  auto service = serviceScope->getTransientService<ServiceWithDependency>();
  ASSERT_NE(nullptr, service);
  auto secondInstance =
      serviceScope->getTransientService<ServiceWithDependency>();
  ASSERT_NE(nullptr, secondInstance);
  ASSERT_NE(service, secondInstance);
  ASSERT_EQ(&service->_leafService, &secondInstance->_leafService);
  auto serviceScope2 = serviceProvider->createScope();
  auto thirdInstance =
      serviceScope2->getTransientService<ServiceWithDependency>();
  ASSERT_NE(nullptr, thirdInstance);
  ASSERT_NE(service, thirdInstance);
  ASSERT_NE(secondInstance, thirdInstance);
  ASSERT_NE(&service->_leafService, &thirdInstance->_leafService);
  ASSERT_NE(&secondInstance->_leafService, &thirdInstance->_leafService);
}

TEST(ServiceProviderTest, CanInjectSingletonServiceIntoScopedService) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addScoped<ServiceWithDependency>();
  auto serviceProvider = serviceCollection.build();
  auto serviceScope = serviceProvider->createScope();
  auto service = serviceScope->getService<ServiceWithDependency>();
  ASSERT_NE(nullptr, service);
  auto secondPointer = serviceScope->getService<ServiceWithDependency>();
  ASSERT_EQ(service, secondPointer);
  ASSERT_EQ(&service->_leafService, &secondPointer->_leafService);
  auto serviceScope2 = serviceProvider->createScope();
  auto secondInstance = serviceScope2->getService<ServiceWithDependency>();
  ASSERT_NE(nullptr, secondInstance);
  ASSERT_NE(service, secondInstance);
  ASSERT_EQ(&service->_leafService, &secondInstance->_leafService);
}

TEST(ServiceProviderTest, CanInjectScopedServiceIntoScopedService) {
  ServiceCollection serviceCollection;
  serviceCollection.addScoped<LeafService1>();
  serviceCollection.addScoped<ServiceWithDependency>();
  auto serviceProvider = serviceCollection.build();
  auto serviceScope = serviceProvider->createScope();
  auto service = serviceScope->getService<ServiceWithDependency>();
  ASSERT_NE(nullptr, service);
  auto secondPointer = serviceScope->getService<ServiceWithDependency>();
  ASSERT_EQ(service, secondPointer);
  ASSERT_EQ(&service->_leafService, &secondPointer->_leafService);
  auto serviceScope2 = serviceProvider->createScope();
  auto secondInstance = serviceScope2->getService<ServiceWithDependency>();
  ASSERT_NE(nullptr, secondInstance);
  ASSERT_NE(service, secondInstance);
  ASSERT_NE(&service->_leafService, &secondInstance->_leafService);
}

TEST(ServiceProviderTest, CanInjectSingletonServiceIntoSingletonService) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addSingleton<ServiceWithDependency>();
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<ServiceWithDependency>();
  ASSERT_NE(nullptr, service);
  auto secondPointer = serviceProvider->getService<ServiceWithDependency>();
  ASSERT_EQ(service, secondPointer);
  auto serviceScope = serviceProvider->createScope();
  auto thirdPointer = serviceScope->getService<ServiceWithDependency>();
  ASSERT_EQ(service, thirdPointer);
}

struct DestructorTestClass0 {
  bool* _failure{nullptr};
  std::bitset<4> _value{};
  ~DestructorTestClass0() {
    _value.set(0, true);
    *_failure = *_failure || !_value.all();
  }
};

struct DestructorTestClass1 {
  DestructorTestClass0& _dependency;
  DestructorTestClass1(DestructorTestClass0& dependency)
      : _dependency(dependency) {}
  ~DestructorTestClass1() {
    _dependency._value.set(1, true);
    *_dependency._failure =
        *_dependency._failure || _dependency._value != 0b1110;
  }
};

struct DestructorTestClass2 {
  DestructorTestClass1& _dependency;
  DestructorTestClass2(DestructorTestClass1& dependency)
      : _dependency(dependency) {}
  ~DestructorTestClass2() {
    _dependency._dependency._value.set(2, true);
    *_dependency._dependency._failure =
        *_dependency._dependency._failure ||
        _dependency._dependency._value != 0b1100;
  }
};

struct DestructorTestClass3 {
  DestructorTestClass2& _dependency;
  DestructorTestClass3(DestructorTestClass2& dependency)
      : _dependency(dependency) {}
  ~DestructorTestClass3() {
    _dependency._dependency._dependency._value.set(3, true);
    *_dependency._dependency._dependency._failure =
        *_dependency._dependency._dependency._failure ||
        _dependency._dependency._dependency._value != 0b1000;
  }
};

TEST(ServiceProviderTest, DestructsSingletonServicesInReversedCreationOrder) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<DestructorTestClass0>();
  serviceCollection.addSingleton<DestructorTestClass1>();
  serviceCollection.addSingleton<DestructorTestClass2>();
  serviceCollection.addSingleton<DestructorTestClass3>();
  bool failure = false;
  {
    auto serviceProvider = serviceCollection.build();
    auto& service = serviceProvider->getRequiredService<DestructorTestClass3>();
    service._dependency._dependency._dependency._failure = &failure;
  }
  ASSERT_FALSE(failure);
}

TEST(ServiceProviderTest, DestructsScopedServicesInReversedCreationOrder) {
  ServiceCollection serviceCollection;
  serviceCollection.addScoped<DestructorTestClass0>();
  serviceCollection.addScoped<DestructorTestClass1>();
  serviceCollection.addScoped<DestructorTestClass2>();
  serviceCollection.addScoped<DestructorTestClass3>();
  bool failure = false;
  {
    auto serviceProvider = serviceCollection.build();
    auto& service = serviceProvider->getRequiredService<DestructorTestClass3>();
    service._dependency._dependency._dependency._failure = &failure;
  }
  ASSERT_FALSE(failure);
}

struct LeafService2 {};
struct LeafService3 {};
struct LeafService4 {};

struct ServiceWithMultipleDependencies1 {
  LeafService1& _leafService1;
  LeafService2& _leafService2;
  std::shared_ptr<LeafService3> _leafService3;
  std::shared_ptr<LeafService4> _leafService4;
  ServiceWithMultipleDependencies1(LeafService1& leafService1,
                                   LeafService2& leafService2,
                                   std::shared_ptr<LeafService3>&& leafService3,
                                   std::shared_ptr<LeafService4>&& leafService4)
      : _leafService1(leafService1),
        _leafService2(leafService2),
        _leafService3(std::move(leafService3)),
        _leafService4(std::move(leafService4)) {}
};

struct ServiceWithMultipleDependencies2 {
  LeafService1& _leafService1;
  LeafService2& _leafService2;
  std::shared_ptr<LeafService3> _leafService3;
  std::shared_ptr<LeafService4> _leafService4;
  ServiceWithMultipleDependencies1& _serviceWithMultipleDependencies1;
  ServiceWithMultipleDependencies2(
      LeafService1& leafService1, LeafService2& leafService2,
      std::shared_ptr<LeafService3>&& leafService3,
      std::shared_ptr<LeafService4>&& leafService4,
      ServiceWithMultipleDependencies1& serviceWithMultipleDependencies1)
      : _leafService1(leafService1),
        _leafService2(leafService2),
        _leafService3(std::move(leafService3)),
        _leafService4(std::move(leafService4)),
        _serviceWithMultipleDependencies1(serviceWithMultipleDependencies1) {}
};

struct ServiceWithMultipleDependencies3 {
  LeafService1& _leafService1;
  std::shared_ptr<LeafService4> _leafService4;
  ServiceWithMultipleDependencies2& _serviceWithMultipleDependencies2;
  ServiceWithMultipleDependencies3(
      LeafService1& leafService1, std::shared_ptr<LeafService4>&& leafService4,
      ServiceWithMultipleDependencies2& serviceWithMultipleDependencies2)
      : _leafService1(leafService1),
        _leafService4(std::move(leafService4)),
        _serviceWithMultipleDependencies2(serviceWithMultipleDependencies2) {}
};

TEST(ServiceProviderTest, CanCreateComplexDependencyTree) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addSingleton<LeafService2>();
  serviceCollection.addTransient<LeafService3>();
  serviceCollection.addTransient<LeafService4>();
  serviceCollection.addSingleton<ServiceWithMultipleDependencies1>();
  serviceCollection.addScoped<ServiceWithMultipleDependencies2>();
  serviceCollection.addScoped<ServiceWithMultipleDependencies3>();
  auto serviceProvider = serviceCollection.build();
  auto serviceScope = serviceProvider->createScope();
  auto& rootService =
      serviceScope->getRequiredService<ServiceWithMultipleDependencies3>();
  auto& service2 =
      serviceScope->getRequiredService<ServiceWithMultipleDependencies2>();
  ASSERT_EQ(&rootService._serviceWithMultipleDependencies2, &service2);
  auto& service1 =
      serviceScope->getRequiredService<ServiceWithMultipleDependencies1>();
  ASSERT_EQ(&service2._serviceWithMultipleDependencies1, &service1);
  auto serviceScope2 = serviceProvider->createScope();
  auto& rootServiceScope2 =
      serviceScope2->getRequiredService<ServiceWithMultipleDependencies3>();
  auto& service2Scope2 =
      serviceScope2->getRequiredService<ServiceWithMultipleDependencies2>();
  ASSERT_EQ(&rootServiceScope2._serviceWithMultipleDependencies2,
            &service2Scope2);
  auto& service1Scope2 =
      serviceScope2->getRequiredService<ServiceWithMultipleDependencies1>();
  ASSERT_EQ(&service2Scope2._serviceWithMultipleDependencies1, &service1Scope2);
  ASSERT_NE(&rootServiceScope2, &rootService);
  ASSERT_NE(&service2Scope2, &service2);
  ASSERT_EQ(&service2Scope2._serviceWithMultipleDependencies1,
            &service2._serviceWithMultipleDependencies1);
  auto& service1FromProvider =
      serviceProvider->getRequiredService<ServiceWithMultipleDependencies1>();
  ASSERT_EQ(&service1FromProvider, &service1);
}

struct IService {
  virtual ~IService() = default;
};

struct Service1 : public IService {};

struct Service2 : public IService {};

struct Service3 : public IService {};

TEST(ServiceProviderTest, CanGetMultipleServicesForSameInterface) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<IService, Service1>();
  serviceCollection.addSingleton<IService, Service2>();
  serviceCollection.addSingleton<IService, Service3>();
  auto serviceProvider = serviceCollection.build();
  auto service3 = serviceProvider->getService<IService>();
  ASSERT_NE(nullptr, dynamic_cast<Service3*>(service3));
  auto services = serviceProvider->getServices<IService>();
  ASSERT_EQ(3, services.size());
  ASSERT_NE(nullptr, dynamic_cast<Service1*>(&services[0].get()));
  ASSERT_NE(nullptr, dynamic_cast<Service2*>(&services[1].get()));
  ASSERT_NE(nullptr, dynamic_cast<Service3*>(&services[2].get()));
  ASSERT_EQ(service3, &services[2].get());
}

TEST(ServiceProviderTest, CanGetMultipleTransientServicesForSameInterface) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<IService, Service1>();
  serviceCollection.addTransient<IService, Service2>();
  serviceCollection.addTransient<IService, Service3>();
  auto serviceProvider = serviceCollection.build();
  auto service3 = serviceProvider->getTransientService<IService>();
  ASSERT_NE(nullptr, std::dynamic_pointer_cast<Service3>(service3));
  auto services = serviceProvider->getTransientServices<IService>();
  ASSERT_EQ(3, services.size());
  ASSERT_NE(nullptr, dynamic_pointer_cast<Service1>(services[0]));
  ASSERT_NE(nullptr, dynamic_pointer_cast<Service2>(services[1]));
  ASSERT_NE(nullptr, dynamic_pointer_cast<Service3>(services[2]));
}

TEST(ServiceProviderTest, QueryingNotAvailableServicesProvidesEmptyVector) {
  ServiceCollection serviceCollection;
  auto serviceProvider = serviceCollection.build();
  auto res1 = serviceProvider->getServices(std::type_index(typeid(Service1)));
  auto res2 = serviceProvider->getServices<Service1>();
  auto res3 = serviceProvider->getTransientServices<Service1>();
  ASSERT_TRUE(res1.empty());
  ASSERT_TRUE(res2.empty());
  ASSERT_TRUE(res3.empty());
}

struct ServiceRequestingVectorOfServices {
  ServiceVector<IService> _services;
  ServiceRequestingVectorOfServices(ServiceVector<IService>&& services)
      : _services(std::move(services)) {}
};

TEST(ServiceProviderTest, CanInjectMultipleServices) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<IService, Service1>();
  serviceCollection.addSingleton<IService, Service2>();
  serviceCollection.addSingleton<IService, Service3>();
  serviceCollection.addSingleton<ServiceRequestingVectorOfServices>();
  auto serviceProvider = serviceCollection.build();
  auto service =
      serviceProvider->getRequiredService<ServiceRequestingVectorOfServices>();
  ASSERT_EQ(3, service._services.size());
}

struct ServiceRequestingVectorOfTransientServices {
  std::vector<std::shared_ptr<IService>> _services;
  ServiceRequestingVectorOfTransientServices(
      std::vector<std::shared_ptr<IService>>&& services)
      : _services(std::move(services)) {}
};

TEST(ServiceProviderTest, CanInjectMultipleTransientServices) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<IService, Service1>();
  serviceCollection.addTransient<IService, Service2>();
  serviceCollection.addTransient<IService, Service3>();
  serviceCollection.addSingleton<ServiceRequestingVectorOfTransientServices>();
  auto serviceProvider = serviceCollection.build();
  auto service =
      serviceProvider
          ->getRequiredService<ServiceRequestingVectorOfTransientServices>();
  ASSERT_EQ(3, service._services.size());
}

TEST(ServiceProviderTest, CanCreateServiceFromFactory) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<IService>(
      [](IServiceProvider&) { return std::make_unique<Service1>(); });
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<IService>();
  ASSERT_NE(nullptr, service);
}

TEST(ServiceProviderTest, CanUseServiceProviderInFactory) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addSingleton([](IServiceProvider& sp) {
    return std::make_unique<ServiceWithDependency>(
        sp.getRequiredService<LeafService1>());
  });
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<ServiceWithDependency>();
  ASSERT_NE(nullptr, service);
}

TEST(ServiceProviderTest, CanCreateScopedServiceFromFactory) {
  ServiceCollection serviceCollection;
  serviceCollection.addScoped<IService>(
      [](IServiceProvider&) { return std::make_unique<Service1>(); });
  auto serviceProvider = serviceCollection.build();
  auto scope = serviceProvider->createScope();
  auto service = scope->getService<IService>();
  ASSERT_NE(nullptr, service);
}

TEST(ServiceProviderTest, CanUseScopedServiceProviderInFactory) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addScoped([](IServiceProvider& sp) {
    return std::make_unique<ServiceWithDependency>(
        sp.getRequiredService<LeafService1>());
  });
  auto serviceProvider = serviceCollection.build();
  auto scope1 = serviceProvider->createScope();
  auto scope2 = serviceProvider->createScope();
  auto service1 = scope1->getService<ServiceWithDependency>();
  auto service2 = scope2->getService<ServiceWithDependency>();
  ASSERT_NE(nullptr, service1);
  ASSERT_NE(nullptr, service2);
  ASSERT_NE(service1, service2);
  ASSERT_EQ(&service1->_leafService, &service2->_leafService);
}

TEST(ServiceProviderTest, CanCreateTransientServiceFromFactory) {
  ServiceCollection serviceCollection;
  serviceCollection.addTransient<IService>(
      [](IServiceProvider&) { return std::make_unique<Service1>(); });
  auto serviceProvider = serviceCollection.build();
  auto scope = serviceProvider->createScope();
  auto service = scope->getTransientService<IService>();
  ASSERT_NE(nullptr, service);
}

TEST(ServiceProviderTest, CanAddExistingService) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<IService>(std::make_shared<Service1>());
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getService<IService>();
  ASSERT_NE(nullptr, service);
}

struct DestructorCheck {
  int value = 1;
  ~DestructorCheck() { --value; }
};

struct CheckService {
  int value;
  CheckService(int v) : value(v) {}
};

TEST(ServiceProviderTest, KeepsFactoryLambdaCapturesInServiceProvider) {
  ServiceCollection serviceCollection;
  { 
    auto checker = std::make_shared<DestructorCheck>();
    serviceCollection.addSingleton([checker](IServiceProvider&) {
      return std::make_unique<CheckService>(checker->value);
    });
  }
  auto serviceProvider = serviceCollection.build();
  auto service = serviceProvider->getRequiredService<CheckService>();
  ASSERT_EQ(1, service.value);
}

static constexpr size_t numberOfConcurrencyTestIterations = 1000;
static constexpr size_t numberOfConcurrentIterations = 32;

TEST(ConcurrencyTest, CanHandleConcurrentGetServiceCalls) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addSingleton<LeafService2>();
  serviceCollection.addTransient<LeafService3>();
  serviceCollection.addTransient<LeafService4>();
  serviceCollection.addSingleton<ServiceWithMultipleDependencies1>();
  serviceCollection.addSingleton<ServiceWithMultipleDependencies2>();
  serviceCollection.addSingleton<ServiceWithMultipleDependencies3>();

  for (size_t i = 0; i < numberOfConcurrencyTestIterations; ++i) {
    auto serviceProvider = serviceCollection.build();
    std::atomic<bool> splinlock{false};

    auto worker = [&serviceProvider, &splinlock]() {
      while (!splinlock.load(std::memory_order_relaxed))
        ;
      auto& rootServiceA =
          serviceProvider
              ->getRequiredService<ServiceWithMultipleDependencies3>();
      auto& service2A =
          serviceProvider
              ->getRequiredService<ServiceWithMultipleDependencies2>();
      auto& service1A =
          serviceProvider
              ->getRequiredService<ServiceWithMultipleDependencies1>();
      auto& rootServiceB =
          serviceProvider
              ->getRequiredService<ServiceWithMultipleDependencies3>();
      auto& service2B =
          serviceProvider
              ->getRequiredService<ServiceWithMultipleDependencies2>();
      auto& service1B =
          serviceProvider
              ->getRequiredService<ServiceWithMultipleDependencies1>();

      return std::make_tuple(&service1A, &service2A, &rootServiceA, &service1B,
                             &service2B, &rootServiceB);
    };

    std::vector<std::future<decltype(worker())>> resultFurtures;
    std::vector<decltype(worker())> results;
    resultFurtures.reserve(numberOfConcurrentIterations);
    results.reserve(numberOfConcurrentIterations);
    for (int j = 0; j < numberOfConcurrentIterations; ++j)
      resultFurtures.emplace_back(std::async(std::launch::async, worker));
    splinlock.store(true, std::memory_order_relaxed);

    for (int j = 0; j < numberOfConcurrentIterations; ++j) {
      results.emplace_back(resultFurtures[j].get());

      ASSERT_EQ(&std::get<2>(results[j])->_serviceWithMultipleDependencies2,
                std::get<1>(results[j]));
      ASSERT_EQ(&std::get<1>(results[j])->_serviceWithMultipleDependencies1,
                std::get<0>(results[j]));
      ASSERT_EQ(&std::get<5>(results[j])->_serviceWithMultipleDependencies2,
                std::get<4>(results[j]));
      ASSERT_EQ(&std::get<4>(results[j])->_serviceWithMultipleDependencies1,
                std::get<3>(results[j]));
      ASSERT_EQ(std::get<5>(results[j]), std::get<2>(results[j]));
      ASSERT_EQ(std::get<4>(results[j]), std::get<1>(results[j]));
      ASSERT_EQ(&std::get<4>(results[j])->_serviceWithMultipleDependencies1,
                &std::get<1>(results[j])->_serviceWithMultipleDependencies1);
      if (j > 0) {
        ASSERT_EQ(std::get<0>(results[j - 1]), std::get<0>(results[j]));
        ASSERT_EQ(std::get<1>(results[j - 1]), std::get<1>(results[j]));
        ASSERT_EQ(std::get<2>(results[j - 1]), std::get<2>(results[j]));
        ASSERT_EQ(std::get<3>(results[j - 1]), std::get<3>(results[j]));
        ASSERT_EQ(std::get<4>(results[j - 1]), std::get<4>(results[j]));
        ASSERT_EQ(std::get<5>(results[j - 1]), std::get<5>(results[j]));
      }
    }
  }
}

TEST(ConcurrencyTest, CanHandleConcurrentScopedGetServiceCalls) {
  ServiceCollection serviceCollection;
  serviceCollection.addSingleton<LeafService1>();
  serviceCollection.addSingleton<LeafService2>();
  serviceCollection.addTransient<LeafService3>();
  serviceCollection.addTransient<LeafService4>();
  serviceCollection.addSingleton<ServiceWithMultipleDependencies1>();
  serviceCollection.addScoped<ServiceWithMultipleDependencies2>();
  serviceCollection.addScoped<ServiceWithMultipleDependencies3>();

  for (size_t i = 0; i < numberOfConcurrencyTestIterations; ++i) {
    auto serviceProvider = serviceCollection.build();
    auto scope1 = serviceProvider->createScope();
    auto scope2 = serviceProvider->createScope();
    std::atomic<bool> splinlock{false};

    auto worker = [&serviceProvider, &scope1, &scope2, &splinlock]() {
      while (!splinlock.load(std::memory_order_relaxed))
        ;
      auto& rootServiceA =
          scope1->getRequiredService<ServiceWithMultipleDependencies3>();
      auto& service2A =
          scope1->getRequiredService<ServiceWithMultipleDependencies2>();
      auto& service1A =
          scope1->getRequiredService<ServiceWithMultipleDependencies1>();
      auto& rootServiceB =
          scope2->getRequiredService<ServiceWithMultipleDependencies3>();
      auto& service2B =
          scope2->getRequiredService<ServiceWithMultipleDependencies2>();
      auto& service1B =
          scope2->getRequiredService<ServiceWithMultipleDependencies1>();

      return std::make_tuple(&service1A, &service2A, &rootServiceA, &service1B,
                             &service2B, &rootServiceB);
    };

    std::vector<std::future<decltype(worker())>> resultFurtures;
    std::vector<decltype(worker())> results;
    resultFurtures.reserve(numberOfConcurrentIterations);
    results.reserve(numberOfConcurrentIterations);
    for (int j = 0; j < numberOfConcurrentIterations; ++j)
      resultFurtures.emplace_back(std::async(std::launch::async, worker));
    splinlock.store(true, std::memory_order_relaxed);

    for (int j = 0; j < numberOfConcurrentIterations; ++j) {
      results.emplace_back(resultFurtures[j].get());

      ASSERT_EQ(&std::get<2>(results[j])->_serviceWithMultipleDependencies2,
                std::get<1>(results[j]));
      ASSERT_EQ(&std::get<1>(results[j])->_serviceWithMultipleDependencies1,
                std::get<0>(results[j]));
      ASSERT_EQ(&std::get<5>(results[j])->_serviceWithMultipleDependencies2,
                std::get<4>(results[j]));
      ASSERT_EQ(&std::get<4>(results[j])->_serviceWithMultipleDependencies1,
                std::get<3>(results[j]));
      ASSERT_NE(std::get<5>(results[j]), std::get<2>(results[j]));
      ASSERT_NE(std::get<4>(results[j]), std::get<1>(results[j]));
      ASSERT_EQ(&std::get<4>(results[j])->_serviceWithMultipleDependencies1,
                &std::get<1>(results[j])->_serviceWithMultipleDependencies1);
      if (j > 0) {
        ASSERT_EQ(std::get<0>(results[j - 1]), std::get<0>(results[j]));
        ASSERT_EQ(std::get<1>(results[j - 1]), std::get<1>(results[j]));
        ASSERT_EQ(std::get<2>(results[j - 1]), std::get<2>(results[j]));
        ASSERT_EQ(std::get<3>(results[j - 1]), std::get<3>(results[j]));
        ASSERT_EQ(std::get<4>(results[j - 1]), std::get<4>(results[j]));
        ASSERT_EQ(std::get<5>(results[j - 1]), std::get<5>(results[j]));
      }
    }
  }
}

}  // namespace DependencyInjectionTest
