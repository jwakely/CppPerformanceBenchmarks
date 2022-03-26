//
//  container_hashmap.h
//
//  Copyright © 2007-2022 Chris Cox. All rights reserved.
//  Distributed under the MIT License
//

#ifndef container_hashmap_h
#define container_hashmap_h

#include <stdexcept>
#include <utility>
#include <deque>

/******************************************************************************/
/******************************************************************************/

// NOTE - could add full hash value storage and comparison for large, expensive key types (where == takes significant time)
// Not sure if there is a clean way to do this automatically using C++14 or C++17
template<typename __keyType, typename __ValueType>
struct HashNodeBase {
    __keyType key_value;
    __ValueType value;
    HashNodeBase *next;
};

struct PooledHashNodeInfo {
    PooledHashNodeInfo() : pool_index(-1) {}
    size_t pool_index;
};

template<typename __keyType, typename __ValueType>
struct PooledHashNode : public HashNodeBase<__keyType,__ValueType>, public PooledHashNodeInfo {};

/******************************************************************************/

template<typename __keyType, typename __ValueType>
struct HashMapForwardIterator {

    typedef HashNodeBase<__keyType,__ValueType> node_T;
    
    typedef ptrdiff_t                   difference_type;
    typedef std::input_iterator_tag     iterator_category;
    typedef __ValueType                 value_type;
    typedef __ValueType*                pointer;
    typedef __ValueType&                reference;

    node_T *currentEntry;
    node_T **currentTable;
    size_t current_bucket;
    size_t table_size;
    std::pair<__keyType,__ValueType>  _compatibility_pair;
    
    HashMapForwardIterator() : currentEntry(NULL), currentTable(NULL), current_bucket(0), table_size(0) {}
    
    HashMapForwardIterator(node_T *node, node_T **table, size_t bucket, size_t limit) :
            currentEntry(node), currentTable(table), current_bucket(bucket), table_size(limit) {}

    reference operator*() const {  return currentEntry->value; }

    // lame, but needed to be compatible with unordered_map
    std::pair<__keyType,__ValueType> * operator->() {
        if (currentEntry == NULL)
            return NULL;
        _compatibility_pair.first = currentEntry->key_value;
        _compatibility_pair.second = currentEntry->value;
        return &_compatibility_pair;
    }
    
    HashMapForwardIterator& operator++() {
        if (currentEntry)
            currentEntry = currentEntry->next;
        advance_to_non_empty();
        return *this;
    }
    
    HashMapForwardIterator operator++(int) {
        HashMapForwardIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const HashMapForwardIterator<__keyType,__ValueType>& y) const {
        if (currentEntry == NULL && y.currentEntry == NULL) // special case NULLs (aka end)
            return true;
        if (currentEntry != y.currentEntry)
            return false;
        if (current_bucket != y.current_bucket)
            return false;
        if (currentTable != y.currentTable)     // iter invalid if the table changed!
            return false;
        if (table_size != y.table_size)       // iter invalid if the table changed!
            return false;
        return true;
    }

    bool operator!=(const HashMapForwardIterator<__keyType,__ValueType>& other) const {
        return !(*this == other);
    }

    // map needs to use this, so can't be private
    void advance_to_non_empty() {
        if (currentEntry == NULL) {
            if (current_bucket >= table_size)
                return;
            
            for ( ++current_bucket; current_bucket < table_size; ++current_bucket)
                if (currentTable[current_bucket] != NULL) {
                    currentEntry = currentTable[current_bucket];
                    break;
                }
        }
    }

};

/******************************************************************************/

template<typename __keyType, typename __ValueType>
struct ConstHashMapForwardIterator {

    typedef HashNodeBase<__keyType,__ValueType> node_T;
    
    typedef ptrdiff_t                   difference_type;
    typedef std::input_iterator_tag     iterator_category;
    typedef __ValueType                 value_type;
    typedef const __ValueType*          pointer;
    typedef const __ValueType&          reference;
    typedef HashMapForwardIterator<__keyType,__ValueType> non_const_iterator;

    const node_T *currentEntry;
    node_T **currentTable;
    size_t current_bucket;
    size_t table_size;
    std::pair<__keyType,__ValueType>  _compatibility_pair;
    
    ConstHashMapForwardIterator() : currentEntry(NULL), currentTable(NULL), current_bucket(0), table_size(0) {}
    
    ConstHashMapForwardIterator(node_T *node, node_T **table, size_t bucket, size_t limit) :
            currentEntry(node), currentTable(table), current_bucket(bucket), table_size(limit) {}
    
