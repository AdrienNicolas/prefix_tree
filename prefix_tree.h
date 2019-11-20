//
// Created by Adrien Briand on 21/4/2018.
//

#ifndef PREFIX_TREE_PREFIX_TREE_H
#define PREFIX_TREE_PREFIX_TREE_H

#include <memory>
#include <utility>
#include <stdexcept>

#include "util/memory.h"
#include "node.h"
#include "iterator.h"
#include "prefixer_traits.h"

template<class K, class T, class Charset, class Allocator = std::allocator<T> >
class prefix_tree_view
{

};

template<class K, class T, class Charset, class Prefixer, class Allocator = std::allocator<T> >
class prefix_tree
{
public:
	typedef std::size_t size_type;

    typedef K key_type;
    typedef T mapped_type;
    typedef Charset charset_type;
    typedef Prefixer prefixer_type;
    typedef Allocator allocator_type;
    typedef prefix_tree<key_type,mapped_type,charset_type,prefixer_type,allocator_type> type;
    typedef mapped_type value_type;

    typedef typename charset_type::index_type index_type;
    typedef typename charset_type::letter_type letter_type;

    typedef node<key_type, mapped_type, charset_type, prefixer_type, allocator_type> node_type;
    typedef typename node_type::parent_link_type parent_link_type;
    typedef typename node_type::node_container node_container;
    typedef typename node_type::prefix_const_iterator prefix_const_iterator;

    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_type> node_allocator_type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<index_type> prefix_allocator_type;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_container> node_container_allocator_type;
    typedef typename node_type::value_holder value_holder;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<value_holder> value_holder_allocator_type;
    typedef typename node_type::value_holder_ptr value_holder_ptr;
    typedef typename node_type::value_holder_deleter_type value_holder_deleter_type;


    typedef mapped_type & reference;

    typedef prefix_tree_iterator<node_type, readonly_type> const_iterator;
    typedef prefix_tree_iterator<node_type, readwrite_type> iterator;

    explicit prefix_tree(const charset_type & abc = charset_type(), const allocator_type & allocator = allocator_type())
    :abc(abc)
    ,allocator(allocator)
    ,node_allocator(this->allocator)
    ,prefix_allocator(this->allocator)
    ,root(parent_link_type(nullptr,0), value_holder_ptr(nullptr, value_holder_deleter_type(this->allocator)), this->node_allocator)
    {
    }
	
    reference at(const key_type & key) const
    {
        const node_type * node = exact_match(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end()));
        if(!node)
        {
            throw std::out_of_range("key not found");
        }
        else
        {
            return node->get_value()->second;
        }
    }

    reference at(const key_type & key)
    {
        const node_type * node = exact_match(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end()));
        if(!node)
        {
            throw std::out_of_range("key not found");
        }
        else
        {
            return node->get_value()->second;
        }
    }

    reference operator[] ( const key_type & k)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, mapped_type());
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = insert_node<node_type>(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return existing->second;
    }

    std::pair<iterator, bool> insert(const key_type & k, const mapped_type & toInsert)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, mapped_type());
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = insert_node<node_type>(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return std::make_pair(iterator(node), value.get() == nullptr);
    }

    std::pair<iterator, bool> insert(const key_type & k, mapped_type && toInsert)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, std::forward<mapped_type>(toInsert));
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = insert_node<node_type>(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return std::make_pair(iterator::make_begin(node), value.get() == nullptr);
    }

    template <typename P>
    std::pair<iterator, bool> insert(const key_type & k, P && toInsert)
    {
        unique_allocation a(this->allocator, 1);
        new((void *)a.get()) value_holder(k, std::forward<P>(toInsert));
        value_holder_ptr value(a.release(), value_holder_deleter_type(this->allocator));

        node_type * node = insert_node<node_type>(&this->root, this->node_container_allocator, this->node_allocator, this->prefix_allocator, abc, value->first, value->first.begin(), value->first.end());
        value_holder_ptr & existing = node->get_value();
        if(!existing)
        {
            existing.reset(value.release());
        }
        return std::make_pair(iterator::make_begin(node), value.get() == nullptr);
    }

	const_iterator erase(const_iterator pos)
	{
        node_type * to_erase = const_cast<node_type *>(pos.get_node());
		if(to_erase)
		{
		    ++pos;
            remove_node<node_type>(to_erase, this->prefix_allocator);
		}
		return pos;
	}
	
	iterator erase(iterator pos)
	{
        node_type * to_erase = pos.get_node();
        if(to_erase)
        {
            ++pos;
            remove_node<node_type>(to_erase, this->prefix_allocator);
        }
        return pos;
	}
	
	const_iterator erase( const_iterator first, const_iterator last )
	{
		while(first != last)
		{
            first = erase(first);
		}
		return first;
	}
	
	iterator erase( iterator first, iterator last )
	{
        while(first != last)
        {
            first = erase(first);
        }
		return first;
	}
	
	size_type erase( const key_type& key )
	{
        size_type result = 0;
        node_type * node = exact_match(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end()));
		if(node)
		{
            remove_node<node_type>(node, this->prefix_allocator);
			result = size_type(1);
		}
		return result;
	}
	
	iterator begin() noexcept
    {
        return iterator::make_begin(&root);
    }
	
    const_iterator begin() const noexcept
    {
        return const_iterator::make_begin(&root);
    }
	
    const_iterator cbegin() const noexcept
    {
        return const_iterator::make_begin(&root);
    }
	
    iterator end() noexcept
    {
        return iterator::make_end(&root);
    }
	
    const_iterator end() const noexcept
    {
        return const_iterator::make_end(&root);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator::make_end(&root);
    }
	
    allocator_type get_allocator() const noexcept
    {
        return allocator;
    }

    bool empty() const noexcept
    {
        return root.empty();
    }

    void clear() noexcept
    {
        root.clear();
    }

    size_type count( const key_type & key ) const
    {
        const node_type * node = exact_match(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end()));
        return node ? 1 : 0;
    }

    iterator find( const key_type& key )
    {
        return iterator::make_begin(exact_match(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end())));
    }

    const_iterator find( const key_type& key ) const
    {
        return const_iterator::make_begin(exact_match(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end())));
    }

    const_iterator lower_bound( const key_type & key) const
    {
        return const_iterator::make_begin(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end()).second);
    }

    iterator lower_bound( const key_type & key)
    {
        return iterator::make_begin(get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end()).second);
    }

    const_iterator upper_bound( const key_type & key) const
    {
        std::pair<prefix_const_iterator, const node_type *> p = get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end());
        const_iterator result(p.second);
        if(exact_match(p))
        {
            ++result;
        }
        return result;
    }

    iterator upper_bound( const key_type & key)
    {
        std::pair<prefix_const_iterator, const node_type *> p = get_node<node_type>(root.prefix_begin(), &root, abc, key.begin(), key.end());
        iterator result(p.second);
        if(exact_match(p))
        {
            ++result;
        }
        return result;
    }

    void start_with( key_type & key ) const
    {
    }

private:
    template<typename NodePtr>
    static NodePtr exact_match(const std::pair<prefix_const_iterator, NodePtr> & p)
    {
        return p.second && p.second->get_value() && p.first == p.second->prefix_end() ? p.second : nullptr;
    }

    const charset_type abc;
    value_holder_allocator_type allocator;
    node_allocator_type node_allocator;
    prefix_allocator_type prefix_allocator;
    node_container_allocator_type node_container_allocator;
    node_type root;
};

#endif //PREFIX_TREE_PREFIX_TREE_H
