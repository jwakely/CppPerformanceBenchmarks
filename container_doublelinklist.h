//
//  container_doublelinklist.h
//
//  Copyright © 2007-2022 Chris Cox. All rights reserved.
//  Distributed under the MIT License
//

#ifndef container_doublelinklist_h
#define container_doublelinklist_h

#include <stdexcept>
#include <utility>
#include <deque>

/******************************************************************************/

template<typename T>
struct DoubleLinkedNodeBase {
    T value;
    DoubleLinkedNodeBase *next;
    DoubleLinkedNodeBase *previous;
};

struct PooledLinkNodeInfo {
    PooledLinkNodeInfo() : pool_index(-1) {}
    size_t pool_index;
};

template<typename T>
struct DoubleLinkedPooledNode : public DoubleLinkedNodeBase<T>, public PooledLinkNodeInfo {};

/******************************************************************************/
/******************************************************************************/

template<typename T>
struct DoubleLinkedForwardIterator {

    DoubleLinkedNodeBase<T> *current;
    
    typedef ptrdiff_t                            difference_type;
    typedef std::bidirectional_iterator_tag      iterator_category;
    typedef T                                    value_type;
    typedef T*                                   pointer;
    typedef T&                                   reference;
    
    DoubleLinkedForwardIterator() {}
    DoubleLinkedForwardIterator(DoubleLinkedNodeBase<T> * x) : current(x) {}
    
    T& operator*() const { return current->value; }
    
    DoubleLinkedForwardIterator& operator++() {
        current = current->next;
        return *this;
    }
    
    DoubleLinkedForwardIterator& operator--() {
        current = current->previous;
        return *this;
    }
    
    DoubleLinkedForwardIterator operator++(int) {
        DoubleLinkedForwardIterator tmp = *this;
        ++*this;
        return tmp;
    }
    
    DoubleLinkedForwardIterator operator--(int) {
        DoubleLinkedForwardIterator tmp = *this;
        --*this;
        return tmp;
    }
        
    bool operator==(const DoubleLinkedForwardIterator<T>& y) const {
        return current == y.current;
    }

    bool operator!=(const DoubleLinkedForwardIterator<T>& other) const {
        return current != other.current;
    }
};

/******************************************************************************/

template<typename T>
struct DoubleLinkedReverseIterator {

    DoubleLinkedNodeBase<T> *current;
    
    typedef ptrdiff_t                            difference_type;
    typedef std::bidirectional_iterator_tag      iterator_category;
    typedef T                                    value_type;
    typedef T*                                   pointer;
    typedef T&                                   reference;
    
    DoubleLinkedReverseIterator() {}
    DoubleLinkedReverseIterator(DoubleLinkedNodeBase<T> * x) : current(x) {}
    
    T& operator*() const { return current->value; }
    
    DoubleLinkedReverseIterator& operator++() {
        current = current->previous;
        return *this;
    }
    
    DoubleLinkedReverseIterator& operator--() {
        current = current->next;
        return *this;
    }
    
    DoubleLinkedReverseIterator operator++(int) {
        DoubleLinkedReverseIterator tmp = *this;
        ++*this;
        return tmp;
    }
    
    DoubleLinkedReverseIterator operator--(int) {
        DoubleLinkedReverseIterator tmp = *this;
        --*this;
        return tmp;
    }
    
    bool operator==(const DoubleLinkedReverseIterator<T>& y) const {
        return current == y.current;
    }

    bool operator!=(const DoubleLinkedReverseIterator<T>& other) const {
        return current != other.current;
    }
};

/******************************************************************************/

template<typename T>
struct ConstDoubleLinkedForwardIterator {

    DoubleLinkedNodeBase<T> *current;
    
    typedef ptrdiff_t                            difference_type;
    typedef std::bidirectional_iterator_tag      iterator_category;
    typedef T                                    value_type;
    typedef const T*                             pointer;
    typedef const T&                            reference;
    
    ConstDoubleLinkedForwardIterator() {}
    ConstDoubleLinkedForwardIterator(DoubleLinkedNodeBase<T> * x) : current(x) {}
    
    reference operator*() const { return current->value; }
    
    ConstDoubleLinkedForwardIterator& operator++() {
        current = current->next;
        return *this;
    }
    
    ConstDoubleLinkedForwardIterator& operator--() {
        current = current->previous;
        return *this;
    }
    
    ConstDoubleLinkedForwardIterator operator++(int) {
        ConstDoubleLinkedForwardIterator tmp = *this;
        ++*this;
        return tmp;
    }
    
