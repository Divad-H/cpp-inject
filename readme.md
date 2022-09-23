# Simple Dependency Injection for c++

![build workflow](https://github.com/Divad-H/cpp-inject/actions/workflows/msbuild.yml/badge.svg)

This is a header-only library that implements simple dependency injection. It is inspired by the [C# dependency injection from Microsoft](https://docs.microsoft.com/en-us/dotnet/core/extensions/dependency-injection).

## Basic usage

* Create a service collection
* Register services in that collection
* Build a service provider from the service collection
* Get services from the service provider

## Service life-time

There are 3 types of service life-times
1. Transient
2. Singleton
3. Scoped

### Transient

Transient services are injected as shared_ptrs.
Each time the service is requested, a new instance is created.
Their life-time is controlled by the consuming service.

### Singleton

Singleton services are injected as references or const references.
When the service is first requested, a single instance is created (one per service provider).
Their life-time is bound to the life-time of the service provider.

In multi threaded applications, it is recommended to implement singleton services thread-safe.

### Scoped

Scoped services are injected as references or const references.
Their life-time is bound to the life-time of a service scope.
A service scope can be created by a service provider.
Scoped services should not be injected into singleton services. They should not be requested from the main service provider, because they would effectively become singleton services.

In multi threaded applications, it is recommended not to access scoped services concurrently.

## Thread safety

The member functions of the service provider and service scope can be accessed concurrently.
