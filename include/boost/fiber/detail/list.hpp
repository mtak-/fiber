
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_LIST_H
#define BOOST_FIBERS_DETAIL_LIST_H

#include <chrono>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class ready_list {
private:
    context *   head_{ nullptr };
    context *   tail_{ nullptr };

public:
    ready_list() noexcept = default;

    bool empty() const noexcept {
        return nullptr == head_;
    }

    void push( context * ctx) {
        BOOST_ASSERT( nullptr == ctx->ready_prev);
        BOOST_ASSERT( nullptr == ctx->ready_next);
        if ( nullptr == head_) {
            head_ = tail_ = ctx;
        } else {
            ctx->ready_prev = tail_;
            ctx->ready_next = tail_->ready_next;
            tail_->ready_next = ctx;
            tail_ = ctx;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * ctx = head_;
        head_ = ctx->ready_next;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->ready_prev = nullptr;
        }
        ctx->ready_prev = nullptr;
        ctx->ready_next = nullptr;
        return ctx;
    }

    bool is_linked( context * ctx) const noexcept {
        return nullptr != ctx->ready_prev || nullptr != ctx->ready_next ||
            head_ == ctx || tail_ == ctx;
    }

    void unlink( context * ctx) {
        if ( ! is_linked( ctx) ) {
            return;
        }
        if ( nullptr != ctx->ready_prev)  {
            ctx->ready_prev->ready_next = ctx->ready_next;
        } else {
            head_ = ctx->ready_next;
        }
        if ( nullptr != ctx->ready_next) {
            ctx->ready_next->ready_prev = ctx->ready_prev;
        } else {
            tail_ = ctx->ready_prev;
        }
        ctx->ready_prev = nullptr;
        ctx->ready_next = nullptr;
    }
};

class worker_list {
private:
    context *   head_{ nullptr };
    context *   tail_{ nullptr };

public:
    worker_list() noexcept = default;

    bool empty() const noexcept {
        return nullptr == head_;
    }

    void push( context * ctx) {
        BOOST_ASSERT( nullptr == ctx->worker_prev);
        BOOST_ASSERT( nullptr == ctx->worker_next);
        if ( nullptr == head_) {
            head_ = tail_ = ctx;
        } else {
            ctx->worker_prev = tail_;
            ctx->worker_next = tail_->worker_next;
            tail_->worker_next = ctx;
            tail_ = ctx;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * ctx = head_;
        head_ = ctx->worker_next;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->worker_prev = nullptr;
        }
        ctx->worker_prev = nullptr;
        ctx->worker_next = nullptr;
        return ctx;
    }

    bool is_linked( context * ctx) const noexcept {
        return nullptr != ctx->worker_prev || nullptr != ctx->worker_next ||
            head_ == ctx || tail_ == ctx;
    }

    void unlink( context * ctx) {
        if ( ! is_linked( ctx) ) {
            return;
        }
        if ( nullptr != ctx->worker_prev)  {
            ctx->worker_prev->worker_next = ctx->worker_next;
        } else {
            head_ = ctx->worker_next;
        }
        if ( nullptr != ctx->worker_next) {
            ctx->worker_next->worker_prev = ctx->worker_prev;
        } else {
            tail_ = ctx->worker_prev;
        }
        ctx->worker_prev = nullptr;
        ctx->worker_next = nullptr;
    }
};

class terminated_list {
private:
    context *   head_{ nullptr };
    context *   tail_{ nullptr };

public:
    terminated_list() noexcept = default;

    bool empty() const noexcept {
        return nullptr == head_;
    }

    void push( context * ctx) {
        BOOST_ASSERT( nullptr == ctx->terminated_prev);
        BOOST_ASSERT( nullptr == ctx->terminated_next);
        if ( nullptr == head_) {
            head_ = tail_ = ctx;
        } else {
            ctx->terminated_prev = tail_;
            ctx->terminated_next = tail_->terminated_next;
            tail_->terminated_next = ctx;
            tail_ = ctx;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * ctx = head_;
        head_ = ctx->terminated_next;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->terminated_prev = nullptr;
        }
        ctx->terminated_prev = nullptr;
        ctx->terminated_next = nullptr;
        return ctx;
    }

    bool is_linked( context * ctx) const noexcept {
        return nullptr != ctx->terminated_prev || nullptr != ctx->terminated_next ||
            head_ == ctx || tail_ == ctx;
    }