    ConstHashMapForwardIterator(const non_const_iterator &iter) :
            currentEntry(iter.currentEntry), currentTable(iter.currentTable),
            current_bucket(iter.current_bucket), table_size(iter.table_size) {}

    reference operator*() const {  return currentEntry->value; }

    // lame, but needed to be compatible with unordered_map
    const std::pair<__keyType,__ValueType> * operator->() {
        if (currentEntry == NULL)
            return NULL;
        _compatibility_pair.first = currentEntry->key_value;
        _compatibility_pair.second = currentEntry->value;
        return &_compatibility_pair;
    }
    
    ConstHashMapForwardIterator& operator++() {
        if (currentEntry)
            currentEntry = currentEntry->next;
        advance_to_non_empty();
        return *this;
    }
    
    ConstHashMapForwardIterator operator++(int) {
        ConstHashMapForwardIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const ConstHashMapForwardIterator<__keyType,__ValueType>& y) const {
        if (currentEntry == NULL && y.currentEntry == NULL) // special case NULLs (aka end)
            return true;
        if (currentEntry != y.currentEntry)
            return false;
        if (current_bucket != y.current_bucket)
            return false;
        if (currentTable != y.currentTable)     // iter invalid if the table changed!
            return false;
        if (table_size != y.table_size)       // iter invalid if the table changed!
            return false;
        return true;
    }

    bool operator!=(const ConstHashMapForwardIterator<__keyType,__ValueType>& other) const {
        return !(*this == other);
    }

    // map needs to use this, so can't be private
    void advance_to_non_empty() {
        if (currentEntry == NULL) {
            if (current_bucket >= table_size)
                return;
            
            for ( ++current_bucket; current_bucket < table_size; ++current_bucket)
                if (currentTable[current_bucket] != NULL) {
                    currentEntry = currentTable[current_bucket];
                    break;
                }
        }
    }

};

/******************************************************************************/
/******************************************************************************/

// the obvious - allocate and delete each thing separately
template<typename __keyType, typename __ValueType>
struct HashMapBaseAllocator {
    
    typedef HashNodeBase<__keyType,__ValueType>      node_base_type;

    node_base_type *allocate_node() {
        node_base_type *node = new node_base_type;
        return node;
    }
    
    void release_node(const node_base_type *node) {
        delete node;
    }
    
    void clear_pool() {}
};

/******************************************************************************/

// less obvious, higher performance: allocate nodes in blocks, track unused nodes
template<typename __keyType, typename __ValueType>
struct HashMapPoolAllocator {

    typedef HashNodeBase<__keyType,__ValueType>      node_base_type;
    typedef PooledHashNode<__keyType,__ValueType>    node_pooled_type;
    typedef std::deque< node_pooled_type >           pool_type;
    typedef std::deque< size_t >                     free_node_list_type;

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
    
    void clear_pool() {
        empty_slots.clear();
        node_pool.clear();
    }
    
    const pool_type &get_node_pool() {
        return node_pool;
    }

private:
    free_node_list_type     empty_slots;
    pool_type               node_pool;
};

/******************************************************************************/
/******************************************************************************/

template<typename __keyType, typename __ValueType, class _Alloc>
struct HashMapBase {

    typedef __ValueType                                           value_type;
    typedef __keyType                                             key_type;
    typedef HashNodeBase<__keyType,__ValueType>                   node_T;
    typedef node_T *                                              node_ptr;
    typedef HashMapForwardIterator<__keyType,__ValueType>         iterator;
    typedef ConstHashMapForwardIterator<__keyType,__ValueType>    const_iterator;
    
    HashMapBase() : hash_table(NULL), hash_table_size(0), hash_reallocation_limit(0), entryCount(0), target_load_factor(1.0) {}
    
    HashMapBase( const HashMapBase &other ) {
        copy_from( other );
    }
    
    ~HashMapBase() {
        delete[] hash_table;
    }
    
    HashMapBase &operator=( const HashMapBase &other ) {
        copy_from(other);
        return *this;
    }
    
    iterator begin() const {
        iterator temp(hash_table[0], hash_table, 0, hash_table_size);
        temp.advance_to_non_empty();
        return temp;
    }
    
    iterator end() const {
        return iterator(NULL, hash_table, 0, 0);
    }
    
    const_iterator cbegin() const {
        const_iterator temp(hash_table[0], hash_table, 0, hash_table_size);
        temp.advance_to_non_empty();
        return temp;
    }
    
    const_iterator cend() const {
        return const_iterator(NULL, hash_table, 0, 0);
    }
    
    bool empty() const {
        return (entryCount == 0);
    }
    
    size_t size() const {
        return entryCount;
    }
    
