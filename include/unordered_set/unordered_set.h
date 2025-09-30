#include <functional>
#include <memory>
#include <utility>
#include <iterator>
#include <initializer_list>
#include <algorithm>

template<typename Value>
struct hash_node {
    Value value;
    hash_node* next;
    
    hash_node(const Value& v) : value(v), next(nullptr) {}
    hash_node(Value&& v) : value(std::move(v)), next(nullptr) {}
};

template<typename Value, typename Hash, typename KeyEqual, typename Allocator>
class unordered_set;

template<typename Value, typename Hash, typename KeyEqual, typename Allocator>
class unordered_set_iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Value;
    using difference_type = std::ptrdiff_t;
    using pointer = const Value*;
    using reference = const Value&;
    using node_type = hash_node<Value>;
    
private:
    node_type** buckets_;
    size_t bucket_count_;
    size_t current_bucket_;
    node_type* current_node_;
    
    friend class unordered_set<Value, Hash, KeyEqual, Allocator>;
    
    unordered_set_iterator(node_type** buckets, size_t bucket_count, 
                          size_t bucket_idx, node_type* node)
        : buckets_(buckets), bucket_count_(bucket_count),
          current_bucket_(bucket_idx), current_node_(node) {}
    
    void advance_to_next_node() {
        if (current_node_ && current_node_->next) {
            current_node_ = current_node_->next;
            return;
        }
        
        for (++current_bucket_; current_bucket_ < bucket_count_; ++current_bucket_) {
            if (buckets_[current_bucket_]) {
                current_node_ = buckets_[current_bucket_];
                return;
            }
        }
        current_node_ = nullptr;
    }
    
public:
    unordered_set_iterator() 
        : buckets_(nullptr), bucket_count_(0), 
          current_bucket_(0), current_node_(nullptr) {}
    
    reference operator*() const { return current_node_->value; }
    pointer operator->() const { return &(current_node_->value); }
    
    unordered_set_iterator& operator++() {
        advance_to_next_node();
        return *this;
    }
    
    unordered_set_iterator operator++(int) {
        unordered_set_iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    
    bool operator==(const unordered_set_iterator& other) const {
        return current_node_ == other.current_node_;
    }
    
    bool operator!=(const unordered_set_iterator& other) const {
        return !(*this == other);
    }
};

template<
    typename Key,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<Key>
>
class unordered_set {
public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using reference = const value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using iterator = unordered_set_iterator<Key, Hash, KeyEqual, Allocator>;
    using const_iterator = iterator;
    
private:
    using node_type = hash_node<Key>;
    using node_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<node_type>;
    using bucket_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<node_type*>;
    
    node_type** buckets_;
    size_type bucket_count_;
    size_type size_;
    float max_load_factor_;
    hasher hash_;
    key_equal equal_;
    node_allocator node_alloc_;
    bucket_allocator bucket_alloc_;
    
    static constexpr size_type default_bucket_count = 16;
    static constexpr float default_max_load_factor = 1.0f;
    
    void allocate_buckets(size_type count) {
        buckets_ = bucket_alloc_.allocate(count);
        for (size_type i = 0; i < count; ++i) {
            buckets_[i] = nullptr;
        }
        bucket_count_ = count;
    }
    
    void deallocate_buckets() {
        if (buckets_) {
            bucket_alloc_.deallocate(buckets_, bucket_count_);
            buckets_ = nullptr;
        }
    }
    
