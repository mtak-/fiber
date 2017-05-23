
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <chrono>
#include <mutex>

#include <boost/assert.hpp>

#include "boost/fiber/algo/round_robin.hpp"
#include "boost/fiber/context.hpp"
#include "boost/fiber/exceptions.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
scheduler::release_terminated_() noexcept {
    context * ctx = nullptr;
    while ( nullptr != ( ctx = terminated_queue_.pop() ) ) {
        BOOST_ASSERT( ctx->is_context( type::worker_context) );
        BOOST_ASSERT( ! ctx->is_context( type::pinned_context) );
        BOOST_ASSERT( this == ctx->get_scheduler() );
        BOOST_ASSERT( ctx->is_resumable() );
        BOOST_ASSERT( ctx->wait_queue_.empty() );
        BOOST_ASSERT( ctx->terminated_);
        // if last reference, e.g. fiber::join() or fiber::detach()
        // have been already called, this will call ~context(),
        // the context is automatically removeid from worker-queue
        intrusive_ptr_release( ctx);
    }
}

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
void
scheduler::remote_ready2ready_() noexcept {
    remote_ready_queue_type tmp;
    detail::spinlock_lock lk{ remote_ready_splk_ };
    remote_ready_queue_.swap( tmp);
    lk.unlock();
    // get context from remote ready-queue
    context * ctx = nullptr;
    while ( nullptr != ( ctx = tmp.pop() ) ) {
        // store context in local queues
        schedule( ctx);
    }
}
#endif

void
scheduler::sleep2ready_() noexcept {
    // move context which the deadline has reached
    // to ready-queue
    // sleep-queue is sorted (ascending)
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    context * ctx = nullptr;
    while ( nullptr != ( ctx = sleep_queue_.pop( now) ) ) {
        // dipatcher context must never be pushed to sleep-queue
        BOOST_ASSERT( ! ctx->is_context( type::dispatcher_context) );
        // push new context to ready-queue
        algo_->awakened( ctx);
    }
}

scheduler::scheduler() noexcept :
    algo_{ new algo::round_robin() } {
}

scheduler::~scheduler() {
    BOOST_ASSERT( nullptr != main_ctx_);
    BOOST_ASSERT( nullptr != dispatcher_ctx_.get() );
    BOOST_ASSERT( context::active() == main_ctx_);
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    // protect for concurrent access
    // required because main-context might have been
    // signaled from remote and algorithm::notify()
    // must be called fro mremote too
    detail::spinlock_lock lk{ remote_ready_splk_ };
#endif
    // signal dispatcher-context termination
    shutdown_ = true;
    // resume pending fibers
    // by joining dispatcher-context
    dispatcher_ctx_->join();
    // no context' in worker-queue
    BOOST_ASSERT( worker_queue_.empty() );
    BOOST_ASSERT( terminated_queue_.empty() );
    BOOST_ASSERT( sleep_queue_.empty() );
    // set active context to nullptr
    context::reset_active();
    // deallocate dispatcher-context
    dispatcher_ctx_.reset();
    // set main-context to nullptr
    main_ctx_ = nullptr;
}

boost::context::continuation
scheduler::dispatch() noexcept {
    BOOST_ASSERT( context::active() == dispatcher_ctx_);
    for (;;) {
        if ( shutdown_) {
            // notify sched-algorithm about termination
            algo_->notify();
            if ( worker_queue_.empty() ) {
                break;
            }
        }
        // release terminated context'
        release_terminated_();
#if ! defined(BOOST_FIBERS_NO_ATOMICS)
        // get context' from remote ready-queue
        remote_ready2ready_();
#endif
        // get sleeping context'
        sleep2ready_();
        // get next ready context
        context * ctx = algo_->pick_next();
        if ( nullptr != ctx) {
            BOOST_ASSERT( ctx->is_resumable() );
            // push dispatcher-context to ready-queue
            // so that ready-queue never becomes empty
            ctx->resume( dispatcher_ctx_.get() );
            BOOST_ASSERT( context::active() == dispatcher_ctx_.get() );
        } else {
            // no ready context, wait till signaled
            // set deadline to highest value
            std::chrono::steady_clock::time_point suspend_time =
                    (std::chrono::steady_clock::time_point::max)();
            // get lowest deadline from sleep-queue
            suspend_time = sleep_queue_.lowest_deadline();
            // no ready context, wait till signaled
            algo_->suspend_until( suspend_time);
        }
    }
    // release termianted context'
    release_terminated_();
    // return to main-context
    return main_ctx_->suspend_with_cc();
}

