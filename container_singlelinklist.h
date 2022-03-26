//
//  container_singlelinklist.h
//
//  Copyright © 2007-2022 Chris Cox. All rights reserved.
//  Distributed under the MIT License
//

#ifndef container_singlelinklist_h
#define container_singlelinklist_h

#include <stdexcept>
#include <utility>
#include <deque>

/******************************************************************************/
/******************************************************************************/

template<typename T>
struct SingleLinkNode {
    T value;
    SingleLinkNode *next;
};

struct PooledNodeInfo {
    PooledNodeInfo() : index(-1) {}
    size_t index;
};

template<typename T>
struct SingleLinkPooledNode : public SingleLinkNode<T>, public PooledNodeInfo {};

/******************************************************************************/

// REVISIT - implement "before begin" concept?          https://en.cppreference.com/w/cpp/container/forward_list/before_begin
template<typename T>
struct SingleLinkForwardIterator {
    SingleLinkNode<T> *current;
    
    typedef ptrdiff_t                   difference_type;
    typedef std::forward_iterator_tag   iterator_category;
    typedef T                           value_type;
    typedef T*                          pointer;
    typedef T&                          reference;
    
    SingleLinkForwardIterator() {}
    SingleLinkForwardIterator(SingleLinkNode<T> *x) : current(x) {}
    
    T& operator*() const { return current->value; }
    
    SingleLinkForwardIterator& operator++() {
        current = current->next;
        return *this;
    }

    SingleLinkForwardIterator operator++(int) {
        SingleLinkForwardIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const SingleLinkForwardIterator<T>& y) const {
        return current == y.current;
    }

    bool operator!=(const SingleLinkForwardIterator<T>& other) const {
        return current != other.current;
    }
};

/******************************************************************************/

template<typename T>
struct ConstSingleLinkForwardIterator {
    SingleLinkNode<T> *current;
    
    typedef ptrdiff_t                   difference_type;
    typedef std::forward_iterator_tag   iterator_category;
    typedef T                           value_type;
    typedef const T*                    pointer;
    typedef const T&                    reference;
    
    ConstSingleLinkForwardIterator() {}
    ConstSingleLinkForwardIterator(SingleLinkNode<T> *x) : current(x) {}
    
    reference operator*() const { return current->value; }
    
    ConstSingleLinkForwardIterator& operator++() {
        current = current->next;
        return *this;
    }

    ConstSingleLinkForwardIterator operator++(int) {
        ConstSingleLinkForwardIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const ConstSingleLinkForwardIterator<T>& y) const {
        return current == y.current;
    }

    bool operator!=(const ConstSingleLinkForwardIterator<T>& other) const {
        return current != other.current;
    }
};

/******************************************************************************/
/******************************************************************************/

// the obvious - allocate and delete each thing separately
template<typename T>
struct SingleLinkListBaseAllocator {
    
    typedef SingleLinkNode<T>   node_base_type;

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
struct SingleLinkListPoolAllocator {

    typedef SingleLinkNode<T>                node_base_type;
    typedef SingleLinkPooledNode<T>          node_pooled_type;
    typedef std::deque< node_pooled_type >   linked_pool_type;
    typedef std::deque< size_t >             free_node_list_type;

    node_base_type *allocate_node() {
        if (empty_slots.size() == 0)
            grow_node_pool();
        
        size_t index = empty_slots.back();
        empty_slots.pop_back();
        
        node_pool[index].index = index;
        node_base_type *node = static_cast<node_base_type *>( &(node_pool[index]) ); // cast up to base type
        return node;
    }
    