    size_t bucket_count() const {
        return hash_table_size;
    }
    
    size_t bucket_size( size_t index ) const {
        node_ptr cur = hash_table[index];
        size_t count = 0;
        while (cur != NULL) {
            ++count;
            cur = cur->next;
        }
        return count;
    }
    
    size_t bucket( const __keyType & key) const {
        if (hash_table_size == 0)
            return 0;
        size_t index = calc_hash_index( key );
        return index;
    }
    
    void clear() {
        // walk the table and delete nodes
        // REVISIT - could be faster for pooled, just clear the pool and set table entries to NULL
        for (size_t index = 0; index < hash_table_size; ++index)
            {
            const node_T * current = hash_table[index];
            hash_table[index] = NULL;
            while (current != NULL)
                {
                auto old = current;
                current = current->next;
                release_node( old );
                }
            }
        
        // reset just the count of items, not the structure of the map
        entryCount = 0;
    }
    
    void insert( const __keyType &key, const __ValueType &value ) {
        add_entry( key, value);
    }
    
    void insert( const std::pair<__keyType, __ValueType> & new_pair ) {
        add_entry( new_pair.first, new_pair.second);
    }
    
    iterator find( const __keyType &key ) {
        return find_entry_iterator( key );
    }
    
    const_iterator find( const __keyType &key ) const {
        return const_iterator( find_entry_iterator( key ) );
    }
    
    std::pair<iterator,iterator> equal_range(const __keyType &key ) const {
        auto iter = find_entry_iterator( key );
        auto next = iter;
        if (iter != end()) ++next;
        return std::pair<iterator,iterator>(iter,next);       // base map only allows a single value per key
    }
    
    bool contains( const __keyType &key ) const {
        node_ptr found = find_entry( key );
        return bool(found != NULL);
    }
    
    size_t count( const __keyType &key ) const {
        node_ptr found = find_entry( key );
        return size_t(found != NULL);       // base map only allows a single value per key
    }
    
    __ValueType lookup( const __keyType &key ) const {
        node_ptr found = find_entry( key );
        if (!found)
            throw(std::out_of_range("hashmap value not found"));
        return found->value;
    }
    
    __ValueType& at( const __keyType &key ) {
        node_ptr found = find_entry( key );
        if (!found)
            throw(std::out_of_range("hashmap value not found"));
        return found->value;
    }
    
    void erase( const iterator &first, const iterator &last ) {
        iterator current = first;
        while (current != last) {
            iterator temp = current;
            ++current;
            remove_entry( temp );
        }
    }
    
    void erase( const iterator &entry ) {
        remove_entry(entry);
    }
    
    void erase( const __keyType &key ) {
        const iterator item = find( key );      // REVISIT - this could be done more efficiently by combining the functions and not using the iterator
        remove_entry( item );
    }
    
    __ValueType & operator[](const __keyType & key) {
        if (hash_reallocation_limit == 0)
            grow_hash_table();
    
        size_t index = calc_hash_index( key );
        node_ptr current = hash_table[index];
        while (current != NULL) {
            if (current->key_value == key)
                break;
            current = current->next;
        }

        // return existing entry, or add a new entry with default value
        if (current != NULL)
            return current->value;
        else {
            node_ptr new_entry = allocate_node();
            new_entry->key_value = key;
            new_entry->value = __ValueType(); // default initializer
            new_entry->next = hash_table[index];
            hash_table[index] = new_entry;
            ++entryCount;
            if ( entryCount > hash_reallocation_limit)
                grow_hash_table();
            return new_entry->value;    // we may not know where it is located anymore, but do know the value
        }
    }
    
    float load_factor() const {
        return float(entryCount) / float(hash_table_size);
    }
    
    float max_load_factor() const {
        return target_load_factor;
    }
    
    void max_load_factor(float x) {
        target_load_factor = x;
        reserve( entryCount );
    }
    
    void rehash( size_t entries ) {
        hash_reallocation_limit = std::max( size_t(8), entries );
        grow_hash_table();
    }
    
    void reserve( size_t entries ) {
        size_t temp_limit( ceilf( float(entries) / target_load_factor ) );
        rehash( temp_limit );
    }

private:

    size_t  calc_hash_index( const __keyType &key ) const {
        size_t hash_value = std::hash<__keyType>{}(key);
        size_t index = hash_value % hash_table_size;    // expensive operation, even in integer, but better results than bit masking and subtracting
        return index;
    }

