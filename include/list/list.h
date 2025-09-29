#include <memory>
#include <iterator>
#include <initializer_list>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <limits>

template <typename T, typename Allocator = std::allocator<T>>
class list {
private:
    struct NodeBase {
        NodeBase* prev;
        NodeBase* next;
        
        NodeBase() : prev(this), next(this) {}
    };
    
    struct Node : NodeBase {
        T data;
        
        template<typename... Args>
        Node(Args&&... args) : NodeBase(), data(std::forward<Args>(args)...) {}
    };
    
    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAllocator>;

public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    template<typename ValueType>
    class list_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = ValueType;
        using difference_type = std::ptrdiff_t;
        using pointer = ValueType*;
        using reference = ValueType&;
        
        NodeBase* node;
        
        list_iterator() : node(nullptr) {}
        explicit list_iterator(NodeBase* n) : node(n) {}
        
        reference operator*() const { 
            return static_cast<Node*>(node)->data; 
        }
        
        pointer operator->() const { 
            return &(static_cast<Node*>(node)->data); 
        }
        
        list_iterator& operator++() {
            node = node->next;
            return *this;
        }
        
        list_iterator operator++(int) {
            list_iterator tmp = *this;
            node = node->next;
            return tmp;
        }
        
        list_iterator& operator--() {
            node = node->prev;
            return *this;
        }
        
        list_iterator operator--(int) {
            list_iterator tmp = *this;
            node = node->prev;
            return tmp;
        }
        
        bool operator==(const list_iterator& other) const { return node == other.node; }
        bool operator!=(const list_iterator& other) const { return node != other.node; }
        
        template<typename U = ValueType>
        operator list_iterator<const U>() const {
            return list_iterator<const U>(node);
        }
    };
    
    using iterator = list_iterator<T>;
    using const_iterator = list_iterator<const T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    NodeBase sentinel;
    size_type sz;
    NodeAllocator alloc;
    
    template<typename... Args>
    Node* create_node(Args&&... args) {
        Node* n = NodeAllocTraits::allocate(alloc, 1);
        try {
            NodeAllocTraits::construct(alloc, n, std::forward<Args>(args)...);
            n->prev = nullptr;
            n->next = nullptr;
        } catch (...) {
            NodeAllocTraits::deallocate(alloc, n, 1);
            throw;
        }
        return n;
    }
    
    void destroy_node(Node* n) {
        NodeAllocTraits::destroy(alloc, n);
        NodeAllocTraits::deallocate(alloc, n, 1);
    }
    
    void link_nodes(NodeBase* prev, NodeBase* next) {
        prev->next = next;
        next->prev = prev;
    }
    
    void insert_node(NodeBase* pos, Node* n) {
        NodeBase* prev = pos->prev;
        link_nodes(prev, n);
        link_nodes(n, pos);
    }
    
    void unlink_node(NodeBase* n) {
        link_nodes(n->prev, n->next);
    }

