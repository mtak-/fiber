
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONTEXT_H
#define BOOST_FIBERS_CONTEXT_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#if defined(BOOST_NO_CXX17_STD_APPLY)
#include <boost/context/detail/apply.hpp>
#endif
#include <boost/context/continuation.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/parent_from_member.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/slist.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/data.hpp>
#include <boost/fiber/detail/decay_copy.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/wait_list.hpp>
#include <boost/fiber/detail/wrap.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/fixedsize_stack.hpp>
#include <boost/fiber/policy.hpp>
#include <boost/fiber/properties.hpp>
#include <boost/fiber/segmented_stack.hpp>
#include <boost/fiber/type.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace boost {
namespace fibers {

class context;
class fiber;
class scheduler;

struct main_context_t {};
const main_context_t main_context{};

struct dispatcher_context_t {};
const dispatcher_context_t dispatcher_context{};

struct worker_context_t {};
const worker_context_t worker_context{};

class BOOST_FIBERS_DECL context {
public:
    typedef detail::wait_list< context >    wait_queue_t;

private:
    friend class scheduler;

    struct fss_data {
        void                                *   vp{ nullptr };
        detail::fss_cleanup_function::ptr_t     cleanup_function{};

        fss_data() noexcept {
        }

        fss_data( void * vp_,
                  detail::fss_cleanup_function::ptr_t const& fn) noexcept :
            vp( vp_),
            cleanup_function( fn) {
            BOOST_ASSERT( cleanup_function);
        }

        void do_cleanup() {
            ( * cleanup_function)( vp);
        }
    };

    typedef std::map< uintptr_t, fss_data >     fss_data_t;

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    std::atomic< std::size_t >                  use_count_{ 0 };
#else
    std::size_t                                 use_count_{ 0 };
#endif
public:
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    context                                 *   remote_ready_prev{ nullptr };
    context                                 *   remote_ready_next{ nullptr };
#endif
    detail::spinlock                            splk_{};
    bool                                        terminated_{ false };
    context                                 *   wait_prev{ nullptr };
    context                                 *   wait_next{ nullptr };
    scheduler                               *   scheduler_{ nullptr };
    fss_data_t                                  fss_data_{};
    context                                 *   sleep_prev{ nullptr };
    context                                 *   sleep_next{ nullptr };
    context                                 *   ready_prev{ nullptr };
    context                                 *   ready_next{ nullptr };
    context                                 *   terminated_prev{ nullptr };
    context                                 *   terminated_next{ nullptr };
    context                                 *   worker_prev{ nullptr };
    context                                 *   worker_next{ nullptr };
    wait_queue_t                                wait_queue_{};
    boost::context::continuation                c_;
    fiber_properties                        *   properties_{ nullptr };
    std::chrono::steady_clock::time_point       tp_{ (std::chrono::steady_clock::time_point::max)() };
    type                                        type_;
    launch                                      policy_;

private:
    void resume_( detail::data_t &) noexcept;
    void schedule_( context *) noexcept;

    template< typename Fn, typename Tpl >
    boost::context::continuation
    run_( boost::context::continuation && c, Fn && fn_, Tpl && tpl_) noexcept {
        {
            // fn and tpl must be destroyed before calling terminate()
            typename std::decay< Fn >::type fn = std::forward< Fn >( fn_);
            typename std::decay< Tpl >::type tpl = std::forward< Tpl >( tpl_);
            c = c.resume();
            detail::data_t * dp = c.get_data< detail::data_t * >();
            // update contiunation of calling fiber
            dp->from->c_ = std::move( c);
            if ( nullptr != dp->lk) {
                dp->lk->unlock();
            } else if ( nullptr != dp->ctx) {
                active()->schedule_( dp->ctx);
            }
#if defined(BOOST_NO_CXX17_STD_APPLY)
           boost::context::detail::apply( std::move( fn), std::move( tpl) );
#else
           std::apply( std::move( fn), std::move( tpl) );
#endif
        }
        // terminate context
        return terminate();
    }

public:
    class id {
    private:
        context  *   impl_{ nullptr };

    public:
        id() = default;

        explicit id( context * impl) noexcept :
            impl_{ impl } {
        }

        bool operator==( id const& other) const noexcept {
            return impl_ == other.impl_;
        }

        bool operator!=( id const& other) const noexcept {
            return impl_ != other.impl_;
        }

        bool operator<( id const& other) const noexcept {
            return impl_ < other.impl_;
        }

        bool operator>( id const& other) const noexcept {
            return other.impl_ < impl_;
        }

        bool operator<=( id const& other) const noexcept {
            return ! ( * this > other);
        }

        bool operator>=( id const& other) const noexcept {
            return ! ( * this < other);
        }

        template< typename charT, class traitsT >
        friend std::basic_ostream< charT, traitsT > &
        operator<<( std::basic_ostream< charT, traitsT > & os, id const& other) {
            if ( nullptr != other.impl_) {
                return os << other.impl_;
            } else {
                return os << "{not-valid}";
            }
        }

