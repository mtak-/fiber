
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

    void push( Ctx * c) {
        if ( nullptr == head_) {
            head_ = tail_ = c;
        } else {
            c->wait_prev = tail_;
            tail_->wait_next = c;
            tail_ = c;
        }
    }

    Ctx * pop() {
        if ( nullptr == head_) {
            return nullptr;
        }
        Ctx * c = head_;
        head_ = c->wait_next;
        c->wait_next = nullptr;
        if ( nullptr == head_) {
            tail_ = head_;
        } else {
            head_->wait_prev = nullptr;
        }
        return c;
    }

    void unlink( Ctx * c) {
        if ( nullptr != c->wait_prev)  {
            c->wait_prev->wait_next = c->wait_next;
        } else {
            head_ = c->wait_next;
        }
        if ( nullptr != c->wait_next) {
            c->wait_next->wait_prev = c->wait_prev;
        } else {
            tail_ = c->wait_prev;
        }
        c->wait_prev = nullptr;
        c->wait_next = nullptr;
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