void
scheduler::schedule( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    // remove context ctx from sleep-queue
    // (might happen if blocked in timed_mutex::try_lock_until())
    // unlink it from sleep-queue
    sleep_queue_.unlink( ctx);
    // push new context to ready-queue
    algo_->awakened( ctx);
}

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
void
scheduler::schedule_from_remote( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    // another thread might signal the main-context of this thread
    BOOST_ASSERT( ! ctx->is_context( type::dispatcher_context) );
    BOOST_ASSERT( this == ctx->get_scheduler() );
    // protect for concurrent access
    detail::spinlock_lock lk{ remote_ready_splk_ };
    BOOST_ASSERT( ! shutdown_);
    BOOST_ASSERT( nullptr != main_ctx_);
    BOOST_ASSERT( nullptr != dispatcher_ctx_.get() );
    // push new context to remote ready-queue
    remote_ready_queue_.push( ctx);
    // notify scheduler
    algo_->notify();
}
#endif

boost::context::continuation
scheduler::terminate( detail::spinlock_lock & lk, context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( context::active() == ctx);
    BOOST_ASSERT( this == ctx->get_scheduler() );
    BOOST_ASSERT( ctx->is_context( type::worker_context) );
    BOOST_ASSERT( ! ctx->is_context( type::pinned_context) );
    BOOST_ASSERT( ctx->wait_queue_.empty() );
    // store the terminated fiber in the terminated-queue
    // the dispatcher-context will call
    // intrusive_ptr_release( ctx);
    terminated_queue_.push( ctx);
    // remove from the worker-queue
    worker_queue_.unlink( ctx);
    // release lock
    lk.unlock();
    // resume another fiber
    return algo_->pick_next()->suspend_with_cc();
}

void
scheduler::yield( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( context::active() == ctx);
    BOOST_ASSERT( ctx->is_context( type::worker_context) || ctx->is_context( type::main_context) );
    // resume another fiber
    algo_->pick_next()->resume( ctx);
}

bool
scheduler::wait_until( context * ctx,
                       std::chrono::steady_clock::time_point const& sleep_tp) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( context::active() == ctx);
    BOOST_ASSERT( ctx->is_context( type::worker_context) || ctx->is_context( type::main_context) );
    // push context to sleep-queue
    sleep_queue_.push( ctx, sleep_tp);
    // resume another context
    algo_->pick_next()->resume();
    // context has been resumed
    // check if deadline has reached
    return std::chrono::steady_clock::now() < sleep_tp;
}

bool
scheduler::wait_until( context * ctx,
                       std::chrono::steady_clock::time_point const& sleep_tp,
                       detail::spinlock_lock & lk) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( context::active() == ctx);
    BOOST_ASSERT( ctx->is_context( type::worker_context) || ctx->is_context( type::main_context) );
    // push active context to sleep-queue
    sleep_queue_.push( ctx, sleep_tp);
    // resume another context
    algo_->pick_next()->resume( lk);
    // context has been resumed
    // check if deadline has reached
    return std::chrono::steady_clock::now() < sleep_tp;
}

void
scheduler::suspend() noexcept {
    // resume another context
    algo_->pick_next()->resume();
}

void
scheduler::suspend( detail::spinlock_lock & lk) noexcept {
    // resume another context
    algo_->pick_next()->resume( lk);
}

bool
scheduler::has_ready_fibers() const noexcept {
    return algo_->has_ready_fibers();
}

void
scheduler::set_algo( algo::algorithm::ptr_t algo) noexcept {
    // move remaining cotnext in current scheduler to new one
    while ( algo_->has_ready_fibers() ) {
        algo->awakened( algo_->pick_next() );
    }
    algo_ = std::move( algo);
}

void
scheduler::attach_main_context( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    // main-context represents the execution context created
    // by the system, e.g. main()- or thread-context
    // should not be in worker-queue
    main_ctx_ = ctx;
    main_ctx_->scheduler_ = this;
}

void
scheduler::attach_dispatcher_context( intrusive_ptr< context > ctx) noexcept {
    BOOST_ASSERT( ctx);
    // dispatcher context has to handle
    //    - remote ready context'
    //    - sleeping context'
    //    - extern event-loops
    //    - suspending the thread if ready-queue is empty (waiting on external event)
    // should not be in worker-queue
    dispatcher_ctx_.swap( ctx);
    // add dispatcher-context to ready-queue
    // so it is the first element in the ready-queue
    // if the main context tries to suspend the first time
    // the dispatcher-context is resumed and
    // scheduler::dispatch() is executed
    dispatcher_ctx_->scheduler_ = this;
    algo_->awakened( dispatcher_ctx_.get() );
}

void
scheduler::attach_worker_context( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( nullptr == ctx->get_scheduler() );
    worker_queue_.push( ctx);
    ctx->scheduler_ = this;
}

void
scheduler::detach_worker_context( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->is_context( type::pinned_context) );
    worker_queue_.unlink( ctx);
    ctx->scheduler_ = nullptr;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