    void release_node(const node_base_type *node) {
        const node_pooled_type *old = static_cast<const node_pooled_type *>(node);    // cast down to our pooled type
        empty_slots.push_back(old->index);
        node_pool[old->index].index = size_t(-1);
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
    
    const linked_pool_type &get_node_pool() {
        return node_pool;
    }

private:
    free_node_list_type     empty_slots;
    linked_pool_type        node_pool;
};

/******************************************************************************/

template<typename T, class _Alloc>
class SingleLinkListBase {
public:
    typedef T                                 value_type;
    typedef T&                                reference;
    typedef const T&                          const_reference;
    typedef SingleLinkForwardIterator<T>      iterator;
    typedef ConstSingleLinkForwardIterator<T> const_iterator;
    typedef SingleLinkNode<T>                 node_type;
    typedef node_type *                       node_ptr;
    
    SingleLinkListBase() : start(NULL), finish(NULL), currentSize(0) {}
    
    SingleLinkListBase( const SingleLinkListBase &other ) : start(NULL), finish(NULL), currentSize(0)
    {
        // REVISIT - could optimize by preallocating size of other in pooled case
        iterator current = other.begin();
        
        while (current != other.end())
            push_back( *current++ );
    }
    
    ~SingleLinkListBase() {}
    
    SingleLinkListBase &operator=( const SingleLinkListBase &other ) {
        // REVISIT - could optimize by preallocating size of other in pooled case
        iterator current = other.begin();
        while (current != other.end())
            push_back( *current++ );
        return *this;
    }
    
    iterator begin() const {
        return iterator(start);
    }
    
    iterator end() const {
        return iterator(NULL);
    }
    
    const_iterator cbegin() const {
        return const_iterator(start);
    }
    
    const_iterator cend() const {
        return const_iterator(NULL);
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
        
        start = item;
        
        if (finish == NULL)
            finish = item;
        
        currentSize++;
    }
    
    void pop_front() {
        node_ptr front_item = start;
        node_ptr next_item = NULL;
        
        if (start)
            next_item = start->next;
        
        if (finish == start)
            finish = NULL;
        
        start = next_item;
        
        release_node(front_item);
        --currentSize;
    }
    
    // this is painfully slow, but works
    void pop_back() {
        node_ptr last_item = finish;
        node_ptr prev_item = start;
        
        while (prev_item && prev_item->next != finish)
            prev_item = prev_item->next;
        
        if (start == finish)
            start = NULL;
        
        if (prev_item)
            prev_item->next = NULL;
        
        finish = prev_item;
        
        release_node(last_item);
        --currentSize;
    }
    
    // remove ALL elements equal to value
    // this can be slow, but works
    void remove(const T &val) {
        
        // remember that the start pointer will change after pop_front!
        while (start && start->value == val) {
            pop_front();
        }
        
        node_ptr prev_item = start;

        do {
            node_ptr cur_item = NULL;

// REVISIT - this logic still does not look right
            // find item previous to value
            while (prev_item && prev_item->next != NULL) {
                cur_item = prev_item->next;
                if (cur_item->value == val)
                    break;
                prev_item = cur_item;
            }
            
            if (cur_item == NULL || cur_item->value != val)
                break;
            
            // patch up the list
            //assert(prev_item != cur_item);
            //assert(start != cur_item);
            //assert(prev_item != finish);
            prev_item->next = cur_item->next;
            
            // delete current item
            release_node(cur_item);
            --currentSize;
        } while (prev_item != NULL);

    }
    
    // this can be painfully slow, but works
    void erase(iterator &first_item) {
        
        node_ptr next_item = NULL;
        node_ptr prev_item = start;
        
        // find item previous to start
        if (first_item.current != start)
            while (prev_item && prev_item->next != first_item.current)
                prev_item = prev_item->next;
        
        // find item after first
        if (first_item.current != finish)
            next_item = first_item.current->next;
        
        // patch up the list
        if (prev_item != first_item.current)
            prev_item->next = next_item;
        
        if (first_item.current == start)
            start = next_item;
        
        if (prev_item == finish)
            finish = NULL;
        
        // delete items
        node_ptr old = first_item.current;
        release_node(old);
        --currentSize;
    }
    
