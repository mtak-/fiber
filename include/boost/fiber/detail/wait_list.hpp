
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_WAITLIST_H
#define BOOST_FIBERS_DETAIL_WAITLIST_H

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Ctx >
class wait_list {
private:
    Ctx *   head_{ nullptr };
    Ctx *   tail_{ nullptr };

public:
    wait_list() noexcept = default;

    bool empty() const noexcept {
        return nullptr == head_;
    }

    void push( Ctx * ctx) {
        BOOST_ASSERT( nullptr == ctx->wait_prev);
        BOOST_ASSERT( nullptr == ctx->wait_next);
        if ( nullptr == head_) {
            head_ = tail_ = ctx;
        } else {
            ctx->wait_prev = tail_;
            ctx->wait_next = tail_->wait_next;
            tail_->wait_next = ctx;
            tail_ = ctx;
        }
    }

    Ctx * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        Ctx * ctx = head_;
        head_ = ctx->wait_next;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->wait_prev = nullptr;
        }
        ctx->wait_prev = nullptr;
        ctx->wait_next = nullptr;
        return ctx;
    }

    bool is_linked( Ctx * ctx) const noexcept {
        return nullptr != ctx->wait_prev || nullptr != ctx->wait_next ||
            head_ == ctx || tail_ == ctx;
    }

    void unlink( Ctx * ctx) {
        if ( ! is_linked( ctx) ) {
            return;
        }
        if ( nullptr != ctx->wait_prev)  {
            ctx->wait_prev->wait_next = ctx->wait_next;
        } else {
            head_ = ctx->wait_next;
        }
        if ( nullptr != ctx->wait_next) {
            ctx->wait_next->wait_prev = ctx->wait_prev;
        } else {
            tail_ = ctx->wait_prev;
        }
        ctx->wait_prev = nullptr;
        ctx->wait_next = nullptr;
    }

    void swap( wait_list & other) {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WAITLIST_H