public:
    list() : sentinel(), sz(0), alloc() {}
    
    explicit list(const Allocator& a) : sentinel(), sz(0), alloc(a) {}
    
    explicit list(size_type count, const Allocator& a = Allocator())
        : sentinel(), sz(0), alloc(a) {
        for (size_type i = 0; i < count; ++i) {
            emplace_back();
        }
    }
    
    list(size_type count, const T& value, const Allocator& a = Allocator())
        : sentinel(), sz(0), alloc(a) {
        for (size_type i = 0; i < count; ++i) {
            push_back(value);
        }
    }
    
    template<typename InputIt, typename = std::enable_if_t<!std::is_integral<InputIt>::value>>
    list(InputIt first, InputIt last, const Allocator& a = Allocator())
        : sentinel(), sz(0), alloc(a) {
        for (; first != last; ++first) {
            push_back(*first);
        }
    }
    
    list(const list& other)
        : sentinel(), sz(0), 
          alloc(NodeAllocTraits::select_on_container_copy_construction(other.alloc)) {
        for (const auto& val : other) {
            push_back(val);
        }
    }
    
    list(list&& other) noexcept
        : sentinel(), sz(other.sz), alloc(std::move(other.alloc)) {
        if (other.sz > 0) {
            NodeBase* first = other.sentinel.next;
            NodeBase* last = other.sentinel.prev;
            
            link_nodes(&sentinel, first);
            link_nodes(last, &sentinel);
            
            other.sentinel.next = &other.sentinel;
            other.sentinel.prev = &other.sentinel;
            other.sz = 0;
        }
    }
    
    list(std::initializer_list<T> init, const Allocator& a = Allocator())
        : sentinel(), sz(0), alloc(a) {
        for (const auto& val : init) {
            push_back(val);
        }
    }
    
    ~list() noexcept {
        clear();
    }
    
    list& operator=(const list& other) {
        if (this != &other) {
            if (NodeAllocTraits::propagate_on_container_copy_assignment::value) {
                clear();
                alloc = other.alloc;
            } else {
                clear();
            }
            for (const auto& val : other) {
                push_back(val);
            }
        }
        return *this;
    }
    
    list& operator=(list&& other) noexcept(
        NodeAllocTraits::propagate_on_container_move_assignment::value ||
        NodeAllocTraits::is_always_equal::value) {
        if (this != &other) {
            clear();
            
            if (NodeAllocTraits::propagate_on_container_move_assignment::value) {
                alloc = std::move(other.alloc);
            }
            
            if (other.sz > 0) {
                NodeBase* first = other.sentinel.next;
                NodeBase* last = other.sentinel.prev;
                
                link_nodes(&sentinel, first);
                link_nodes(last, &sentinel);
                
                sz = other.sz;
                
                other.sentinel.next = &other.sentinel;
                other.sentinel.prev = &other.sentinel;
                other.sz = 0;
            }
        }
        return *this;
    }
    
    list& operator=(std::initializer_list<T> init) {
        assign(init);
        return *this;
    }
    
    void assign(size_type count, const T& value) {
        clear();
        for (size_type i = 0; i < count; ++i) {
            push_back(value);
        }
    }
    
    template<typename InputIt, typename = std::enable_if_t<!std::is_integral<InputIt>::value>>
    void assign(InputIt first, InputIt last) {
        clear();
        for (; first != last; ++first) {
            push_back(*first);
        }
    }
    
    void assign(std::initializer_list<T> init) {
        clear();
        for (const auto& val : init) {
            push_back(val);
        }
    }
    
    allocator_type get_allocator() const { return allocator_type(alloc); }
    
    reference front() { return static_cast<Node*>(sentinel.next)->data; }
    const_reference front() const { return static_cast<Node*>(sentinel.next)->data; }
    reference back() { return static_cast<Node*>(sentinel.prev)->data; }
    const_reference back() const { return static_cast<Node*>(sentinel.prev)->data; }
    
    iterator begin() noexcept { return iterator(sentinel.next); }
    const_iterator begin() const noexcept { return const_iterator(sentinel.next); }
    const_iterator cbegin() const noexcept { return const_iterator(sentinel.next); }
    
    iterator end() noexcept { return iterator(&sentinel); }
    const_iterator end() const noexcept { return const_iterator(const_cast<NodeBase*>(&sentinel)); }
    const_iterator cend() const noexcept { return const_iterator(const_cast<NodeBase*>(&sentinel)); }
    
    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }
    
    [[nodiscard]] bool empty() const noexcept { return sz == 0; }
    size_type size() const noexcept { return sz; }
    size_type max_size() const noexcept { return NodeAllocTraits::max_size(alloc); }
    
    void clear() noexcept {
        NodeBase* curr = sentinel.next;
        while (curr != &sentinel) {
            NodeBase* next = curr->next;
            destroy_node(static_cast<Node*>(curr));
            curr = next;
        }
        sentinel.next = &sentinel;
        sentinel.prev = &sentinel;
        sz = 0;
    }
    
    iterator insert(const_iterator pos, const T& value) {
        Node* n = create_node(value);
        insert_node(pos.node, n);
        ++sz;
        return iterator(n);
    }
    
    iterator insert(const_iterator pos, T&& value) {
        Node* n = create_node(std::move(value));
        insert_node(pos.node, n);
        ++sz;
        return iterator(n);
    }
    
    iterator insert(const_iterator pos, size_type count, const T& value) {
        if (count == 0) {
            return iterator(pos.node);
        }
        
        iterator ret = insert(pos, value);
        for (size_type i = 1; i < count; ++i) {
            insert(pos, value);
        }
        return ret;
    }
    
    template<typename InputIt, typename = std::enable_if_t<!std::is_integral<InputIt>::value>>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        if (first == last) {
            return iterator(pos.node);
        }
        
        iterator ret = insert(pos, *first);
        ++first;
        for (; first != last; ++first) {
            insert(pos, *first);
        }
        return ret;
    }
    
    iterator insert(const_iterator pos, std::initializer_list<T> init) {
        return insert(pos, init.begin(), init.end());
    }
    
    template<typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        Node* n = create_node(std::forward<Args>(args)...);
        insert_node(pos.node, n);
        ++sz;
        return iterator(n);
    }
    
    iterator erase(const_iterator pos) {
        NodeBase* n = pos.node;
        NodeBase* next = n->next;
        unlink_node(n);
        destroy_node(static_cast<Node*>(n));
        --sz;
        return iterator(next);
    }
    
    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            first = erase(first);
        }
        return iterator(last.node);
    }
    
    void push_back(const T& value) {
        insert(end(), value);
    }
    
    void push_back(T&& value) {
        insert(end(), std::move(value));
    }
    
    template<typename... Args>
    reference emplace_back(Args&&... args) {
        emplace(end(), std::forward<Args>(args)...);
        return back();
    }
    
    void pop_back() {
        erase(--end());
    }
    
    void push_front(const T& value) {
        insert(begin(), value);
    }
    
    void push_front(T&& value) {
        insert(begin(), std::move(value));
    }
    
    template<typename... Args>
    reference emplace_front(Args&&... args) {
        emplace(begin(), std::forward<Args>(args)...);
        return front();
    }
    
    void pop_front() {
        erase(begin());
    }
    
    void resize(size_type count) {
        if (count < sz) {
            auto it = begin();
            std::advance(it, count);
            erase(it, end());
        } else {
            for (size_type i = sz; i < count; ++i) {
                emplace_back();
            }
        }
    }
    
    void resize(size_type count, const value_type& value) {
        if (count < sz) {
            auto it = begin();
            std::advance(it, count);
            erase(it, end());
        } else {
            for (size_type i = sz; i < count; ++i) {
                push_back(value);
            }
        }
    }
    
    void swap(list& other) noexcept(NodeAllocTraits::is_always_equal::value) {
        if (NodeAllocTraits::propagate_on_container_swap::value) {
            std::swap(alloc, other.alloc);
        }
        
        // Handle empty lists
        if (sz == 0 && other.sz == 0) {
            return;
        }
        
        if (sz == 0) {
            NodeBase* first = other.sentinel.next;
            NodeBase* last = other.sentinel.prev;
            link_nodes(&sentinel, first);
            link_nodes(last, &sentinel);
            other.sentinel.next = &other.sentinel;
            other.sentinel.prev = &other.sentinel;
        } else if (other.sz == 0) {
            NodeBase* first = sentinel.next;
            NodeBase* last = sentinel.prev;
            link_nodes(&other.sentinel, first);
            link_nodes(last, &other.sentinel);
            sentinel.next = &sentinel;
            sentinel.prev = &sentinel;
        } else {
            NodeBase* this_first = sentinel.next;
            NodeBase* this_last = sentinel.prev;
            NodeBase* other_first = other.sentinel.next;
            NodeBase* other_last = other.sentinel.prev;
            
            link_nodes(&sentinel, other_first);
            link_nodes(other_last, &sentinel);
            link_nodes(&other.sentinel, this_first);
            link_nodes(this_last, &other.sentinel);
        }
        
        std::swap(sz, other.sz);
    }
    
    void merge(list& other) {
        merge(std::move(other), std::less<T>());
    }
    
    void merge(list&& other) {
        merge(std::move(other), std::less<T>());
    }
    
    template<typename Compare>
    void merge(list& other, Compare comp) {
        merge(std::move(other), comp);
    }
    
    template<typename Compare>
    void merge(list&& other, Compare comp) {
        if (this == &other) return;
        
        auto it1 = begin();
        auto it2 = other.begin();
        
        while (it1 != end() && it2 != other.end()) {
            if (comp(*it2, *it1)) {
                auto next = std::next(it2);
                splice(it1, other, it2);
                it2 = next;
            } else {
                ++it1;
            }
        }
        
        if (it2 != other.end()) {
            splice(end(), other, it2, other.end());
        }
    }
    
    void splice(const_iterator pos, list& other) {
        splice(pos, std::move(other));
    }
    
    void splice(const_iterator pos, list&& other) {
        if (other.empty()) return;
        splice(pos, std::move(other), other.begin(), other.end());
    }
    
    void splice(const_iterator pos, list& other, const_iterator it) {
        splice(pos, std::move(other), it);
    }
    
    void splice(const_iterator pos, list&& other, const_iterator it) {
        auto next = std::next(it);
        if (pos == it || pos == next) return;
        
        NodeBase* n = it.node;
        unlink_node(n);
        insert_node(pos.node, n);
        
        --other.sz;
        ++sz;
    }
    
    void splice(const_iterator pos, list& other, const_iterator first, const_iterator last) {
        splice(pos, std::move(other), first, last);
    }
    
    void splice(const_iterator pos, list&& other, const_iterator first, const_iterator last) {
        if (first == last) return;
        
        size_type count = std::distance(first, last);
        
        NodeBase* first_node = first.node;
        NodeBase* last_node = last.node;
        NodeBase* last_prev = last_node->prev;
        
        link_nodes(first_node->prev, last_node);
        
        NodeBase* pos_prev = pos.node->prev;
        link_nodes(pos_prev, first_node);
        link_nodes(last_prev, pos.node);
        
        other.sz -= count;
        sz += count;
    }
    
    size_type remove(const T& value) {
        return remove_if([&value](const T& x) { return x == value; });
    }
    
    template<typename UnaryPredicate>
    size_type remove_if(UnaryPredicate p) {
        size_type removed = 0;
        auto it = begin();
        while (it != end()) {
            if (p(*it)) {
                it = erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        return removed;
    }
    
    void reverse() noexcept {
        if (sz <= 1) return;
        
        NodeBase* curr = &sentinel;
        do {
            std::swap(curr->prev, curr->next);
            curr = curr->prev;
        } while (curr != &sentinel);
    }
    
    size_type unique() {
        return unique(std::equal_to<T>());
    }
    
    template<typename BinaryPredicate>
    size_type unique(BinaryPredicate p) {
        if (sz <= 1) return 0;
        
        size_type removed = 0;
        auto it = begin();
        auto prev = it++;
        
        while (it != end()) {
            if (p(*prev, *it)) {
                it = erase(it);
                ++removed;
            } else {
                prev = it++;
            }
        }
        return removed;
    }
    
    void sort() {
        sort(std::less<T>());
    }
    
    template<typename Compare>
    void sort(Compare comp) {
        if (sz <= 1) return;
        
        list carry;
        list counter[64];
        int fill = 0;
        
        while (!empty()) {
            carry.splice(carry.begin(), *this, begin());
            
            int i = 0;
            while (i < fill && !counter[i].empty()) {
                counter[i].merge(carry, comp);
                carry.swap(counter[i++]);
            }
            
            carry.swap(counter[i]);
            if (i == fill) ++fill;
        }
        
        for (int i = 1; i < fill; ++i) {
            counter[i].merge(counter[i - 1], comp);
        }
        
        swap(counter[fill - 1]);
    }
};

// Non-member functions
template<typename T, typename Alloc>
bool operator==(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename T, typename Alloc>
bool operator!=(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return !(lhs == rhs);
}

template<typename T, typename Alloc>
bool operator<(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename T, typename Alloc>
bool operator<=(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return !(rhs < lhs);
}

template<typename T, typename Alloc>
bool operator>(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return rhs < lhs;
}

template<typename T, typename Alloc>
bool operator>=(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return !(lhs < rhs);
}

template<typename T, typename Alloc>
void swap(list<T, Alloc>& lhs, list<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}