    void unlink( context * ctx) {
        if ( ! is_linked( ctx) ) {
            return;
        }
        if ( nullptr != ctx->terminated_prev)  {
            ctx->terminated_prev->terminated_next = ctx->terminated_next;
        } else {
            head_ = ctx->terminated_next;
        }
        if ( nullptr != ctx->terminated_next) {
            ctx->terminated_next->terminated_prev = ctx->terminated_prev;
        } else {
            tail_ = ctx->terminated_prev;
        }
        ctx->terminated_prev = nullptr;
        ctx->terminated_next = nullptr;
    }
};

class remote_ready_list {
private:
    context *   head_{ nullptr };
    context *   tail_{ nullptr };

public:
    remote_ready_list() noexcept = default;

    bool empty() const noexcept {
        return nullptr == head_;
    }

    void push( context * ctx) {
        BOOST_ASSERT( nullptr == ctx->remote_ready_prev);
        BOOST_ASSERT( nullptr == ctx->remote_ready_next);
        if ( nullptr == head_) {
            head_ = tail_ = ctx;
        } else {
            ctx->remote_ready_prev = tail_;
            ctx->remote_ready_next = tail_->remote_ready_next;
            tail_->remote_ready_next = ctx;
            tail_ = ctx;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * ctx = head_;
        head_ = ctx->remote_ready_next;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->remote_ready_prev = nullptr;
        }
        ctx->remote_ready_prev = nullptr;
        ctx->remote_ready_next = nullptr;
        return ctx;
    }

    bool is_linked( context * ctx) const noexcept {
        return nullptr != ctx->remote_ready_prev || nullptr != ctx->remote_ready_next ||
            head_ == ctx || tail_ == ctx;
    }

    void unlink( context * ctx) {
        if ( ! is_linked( ctx) ) {
            return;
        }
        if ( nullptr != ctx->remote_ready_prev)  {
            ctx->remote_ready_prev->remote_ready_next = ctx->remote_ready_next;
        } else {
            head_ = ctx->remote_ready_next;
        }
        if ( nullptr != ctx->remote_ready_next) {
            ctx->remote_ready_next->remote_ready_prev = ctx->remote_ready_prev;
        } else {
            tail_ = ctx->remote_ready_prev;
        }
        ctx->remote_ready_prev = nullptr;
        ctx->remote_ready_next = nullptr;
    }

    void swap( remote_ready_list & other) {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }
};

class sleep_list {
private:
    context *   head_{ nullptr };
    context *   tail_{ nullptr };

public:
    sleep_list() noexcept = default;

    bool empty() const noexcept {
        return nullptr == head_;
    }

    void push( context * ctx, std::chrono::steady_clock::time_point const& tp) {
        BOOST_ASSERT( nullptr == ctx->sleep_prev);
        BOOST_ASSERT( nullptr == ctx->sleep_next);
        ctx->tp_ = tp;
        if ( nullptr == head_) {
            head_ = tail_ = ctx;
            return;
        }
        for ( context * tmp = head_; nullptr != tmp; tmp = tmp->sleep_next) {
            if ( ctx->tp_ < tmp->tp_) {
                ctx->sleep_next = tmp;
                ctx->sleep_prev = tmp->sleep_prev;
                if ( nullptr != tmp->sleep_prev) {
                    tmp->sleep_prev->sleep_next = ctx;
                } else {
                    head_ = ctx;
                }
                tmp->sleep_prev = ctx;
                return;
            }
        }
        ctx->sleep_prev = tail_;
        ctx->sleep_next = tail_->sleep_next;
        tail_->sleep_next = ctx;
        tail_ = ctx;
    }

    context * pop( std::chrono::steady_clock::time_point const& tp) {
        if ( ! empty() && head_->tp_ <= tp) {
            context * ctx = head_;
            unlink( ctx);
            ctx->tp_ = (std::chrono::steady_clock::time_point::max)();
            return ctx;
        } else {
            return nullptr;
        }
    }

    bool is_linked( context * ctx) const noexcept {
        return nullptr != ctx->sleep_prev || nullptr != ctx->sleep_next ||
            head_ == ctx || tail_ == ctx;
    }

    void unlink( context * ctx) {
        if ( ! is_linked( ctx) ) {
            return;
        }
        if ( nullptr != ctx->sleep_prev)  {
            ctx->sleep_prev->sleep_next = ctx->sleep_next;
        } else {
            head_ = ctx->sleep_next;
        }
        if ( nullptr != ctx->sleep_next) {
            ctx->sleep_next->sleep_prev = ctx->sleep_prev;
        } else {
            tail_ = ctx->sleep_prev;
        }
        ctx->sleep_prev = nullptr;
        ctx->sleep_next = nullptr;
    }

    std::chrono::steady_clock::time_point lowest_deadline() const noexcept {
        if ( nullptr == head_) {
            return (std::chrono::steady_clock::time_point::max)();
        } else {
            return head_->tp_;
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_LIST_H
