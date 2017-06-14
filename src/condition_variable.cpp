
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/condition_variable.hpp"

#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
condition_variable_any::notify_one() noexcept {
    // get one context' from wait-queue
loop:
    detail::spinlock_lock lk{ wait_queue_splk_ };
    if ( wait_queue_.empty() ) {
        return;
    }
    context * ctx = & wait_queue_.front();
    if ( nullptr != ctx->wait_splk_) {
        BOOST_ASSERT( nullptr != ctx->sleep_splk_);
        BOOST_ASSERT( & wait_queue_splk_ == ctx->wait_splk_);
        if ( ! ctx->sleep_splk_->try_lock() ) {
            lk.unlock();
            goto loop;
        }
        ctx->sleep_unlink();
        ctx->sleep_splk_ = nullptr;
        ctx->wait_splk_ = nullptr;
    }
    BOOST_ASSERT( nullptr == ctx->sleep_splk_);
    BOOST_ASSERT( nullptr == ctx->wait_splk_);
    wait_queue_.pop_front();
    // notify context
    context::active()->schedule( ctx);
}

void
condition_variable_any::notify_all() noexcept {
    // get all context' from wait-queue
loop:
    detail::spinlock_lock lk{ wait_queue_splk_ };
    // notify all context'
    while ( ! wait_queue_.empty() ) {
        context * ctx = & wait_queue_.front();
        if ( nullptr != ctx->wait_splk_) {
            BOOST_ASSERT( nullptr != ctx->sleep_splk_);
            BOOST_ASSERT( & wait_queue_splk_ == ctx->wait_splk_);
            if ( ! ctx->sleep_splk_->try_lock() ) {
                lk.unlock();
                goto loop;
            }
            ctx->sleep_unlink();
            ctx->sleep_splk_ = nullptr;
            ctx->wait_splk_ = nullptr;
        }
        BOOST_ASSERT( nullptr == ctx->sleep_splk_);
        BOOST_ASSERT( nullptr == ctx->wait_splk_);
        wait_queue_.pop_front();
        context::active()->schedule( ctx);
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
