
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_LIST_H
#define BOOST_FIBERS_DETAIL_LIST_H

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

    void push( context * c) {
        BOOST_ASSERT( nullptr == c->ready_prev);
        BOOST_ASSERT( nullptr == c->ready_next);
        if ( nullptr == head_) {
            head_ = tail_ = c;
        } else {
            c->ready_prev = tail_;
            tail_->ready_next = c;
            tail_ = c;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * c = head_;
        head_ = c->ready_next;
        c->ready_next = nullptr;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->ready_prev = nullptr;
        }
        return c;
    }

    void unlink( context * c) {
        if ( nullptr != c->ready_prev)  {
            c->ready_prev->ready_next = c->ready_next;
        } else {
            head_ = c->ready_next;
        }
        if ( nullptr != c->ready_next) {
            c->ready_next->ready_prev = c->ready_prev;
        } else {
            tail_ = c->ready_prev;
        }
        c->ready_prev = nullptr;
        c->ready_next = nullptr;
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

    void push( context * c) {
        BOOST_ASSERT( nullptr == c->worker_prev);
        BOOST_ASSERT( nullptr == c->worker_next);
        if ( nullptr == head_) {
            head_ = tail_ = c;
        } else {
            c->worker_prev = tail_;
            tail_->worker_next = c;
            tail_ = c;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * c = head_;
        head_ = c->worker_next;
        c->worker_next = nullptr;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->worker_prev = nullptr;
        }
        return c;
    }

    void unlink( context * c) {
        if ( nullptr != c->worker_prev)  {
            c->worker_prev->worker_next = c->worker_next;
        } else {
            head_ = c->worker_next;
        }
        if ( nullptr != c->worker_next) {
            c->worker_next->worker_prev = c->worker_prev;
        } else {
            tail_ = c->worker_prev;
        }
        c->worker_prev = nullptr;
        c->worker_next = nullptr;
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

    void push( context * c) {
        BOOST_ASSERT( nullptr == c->terminated_prev);
        BOOST_ASSERT( nullptr == c->terminated_next);
        if ( nullptr == head_) {
            head_ = tail_ = c;
        } else {
            c->terminated_prev = tail_;
            tail_->terminated_next = c;
            tail_ = c;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * c = head_;
        head_ = c->terminated_next;
        c->terminated_next = nullptr;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->terminated_prev = nullptr;
        }
        return c;
    }

    void unlink( context * c) {
        if ( nullptr != c->terminated_prev)  {
            c->terminated_prev->terminated_next = c->terminated_next;
        } else {
            head_ = c->terminated_next;
        }
        if ( nullptr != c->terminated_next) {
            c->terminated_next->terminated_prev = c->terminated_prev;
        } else {
            tail_ = c->terminated_prev;
        }
        c->terminated_prev = nullptr;
        c->terminated_next = nullptr;
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

    void push( context * c) {
        BOOST_ASSERT( nullptr == c->remote_ready_prev);
        BOOST_ASSERT( nullptr == c->remote_ready_next);
        if ( nullptr == head_) {
            head_ = tail_ = c;
        } else {
            c->remote_ready_prev = tail_;
            tail_->remote_ready_next = c;
            tail_ = c;
        }
    }

    context * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        context * c = head_;
        head_ = c->remote_ready_next;
        c->remote_ready_next = nullptr;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->remote_ready_prev = nullptr;
        }
        return c;
    }

    void unlink( context * c) {
        if ( nullptr != c->remote_ready_prev)  {
            c->remote_ready_prev->remote_ready_next = c->remote_ready_next;
        } else {
            head_ = c->remote_ready_next;
        }
        if ( nullptr != c->remote_ready_next) {
            c->remote_ready_next->remote_ready_prev = c->remote_ready_prev;
        } else {
            tail_ = c->remote_ready_prev;
        }
        c->remote_ready_prev = nullptr;
        c->remote_ready_next = nullptr;
    }

    void swap( remote_ready_list & other) {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_LIST_H
