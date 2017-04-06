
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_SPINLOCK_TTAS_RTM_H
#define BOOST_FIBERS_SPINLOCK_TTAS_RTM_H

#include <atomic>
#include <chrono>
#include <random>
#include <thread>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/cpu_relax.hpp>
#include <boost/fiber/detail/rtm.hpp>

// based on informations from:
// https://software.intel.com/en-us/articles/benefitting-power-and-performance-sleep-loops
// https://software.intel.com/en-us/articles/long-duration-spin-wait-loops-on-hyper-threading-technology-enabled-intel-processors

#if BOOST_COMP_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

namespace boost {
namespace fibers {
namespace detail {

class spinlock_ttas_rtm {
private:
    enum class spinlock_status {
        locked = 0,
        unlocked
    };

    std::atomic< spinlock_status >              state_{ spinlock_status::unlocked };

    void fallback_lock_() noexcept {
        std::size_t collisions = 0 ;
        std::minstd_rand generator;
        for (;;) {
            // avoid using multiple pause instructions for a delay of a specific cycle count
            // the delay of cpu_relax() (pause on Intel) depends on the processor family
            // the cycle count can not guaranteed from one system to the next
            // -> check the shared variable 'state_' in between each cpu_relax() to prevent
            //    unnecessarily long delays on some systems
            std::size_t count = 0;
            // test shared variable 'status_'
            // first access to 'state_' -> chache miss
            // sucessive acccess to 'state_' -> cache hit
            // if 'state_' was released by other fiber
            // cached 'state_' is invalidated -> cache miss
            while ( spinlock_status::locked == state_.load( std::memory_order_relaxed) ) {
                if ( BOOST_FIBERS_SPIN_BEFORE_SLEEP0 > count) {
                    ++count;
                    // give CPU a hint that this thread is in a "spin-wait" loop
                    // delays the next instruction's execution for a finite period of time (depends on processor family)
                    // the CPU is not under demand, parts of the pipeline are no longer being used
                    // -> reduces the power consumed by the CPU
                    // -> prevent pipeline stalls
                    cpu_relax();
                } else if ( BOOST_FIBERS_SPIN_BEFORE_YIELD > count) {
                    // std::this_thread::sleep_for( 0us) has a fairly long instruction path length,
                    // combined with an expensive ring3 to ring 0 transition costing about 1000 cycles
                    // std::this_thread::sleep_for( 0us) lets give up this_thread the remaining part of its time slice
                    // if and only if a thread of equal or greater priority is ready to run
                    static constexpr std::chrono::microseconds us0{ 0 };
                    std::this_thread::sleep_for( us0);
#if 0
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for( 0ms);
#endif
                } else {
                    // std::this_thread::yield() allows this_thread to give up the remaining part of its time slice,
                    // but only to another thread on the same processor
                    // instead of constant checking, a thread only checks if no other useful work is pending
                    std::this_thread::yield();
                }
            }
            // test-and-set shared variable 'status_'
            // everytime 'status_' is signaled over the bus, even if the test failes
            if ( spinlock_status::locked == state_.exchange( spinlock_status::locked, std::memory_order_acquire) ) {
                // spinlock now contended
                // utilize 'Binary Exponential Backoff' algorithm
                // linear_congruential_engine is a random number engine based on Linear congruential generator (LCG)
                std::uniform_int_distribution< std::size_t > distribution{
                    0, static_cast< std::size_t >( 1) << collisions };
                const std::size_t z = distribution( generator);
                ++collisions;
                for ( std::size_t i = 0; i < z; ++i) {
                    // -> reduces the power consumed by the CPU
                    // -> prevent pipeline stalls
                    cpu_relax();
                }
            } else {
                // success, thread has acquired the lock
                break;
            }
        }
    }

public:
    spinlock_ttas_rtm() noexcept = default;

    spinlock_ttas_rtm( spinlock_ttas_rtm const&) = delete;
    spinlock_ttas_rtm & operator=( spinlock_ttas_rtm const&) = delete;

    void lock() noexcept {
        std::size_t collisions = 0 ;
        std::minstd_rand generator;
        for ( std::size_t retries = 0; retries < BOOST_FIBERS_RETRY_THRESHOLD; ++retries) {
            std::uint32_t status;
            if ( rtm_status::success == ( status = rtm_begin() ) ) {
                // add lock to read-set
                if ( spinlock_status::unlocked == state_.load( std::memory_order_acquire) ) {
                    // lock is free, enter critical section
                    return;
                }
                // lock was acquired by another thread
                // explicit abort of transaction with abort argument 'lock not free'
                rtm_abort_lock_not_free();
            }
            // transaction aborted
            if ( rtm_status::none != (status & rtm_status::may_retry) ) {
                // transaction may succeed on a retry
                cpu_relax();
            } else if ( rtm_status::none != (status & rtm_status::memory_conflict) ) {
                if ( BOOST_FIBERS_COLLISION_THRESHOLD > collisions) {
                    // another logical processor conflicted with a memory address that was part or the read-/write-set
                    // at least two logical processors must have started a transaction the same time
                    std::uniform_int_distribution< std::size_t > distribution{
                        0, static_cast< std::size_t >( 1) << collisions };
                    const std::size_t z = distribution( generator);
                    ++collisions;
                    for ( std::size_t i = 0; i < z; ++i) {
                        cpu_relax();
                    }
                } else {
                    std::this_thread::yield();
                }
            } else if ( rtm_status::none != (status & rtm_status::explicit_abort) ) {
                // another logical processor has acquired the lock
                // wait till lock becomes free again
                std::size_t count = 0;
                while ( spinlock_status::locked == state_.load( std::memory_order_relaxed) ) {
                    if ( BOOST_FIBERS_SPIN_BEFORE_SLEEP0 > count) {
                        ++count;
                        cpu_relax();
                    } else if ( BOOST_FIBERS_SPIN_BEFORE_YIELD > count) {
                        ++count; 
                        static constexpr std::chrono::microseconds us0{ 0 };
                        std::this_thread::sleep_for( us0);
#if 0
                        using namespace std::chrono_literals;
                        std::this_thread::sleep_for( 0ms);
#endif
                    } else {
                        std::this_thread::yield();
                    }
                }
            } else {
                // transaction aborted due: 
                //  - internal buffer to track transactional state overflowed
                //  - debug exception or breakpoint exception was hit
                //  - abort during execution of nested transactions (max nesting limit exceeded)
                // -> use fallback path
                break;
            }
        }
        fallback_lock_();
    }

    void unlock() noexcept {
        if ( spinlock_status::unlocked == state_.load( std::memory_order_acquire) ) {
            rtm_end();
        } else {
            state_.store( spinlock_status::unlocked, std::memory_order_release);
        }
    }
};

}}}

#if BOOST_COMP_CLANG
#pragma clang diagnostic pop
#endif

#endif // BOOST_FIBERS_SPINLOCK_TTAS_RTM_H
