
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBER_DETAIL_HLE_H
#define BOOST_FIBER_DETAIL_HLE_H

#include <atomic>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename StorageType >
static BOOST_FORCEINLINE
StorageType hle_load( volatile StorageType & storage) noexcept {
    return storage;
}

template< typename StorageType >
static BOOST_FORCEINLINE
void hle_store( volatile StorageType & storage, StorageType desired) noexcept {
    __asm__ __volatile__
    (
        "xrelease ; mov %1, %0"
        : "=m" (storage)
        : "q" (desired)
        : "memory"
    );
}

template< typename StorageType >
static BOOST_FORCEINLINE
StorageType hle_exchange( volatile StorageType & storage, StorageType desired) noexcept {
    __asm__ __volatile__
    (
        // "When a memory operand is used with the XCHG instruction,
        // the processor’s LOCK signal is automatically asserted.",
        // Intel® 64 and IA-32 Architectures Developer's Manual
        "xacquire ; xchg %0, %1"
        : "+q" (desired), "+m" (storage)
        :
        : "memory"
    );
    return desired;
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBER_DETAIL_HLE_H