    // this can be painfully slow, but works
    void erase(iterator &first_item, iterator &end_item) {
        
        node_ptr next_item = NULL;
        node_ptr prev_item = start;
        
        // find item previous to start
        if (first_item.current != start)
            while (prev_item && prev_item->next != first_item.current)
                prev_item = prev_item->next;
        
        // find item after stop
        if (end_item.current)
            next_item = end_item.current->next;
        
        // patch up the list
        if (prev_item != first_item.current)
            prev_item->next = next_item;
        
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

protected:
    node_type *allocate_node() { return allocator_data.allocate_node(); }
    
    void release_node(const node_type *node) { allocator_data.release_node( node ); }
    
    _Alloc       allocator_data;

private:
    int currentSize;
    node_ptr start;
    node_ptr finish;
};

/******************************************************************************/

template<typename T>
struct SingleLinkList : public SingleLinkListBase<T, SingleLinkListBaseAllocator<T> >
{
    typedef SingleLinkListBase<T, SingleLinkListBaseAllocator<T> > _parent;

    SingleLinkList() : _parent() {}
    
    ~SingleLinkList() {
        _parent::clear();        // here we must delete each node
    }
};

/******************************************************************************/
/******************************************************************************/

template<typename T>
struct ConstSingleLinkPoolIterator {

    typedef ptrdiff_t                            difference_type;
    typedef std::bidirectional_iterator_tag      iterator_category;
    typedef const T                              value_type;
    typedef const T*                             pointer;
    typedef const T&                             reference;
    typedef SingleLinkNode<T>                    node_base_type;
    typedef SingleLinkPooledNode<T>              node_pooled_type;
    typedef std::deque< node_pooled_type >       linked_pool_type;
    typedef typename linked_pool_type::const_iterator   pool_iter;

    const linked_pool_type &node_pool;
    pool_iter              current_iter;
    
    ConstSingleLinkPoolIterator() {}
    ConstSingleLinkPoolIterator(const linked_pool_type &pool, pool_iter iter) :
                            node_pool(pool), current_iter(iter) {}
    
    reference operator*() const { return current_iter->value; }
    
    reference operator->() const { return current_iter->value; }
    
    ConstSingleLinkPoolIterator& operator++() {
        ++current_iter;
        while (current_iter != node_pool.end() && current_iter->index == size_t(-1))        // skip unused entries
            ++current_iter;
        return *this;
    }
    
    ConstSingleLinkPoolIterator& operator--() {
        --current_iter;
        while (current_iter != node_pool.begin() && current_iter->index == size_t(-1))        // skip unused entries
            --current_iter;
        return *this;
    }
    
    ConstSingleLinkPoolIterator operator++(int) {
        ConstSingleLinkPoolIterator tmp = *this;
        ++current_iter;
        while (current_iter != node_pool.end() && current_iter->index == size_t(-1))        // skip unused entries
            ++current_iter;
        return tmp;
    }
    
    ConstSingleLinkPoolIterator operator--(int) {
        ConstSingleLinkPoolIterator tmp = *this;
        --current_iter;
        while (current_iter != node_pool.begin() && current_iter->index == size_t(-1))        // skip unused entries
            --current_iter;
        return tmp;
    }
    
    bool operator==(const ConstSingleLinkPoolIterator<T>& y) const {
        return current_iter == y.current_iter;
    }

    bool operator!=(const ConstSingleLinkPoolIterator<T>& other) const {
        return !(*this == other);
    }
};

/******************************************************************************/

template<typename T>
struct PooledSingleLinkList : public SingleLinkListBase<T, SingleLinkListPoolAllocator<T> >
{
    typedef SingleLinkListBase<T, SingleLinkListPoolAllocator<T> > _parent;
    typedef ConstSingleLinkPoolIterator<T>    pool_iter;

    PooledSingleLinkList() : _parent() {}
    
    ~PooledSingleLinkList() {
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

#endif /* container_singlelinklist_h */
