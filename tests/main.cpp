#include "vector.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ====================== Test infrastructure ======================
static int tests_total = 0;
static int tests_passed = 0;

#define TEST(name) \
    void test_##name(); \
    struct Register_##name { \
        Register_##name() { \
            std::cout << "Running " #name << "... "; \
            try { \
                test_##name(); \
                std::cout << "PASSED" << std::endl; \
                ++tests_passed; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << std::endl; \
            } catch (...) { \
                std::cout << "FAILED (unknown)" << std::endl; \
            } \
            ++tests_total; \
        } \
    } register_##name; \
    void test_##name()

// ====================== Helper classes ===========================
// Allocator that tracks copies, moves, and propagation traits
template <typename T>
struct TrackingAllocator {
    using value_type = T;
    TrackingAllocator() = default;
    TrackingAllocator(int id) : id(id) {}
    int id = 0;

    T* allocate(std::size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
    void deallocate(T* p, std::size_t) { ::operator delete(p); }

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    template <typename U> bool operator==(const TrackingAllocator<U>& other) const { return id == other.id; }
    template <typename U> bool operator!=(const TrackingAllocator<U>& other) const { return id != other.id; }
};

template<typename T>
struct NoPropMoveAlloc {
    using value_type = T;
    NoPropMoveAlloc() = default;
    NoPropMoveAlloc(int id) : id(id) {}
    int id = 0;

    T* allocate(std::size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
    void deallocate(T* p, std::size_t) { ::operator delete(p); }

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    template<typename U> bool operator==(const NoPropMoveAlloc<U>& other) const { return id == other.id; }
    template<typename U> bool operator!=(const NoPropMoveAlloc<U>& other) const { return id != other.id; }
};

// Type that throws on copy/move after a certain count
static int throw_counter = 0;
static int throw_limit = -1;
struct ThrowOnCopy {
    int value;
    ThrowOnCopy(int v = 0) : value(v) {}
    ThrowOnCopy(const ThrowOnCopy& other) {
        if (throw_limit >= 0 && throw_counter++ >= throw_limit)
            throw std::runtime_error("ThrowOnCopy copy exception");
        value = other.value;
    }
    ThrowOnCopy(ThrowOnCopy&& other) noexcept(false) {
        if (throw_limit >= 0 && throw_counter++ >= throw_limit)
            throw std::runtime_error("ThrowOnCopy move exception");
        value = other.value;
        other.value = -1;
    }
    ThrowOnCopy& operator=(const ThrowOnCopy& other) {
        if (throw_limit >= 0 && throw_counter++ >= throw_limit)
            throw std::runtime_error("ThrowOnCopy copy assignment exception");
        value = other.value;
        return *this;
    }
    ThrowOnCopy& operator=(ThrowOnCopy&& other) noexcept(false) {
        if (throw_limit >= 0 && throw_counter++ >= throw_limit)
            throw std::runtime_error("ThrowOnCopy move assignment exception");
        value = other.value;
        other.value = -1;
        return *this;
    }
    ~ThrowOnCopy() = default;
};
bool operator==(const ThrowOnCopy& a, const ThrowOnCopy& b) { return a.value == b.value; }
bool operator!=(const ThrowOnCopy& a, const ThrowOnCopy& b) { return !(a == b); }

// Input iterator wrapper (not forward) that throws after some steps
class InputIt {
    int val_;
    int max_;
    int step_ = 0;
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int*;
    using reference = const int&;

