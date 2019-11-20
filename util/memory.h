//
// Created by Adrien Briand on 18/8/2018.
//

#ifndef PREFIX_TREE_MEMORY_H
#define PREFIX_TREE_MEMORY_H

template<class Allocator>
class unique_allocation
{
public:
    typedef Allocator allocator_type;
    typedef unique_allocation<allocator_type> type;

    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;

    explicit unique_allocation(allocator_type & allocator, size_type n = 1)
    :allocator(allocator)
    ,n(n)
    ,p(allocator.allocate(n))
    {
    }

    ~unique_allocation()
    {
        if(p)
        {
            allocator.deallocate(p, n);
        }
    }

    value_type * get() const noexcept
    {
        return p;
    }

    value_type * release() noexcept
    {
        value_type * result = p;
        p = nullptr;
        return result;
    }

private:
    allocator_type & allocator;
    size_type n;
    value_type * p;
};

template<class Allocator>
class allocator_deleter
{
public:
    typedef Allocator allocator_type;
    typedef allocator_deleter<allocator_type> type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::value_type value_type;

    explicit allocator_deleter(allocator_type & allocator) noexcept
    :length(1)
    ,allocator(allocator)
    {
    }

    allocator_deleter(const size_type length, allocator_type & allocator) noexcept
    :length(length)
    ,allocator(allocator)
    {
    }

    allocator_deleter(const allocator_deleter & deleter) noexcept
    :length(deleter.length)
    ,allocator(deleter.allocator)
    {
    }

    void operator()(value_type * t)
    {
        t->~value_type();
        allocator.deallocate(t, length);
    }

private:
    size_type length;
    allocator_type & allocator;
};

#endif //PREFIX_TREE_MEMORY_H
