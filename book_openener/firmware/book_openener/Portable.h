#pragma once
#include <stdint.h>
#include <stddef.h>

// Small, allocation-free replacements for the Pico driver's C++23 facilities.
// The Uno AVR core builds as C++11 and does not ship the full C++ standard library.
namespace book {
enum class Error {
        I2cFailure,
        I2cTimeout,
        Timeout,
        WrongDevice,
        InvalidArgument,
        NotInitialized,
        NotRanging,
        AlreadyRanging,
        NotReady,
        InvalidMeasurement,
    };
struct Failure { Error value; explicit Failure(Error e) : value(e) {} };
template<class T> class Result {
    T value_{};
    Error error_{Error::NotInitialized};
    bool ok_{true};
public:
    Result() = default;
    Result(T value) : value_(value) {}
    Result(Failure failure) : error_(failure.value), ok_(false) {}
    explicit operator bool() const { return ok_; }
    T& operator*() { return value_; }
    const T& operator*() const { return value_; }
    T* operator->() { return &value_; }
    const T* operator->() const { return &value_; }
    Error error() const { return error_; }
};
template<> class Result<void> {
    Error error_{Error::NotInitialized};
    bool ok_{true};
public:
    Result() = default;
    Result(Failure failure) : error_(failure.value), ok_(false) {}
    explicit operator bool() const { return ok_; }
    Error error() const { return error_; }
};
template<class T, size_t N> struct Array {
    T values[N];
    constexpr size_t size() const { return N; }
    T* data() { return values; }
    const T* data() const { return values; }
    T* begin() { return values; }
    T* end() { return values + N; }
    const T* begin() const { return values; }
    const T* end() const { return values + N; }
    T& operator[](size_t n) { return values[n]; }
    const T& operator[](size_t n) const { return values[n]; }
    T& front() { return values[0]; }
};
template<class T> class Span {
    T* pointer_;
    size_t length_;
public:
    template<class U, size_t N> Span(Array<U,N>& a) : pointer_(a.data()), length_(N) {}
    template<class U, size_t N> Span(const Array<U,N>& a) : pointer_(a.data()), length_(N) {}
    size_t size() const { return length_; }
    T* data() const { return pointer_; }
    T& operator[](size_t n) const { return pointer_[n]; }
};
template<class T> struct Identity { typedef T type; };
}
