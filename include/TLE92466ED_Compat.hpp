/**
 * @file TLE92466ED_Compat.hpp
 * @brief Compatibility layer for C++20/23 features
 * @author AI Generated Driver
 * @date 2025-10-18
 *
 * @details
 * Provides compatibility for std::expected (C++23) when compiling with C++20.
 * Uses a simple implementation for C++20, or native std::expected for C++23+.
 */

#ifndef TLE92466ED_COMPAT_HPP
#define TLE92466ED_COMPAT_HPP

#include <variant>
#include <utility>

// Check if std::expected is available (C++23)
#if __cplusplus >= 202302L && __has_include(<expected>)
    #include <expected>
    namespace tle_compat {
        using std::expected;
        using std::unexpected;
    }
#else
    // Provide a simple std::expected implementation for C++20
    namespace tle_compat {
        
        /**
         * @brief Simple unexpected type wrapper
         */
        template<typename E>
        class unexpected {
        public:
            constexpr explicit unexpected(const E& e) : error_(e) {}
            constexpr explicit unexpected(E&& e) : error_(std::move(e)) {}
            
            constexpr const E& error() const& noexcept { return error_; }
            constexpr E& error() & noexcept { return error_; }
            constexpr const E&& error() const&& noexcept { return std::move(error_); }
            constexpr E&& error() && noexcept { return std::move(error_); }
            
        private:
            E error_;
        };
        
        /**
         * @brief Simple expected implementation for C++20
         * 
         * @tparam T Success type
         * @tparam E Error type
         * 
         * This is a simplified implementation of C++23's std::expected
         * for use with C++20 compilers.
         */
        template<typename T, typename E>
        class expected {
        public:
            using value_type = T;
            using error_type = E;
            using unexpected_type = unexpected<E>;
            
            // Constructors
            constexpr expected() : storage_(T{}) {}
            constexpr expected(const T& value) : storage_(value) {}
            constexpr expected(T&& value) : storage_(std::move(value)) {}
            constexpr expected(const unexpected_type& unex) : storage_(unex.error()) {}
            constexpr expected(unexpected_type&& unex) : storage_(std::move(unex.error())) {}
            
            // Assignment
            constexpr expected& operator=(const T& value) {
                storage_ = value;
                return *this;
            }
            
            constexpr expected& operator=(T&& value) {
                storage_ = std::move(value);
                return *this;
            }
            
            constexpr expected& operator=(const unexpected_type& unex) {
                storage_ = unex.error();
                return *this;
            }
            
            // Observers
            constexpr explicit operator bool() const noexcept {
                return std::holds_alternative<T>(storage_);
            }
            
            constexpr bool has_value() const noexcept {
                return std::holds_alternative<T>(storage_);
            }
            
            constexpr const T& value() const& {
                return std::get<T>(storage_);
            }
            
            constexpr T& value() & {
                return std::get<T>(storage_);
            }
            
            constexpr const T&& value() const&& {
                return std::get<T>(std::move(storage_));
            }
            
            constexpr T&& value() && {
                return std::get<T>(std::move(storage_));
            }
            
            constexpr const E& error() const& {
                return std::get<E>(storage_);
            }
            
            constexpr E& error() & {
                return std::get<E>(storage_);
            }
            
            constexpr const E&& error() const&& {
                return std::get<E>(std::move(storage_));
            }
            
            constexpr E&& error() && {
                return std::get<E>(std::move(storage_));
            }
            
            // Value access
            constexpr const T& operator*() const& { return value(); }
            constexpr T& operator*() & { return value(); }
            constexpr const T&& operator*() const&& { return std::move(value()); }
            constexpr T&& operator*() && { return std::move(value()); }
            
            constexpr const T* operator->() const { return &value(); }
            constexpr T* operator->() { return &value(); }
            
            // Value or default
            template<typename U>
            constexpr T value_or(U&& default_value) const& {
                return has_value() ? value() : static_cast<T>(std::forward<U>(default_value));
            }
            
            template<typename U>
            constexpr T value_or(U&& default_value) && {
                return has_value() ? std::move(value()) : static_cast<T>(std::forward<U>(default_value));
            }
            
        private:
            std::variant<T, E> storage_;
        };
        
        /**
         * @brief Specialization for void value type
         */
        template<typename E>
        class expected<void, E> {
        public:
            using value_type = void;
            using error_type = E;
            using unexpected_type = unexpected<E>;
            
            // Constructors
            constexpr expected() : has_value_(true), error_() {}
            constexpr expected(const unexpected_type& unex) : has_value_(false), error_(unex.error()) {}
            constexpr expected(unexpected_type&& unex) : has_value_(false), error_(std::move(unex.error())) {}
            
            // Observers
            constexpr explicit operator bool() const noexcept {
                return has_value_;
            }
            
            constexpr bool has_value() const noexcept {
                return has_value_;
            }
            
            constexpr const E& error() const& {
                return error_;
            }
            
            constexpr E& error() & {
                return error_;
            }
            
            constexpr const E&& error() const&& {
                return std::move(error_);
            }
            
            constexpr E&& error() && {
                return std::move(error_);
            }
            
        private:
            bool has_value_;
            E error_;
        };
        
    } // namespace tle_compat
#endif

// Make tle_compat types available globally (but not in std to avoid conflicts)
using tle_compat::expected;
using tle_compat::unexpected;

#endif // TLE92466ED_COMPAT_HPP