    void clear_all_nodes() {
        for (size_type i = 0; i < bucket_count_; ++i) {
            node_type* node = buckets_[i];
            while (node) {
                node_type* next = node->next;
                std::allocator_traits<node_allocator>::destroy(node_alloc_, node);
                node_alloc_.deallocate(node, 1);
                node = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }
    
    size_type bucket_index(const Key& key) const {
        return hash_(key) % bucket_count_;
    }
    
    void rehash_impl(size_type new_bucket_count) {
        if (new_bucket_count < size_ / max_load_factor_) {
            new_bucket_count = static_cast<size_type>(size_ / max_load_factor_) + 1;
        }
        
        node_type** new_buckets = bucket_alloc_.allocate(new_bucket_count);
        for (size_type i = 0; i < new_bucket_count; ++i) {
            new_buckets[i] = nullptr;
        }
        
        for (size_type i = 0; i < bucket_count_; ++i) {
            node_type* node = buckets_[i];
            while (node) {
                node_type* next = node->next;
                size_type new_idx = hash_(node->value) % new_bucket_count;
                node->next = new_buckets[new_idx];
                new_buckets[new_idx] = node;
                node = next;
            }
        }
        
        deallocate_buckets();
        buckets_ = new_buckets;
        bucket_count_ = new_bucket_count;
    }
    
    void check_and_rehash() {
        if (static_cast<float>(size_ + 1) > bucket_count_ * max_load_factor_) {
            rehash_impl(bucket_count_ * 2);
        }
    }
    
public:
    unordered_set() 
        : buckets_(nullptr), bucket_count_(0), size_(0),
          max_load_factor_(default_max_load_factor) {
        allocate_buckets(default_bucket_count);
    }
    
    explicit unordered_set(size_type bucket_count,
                          const Hash& hash = Hash(),
                          const KeyEqual& equal = KeyEqual(),
                          const Allocator& alloc = Allocator())
        : buckets_(nullptr), bucket_count_(0), size_(0),
          max_load_factor_(default_max_load_factor),
          hash_(hash), equal_(equal), node_alloc_(alloc), bucket_alloc_(alloc) {
        allocate_buckets(bucket_count > 0 ? bucket_count : default_bucket_count);
    }
    
    unordered_set(std::initializer_list<value_type> init,
                 size_type bucket_count = default_bucket_count,
                 const Hash& hash = Hash(),
                 const KeyEqual& equal = KeyEqual(),
                 const Allocator& alloc = Allocator())
        : unordered_set(bucket_count, hash, equal, alloc) {
        for (const auto& val : init) {
            insert(val);
        }
    }
    
    unordered_set(const unordered_set& other)
        : buckets_(nullptr), bucket_count_(0), size_(0),
          max_load_factor_(other.max_load_factor_),
          hash_(other.hash_), equal_(other.equal_),
          node_alloc_(other.node_alloc_), bucket_alloc_(other.bucket_alloc_) {
        allocate_buckets(other.bucket_count_);
        for (const auto& val : other) {
            insert(val);
        }
    }
    
    unordered_set(unordered_set&& other) noexcept
        : buckets_(other.buckets_), bucket_count_(other.bucket_count_),
          size_(other.size_), max_load_factor_(other.max_load_factor_),
          hash_(std::move(other.hash_)), equal_(std::move(other.equal_)),
          node_alloc_(std::move(other.node_alloc_)),
          bucket_alloc_(std::move(other.bucket_alloc_)) {
        other.buckets_ = nullptr;
        other.bucket_count_ = 0;
        other.size_ = 0;
    }
    
    ~unordered_set() {
        clear_all_nodes();
        deallocate_buckets();
    }
    
    unordered_set& operator=(const unordered_set& other) {
        if (this != &other) {
            unordered_set tmp(other);
            swap(tmp);
        }
        return *this;
    }
    
    unordered_set& operator=(unordered_set&& other) noexcept {
        if (this != &other) {
            clear_all_nodes();
            deallocate_buckets();
            
            buckets_ = other.buckets_;
            bucket_count_ = other.bucket_count_;
            size_ = other.size_;
            max_load_factor_ = other.max_load_factor_;
            hash_ = std::move(other.hash_);
            equal_ = std::move(other.equal_);
            node_alloc_ = std::move(other.node_alloc_);
            bucket_alloc_ = std::move(other.bucket_alloc_);
            
            other.buckets_ = nullptr;
            other.bucket_count_ = 0;
            other.size_ = 0;
        }
        return *this;
    }
    
    iterator begin() {
        for (size_type i = 0; i < bucket_count_; ++i) {
            if (buckets_[i]) {
                return iterator(buckets_, bucket_count_, i, buckets_[i]);
            }
        }
        return end();
    }
    
    const_iterator begin() const {
        for (size_type i = 0; i < bucket_count_; ++i) {
            if (buckets_[i]) {
                return const_iterator(buckets_, bucket_count_, i, buckets_[i]);
            }
        }
        return end();
    }
    
    const_iterator cbegin() const { return begin(); }
    
    iterator end() {
        return iterator(buckets_, bucket_count_, bucket_count_, nullptr);
    }
    
    const_iterator end() const {
        return const_iterator(buckets_, bucket_count_, bucket_count_, nullptr);
    }
    
    const_iterator cend() const { return end(); }
    
    bool empty() const { return size_ == 0; }
    size_type size() const { return size_; }
    size_type max_size() const { 
        return std::allocator_traits<node_allocator>::max_size(node_alloc_); 
    }
    
    void clear() {
        clear_all_nodes();
    }
    
    std::pair<iterator, bool> insert(const value_type& value) {
        check_and_rehash();
        
        size_type idx = bucket_index(value);
        node_type* node = buckets_[idx];
        
        while (node) {
            if (equal_(node->value, value)) {
                return {iterator(buckets_, bucket_count_, idx, node), false};
            }
            node = node->next;
        }

        node_type* new_node = node_alloc_.allocate(1);
        std::allocator_traits<node_allocator>::construct(node_alloc_, new_node, value);
        new_node->next = buckets_[idx];
        buckets_[idx] = new_node;
        ++size_;
        
        return {iterator(buckets_, bucket_count_, idx, new_node), true};
    }
    
    std::pair<iterator, bool> insert(value_type&& value) {
        check_and_rehash();
        
        size_type idx = bucket_index(value);
        node_type* node = buckets_[idx];
        
        while (node) {
            if (equal_(node->value, value)) {
                return {iterator(buckets_, bucket_count_, idx, node), false};
            }
            node = node->next;
        }
        
        node_type* new_node = node_alloc_.allocate(1);
        std::allocator_traits<node_allocator>::construct(node_alloc_, new_node, std::move(value));
        new_node->next = buckets_[idx];
        buckets_[idx] = new_node;
        ++size_;
        
        return {iterator(buckets_, bucket_count_, idx, new_node), true};
    }
    
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        node_type* temp_node = node_alloc_.allocate(1);
        std::allocator_traits<node_allocator>::construct(
            node_alloc_, temp_node, std::forward<Args>(args)...);
        
        size_type idx = bucket_index(temp_node->value);
        node_type* node = buckets_[idx];
        
        while (node) {
            if (equal_(node->value, temp_node->value)) {
                std::allocator_traits<node_allocator>::destroy(node_alloc_, temp_node);
                node_alloc_.deallocate(temp_node, 1);
                return {iterator(buckets_, bucket_count_, idx, node), false};
            }
            node = node->next;
        }
        
        check_and_rehash();
        idx = bucket_index(temp_node->value); 
        
        // Insert the node
        temp_node->next = buckets_[idx];
        buckets_[idx] = temp_node;
        ++size_;
        
        return {iterator(buckets_, bucket_count_, idx, temp_node), true};
    }
    
    size_type erase(const key_type& key) {
        size_type idx = bucket_index(key);
        node_type* node = buckets_[idx];
        node_type* prev = nullptr;
        
        while (node) {
            if (equal_(node->value, key)) {
                if (prev) {
                    prev->next = node->next;
                } else {
                    buckets_[idx] = node->next;
                }
                std::allocator_traits<node_allocator>::destroy(node_alloc_, node);
                node_alloc_.deallocate(node, 1);
                --size_;
                return 1;
            }
            prev = node;
            node = node->next;
        }
        return 0;
    }
    
    void swap(unordered_set& other) noexcept {
        std::swap(buckets_, other.buckets_);
        std::swap(bucket_count_, other.bucket_count_);
        std::swap(size_, other.size_);
        std::swap(max_load_factor_, other.max_load_factor_);
        std::swap(hash_, other.hash_);
        std::swap(equal_, other.equal_);
        std::swap(node_alloc_, other.node_alloc_);
        std::swap(bucket_alloc_, other.bucket_alloc_);
    }
    
    size_type count(const key_type& key) const {
        return find(key) != end() ? 1 : 0;
    }
    
    iterator find(const key_type& key) {
        size_type idx = bucket_index(key);
        node_type* node = buckets_[idx];
        
        while (node) {
            if (equal_(node->value, key)) {
                return iterator(buckets_, bucket_count_, idx, node);
            }
            node = node->next;
        }
        return end();
    }
    
    const_iterator find(const key_type& key) const {
        size_type idx = bucket_index(key);
        node_type* node = buckets_[idx];
        
        while (node) {
            if (equal_(node->value, key)) {
                return const_iterator(buckets_, bucket_count_, idx, node);
            }
            node = node->next;
        }
        return end();
    }
    
    bool contains(const key_type& key) const {
        return find(key) != end();
    }
    
    size_type bucket_count() const { return bucket_count_; }
    size_type max_bucket_count() const { 
        return std::allocator_traits<bucket_allocator>::max_size(bucket_alloc_); 
    }
    size_type bucket_size(size_type n) const {
        size_type count = 0;
        node_type* node = buckets_[n];
        while (node) {
            ++count;
            node = node->next;
        }
        return count;
    }
    size_type bucket(const key_type& key) const {
        return bucket_index(key);
    }
    
    float load_factor() const {
        return bucket_count_ > 0 ? static_cast<float>(size_) / bucket_count_ : 0.0f;
    }
    float max_load_factor() const { return max_load_factor_; }
    void max_load_factor(float ml) { max_load_factor_ = ml; }
    
    void rehash(size_type count) {
        rehash_impl(count);
    }
    
    void reserve(size_type count) {
        rehash(static_cast<size_type>(count / max_load_factor_) + 1);
    }
    
    hasher hash_function() const { return hash_; }
    key_equal key_eq() const { return equal_; }
};