    void grow_hash_table() {        // can also reduce via reserve() and load_factor
        size_t new_size = hash_reallocation_limit;  // zero, or our next growth size
        size_t old_size = hash_table_size;
        
        if (new_size == 0) {
            new_size = 8;
        }
        
        size_t new_max( ceilf( float(new_size) / target_load_factor ) );
        new_max = std::max( size_t(8), new_max );
        size_t new_limit = new_max + (new_max / 2);    // * 1.5
        
        // save our new size and upper limit
        hash_table_size = new_size;
        hash_reallocation_limit = new_limit;
        
        if (old_size == new_size)
            return;
        
        // allocate a new table
        node_T **new_table = new node_T *[new_size];
        
        // initialize table to NULL
        for (size_t i = 0; i < new_size; ++i)
            new_table[i] = NULL;
        
        // rehash everything from the old table into the new table
        hash_table2table( old_size, hash_table, new_table );
        
        delete[] hash_table;        // delete the old table (does not delete nodes)
        hash_table = new_table;     // replace with the new table
    }
    
    void hash_table2table( size_t oldsize, node_T **oldtable, node_T **newtable ) {
        // iterate old table, move entries to new table
        for (size_t i = 0; i < oldsize; ++i ) {
            node_ptr current = oldtable[i];
            while (current) {
                node_ptr next = current->next;
                size_t index = calc_hash_index( current->key_value );
                current->next = newtable[index];
                newtable[index] = current;
                current = next;
            }
        }
    }
    
    void add_entry( const __keyType &key, const __ValueType &value) {
        if (hash_reallocation_limit == 0)
            grow_hash_table();
        
        size_t index = calc_hash_index( key );
        node_ptr current = hash_table[index];
        
        // find any existing entry matching this key
        while (current) {
            if (current->key_value == key)
                break;
            current = current->next;
        }
        
        // replace existing entry, or add a new entry
        if (current)
            current->value = value;
        else {
            node_ptr new_entry = allocate_node();
            new_entry->key_value = key;
            new_entry->value = value;
            new_entry->next = hash_table[index];
            hash_table[index] = new_entry;
            ++entryCount;
            if ( entryCount > hash_reallocation_limit)
                grow_hash_table();
        }
    }
    
    void remove_entry( const iterator &entry ) {
        size_t index = entry.current_bucket;
        node_ptr prev = hash_table[index];
        node_ptr next = NULL;
        
        // find previous node
        while (prev && prev->next != entry.currentEntry) {
            prev = prev->next;
        }
        
        // find next node
        if (entry.currentEntry)
            next = entry.currentEntry->next;
        
        // patch up the list
        if (prev && prev != entry.currentEntry)
            prev->next = next;
            
        if (hash_table[index] == entry.currentEntry)
            hash_table[index] = next;

        release_node(entry.currentEntry);
        --entryCount;
    }

    node_T *find_entry( const __keyType & key ) const {
        if (hash_table_size == 0)
            return NULL;
        size_t index = calc_hash_index( key );
        node_ptr current = hash_table[index];
        while (current) {
            if (current->key_value == key)
                break;
            current = current->next;
        }
        return current;
    }
    
    iterator find_entry_iterator( const __keyType & key ) const {
        if (hash_table_size == 0) {
            iterator temp(NULL, hash_table, 0, 0);
            return temp;
        }
        size_t index = calc_hash_index( key );
        node_ptr current = hash_table[index];
        while (current) {
            if (current->key_value == key)
                break;
            current = current->next;
        }
        iterator temp(current, hash_table, index, hash_table_size);
        return temp;
    }
    
    void copy_from( const HashMapBase &other )
        {
        hash_table_size = other.hash_table_size;
        hash_reallocation_limit = other.hash_reallocation_limit;
        entryCount = other.entryCount;
        target_load_factor = other.target_load_factor;
        
        // free existing tables and data
        delete[] hash_table;
        clear_pool();
        
        // allocate new table
        hash_table = new node_T *[hash_table_size];

        // walk the existing table and duplicate nodes (saves a lot of hashing over iterate and insert)
        for (size_t index = 0; index < hash_table_size; ++index)
            {
            node_ptr current_old = other.hash_table[index];
            hash_table[index] = NULL;
            while (current_old != NULL)
                {
                node_ptr new_entry = allocate_node();
                new_entry->key_value = current_old->key_value;
                new_entry->value = current_old->value;
                new_entry->next = hash_table[index];
                hash_table[index] = new_entry;
                current_old = current_old->next;
                }
            }
        }

protected:
    node_T *allocate_node() { return allocator_data.allocate_node(); }
    
    void release_node(const node_T *node) { allocator_data.release_node( node ); }
    
    void clear_pool() { return allocator_data.clear_pool(); }
    