    ConstDoubleLinkedForwardIterator operator--(int) {
        ConstDoubleLinkedForwardIterator tmp = *this;
        --*this;
        return tmp;
    }
    
    bool operator==(const ConstDoubleLinkedForwardIterator<T>& y) const {
        return current == y.current;
    }

    bool operator!=(const ConstDoubleLinkedForwardIterator<T>& other) const {
        return current != other.current;
    }
};

/******************************************************************************/

template<typename T>
struct ConstDoubleLinkedReverseIterator {

    DoubleLinkedNodeBase<T> *current;
    
    typedef ptrdiff_t                            difference_type;
    typedef std::bidirectional_iterator_tag      iterator_category;
    typedef T                                    value_type;
    typedef const T*                             pointer;
    typedef const T&                             reference;
    
    ConstDoubleLinkedReverseIterator() {}
    ConstDoubleLinkedReverseIterator(DoubleLinkedNodeBase<T> * x) : current(x) {}
    
    reference operator*() const { return current->value; }
    
    ConstDoubleLinkedReverseIterator& operator++() {
        current = current->previous;
        return *this;
    }
    
    ConstDoubleLinkedReverseIterator& operator--() {
        current = current->next;
        return *this;
    }
    
    ConstDoubleLinkedReverseIterator operator++(int) {
        ConstDoubleLinkedReverseIterator tmp = *this;
        ++*this;
        return tmp;
    }
    
    ConstDoubleLinkedReverseIterator operator--(int) {
        ConstDoubleLinkedReverseIterator tmp = *this;
        --*this;
        return tmp;
    }
    
    bool operator==(const ConstDoubleLinkedReverseIterator<T>& y) const {
        return current == y.current;
    }

    bool operator!=(const ConstDoubleLinkedReverseIterator<T>& other) const {
        return current != other.current;
    }
};

/******************************************************************************/
/******************************************************************************/

// the obvious - allocate and delete each thing separately
template<typename T>
struct DoubleLinkListBaseAllocator {
    
    typedef DoubleLinkedNodeBase<T>   node_base_type;

    node_base_type *allocate_node() {
        node_base_type *node = new node_base_type;
        return node;
    }
    
    void release_node(const node_base_type *node) {
        delete node;
    }
};

/******************************************************************************/

// less obvious, higher performance: allocate nodes in blocks, track unused nodes
template<typename T>
struct DoubleLinkListPoolAllocator {

    typedef DoubleLinkedNodeBase<T>          node_base_type;
    typedef DoubleLinkedPooledNode<T>        node_pooled_type;
    typedef std::deque< node_pooled_type >   linked_pool_type;
    typedef std::deque< size_t >             free_node_list_type;

    node_base_type *allocate_node() {
        if (empty_slots.size() == 0)
            grow_node_pool();
        
        size_t index = empty_slots.back();
        empty_slots.pop_back();
        
        node_pool[index].pool_index = index;
        node_base_type *node = static_cast<node_base_type *>( &(node_pool[index]) ); // cast up to base type
        return node;
    }
    
    void release_node(const node_base_type *node) {
        const node_pooled_type *old = static_cast<const node_pooled_type *>(node);    // cast down to our pooled type
        empty_slots.push_back(old->pool_index);
        node_pool[old->pool_index].pool_index = size_t(-1);
    }
    
    void grow_node_pool() {
        const size_t delta = std::max( size_t(20), size_t(4096)/sizeof(node_pooled_type) );
        size_t old_size = node_pool.size();
        size_t new_size = old_size + delta;
        node_pool.resize( new_size );
        
        // push in reverse so we'll use contiguous (lowest) values first
        for (size_t index = new_size; index > old_size; --index)
            empty_slots.push_back(index-1);
    }
    
    const linked_pool_type &get_node_pool() const {
        return node_pool;
    }

private:
    free_node_list_type     empty_slots;
    linked_pool_type        node_pool;
};

/******************************************************************************/
/******************************************************************************/

template<typename T, class _Alloc>
struct DoubleLinkListBase {
    
    typedef T                                   value_type;
    typedef T&                                  reference;
    typedef const T&                            const_reference;
    typedef DoubleLinkedForwardIterator<T>      iterator;
    typedef ConstDoubleLinkedForwardIterator<T> const_iterator;
    typedef DoubleLinkedReverseIterator<T>      reverse_iterator;
    typedef ConstDoubleLinkedReverseIterator<T> const_reverse_iterator;
    typedef DoubleLinkedNodeBase<T>             node_type;
    typedef node_type *                         node_ptr;
    
    DoubleLinkListBase() : start(NULL), finish(NULL), currentSize(0) {}
    
