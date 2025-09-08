#include <memory>

template <typename T, typename Alloc = std::allocator<T>>
class deque
{
    using value_type = T;
    using pointer = T*;
    using map_pointer = pointer*;
    using reference = T&;

    using segment_allocator = std::allocator_traits<Alloc>
    using map_alloc = typename segment_allocator::template rebind_alloc<pointer>;

    static constexpr size_t block_size = (sizeof(T) < 256) ? 4096 / sizeof(T) : 16;

    map_pointer map_;
    size_t map_size_;

    segment_allocator seg_alloc_;
    map_allocator map_alloc_;


private:
    pointer allocate_segment(){
        return traits::allocate(alloc_, block_size);
    }

    void allocate_map(size_t size){
        pointer new_map = traits
    }
    
};