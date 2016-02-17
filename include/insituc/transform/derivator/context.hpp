#pragma once

#include <insituc/ast/ast.hpp>

#include <type_traits>
#include <utility>
#include <functional>
#include <deque>
#include <stack>

#include <cassert>

#if defined(_DEBUG) || defined(DEBUG)
#include <insituc/ast/io.hpp>
#include <ostream>
#endif

namespace insituc
{
namespace transform
{

struct descriptor
{

    struct block
    {

        ast::symbol wrt_;
        size_type first_;
        size_type offset_;
        size_type size_;

#if defined(_DEBUG) || defined(DEBUG)
        friend
        std::ostream &
        operator << (std::ostream & _out, block const & _block)
        {
            return _out << "block: name " << _block.wrt_
                        << ", first " << _block.first_
                        << ", offset " << _block.offset_
                        << ", size " << _block.size_;
        }
#endif

    };

    using blocks = std::deque< block >;

private :

    blocks head_;

    void
    append(ast::symbol _wrt)
    {
        if (head_.empty()) {
            head_.push_back({{""}, 0, 0, 1});
        }
        head_.push_back({std::move(_wrt), head_.size(), arity(), 0});
        block & tail_ = head_.back();
        while (0 < tail_.first_) {
            --tail_.first_;
            block const & block_ = head_[tail_.first_];
            tail_.size_ += block_.size_;
            if (block_.wrt_ == tail_.wrt_) {
                break;
            }
        }
    }

public :

    blocks const &
    head() const
    {
        return head_;
    }

    void
    swap(descriptor & _other)
    {
        head_.swap(_other.head_);
    }

    size_type
    prev_arity() const
    {
        assert(!head_.empty());
        return head_.back().offset_;
    }

    size_type
    arity() const
    {
        assert(!head_.empty());
        block const & last_ = head_.back();
        return last_.offset_ + last_.size_;
    }

    ast::symbol const &
    top() const
    {
        assert(!head_.empty());
        return head_.back().wrt_;
    }

    void
    push(ast::symbol _wrt)
    {
        append(std::move(_wrt));
    }

    void
    push(ast::symbols _wrts)
    {
        for (ast::symbol & wrt_ : _wrts) {
            append(std::move(wrt_));
        }
    }

};

static_assert(std::is_nothrow_move_assignable_v< descriptor >);

struct rollback
{

    rollback(descriptor && _descriptor)
        : descriptor_(_descriptor)
        , snapshot_()
    {
        descriptor_.swap(snapshot_);
    }

    rollback(descriptor & _descriptor)
        : descriptor_(_descriptor)
        , snapshot_(_descriptor)
    { ; }

    ~rollback()
    {
        descriptor_.swap(snapshot_);
    }

private :

    descriptor & descriptor_;
    descriptor snapshot_;

};

}
}