    DoubleLinkListBase( const DoubleLinkListBase &other ) : start(NULL), finish(NULL), currentSize(0)
    {
        // REVISIT - could optimize by preallocating size of other in pooled case
        iterator current = other.begin();
        while (current != other.end())
            push_back( *current++ );
    }
    
    DoubleLinkListBase &operator=( DoubleLinkListBase &other ) {
        // REVISIT - could optimize by preallocating size of other in pooled case
        clear();
        iterator current = other.begin();
        while (current != other.end())
            push_back( *current++ );
        return *this;
    }
    
    iterator begin() {
        return iterator(start);
    }
    
    iterator end() {
        return iterator(NULL);
    }
    
    reverse_iterator rbegin() {
        return reverse_iterator(finish);
    }
    
    reverse_iterator rend() {
        return reverse_iterator(NULL);
    }
    
    const_iterator cbegin() {
        return const_iterator(start);
    }
    
    const_iterator cend() {
        return const_iterator(NULL);
    }
    
    const_reverse_iterator crbegin() {
        return const_reverse_iterator(finish);
    }
    
    const_reverse_iterator crend() {
        return const_reverse_iterator(NULL);
    }
    
    reference front() {
        if (start == NULL)
            throw(std::out_of_range("list is empty"));
        return start->value;
    }
    
    const_reference front() const {
        if (start == NULL)
            throw(std::out_of_range("list is empty"));
        return start->value;
    }
    
    reference back() {
        if (finish == NULL)
            throw(std::out_of_range("list is empty"));
        return finish->value;
    }
    
    const_reference back() const {
        if (finish == NULL)
            throw(std::out_of_range("list is empty"));
        return finish->value;
    }
    
    bool empty() const {
        return (currentSize == 0);
    }
    
    size_t size() const {
        return currentSize;
    }
    
    void clear() {
        // iterate and delete all items
        node_ptr current = start;
        while (current != NULL) {
            node_ptr old = current;
            current = current->next;
            release_node(old);
        }
        
        start =  NULL;
        finish = NULL;
        currentSize = 0;
    }
    
    void resize( size_t newSize ) {
        ptrdiff_t diff = newSize - currentSize;
        // ignore reduction of size
        if (diff > 0) {
            while(diff--) {
                push_back(T());
            }
        }
    }
    
    void push_back( const T &value ) {
        node_ptr item = allocate_node();
        item->value = value;
        item->next = NULL;
        item->previous = finish;
        
        if (finish != NULL)
            finish->next = item;
        
        finish = item;
        
        if (start == NULL)
            start = item;
        
        currentSize++;
    }
    
    void push_front( const T &value ) {
        node_ptr item = allocate_node();
        item->value = value;
        item->next = start;
        item->previous = NULL;
        
        if (start != NULL)
            start->previous = item;
        
        start = item;
        
        if (finish == NULL)
            finish = item;
        
        currentSize++;
    }
    
    void pop_front() {
        node_ptr next_item = NULL;
        
        if (start) {
            next_item = start->next;
            release_node(start);
        }
        
        if (finish == start)
            finish = NULL;
        
        if (next_item)
            next_item->previous = NULL;
        
        start = next_item;
        
        --currentSize;
    }
    
    void pop_back() {
        node_ptr prev_item = NULL;
        
        if (finish) {
            prev_item = finish->previous;
            release_node(finish);
        }
        
        if (start == finish)
            start = NULL;
        
        if (prev_item)
            prev_item->next = NULL;
        
        finish = prev_item;
        
        --currentSize;
    }
    
    void erase(iterator &first_item, iterator &end_item) {
        
        node_ptr next_item = NULL;
        node_ptr prev_item = NULL;
        
        // find item previous to first
        if (first_item.current)
            prev_item = first_item.current->previous;
        
        // find item after stop
        if (end_item.current)
            next_item = end_item.current->next;
        
        // patch up the list
        if (prev_item && prev_item != first_item.current)
            prev_item->next = next_item;
        
        if (next_item && next_item != end_item.current)
            next_item->previous = prev_item;
        
        if (first_item.current == start)
            start = next_item;
        
        if (prev_item == finish)
            finish = NULL;
        
        // delete items in between
        node_ptr current = first_item.current;
        while (current != end_item.current) {
            node_ptr old = current;
            current = current->next;
            release_node(old);
            --currentSize;
        }
    }
    