        explicit operator bool() const noexcept {
            return nullptr != impl_;
        }

        bool operator!() const noexcept {
            return nullptr == impl_;
        }
    };

    static context * active() noexcept;

    static void reset_active() noexcept;

    // main fiber context
    explicit context( main_context_t) noexcept;

    // dispatcher fiber context
    context( dispatcher_context_t, boost::context::preallocated const&,
             default_stack const&, scheduler *);

    // worker fiber context
    template< typename StackAlloc,
              typename Fn,
              typename Tpl
    >
    context( worker_context_t,
             launch policy,
             boost::context::preallocated palloc, StackAlloc salloc,
             Fn && fn, Tpl && tpl) :
        use_count_{ 1 }, // fiber instance or scheduler owner
        c_{},
        type_{ type::worker_context },
        policy_{ policy } {
# if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
            c_ = boost::context::callcc(
                    std::allocator_arg, palloc, salloc,
                      detail::wrap(
                          [this]( typename std::decay< Fn >::type & fn, typename std::decay< Tpl >::type & tpl,
                                  boost::context::continuation && c) mutable noexcept {
                                return run_( std::forward< boost::context::continuation >( c), std::move( fn), std::move( tpl) );
                          },
                          std::forward< Fn >( fn),
                          std::forward< Tpl >( tpl) ) );
# else
            c_ = boost::context::callcc(
                    std::allocator_arg, palloc, salloc,
                    [this,fn=detail::decay_copy( std::forward< Fn >( fn) ),tpl=std::forward< Tpl >( tpl)]
                    (boost::context::continuation && c) mutable noexcept {
                          return run_( std::forward< boost::context::continuation >( c), std::move( fn), std::move( tpl) );
                    });
# endif
    }

    context( context const&) = delete;
    context & operator=( context const&) = delete;

    friend bool
    operator==( context const& lhs, context const& rhs) noexcept {
        return & lhs == & rhs;
    }

    virtual ~context();

    scheduler * get_scheduler() const noexcept {
        return scheduler_;
    }

    id get_id() const noexcept;

    bool is_resumable() const noexcept {
        if ( c_) return true;
        else return false;
    }

    void resume() noexcept;
    void resume( detail::spinlock_lock &) noexcept;
    void resume( context *) noexcept;

    void suspend() noexcept;
    void suspend( detail::spinlock_lock &) noexcept;

    boost::context::continuation suspend_with_cc() noexcept;
    boost::context::continuation terminate() noexcept;

    void join();

    void yield() noexcept;

    bool wait_until( std::chrono::steady_clock::time_point const&) noexcept;
    bool wait_until( std::chrono::steady_clock::time_point const&,
                     detail::spinlock_lock &) noexcept;

    void schedule( context *) noexcept;

    bool is_context( type t) const noexcept {
        return type::none != ( type_ & t);
    }

    void * get_fss_data( void const * vp) const;

    void set_fss_data(
        void const * vp,
        detail::fss_cleanup_function::ptr_t const& cleanup_fn,
        void * data,
        bool cleanup_existing);

    void set_properties( fiber_properties * props) noexcept;

    fiber_properties * get_properties() const noexcept {
        return properties_;
    }

    launch get_policy() const noexcept {
        return policy_;
    }

    void attach( context *) noexcept;
    void detach() noexcept;

    friend void intrusive_ptr_add_ref( context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        ctx->use_count_.fetch_add( 1, std::memory_order_relaxed);
    }

    friend void intrusive_ptr_release( context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        if ( 1 == ctx->use_count_.fetch_sub( 1, std::memory_order_release) ) {
            std::atomic_thread_fence( std::memory_order_acquire);
            boost::context::continuation c = std::move( ctx->c_);
            // destruct context
            ctx->~context();
            // deallocated stack
            c.resume( nullptr);
        }
    }
};

inline
bool operator<( context const& l, context const& r) noexcept {
    return l.get_id() < r.get_id();
}

template< typename StackAlloc, typename Fn, typename ... Args >
static intrusive_ptr< context > make_worker_context( launch policy,
                                                     StackAlloc salloc,
                                                     Fn && fn, Args && ... args) {
    auto sctx = salloc.allocate();
    BOOST_ASSERT( ( sizeof( context) + 2048) < sctx.size); // stack at least of 2kB
	const std::size_t offset = sizeof( context) + 63; 
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( offset) )
            & ~ static_cast< uintptr_t >( 0xff) );
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sctx.size) );
    const std::size_t size = reinterpret_cast< uintptr_t >( storage) - reinterpret_cast< uintptr_t >( stack_bottom);
    // placement new of context on top of fiber's stack
    return intrusive_ptr< context >{ 
            new ( storage) context{
                worker_context,
                policy,
                boost::context::preallocated{ storage, size, sctx },
                salloc,
                std::forward< Fn >( fn),
                std::make_tuple( std::forward< Args >( args) ... ) } };
}

}}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CONTEXT_H