    _Alloc       allocator_data;

private:
    size_t entryCount;              // actual number of entries added
    size_t hash_reallocation_limit; // entry count when we need to reallocate and rehash
    
    size_t hash_table_size;     // number of table entries
    node_T **hash_table;        // primary storage of pointers to linked lists
    
    float  target_load_factor; // maximum load factor allowed, normally 1.0
};

/******************************************************************************/

template<typename __keyType, typename __ValueType>
struct HashMap : public HashMapBase<__keyType, __ValueType, HashMapBaseAllocator<__keyType, __ValueType> >
{
    typedef HashMapBase<__keyType, __ValueType, HashMapBaseAllocator<__keyType, __ValueType> > _parent;

    HashMap() : _parent() {}
    
    ~HashMap() {
        _parent::clear();        // here we must delete each node
    }
};

/******************************************************************************/
/******************************************************************************/

template<typename __keyType, typename __ValueType>
struct ConstHashPoolIterator {

    typedef HashNodeBase<__keyType,__ValueType>      node_base_type;
    typedef PooledHashNode<__keyType,__ValueType>    node_pooled_type;
    typedef ptrdiff_t                            difference_type;
    typedef std::bidirectional_iterator_tag      iterator_category;
    typedef __ValueType                 value_type;
    typedef __ValueType*                pointer;
    typedef __ValueType&                reference;
    typedef std::deque< node_pooled_type >              linked_pool_type;
    typedef typename linked_pool_type::const_iterator   pool_iter;

    const linked_pool_type & node_pool;
    pool_iter                current_iter;
    std::pair<__keyType,__ValueType>  _compatibility_pair;
    
    ConstHashPoolIterator() {}
    ConstHashPoolIterator(const linked_pool_type &pool, pool_iter iter) :
                            node_pool(pool), current_iter(iter) {}
    
    reference operator*() const { return current_iter->value; }

    // lame, but needed to be compatible with unordered_map
    const std::pair<__keyType,__ValueType> * operator->() {
        _compatibility_pair.first = current_iter->key_value;
        _compatibility_pair.second = current_iter->value;
        return &_compatibility_pair;
    }
    
    ConstHashPoolIterator& operator++() {
        ++current_iter;
        while (current_iter != node_pool.end() && current_iter->pool_index == size_t(-1))        // skip unused entries
            ++current_iter;
        return *this;
    }
    
    ConstHashPoolIterator& operator--() {
        --current_iter;
        while (current_iter != node_pool.begin() && current_iter->pool_index == size_t(-1))        // skip unused entries
            --current_iter;
        return *this;
    }
    
    ConstHashPoolIterator operator++(int) {
        ConstHashPoolIterator tmp = *this;
        ++current_iter;
        while (current_iter != node_pool.end() && current_iter->pool_index == size_t(-1))        // skip unused entries
            ++current_iter;
        return tmp;
    }
    
    ConstHashPoolIterator operator--(int) {
        ConstHashPoolIterator tmp = *this;
        --current_iter;
        while (current_iter != node_pool.begin() && current_iter->pool_index == size_t(-1))        // skip unused entries
            --current_iter;
        return tmp;
    }
    
    bool operator==(const ConstHashPoolIterator<__keyType,__ValueType> & y) const {
        return current_iter == y.current_iter;
    }

    bool operator!=(const ConstHashPoolIterator<__keyType,__ValueType> & other) const {
        return !(*this == other);
    }
};

/******************************************************************************/

template<typename __keyType, typename __ValueType>
struct PooledHashMap : public HashMapBase<__keyType, __ValueType, HashMapPoolAllocator<__keyType, __ValueType> >
{
    typedef HashMapBase<__keyType, __ValueType, HashMapPoolAllocator<__keyType, __ValueType> > _parent;
    typedef ConstHashPoolIterator<__keyType,__ValueType>    pool_iter;

    PooledHashMap() : _parent() {}
    
    ~PooledHashMap() {
        // here, deleting the allocator deletes all nodes at once
    }
    
    // unordered iterators -- iterate the pool instead of the linked list (better cache coherency, no pointer chasing)
    // Always const, because changing the data in the pool could be dangerous  (though I can see some situations where it might be useful)
    // No reverse, because this is unordered data.
    pool_iter cubegin() {
        return pool_iter(_parent::allocator_data.get_node_pool(), _parent::allocator_data.get_node_pool().begin());
    }
    
    pool_iter cuend() {
        return pool_iter(_parent::allocator_data.get_node_pool(), _parent::allocator_data.get_node_pool().end());
    }
};

/******************************************************************************/

#endif /* container_hashmap_h */