    void erase(iterator &first_item) {
        
        node_ptr next_item = NULL;
        node_ptr prev_item = start;
        
        // find item previous to first
        if (first_item.current)
            prev_item = first_item.current->previous;
        
        // find item after
        if (first_item.current)
            next_item = first_item.current->next;
        
        // patch up the list
        if (prev_item && prev_item != first_item.current)
            prev_item->next = next_item;
        
        if (next_item && next_item != first_item.current)
            next_item->previous = prev_item;
        
        if (first_item.current == start)
            start = next_item;
        
        if (prev_item == finish)
            finish = NULL;
        
        // delete item
        node_ptr old = first_item.current;
        release_node(old);
        --currentSize;
    }

protected:
    node_type *allocate_node() { return allocator_data.allocate_node(); }
    
    void release_node(const node_type *node) { allocator_data.release_node( node ); }
    
    _Alloc       allocator_data;

private:
    size_t      currentSize;
    node_ptr    start;
    node_ptr    finish;
};

/******************************************************************************/

template<typename T>
struct DoubleLinkList : public DoubleLinkListBase<T, DoubleLinkListBaseAllocator<T> >
{
    typedef DoubleLinkListBase<T, DoubleLinkListBaseAllocator<T> > _parent;

    DoubleLinkList() : _parent() {}
    
    ~DoubleLinkList() {
        _parent::clear();        // here we must delete each node
    }
};

/******************************************************************************/
/******************************************************************************/

template<typename T>
struct ConstDoubleLinkPoolIterator {

    typedef ptrdiff_t                            difference_type;
    typedef std::bidirectional_iterator_tag      iterator_category;
    typedef const T                              value_type;
    typedef const T*                             pointer;
    typedef const T&                             reference;
    typedef DoubleLinkedNodeBase<T>              node_base_type;
    typedef DoubleLinkedPooledNode<T>            node_pooled_type;
    typedef std::deque< node_pooled_type >       linked_pool_type;
    typedef typename linked_pool_type::const_iterator   pool_iter;

    const linked_pool_type &node_pool;
    pool_iter              current_iter;
    
    ConstDoubleLinkPoolIterator() {}
    ConstDoubleLinkPoolIterator(const linked_pool_type &pool, pool_iter iter) :
                            node_pool(pool), current_iter(iter) {}
    
    reference operator*() const { return current_iter->value; }
    
    reference operator->() const { return current_iter->value; }
    
    ConstDoubleLinkPoolIterator& operator++() {
        ++current_iter;
        while (current_iter != node_pool.end() && current_iter->pool_index == size_t(-1))        // skip unused entries
            ++current_iter;
        return *this;
    }
    
    ConstDoubleLinkPoolIterator& operator--() {
        --current_iter;
        while (current_iter != node_pool.begin() && current_iter->pool_index == size_t(-1))        // skip unused entries
            --current_iter;
        return *this;
    }
    
    ConstDoubleLinkPoolIterator operator++(int) {
        ConstDoubleLinkPoolIterator tmp = *this;
        ++current_iter;
        while (current_iter != node_pool.end() && current_iter->pool_index == size_t(-1))        // skip unused entries
            ++current_iter;
        return tmp;
    }
    
    ConstDoubleLinkPoolIterator operator--(int) {
        ConstDoubleLinkPoolIterator tmp = *this;
        --current_iter;
        while (current_iter != node_pool.begin() && current_iter->pool_index == size_t(-1))        // skip unused entries
            --current_iter;
        return tmp;
    }
    
    bool operator==(const ConstDoubleLinkPoolIterator<T>& y) const {
        return current_iter == y.current_iter;
    }

    bool operator!=(const ConstDoubleLinkPoolIterator<T>& other) const {
        return !(*this == other);
    }
};

/******************************************************************************/

template<typename T>
struct PooledDoubleLinkList : public DoubleLinkListBase<T, DoubleLinkListPoolAllocator<T> >
{
    typedef DoubleLinkListBase<T, DoubleLinkListPoolAllocator<T> > _parent;
    typedef ConstDoubleLinkPoolIterator<T>    pool_iter;

    PooledDoubleLinkList() : _parent() {}
    
    ~PooledDoubleLinkList() {
        // here, deleting the allocator deletes all nodes at once
    }
    
    // unordered iterators -- iterate the pool instead of the linked list (better cache coherency, no pointer chasing)
    // Always const, because changing the data in the pool could be dangerous  (though I can see some situations where it might be useful)
    // No reverse, because this is unordered data
    pool_iter cubegin() {
        return pool_iter(_parent::allocator_data.get_node_pool(), _parent::allocator_data.get_node_pool().begin());
    }
    
    pool_iter cuend() {
        return pool_iter(_parent::allocator_data.get_node_pool(), _parent::allocator_data.get_node_pool().end());
    }

};

/******************************************************************************/

#endif /* container_doublelinklist_h */