    InputIt() : val_(0), max_(0) {}
    InputIt(int start, int count) : val_(start), max_(start + count) {}
    int operator*() const { return val_; }
    InputIt& operator++() {
        ++val_; ++step_; return *this;
    }
    InputIt operator++(int) { InputIt tmp = *this; ++(*this); return tmp; }
    bool operator==(const InputIt& other) const { return val_ == other.val_; }
    bool operator!=(const InputIt& other) const { return !(*this == other); }
};

// ====================== Tests ================================

TEST(DefaultConstructor) {
    my::vector<int> v;
    assert(v.size() == 0);
    assert(v.capacity() == 0);
    assert(v.empty());
    assert(v.begin() == v.end());
}

TEST(ConstructorWithAllocator) {
    TrackingAllocator<int> alloc(42);
    my::vector<int, TrackingAllocator<int>> v(alloc);
    assert(v.size() == 0);
    assert(v.capacity() == 0);
    assert(v.get_allocator().id == 42);
}

TEST(ConstructorSize) {
    my::vector<int> v(5);
    assert(v.size() == 5);
    assert(v.capacity() >= 5);
    for (size_t i = 0; i < 5; ++i) assert(v[i] == 0);
}

TEST(ConstructorSizeAndValue) {
    my::vector<std::string> v(3, "hello");
    assert(v.size() == 3);
    assert(v[0] == "hello");
    assert(v[1] == "hello");
    assert(v[2] == "hello");
}

TEST(ConstructorSizeAlloc) {
    TrackingAllocator<int> alloc(100);
    my::vector<int, TrackingAllocator<int>> v(5, alloc);
    assert(v.size() == 5);
    assert(v.capacity() >= 5);
    assert(v.get_allocator().id == 100);
}

TEST(ConstructorForwardIterators) {
    std::vector<int> src = { 1, 2, 3, 4, 5 };
    my::vector<int> v(src.begin(), src.end());
    assert(v.size() == 5);
    for (int i = 0; i < 5; ++i) assert(v[i] == i + 1);
}

TEST(ConstructorInputIterators) {
    InputIt begin(10, 4);
    InputIt end(14, 0);
    my::vector<int> v(begin, end);
    assert(v.size() == 4);
    assert(v[0] == 10);
    assert(v[1] == 11);
    assert(v[2] == 12);
    assert(v[3] == 13);
}

TEST(ConstructorInitializerList) {
    my::vector<double> v = { 1.1, 2.2, 3.3 };
    assert(v.size() == 3);
    assert(v[0] == 1.1);
    assert(v[2] == 3.3);
}

TEST(CopyConstructor) {
    my::vector<int> original = { 7, 8, 9 };
    my::vector<int> copy(original);
    assert(copy.size() == 3);
    assert(copy[0] == 7);
    assert(copy[1] == 8);
    assert(copy[2] == 9);
    copy[0] = 100;
    assert(original[0] == 7);
}

TEST(MoveConstructor) {
    my::vector<int> original = { 1, 2, 3 };
    int* old_data = original.data();
    size_t old_size = original.size();
    my::vector<int> moved(std::move(original));
    assert(moved.data() == old_data);
    assert(moved.size() == old_size);
    assert(original.empty());
    assert(original.data() == nullptr);
    assert(original.size() == 0);
}

TEST(CopyWithAllocator) {
    TrackingAllocator<int> alloc(77);
    using Vec = my::vector<int, TrackingAllocator<int>>;
    Vec original = { 10, 20 };
    Vec copy(original, alloc);
    assert(copy.size() == 2);
    assert(copy.get_allocator().id == 77);
    assert(original.get_allocator().id != 77);
}

TEST(MoveWithAllocator) {
    TrackingAllocator<int> alloc1(1), alloc2(2);
    using Vec = my::vector<int, TrackingAllocator<int>>;
    Vec original({ 1,2,3 }, alloc1);
    
    int* old_data = original.data();
    Vec moved(std::move(original), alloc2);
    assert(moved.size() == 3);
    assert(moved.get_allocator().id == 2);
    assert(moved.data() != old_data);
    assert(original.empty());
}

TEST(MoveWithSameAllocator) {
    TrackingAllocator<int> alloc1(1);
    using Vec = my::vector<int, TrackingAllocator<int>>;
    Vec original({ 1,2,3 }, alloc1);
    
    int* old_data = original.data();
    Vec moved(std::move(original), alloc1);
   
    assert(moved.size() == 3);
    assert(moved.get_allocator().id == 1);
    assert(moved.data() == old_data);
    assert(original.empty());
}

TEST(AssignCount) {
    my::vector<int> v;
    v.assign(4, 42);
    assert(v.size() == 4);
    assert(v[0] == 42 && v[3] == 42);
    v.assign(0, 99);
    assert(v.empty());
}

TEST(AssignForwardIterators) {
    my::vector<int> v;
    std::vector<int> src = { 10, 20, 30 };
    v.assign(src.begin(), src.end());
    assert(v.size() == 3);
    assert(v[0] == 10 && v[2] == 30);
}

TEST(AssignInputIterators) {
    my::vector<int> v;
    InputIt begin(5, 3);
    InputIt end(8, 0);
    v.assign(begin, end);
    assert(v.size() == 3);
    assert(v[0] == 5);
    assert(v[1] == 6);
    assert(v[2] == 7);
}

TEST(AssignInitializerList) {
    my::vector<int> v;
    v.assign({ 100, 200, 300 });
    assert(v.size() == 3);
    assert(v[0] == 100);
    assert(v[2] == 300);
}

TEST(CopyAssignment) {
    my::vector<int> v1 = { 1, 2, 3 };
    my::vector<int> v2 = { 4, 5 };
    v2 = v1;
    assert(v2.size() == 3);
    assert(v2[0] == 1);
    assert(v1.size() == 3);
    v1 = v1;
    assert(v1.size() == 3);
    assert(v1[0] == 1);
}

TEST(CopyMethod) {
    my::vector<int> v1 = { 1, 2, 3 };
    my::vector<int> v2 = { 4, 5 };
    v2.copy(v1);
    assert(v2.size() == 3);
    assert(v2[0] == 1);
    assert(v1.size() == 3);
    
    v1.copy(v1);
    assert(v1.size() == 3);
    assert(v1[0] == 1);
}

TEST(MoveAssignment) {
    my::vector<int> v1 = { 10, 20, 30 };
    my::vector<int> v2;
    v2 = std::move(v1);
    assert(v2.size() == 3);
    assert(v2[0] == 10);
    assert(v1.empty());

    v2 = std::move(v2);
    assert(v2.size() == 3);
}

TEST(MoveAssignmentNoPropagateSameAlloc) {
    using Vec = my::vector<int, NoPropMoveAlloc<int>>;
    NoPropMoveAlloc<int> alloc(10);
    Vec v1({1,2,3}, alloc);
    Vec v2({4,5}, alloc);

    int* old_v1_data = v1.data();
    v2 = std::move(v1);

    assert(v2.get_allocator().id == 10);
    assert(v2.size() == 3);
    assert(v2[0] == 1);
    assert(v2.data() == old_v1_data);
    assert(v1.empty());
}

TEST(MoveAssignmentNoPropagateDifferentAlloc) {
    using Vec = my::vector<int, NoPropMoveAlloc<int>>;
    Vec v1({ 1,2,3 }, NoPropMoveAlloc<int>(10));
    Vec v2({ 4,5 }, NoPropMoveAlloc<int>(20));

    int* old_v1_data = v1.data();
    v2 = std::move(v1);

    assert(v2.get_allocator().id == 20);
    assert(v2.size() == 3);
    assert(v2[0] == 1);
    assert(v2.data() != old_v1_data);
    assert(v1.empty());
}

TEST(AssignmentInitializerList) {
    my::vector<int> v;
    v = { 7, 8, 9 };
    assert(v.size() == 3);
    assert(v[0] == 7);
    v = {};
    assert(v.empty());
}

TEST(ElementAccess) {
    my::vector<int> v = { 10, 20, 30 };
    assert(v[0] == 10);
    assert(v.at(1) == 20);
    assert(v.front() == 10);
    assert(v.back() == 30);
    const auto& cv = v;
    assert(cv[2] == 30);
    assert(cv.at(0) == 10);
    assert(cv.front() == 10);
    assert(cv.back() == 30);
    try {
        v.at(100);
        assert(false && "should throw out_of_range");
    }
    catch (std::out_of_range&) {}
    try {
        my::vector<int> empty_vec;
        empty_vec.front();
        assert(false && "front on empty should throw");
    }
    catch (std::out_of_range&) {}
    try {
        my::vector<int> empty_vec;
        empty_vec.back();
        assert(false && "back on empty should throw");
    }
    catch (std::out_of_range&) {}
}

TEST(DataAccess) {
    my::vector<int> v = { 1, 2, 3 };
    int* p = v.data();
    assert(p[0] == 1);
    assert(p[1] == 2);
    const auto& cv = v;
    const int* cp = cv.data();
    assert(cp[2] == 3);
}

TEST(ResizeDefault) {
    my::vector<int> v = { 1, 2, 3 };
    v.resize(5);
    assert(v.size() == 5);
    assert(v[0] == 1);
    assert(v[3] == 0);
    assert(v[4] == 0);
    v.resize(2);
    assert(v.size() == 2);
    assert(v[0] == 1 && v[1] == 2);
    v.resize(0);
    assert(v.empty());
}

TEST(ResizeWithValue) {
    my::vector<std::string> v;
    v.resize(3, "yes");
    assert(v.size() == 3);
    assert(v[0] == "yes");
    v.resize(1, "no"); 
    assert(v.size() == 1);
    assert(v[0] == "yes");
}

TEST(ReserveAndShrink) {
    my::vector<int> v;
    v.reserve(100);
    assert(v.capacity() >= 100);
    size_t cap = v.capacity();
    v.push_back(1);
    assert(v.capacity() == cap);
    v.shrink_to_fit();
    assert(v.capacity() == v.size());
}

TEST(Clear) {
    my::vector<int> v = { 1, 2, 3 };
    v.clear();
    assert(v.empty());
    assert(v.size() == 0);
    v.clear();
}

TEST(Swap) {
    my::vector<int> a = { 1, 2, 3 };
    my::vector<int> b = { 4, 5 };
    int* a_data = a.data();
    int* b_data = b.data();
    a.swap(b);
    assert(a.size() == 2);
    assert(b.size() == 3);
    assert(a.data() == b_data);
    assert(b.data() == a_data);
}

TEST(PushBack) {
    my::vector<int> v;
    v.push_back(10);
    assert(v.size() == 1 && v[0] == 10);
    const int x = 20;
    v.push_back(x);
    assert(v.size() == 2 && v[1] == 20);
    v.push_back(30);
    assert(v.size() == 3 && v[2] == 30);
}

TEST(PopBack) {
    my::vector<int> v = { 1, 2, 3 };
    v.pop_back();
    assert(v.size() == 2);
    assert(v[0] == 1 && v[1] == 2);
    v.pop_back();
    v.pop_back();
    assert(v.empty());
    try {
        v.pop_back();
        assert(false && "pop_back on empty should throw");
    }
    catch (std::out_of_range&) {}
}

TEST(InsertSingleElement) {
    my::vector<int> v = { 10, 20, 30 };
    // insert in middle
    auto it = v.insert(v.begin() + 1, 15);
    assert(*it == 15);
    assert(v.size() == 4);
    assert(v[0] == 10 && v[1] == 15 && v[2] == 20 && v[3] == 30);
    // insert at end
    it = v.insert(v.end(), 40);
    assert(*it == 40);
    assert(v.size() == 5 && v[4] == 40);
    // insert at begin
    it = v.insert(v.begin(), 5);
    assert(*it == 5);
    assert(v[0] == 5);
}

TEST(InsertMoveElement) {
    my::vector<std::string> v = { "a", "b" };
    std::string s = "c";
    v.insert(v.begin() + 1, std::move(s));
    assert(v[1] == "c");
    assert(s.empty());
}

TEST(InsertCount) {
    my::vector<int> v = { 1, 2, 3 };
    auto it = v.insert(v.begin() + 2, 3, 99);
    assert(*it == 99);
    assert(v.size() == 6);
    assert(v[0] == 1 && v[1] == 2);
    assert(v[2] == 99 && v[3] == 99 && v[4] == 99);
    assert(v[5] == 3);
}

TEST(InsertForwardRange) {
    my::vector<int> v = { 1, 5 };
    std::vector<int> src = { 2, 3, 4 };
    auto it = v.insert(v.begin() + 1, src.begin(), src.end());
    assert(*it == 2);
    assert(v.size() == 5);
    assert(v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4 && v[4] == 5);
}

TEST(InsertInputRange) {
    my::vector<int> v = { 10, 40 };
    InputIt begin(20, 2);
    InputIt end(22, 0);
    auto it = v.insert(v.begin() + 1, begin, end);
    assert(*it == 20);
    assert(v.size() == 4);
    assert(v[0] == 10 && v[1] == 20 && v[2] == 21 && v[3] == 40);
}

TEST(InsertInitializerList) {
    my::vector<int> v = { 100, 200 };
    v.insert(v.end(), { 300, 400 });
    assert(v.size() == 4);
    assert(v[2] == 300 && v[3] == 400);
}

TEST(EraseSingle) {
    my::vector<int> v = { 1, 2, 3, 4 };
    auto it = v.erase(v.begin() + 1);
    assert(*it == 3);
    assert(v.size() == 3);
    assert(v[0] == 1 && v[1] == 3 && v[2] == 4);
    
    it = v.erase(v.end() - 1);
    assert(it == v.end());
    assert(v.size() == 2);
}

TEST(EraseCount) {
    my::vector<int> v = { 1, 2, 3, 4, 5 };
    auto it = v.erase(v.begin() + 1, 2);
    assert(*it == 4);
    assert(v.size() == 3);
    assert(v[1] == 4 && v[2] == 5);
    
    it = v.erase(v.end() - 2, 2);
    assert(it == v.end());
    assert(v.size() == 1);
}

TEST(EraseRange) {
    my::vector<int> v = { 1, 2, 3, 4, 5 };
    auto it = v.erase(v.begin() + 1, v.begin() + 4);
    assert(*it == 5);
    assert(v.size() == 2);
    assert(v[0] == 1 && v[1] == 5);
    
    it = v.erase(v.begin(), v.begin());
    assert(it == v.begin());
    assert(v.size() == 2);
}

TEST(EmplaceBack) {
    my::vector<std::pair<int, int>> v;
    auto& ref = v.emplace_back(1, 2);
    assert(ref.first == 1 && ref.second == 2);
    assert(v.size() == 1);
    v.emplace_back(3, 4);
    assert(v[1].first == 3);
}

TEST(Emplace) {
    my::vector<std::string> v = { "hello", "world" };
    auto it = v.emplace(v.begin() + 1, "beautiful");
    assert(*it == "beautiful");
    assert(v.size() == 3);
    assert(v[0] == "hello" && v[1] == "beautiful" && v[2] == "world");
    
    it = v.emplace(v.end(), "!");
    assert(*it == "!");
    assert(v.size() == 4);
}

TEST(ComparisonOperators) {
    my::vector<int> a = { 1, 2, 3 };
    my::vector<int> b = { 1, 2, 3 };
    my::vector<int> c = { 1, 2, 4 };
    my::vector<int> d = { 1, 2 };
    assert(a == b);
    assert(a != c);
    assert(a < c);
    assert(c > a);
    assert(a > d);
    assert(d < a);
    assert(a >= b);
    assert(a <= b);
    assert(c >= a);
    assert(d <= a);
}

// Allocator propagation tests
TEST(AllocatorPropagateOnCopy) {
    using Vec = my::vector<int, TrackingAllocator<int>>;
    Vec v1(5, TrackingAllocator<int>(1)); // v1.alloc_.id = 1
    v1 = { 1,2,3,4,5 };
    Vec v2(TrackingAllocator<int>(2));
    v2 = v1; // copy assignment, allocator should propagate (true_type)
    assert(v2.get_allocator().id == 1);
    assert(v2.size() == 5);
}

TEST(AllocatorMoveAssignPropagate) {
    using Vec = my::vector<int, TrackingAllocator<int>>;
    Vec v1(5, TrackingAllocator<int>(1));
    v1 = { 1,2,3,4,5 };
    Vec v2(TrackingAllocator<int>(2));
    v2 = std::move(v1);
    assert(v2.get_allocator().id == 1);
}

TEST(AllocatorSwap) {
    using Vec = my::vector<int, TrackingAllocator<int>>;
    Vec v1(5, TrackingAllocator<int>(10));
    Vec v2(3, TrackingAllocator<int>(20));
    v1 = { 1,2,3,4,5 };
    v2 = { 6,7,8 };
    v1.swap(v2);
    assert(v1.get_allocator().id == 20);
    assert(v2.get_allocator().id == 10);
}

TEST(AllocatorSelectOnCopyConstruct) {
    using Vec = my::vector<int, TrackingAllocator<int>>;
    Vec v1(3, 7, TrackingAllocator<int>(99));
    Vec v2(v1); 
    assert(v2.get_allocator().id == 99);
}

TEST(ExceptionSafetyReserve) {
    throw_counter = 0;
    throw_limit = 3; // throw on second move construction
    my::vector<ThrowOnCopy> v;
    v.push_back(ThrowOnCopy(10));
    v.push_back(ThrowOnCopy(20));
    size_t old_cap = v.capacity();
    try {
        v.reserve(100);
    }
    catch (const std::runtime_error&) {
        assert(v.size() == 2);
        assert(v.capacity() == old_cap);
        assert(v[0].value == 10 && v[1].value == 20);
    }
}

TEST(ExceptionSafetyOperatorCopy) {
    using Vec = my::vector<ThrowOnCopy>;
    Vec v1; v1.reserve(4);
    v1.emplace_back(1);
    v1.emplace_back(2);
    v1.emplace_back(3);

    Vec v2; v2.reserve(3);
    v2.emplace_back(4);
    v2.emplace_back(5);
    size_t old_size2 = v2.size();
    size_t old_cap2 = v2.capacity();

    throw_counter = 0;
    throw_limit = 2;
    try {
        v2 = v1;
        assert(false && "should have thrown");
    }
    catch (const std::runtime_error&) {
        assert(v2.size() == old_size2);
        assert(v2.capacity() == old_cap2);
        assert(v2[0].value == 4);
        assert(v2[1].value == 5);
    }
    
    assert(v1.size() == 3);
    assert(v1[0].value == 1);
    assert(v1[1].value == 2);
    assert(v1[2].value == 3);
}

// Iterator tests in debug mode
#ifndef NDEBUG
TEST(DebugIteratorCreate) {
    my::vector<int> v = { 1, 2, 3 };
    auto it = v.begin();
    assert(*it == 1);
    auto it2 = v.end();
    assert(it2 - it == 3);
}

TEST(DebugIteratorCopyMove) {
    my::vector<int> v = { 10, 20 };
    auto it1 = v.begin();
    auto it2(it1);
    assert(*it2 == 10);
    auto it3(std::move(it1));
    assert(*it3 == 10);
    
    try {
        *it1;
        assert(false);
    }
    catch (std::invalid_argument&) {}
}

TEST(DebugIteratorInvalidationOnReserve) {
    my::vector<int> v = { 1, 2, 3 };
    auto it = v.begin() + 1;
    v.reserve(100);
    try {
        *it;
        assert(false);
    }
    catch (std::invalid_argument&) {}
}

TEST(DebugIteratorInvalidationOnPushBack) {
    my::vector<int> v;
    v.reserve(2);
    v.push_back(1);
    v.push_back(2);
    auto it = v.begin(); 
    v.push_back(3); 
    try {
        *it;
        assert(false);
    }
    catch (std::invalid_argument&) {}
    
    my::vector<int> v2;
    v2.reserve(2);
    v2.push_back(1);
    auto it2 = v2.end();
    v2.push_back(2); 
    try {
        *it2;
        assert(false);
    }
    catch (std::invalid_argument&) {}
}

TEST(DebugIteratorInvalidationOnSwap) {
    my::vector<int> a = { 1, 2 };
    my::vector<int> b = { 3, 4 };
    auto it_a = a.begin();
    auto it_b = b.begin();
    a.swap(b);
    
    assert(*it_a == 1);
    assert(it_a - b.begin() == 0);
    assert(*it_b == 3);
    assert(it_b - a.begin() == 0);
}

TEST(DebugIteratorSeek) {
    my::vector<int> v = { 0, 1, 2, 3 };
    auto it = v.begin();
    it += 2;
    assert(*it == 2);
    it -= 1;
    assert(*it == 1);
    ++it;
    assert(*it == 2);
    --it;
    assert(*it == 1);
    auto it2 = it + 2;
    assert(*it2 == 3);
    auto diff = it2 - it;
    assert(diff == 2);
}

TEST(DebugIteratorComparisons) {
    my::vector<int> v = { 5, 6, 7 };
    auto a = v.begin();
    auto b = v.begin() + 1;
    assert(a < b);
    assert(a <= b);
    assert(b > a);
    assert(b >= a);
    assert(a != b);
    assert(a == a);
}

TEST(DebugIteratorOutOfRange) {
    my::vector<int> v = { 1, 2 };
    auto it = v.end();
    try {
        *it;
        assert(false);
    }
    catch (std::out_of_range&) {}
    
    try {
        it[1];
        assert(false);
    }
    catch (std::out_of_range&) {}
}
#endif

int main() {
    std::cout << "\n" << tests_passed << "/" << tests_total << " tests passed.\n";
    return (tests_passed == tests_total) ? 0 : 1;
}