#pragma once
#include <initializer_list>
#include <algorithm> //swap
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <memory>

#ifndef NDEBUG
#define IF_CONFIGURATION_IS_DEBUG(code) code
#else
#define IF_CONFIGURATION_IS_DEBUG(code) 
#endif

//Поскольку итераторы это отладочный инструмент под него память выделяться аллокатором не будет

namespace my {
#pragma region Class
	template<typename T, typename Alloc = std::allocator<T>>
	class vector {
	public: //assert && using
		
		static_assert(std::is_same_v<T, typename Alloc::value_type>,
			"vector<T, Allocator> requires that Allocator's value_type match T");

		static_assert(!std::is_const_v<T>, "The C++ Standard forbids containers of const elements");
		static_assert(!std::is_function_v<T>, "The C++ Standard forbids containers for function elements");
		static_assert(!std::is_reference_v<T>, "The C++ Standard forbids containers of reference elements");
		static_assert(std::is_object_v<T>, "The C++ Standard forbids containers of non-object types");
			
		using value_type = T;
		using allocator_type = Alloc;
		using pointer = typename std::allocator_traits<Alloc>::pointer;
		using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;
		using reference = T&;
		using const_reference = const T&;
		using size_type = typename std::allocator_traits<Alloc>::size_type;
		using difference_type = typename std::allocator_traits<Alloc>::difference_type;

	private: //data
		
		T* data_ = nullptr;
		size_type size_ = 0;
		size_type capacity_ = 0;
		Alloc alloc_;
		using Traits = std::allocator_traits<Alloc>;

		static const size_type factor_ = 2;

#pragma region iterator
#ifndef NDEBUG
		template<typename PtrT, typename RefT>
		struct IteratorNode; //forward declaration

		template <typename PtrT, typename RefT>
		struct DebugIterator {
			PtrT ptr_ = nullptr;
			const vector<T, Alloc>* container_ = nullptr;
			IteratorNode<PtrT, RefT>* node_ = nullptr;
			bool is_valid_ = true;

			DebugIterator() = default;
			DebugIterator(PtrT ptr, const vector<T, Alloc>* container);
			DebugIterator(const DebugIterator& other);
			DebugIterator(DebugIterator&& other) noexcept;

			RefT operator*() const;
			PtrT operator->() const;
			RefT operator[](difference_type off) const;
			void seek_to(const PtrT it);

			DebugIterator& operator=(const DebugIterator& other);
			DebugIterator& operator=(DebugIterator&& other) noexcept;

			DebugIterator operator+(difference_type off) const;
			DebugIterator& operator+=(difference_type off);
			DebugIterator& operator++();
			DebugIterator operator++(int);

			DebugIterator operator-(difference_type off) const;
			DebugIterator& operator-=(difference_type off);
			DebugIterator& operator--();
			DebugIterator operator--(int);

			difference_type operator-(const DebugIterator& other) const;

			bool operator<(const DebugIterator& other) const;
			bool operator<=(const DebugIterator& other) const;
			bool operator>(const DebugIterator& other) const;
			bool operator>=(const DebugIterator& other) const;
			bool operator==(const DebugIterator& other) const;
			bool operator!=(const DebugIterator& other) const;

			~DebugIterator();

		private:
			static void validity_check_std(const DebugIterator& it);
			static void validity_check_std(const DebugIterator& it, PtrT ptr);
			static void validity_check_seek(const DebugIterator& it, PtrT ptr);
			static void validity_check_for_two(const DebugIterator& it, const DebugIterator& other);
		};

		template<typename PtrT, typename RefT>
		static void validity_check_from_vector_std(const vector& vec, const DebugIterator<PtrT, RefT>& it);
		template<typename PtrT, typename RefT>
		static void validity_check_from_vector_for_erase(const vector& vec, const DebugIterator<PtrT, RefT>& it);
#endif
#pragma endregion

#pragma region iterator node
#ifndef NDEBUG
		template<typename PtrT, typename RefT>
		struct IteratorNode {
			DebugIterator<PtrT, RefT>* iterator_ = nullptr;
			IteratorNode<PtrT, RefT>* prev_ = nullptr;
			IteratorNode<PtrT, RefT>* next_ = nullptr;

			~IteratorNode() {
				if (iterator_) {
					iterator_->node_ = nullptr;
				}
			}
		};

		mutable IteratorNode<T*, T&>* head_node_ = nullptr;
		mutable IteratorNode<const T*, const T&>* head_node_const = nullptr;

		template<typename PtrT, typename RefT>
		void create_node(DebugIterator<PtrT, RefT>* it) const;
		template<typename PtrT, typename RefT>
		void delete_node(IteratorNode<PtrT, RefT>* node) const noexcept;

		void invalidate_iterators_from(size_type index) const noexcept;
		void invalidate_all_iterators() const noexcept;

		IteratorNode<T*, T&>*& get_head(T*) const { return head_node_; }
		IteratorNode<const T*, const T&>*& get_head(const T*) const { return head_node_const; }
#endif
#pragma endregion
	
	public: //using

#pragma region iterator
#ifdef NDEBUG
			using iterator = T*;
			using const_iterator = const T*;
#else
			using iterator = DebugIterator<T*, T&>;
			using const_iterator = DebugIterator<const T*, const T&>;
#endif
#pragma endregion

	private: //methods
		
		size_type max_size() const noexcept {
			return Traits::max_size(alloc_);
		}

		template <typename TArgsGenerator>
		void reserve_and_insert(size_type index, size_type count, TArgsGenerator&& insert_generator);
		template <typename TArgsGenerator>
		void insert_in_place(size_type index, size_type count, TArgsGenerator && insert_generator);

		template<typename... Args>
		void reserve_and_emplace(size_type new_capacity, size_type index, Args&&... args);

	public: //methods

#ifdef NDEBUG
		iterator begin() noexcept { return data_; }
		iterator end() noexcept { return data_ + size_; }
		const_iterator begin() const noexcept { return data_; }
		const_iterator end() const noexcept { return data_ + size_; }
		const_iterator сbegin() const noexcept { return data_; }
		const_iterator сend() const noexcept { return data_ + size_; }
#else
		iterator begin() noexcept { return { data_, this }; }
		iterator end() noexcept { return { data_ + size_, this }; }
		const_iterator begin() const noexcept { return { data_, this }; }
		const_iterator end() const noexcept { return { data_ + size_, this }; }
		const_iterator cbegin() const noexcept { return { data_, this }; }
		const_iterator cend() const noexcept { return { data_ + size_, this }; }
#endif

		vector() noexcept(std::is_nothrow_default_constructible_v<Alloc>) : alloc_(Alloc()) {}
		vector(const Alloc& alloc) noexcept(std::is_nothrow_copy_constructible_v<Alloc>) : alloc_(alloc) {}
		vector(size_type size, const Alloc& alloc = Alloc());
		vector(size_type size, const T& val, const Alloc& alloc = Alloc());
		template<class It>
		requires std::forward_iterator<It>
		vector(It first, It last, const Alloc& alloc = Alloc());
		template<class It>
		requires std::input_iterator<It> && (!std::forward_iterator<It>)
		vector(It first, It last, const Alloc& alloc = Alloc());
		vector(std::initializer_list<T> list, const Alloc& alloc = Alloc()) 
			: vector(list.begin(), list.end(), alloc) {}
		vector(const vector& other);
		vector(const vector& other, const Alloc& alloc);
		vector(vector&& other) noexcept(std::is_nothrow_move_constructible_v<Alloc>);
		vector(vector&& other, const Alloc& alloc) noexcept(Traits::is_always_equal::value);

		T& operator[](size_type index);
		const T& operator[](size_type index) const;
		T& at(size_type index);
		const T& at(size_type index) const;

		T& front();
		const T& front() const;
		T& back();
		const T& back() const;
		T* data() noexcept { return data_; }
		const T* data() const noexcept { return data_; }

		void assign(size_type count, const T& val); //-------Strong Exception Safety Guarantee
		template<class It>
		requires std::forward_iterator<It>
		void assign(It first, It last); //-------------------Strong Exception Safety Guarantee
		template<class It>
		requires std::input_iterator<It> && (!std::forward_iterator<It>)
		void assign(It first, It last); //-------------------Strong Exception Safety Guarantee
		void assign(std::initializer_list<T> list); //-------Strong Exception Safety Guarantee

		vector& operator=(const vector& other); //-----------Strong Exception Safety Guarantee
		vector& copy(const vector& other); //----------------Basic Exception Safety Guarantee
		vector& operator=(vector&& other) noexcept(
			Traits::propagate_on_container_move_assignment::value || Traits::is_always_equal::value
		); //------------------------------------------------Basic Exception Safety Guarantee (if not noexcept)
		vector& operator=(std::initializer_list<T> list); //-Strong Exception Safety Guarantee

		bool operator!=(const vector& other) const;
		bool operator==(const vector& other) const;
		bool operator<=(const vector& other) const;
		bool operator>=(const vector& other) const;
		bool operator<(const vector& other) const;
		bool operator>(const vector& other) const;

		size_type size() const noexcept { return size_; }
		size_type capacity() const noexcept { return capacity_; }
		bool empty() const noexcept { return size_ == 0; }
		Alloc get_allocator() const noexcept { return alloc_; }

		void resize(size_type new_size); //------------------Strong Exception Safety Guarantee
		void resize(size_type new_size, const T& val); //----Strong Exception Safety Guarantee
		void clear() noexcept;

		void reserve(size_type new_capacity); //-------------Strong Exception Safety Guarantee
		void shrink_to_fit(); //-----------------------------Strong Exception Safety Guarantee
		void swap(vector& other) noexcept;

		void push_back(const T& val); //---------------------Strong Exception Safety Guarantee
		void push_back(T&& val); //--------------------------Strong Exception Safety Guarantee
		iterator insert(iterator pos, const T& val); //------All inserts have a Basic ESG
		iterator insert(iterator pos, T&& val);
		iterator insert(iterator pos, size_type count, const T& val);
		template<class It>
		requires std::forward_iterator<It>
		iterator insert(iterator pos, It first, It last);
		template<class It>
		requires std::input_iterator<It> && (!std::forward_iterator<It>)
		iterator insert(iterator pos, It first, It last);
		iterator insert(iterator pos, std::initializer_list<T> list);

		void pop_back(); //----------------------------------Strong Exception Safety Guarantee
		iterator erase(iterator pos); //---------------------All erases have a Basic ESG
		iterator erase(iterator pos, size_type count);
		iterator erase(iterator first, iterator last);

		template<typename... Args>
		T& emplace_back(Args&&... args); //------------------Strong Exception Safety Guarantee
		template<typename... Args>
		iterator emplace(iterator pos, Args&&... args); //---Strong Exception Safety Guarantee

		~vector() noexcept;
	};
#pragma endregion

	/// ---- MEMBER FUNCTIONS IMPLEMENTATION ---- ///
#pragma region private
	template<typename T, typename Alloc>
	template <typename TArgsGenerator>
	inline void vector<T, Alloc>::reserve_and_insert(size_type index, size_type count, TArgsGenerator&& insert_generator) {
		size_type new_capacity = (size_ + count < factor_ * capacity_ && factor_ * capacity_ <= max_size()) ? (factor_ * capacity_) : (size_ + count);

		T* new_data = Traits::allocate(alloc_, new_capacity);
		
		size_type i = 0;
		try {
			for (; i < index; i++) {
				Traits::construct(alloc_, new_data + i, std::move_if_noexcept(data_[i]));
			}

			insert_generator(new_data + i);
			i += count;

			for (size_type j = index; j < size_; j++, i++) {
				Traits::construct(alloc_, new_data + i, std::move_if_noexcept(data_[j]));
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, new_data + j);
			}
			Traits::deallocate(alloc_, new_data, new_capacity);
			throw;
		}

		for (size_type j = 0; j < size_; j++) {
			Traits::destroy(alloc_, data_ + j);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}
		data_ = new_data;
		capacity_ = new_capacity;
		size_ += count;

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
	}

	template<typename T, typename Alloc>
	template <typename TArgsGenerator>
	inline void vector<T, Alloc>::insert_in_place(size_type index, size_type count, TArgsGenerator&& insert_generator) {
		
		size_type i = size_ + count - 1;
		
		for (; i >= size_ && i >= index + count; i--) {
			Traits::construct(alloc_, data_ + i, std::move(data_[i - count]));
		}
		for (; i >= index + count; i--) {
			data_[i] = std::move(data_[i - count]);
		}
		
		size_type destroy_limit = (index + count < size_) ? (index + count) : size_;
		for (size_type j = index; j < destroy_limit; j++) {
			Traits::destroy(alloc_, data_ + j);
		}

		try {
			insert_generator(data_ + index);
		}
		catch(...) {
			for (size_type j = index; j < size_; j++) {
				Traits::construct(alloc_, data_ + j, std::move(data_[j + count]));
			}
			for (size_type j = size_; j < size_ + count; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			throw;
		}
		size_ += count;
		IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(index);)
	}

	template<typename T, typename Alloc>
	template<typename... Args>
	inline void vector<T, Alloc>::reserve_and_emplace(size_type new_capacity, size_type index, Args&&... args) {
		if (new_capacity > max_size()) throw std::length_error("vector too long");
		
		T* tmp = Traits::allocate(alloc_, new_capacity);

		size_type i = 0;
		bool new_element_constructed = false;
		try {
			Traits::construct(alloc_, tmp + index, std::forward<Args>(args)...);
			new_element_constructed = true;

			for (; i < index; i++) {
				Traits::construct(alloc_, tmp + i, std::move_if_noexcept(data_[i]));
			}

			for (; i < size_; i++) {
				Traits::construct(alloc_, tmp + i + 1, std::move_if_noexcept(data_[i]));
			}
		}
		catch (...) {
			if (i > index) {
				for (size_type j = index; j < i; j++) {
					Traits::destroy(alloc_, tmp + j + 1);
				}
			}

			if (new_element_constructed) {
				Traits::destroy(alloc_, tmp + index);
			}

			size_type bound = (i < index) ? i : index;
			for (size_type j = 0; j < bound; j++) {
				Traits::destroy(alloc_, tmp + j);
			}

			Traits::deallocate(alloc_, tmp, new_capacity);
			throw;
		}

		for (size_type j = 0; j < size_; j++) {
			Traits::destroy(alloc_, data_ + j);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}

		data_ = tmp;
		capacity_ = new_capacity;
		size_++;

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
	}
#pragma endregion

#pragma region iterator functions
#ifndef NDEBUG
#pragma region iterator node functions
	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::create_node(DebugIterator<PtrT, RefT>* it) const {
		IteratorNode<PtrT, RefT>*& head = get_head(static_cast<PtrT>(nullptr));

		IteratorNode<PtrT, RefT>* node = new IteratorNode<PtrT, RefT>;
		node->iterator_ = it;
		node->next_ = head;

		if (head) head->prev_ = node;
		head = node;

		it->node_ = node;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::delete_node(IteratorNode<PtrT, RefT>* node) const noexcept {
		IteratorNode<PtrT, RefT>*& head = get_head(static_cast<PtrT>(nullptr));

		if (node->prev_) node->prev_->next_ = node->next_;
		if (node->next_) node->next_->prev_ = node->prev_;
		if (head == node) head = node->next_;
		delete node;
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::invalidate_iterators_from(size_type index) const noexcept {
		IteratorNode<T*, T&>* cur_node = head_node_;
		while (cur_node) {
			IteratorNode<T*, T&>* next_node = cur_node->next_;
			if (cur_node->iterator_) {
				size_type ind = cur_node->iterator_->ptr_ - data_;
				if (ind >= index) {
					cur_node->iterator_->is_valid_ = false;
					delete_node(cur_node);
				}
			}
			else delete_node(cur_node);
			cur_node = next_node;
		}
		IteratorNode<const T*, const T&>* cur_node_c = head_node_const;
		while (cur_node_c) {
			IteratorNode<const T*, const T&>* next_node_c = cur_node_c->next_;
			if (cur_node_c->iterator_) {
				size_type ind = cur_node_c->iterator_->ptr_ - data_;
				if (ind >= index) {
					cur_node_c->iterator_->is_valid_ = false;
					delete_node(cur_node_c);
				}
			}
			else delete_node(cur_node_c);
			cur_node_c = next_node_c;
		}
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::invalidate_all_iterators() const noexcept {
		IteratorNode<T*, T&>* cur_node = head_node_;
		while (cur_node) {
			IteratorNode<T*, T&>* next_node = cur_node->next_;
			if (cur_node->iterator_) {
				cur_node->iterator_->is_valid_ = false;
			}
			delete_node(cur_node);
			cur_node = next_node;
		}
		IteratorNode<const T*, const T&>* cur_node_c = head_node_const;
		while (cur_node_c) {
			IteratorNode<const T*, const T&>* next_node_c = cur_node_c->next_;
			if (cur_node_c->iterator_) {
				cur_node_c->iterator_->is_valid_ = false;
			}
			delete_node(cur_node_c);
			cur_node_c = next_node_c;
		}
	}
#pragma endregion

#pragma region check validity
	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::DebugIterator<PtrT, RefT>::validity_check_std(const vector<T, Alloc>::DebugIterator<PtrT, RefT>& it) {
		if (!it.is_valid_) {
			throw std::invalid_argument("Error: can't dereference invalidated vector iterator");
		}
		if (it.container_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator has no container");
		}
		if (it.node_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator doesn't belong to any container");
		}
		if (it.ptr_ < it.container_->data_ || it.ptr_ >= it.container_->data_ + it.container_->size_) {
			throw std::out_of_range("Error: vector subscript out of range");
		}
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::DebugIterator<PtrT, RefT>::validity_check_std(const vector<T, Alloc>::DebugIterator<PtrT, RefT>& it, PtrT ptr) {
		if (!it.is_valid_) {
			throw std::invalid_argument("Error: can't dereference invalidated vector iterator");
		}
		if (it.container_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator has no container");
		}
		if (it.node_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator doesn't belong to any container");
		}
		if (ptr < it.container_->data_ || ptr >= it.container_->data_ + it.container_->size_) {
			throw std::out_of_range("Error: vector subscript out of range");
		}
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::DebugIterator<PtrT, RefT>::validity_check_seek(const vector<T, Alloc>::DebugIterator<PtrT, RefT>& it, PtrT ptr) {
		if (!it.is_valid_) {
			throw std::invalid_argument("Error: cannot seek invalidated vector iterator");
		}
		if (it.container_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator has no container");
		}
		if (it.node_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator doesn't belong to any container");
		}
		if (ptr < it.container_->data_) {
			throw std::out_of_range("Error: cannot seek vector iterator before begin");
		}
		if (ptr > it.container_->data_ + it.container_->size_) {
			throw std::out_of_range("Error: cannot seek vector iterator after end");
		}
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::DebugIterator<PtrT, RefT>::validity_check_for_two(const vector<T, Alloc>::DebugIterator<PtrT, RefT>& it, const vector<T, Alloc>::DebugIterator<PtrT, RefT>& other) {
		if (it.container_ != other.container_) {
			throw std::invalid_argument("Error: vector iterators incompatible");
		}

		if (!it.is_valid_) {
			throw std::invalid_argument("Error: left vector iterator invalidated");
		}
		if (it.container_ == nullptr) {
			throw std::invalid_argument("Error: left vector iterator has no container");
		}
		if (it.node_ == nullptr) {
			throw std::invalid_argument("Error: left vector iterator doesn't belong to any container");
		}
		if (it.ptr_ < it.container_->data_ || it.ptr_ > it.container_->data_ + it.container_->size_) {
			throw std::out_of_range("Error: left vector iterator subscript out of range");
		}

		if (!other.is_valid_) {
			throw std::invalid_argument("Error: right vector iterator invalidated");
		}
		if (other.node_ == nullptr) {
			throw std::invalid_argument("Error: right vector iterator doesn't belong to any container");
		}
		if (other.ptr_ < other.container_->data_ || other.ptr_ > other.container_->data_ + other.container_->size_) {
			throw std::out_of_range("Error: right vector iterator subscript out of range");
		}
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::validity_check_from_vector_std(const vector<T, Alloc>& vec, const vector<T, Alloc>::DebugIterator<PtrT, RefT>& it) {
		if (it.container_ != &vec) {
			throw std::invalid_argument("Error: iterator doesn't belong to this vector");
		}
		if (!it.is_valid_) {
			throw std::invalid_argument("Error: can't dereference invalidated vector iterator");
		}
		if (it.node_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator doesn't belong to any container");
		}
		if (it.ptr_ < vec.data_ || it.ptr_ > vec.data_ + vec.size_) {
			throw std::out_of_range("Error: vector subscript out of range");
		}
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::validity_check_from_vector_for_erase(const vector<T, Alloc>& vec, const vector<T, Alloc>::DebugIterator<PtrT, RefT>& it) {
		if (it.container_ != &vec) {
			throw std::invalid_argument("Error: iterator does not belong to this vector");
		}
		if (!it.is_valid_) {
			throw std::invalid_argument("Error: can't dereference invalidated vector iterator");
		}
		if (it.node_ == nullptr) {
			throw std::invalid_argument("Error: vector iterator doesn't belong to any container");
		}
		if (it.ptr_ < vec.data_ || it.ptr_ >= vec.data_ + vec.size_) {
			throw std::out_of_range("Error: vector subscript out of range");
		}
	}
#pragma endregion

#pragma region rule of five
	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline vector<T, Alloc>::DebugIterator<PtrT, RefT>::DebugIterator(PtrT ptr, const vector<T, Alloc>* container)
		: ptr_(ptr), container_(container) {
		if (container_) {
			container_->create_node(this);
		}
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline vector<T, Alloc>::DebugIterator<PtrT, RefT>::DebugIterator(const DebugIterator<PtrT, RefT>& other)
		: ptr_(other.ptr_), container_(other.container_), is_valid_(other.is_valid_) {
		if (container_ && is_valid_) {
			container_->create_node(this);
		}
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline vector<T, Alloc>::DebugIterator<PtrT, RefT>::DebugIterator(DebugIterator<PtrT, RefT>&& other) noexcept
		: ptr_(other.ptr_), container_(other.container_), node_(other.node_), is_valid_(other.is_valid_) {
		if (node_) {
			node_->iterator_ = this;
		}

		other.ptr_ = nullptr;
		other.container_ = nullptr;
		other.node_ = nullptr;
		other.is_valid_ = false;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline vector<T, Alloc>::DebugIterator<PtrT, RefT>& vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator=(const DebugIterator<PtrT, RefT>& other) {
		if (&other == this) return *this;

		if (container_ && node_) {
			container_->delete_node(node_);
		}

		ptr_ = other.ptr_;
		container_ = other.container_;
		is_valid_ = other.is_valid_;

		if (container_ && is_valid_) {
			container_->create_node(this);
		}

		return *this;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline vector<T, Alloc>::DebugIterator<PtrT, RefT>& vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator=(DebugIterator<PtrT, RefT>&& other) noexcept {
		if (&other == this) return *this;

		if (container_ && node_) {
			container_->delete_node(node_);
		}

		ptr_ = other.ptr_;
		container_ = other.container_;
		node_ = other.node_;
		is_valid_ = other.is_valid_;

		if (node_) {
			node_->iterator_ = this;
		}

		other.ptr_ = nullptr;
		other.container_ = nullptr;
		other.node_ = nullptr;
		other.is_valid_ = false;

		return *this;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline vector<T, Alloc>::DebugIterator<PtrT, RefT>::~DebugIterator() {
		if (container_ && node_) {
			container_->delete_node(node_);
		}
	}
#pragma endregion 

#pragma region others
	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline RefT vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator*() const {
		validity_check_std(*this);
		return *ptr_;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline PtrT vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator->() const {
		validity_check_std(*this);
		return ptr_;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline RefT vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator[](difference_type off) const {
		validity_check_std(*this, ptr_ + off);
		return ptr_[off];
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline void vector<T, Alloc>::DebugIterator<PtrT, RefT>::seek_to(const PtrT it) {
		validity_check_seek(*this, it);
		ptr_ = it;
	}
#pragma endregion

#pragma region addition
	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT> vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator+(difference_type off) const {
		validity_check_seek(*this, ptr_ + off);
		return { ptr_ + off, container_ };
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT>& vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator+=(difference_type off) {
		validity_check_seek(*this, ptr_ + off);
		ptr_ += off;
		return (*this);
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT>& vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator++() {
		validity_check_seek(*this, ptr_ + 1);
		ptr_++;
		return (*this);
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT> vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator++(int) {
		validity_check_seek(*this, ptr_ + 1);
		DebugIterator<PtrT, RefT> tmp = *this;
		ptr_++;
		return tmp;
	}
#pragma endregion

#pragma region subtraction
	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT> vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator-(difference_type off) const {
		validity_check_seek(*this, ptr_ - off);
		return { ptr_ - off, container_ };
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT>& vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator-=(difference_type off) {
		validity_check_seek(*this, ptr_ - off);
		ptr_ -= off;
		return (*this);
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT>& vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator--() {
		validity_check_seek(*this, ptr_ - 1);
		ptr_--;
		return (*this);
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline typename vector<T, Alloc>::DebugIterator<PtrT, RefT> vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator--(int) {
		validity_check_seek(*this, ptr_ - 1);
		DebugIterator<PtrT, RefT> tmp = *this;
		ptr_--;
		return tmp;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline vector<T,Alloc>::difference_type vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator-(const DebugIterator<PtrT, RefT>& other) const {
		validity_check_for_two(*this, other);
		return ptr_ - other.ptr_;
	}
#pragma endregion

#pragma region comparison
	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline bool vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator<(const DebugIterator<PtrT, RefT>& other) const {
		validity_check_for_two(*this, other);
		return ptr_ < other.ptr_;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline bool vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator<=(const DebugIterator<PtrT, RefT>& other) const {
		validity_check_for_two(*this, other);
		return ptr_ <= other.ptr_;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline bool vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator>(const DebugIterator<PtrT, RefT>& other) const {
		validity_check_for_two(*this, other);
		return ptr_ > other.ptr_;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline bool vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator>=(const DebugIterator<PtrT, RefT>& other) const {
		validity_check_for_two(*this, other);
		return ptr_ >= other.ptr_;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline bool vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator==(const DebugIterator<PtrT, RefT>& other) const {
		validity_check_for_two(*this, other);
		return ptr_ == other.ptr_;
	}

	template<typename T, typename Alloc>
	template<typename PtrT, typename RefT>
	inline bool vector<T, Alloc>::DebugIterator<PtrT, RefT>::operator!=(const DebugIterator<PtrT, RefT>& other) const {
		validity_check_for_two(*this, other);
		return ptr_ != other.ptr_;
	}
#pragma endregion
#endif
#pragma endregion

#pragma region constructors && destructor
	template<typename T, typename Alloc>
	inline vector<T, Alloc>::vector(size_type size, const Alloc& alloc) : size_(size), capacity_(size), alloc_(alloc) {
		if (size > max_size()) throw std::length_error("vector too long");
		
		if (capacity_ == 0) {
			data_ = nullptr;
			return;
		}
		data_ = Traits::allocate(alloc_, capacity_);

		size_type i = 0;
		try {
			for (; i < size_; i++) {
				Traits::construct(alloc_, data_ + i);
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			Traits::deallocate(alloc_, data_, capacity_);
			throw;
		}
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>::vector(size_type size, const T& val, const Alloc& alloc) : size_(size), capacity_(size), alloc_(alloc) {
		if (size > max_size()) throw std::length_error("vector too long");
		
		if (capacity_ == 0) {
			data_ = nullptr;
			return;
		}
		data_ = Traits::allocate(alloc_, capacity_);

		size_type i = 0;
		try {
			for (; i < size_; i++) {
				Traits::construct(alloc_, data_ + i, val);
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			Traits::deallocate(alloc_, data_, capacity_);
			throw;
		}
	}

	template <typename T, typename Alloc>
	template<class It>
	requires std::forward_iterator<It>
	inline vector<T, Alloc>::vector(It first, It last, const Alloc& alloc) : alloc_(alloc) {
		size_type size = static_cast<size_t>(std::distance(first, last));
		size_ = size;
		capacity_ = size;
		
		if (capacity_ == 0) {
			data_ = nullptr;
			return;
		}
		data_ = Traits::allocate(alloc_, capacity_);

		size_type i = 0;
		try {
			for (It it = first; it != last; ++it) {
				Traits::construct(alloc_, data_ + i, *it);
				i++;
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			Traits::deallocate(alloc_, data_, capacity_);
			throw;
		}
	}

	template <typename T, typename Alloc>
	template<class It>
	requires std::input_iterator<It> && (!std::forward_iterator<It>)
	inline vector<T, Alloc>::vector(It first, It last, const Alloc& alloc) : alloc_(alloc) {
		try {
			for (It it = first; it != last; ++it) {
				emplace_back(*it);
			}
		}
		catch (...) {
			for (size_type j = 0; j < size_; j++) Traits::destroy(alloc_, data_ + j);
			if (data_) Traits::deallocate(alloc_, data_, capacity_);
			throw;
		}
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>::vector(const vector<T, Alloc>& other) 
		: size_(other.size_), capacity_(other.capacity_), alloc_(Traits::select_on_container_copy_construction(other.alloc_)) {
		
		if (capacity_ == 0) {
			data_ = nullptr;
			return;
		}
		data_ = Traits::allocate(alloc_, capacity_);
		
		size_type i = 0;
		try {
			for (; i < size_; i++) {
				Traits::construct(alloc_, data_ + i, other.data_[i]);
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			Traits::deallocate(alloc_, data_, capacity_);
			throw;
		}
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>::vector(const vector<T, Alloc>& other, const Alloc& alloc)
		: size_(other.size_), capacity_(other.capacity_), alloc_(alloc) {
		
		if (capacity_ == 0) {
			data_ = nullptr;
			return;
		}
		data_ = Traits::allocate(alloc_, capacity_);

		size_type i = 0;
		try {
			for (; i < size_; i++) {
				Traits::construct(alloc_, data_ + i, other.data_[i]);
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			Traits::deallocate(alloc_, data_, capacity_);
			throw;
		}
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>::vector(vector<T, Alloc>&& other) noexcept(std::is_nothrow_move_constructible_v<Alloc>)
		: size_(other.size_), capacity_(other.capacity_), data_(other.data_), alloc_(std::move(other.alloc_)) {
		other.size_ = 0;
		other.capacity_ = 0;
		other.data_ = nullptr;

#ifndef NDEBUG
		for (IteratorNode<T*, T&>* node = other.head_node_; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}
		for (IteratorNode<const T*, const T&>* node = other.head_node_const; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}

		head_node_ = other.head_node_;
		head_node_const = other.head_node_const;
		other.head_node_ = nullptr;
		other.head_node_const = nullptr;
#endif
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>::vector(vector<T, Alloc>&& other, const Alloc& alloc) noexcept(Traits::is_always_equal::value)
		: size_(other.size_), capacity_(other.capacity_), alloc_(alloc) {
		
		if constexpr (Traits::is_always_equal::value) {
			data_ = other.data_;

			other.size_ = 0;
			other.capacity_ = 0;
			other.data_ = nullptr;
		}
		else {
			if (this->alloc_ == other.alloc_) {
				data_ = other.data_;

				other.size_ = 0;
				other.capacity_ = 0;
				other.data_ = nullptr;
			}
			else {
				if (capacity_ == 0) {
					data_ = nullptr;
#ifndef NDEBUG
					for (IteratorNode<T*, T&>* node = other.head_node_; node; node = node->next_) {
						if (node->iterator_) node->iterator_->container_ = this;
					}
					for (IteratorNode<const T*, const T&>* node = other.head_node_const; node; node = node->next_) {
						if (node->iterator_) node->iterator_->container_ = this;
					}

					head_node_ = other.head_node_;
					head_node_const = other.head_node_const;
					other.head_node_ = nullptr;
					other.head_node_const = nullptr;
#endif
					return;
				}
				data_ = Traits::allocate(alloc_, capacity_);

				size_type i = 0;
				try {
					for (; i < other.size_; i++) {
						Traits::construct(alloc_, data_ + i, std::move(other.data_[i]));
					}
				}
				catch (...) {
					for (size_type j = 0; j < i; j++) Traits::destroy(alloc_, data_ + j);
					Traits::deallocate(alloc_, data_, capacity_);
					throw;
				}

				for (size_type j = 0; j < other.size_; j++) {
					Traits::destroy(other.alloc_, other.data_ + j);
				}
				if (other.data_) {
					Traits::deallocate(other.alloc_, other.data_, other.capacity_);
				}

				other.data_ = nullptr;
				other.size_ = 0;
				other.capacity_ = 0;
			}
		}

#ifndef NDEBUG
		for (IteratorNode<T*, T&>* node = other.head_node_; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}
		for (IteratorNode<const T*, const T&>* node = other.head_node_const; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}

		head_node_ = other.head_node_;
		head_node_const = other.head_node_const;
		other.head_node_ = nullptr;
		other.head_node_const = nullptr;
#endif
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>::~vector() noexcept {
#ifndef NDEBUG
		IteratorNode<T*, T&>* cur_node = head_node_;
		while (cur_node) {
			IteratorNode<T*, T&>* next_node = cur_node->next_;
			if (cur_node->iterator_) {
				cur_node->iterator_->is_valid_ = false;
				cur_node->iterator_->container_ = nullptr;
			}
			delete_node(cur_node);
			cur_node = next_node;
		}
		IteratorNode<const T*, const T&>* cur_node_c = head_node_const;
		while (cur_node_c) {
			IteratorNode<const T*, const T&>* next_node_c = cur_node_c->next_;
			if (cur_node_c->iterator_) {
				cur_node_c->iterator_->is_valid_ = false;
				cur_node_c->iterator_->container_ = nullptr;
			}
			delete_node(cur_node_c);
			cur_node_c = next_node_c;
		}
#endif
		for (size_type i = 0; i < size_; i++) Traits::destroy(alloc_, data_ + i);
		if (data_) Traits::deallocate(alloc_, data_, capacity_);
	}
#pragma endregion

#pragma region element access
	template<typename T, typename Alloc>
	inline T& vector<T, Alloc>::operator[](size_type index) {
#ifdef NDEBUG
		return data_[index];
#else
		return iterator{ data_, this } [index] ;
#endif
	}

	template<typename T, typename Alloc>
	inline const T& vector<T, Alloc>::operator[](size_type index) const {
#ifdef NDEBUG
		return data_[index];
#else
		return iterator{ data_, this } [index] ;
#endif
	}

	template<typename T, typename Alloc>
	inline T& vector<T, Alloc>::at(size_type index) {
		if (index < size_) return data_[index];
		throw std::out_of_range("vector::at() index out of range");
	}

	template<typename T, typename Alloc>
	inline const T& vector<T, Alloc>::at(size_type index) const {
		if (index < size_) return data_[index];
		throw std::out_of_range("vector::at() index out of range");
	}

	template<typename T, typename Alloc>
	inline T& vector<T, Alloc>::front() {
		if (size_ == 0) throw std::out_of_range("vector::front() on empty vector");
		return *data_;
	}

	template<typename T, typename Alloc>
	inline const T& vector<T, Alloc>::front() const {
		if (size_ == 0) throw std::out_of_range("vector::front() on empty vector");
		return *data_;
	}

	template<typename T, typename Alloc>
	inline T& vector<T, Alloc>::back() {
		if (size_ == 0) throw std::out_of_range("vector::back() on empty vector");
		return *(data_ + size_ - 1);
	}

	template<typename T, typename Alloc>
	inline const T& vector<T, Alloc>::back() const {
		if (size_ == 0) throw std::out_of_range("vector::back() on empty vector");
		return *(data_ + size_ - 1);
	}
#pragma endregion

#pragma region assign && operators
	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::assign(size_type count, const T& val) {
		if (count > max_size()) throw std::length_error("vector too long");

		if (count == 0) {
			for (size_type i = 0; i < size_; i++) {
				Traits::destroy(alloc_, data_ + i);
			}
			if (data_) {
				Traits::deallocate(alloc_, data_, capacity_);
			}

			data_ = nullptr;
			size_ = 0;
			capacity_ = 0;
			IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
			return;
		}

		T* new_data = Traits::allocate(alloc_, count);
		size_type i = 0;
		try {
			for (; i < count; i++) {
				Traits::construct(alloc_, new_data + i, val);
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, new_data + j);
			}
			Traits::deallocate(alloc_, new_data, count);
			throw;
		}

		for (size_type j = 0; j < size_; j++) {
			Traits::destroy(alloc_, data_ + j);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}
		data_ = new_data;
		size_ = count;
		capacity_ = count;

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
	}

	template<typename T, typename Alloc>
	template<class It>
	requires std::forward_iterator<It>
	inline void vector<T, Alloc>::assign(It first, It last) {
		size_type count = static_cast<size_t>(std::distance(first, last));

		if (count == 0) {
			for (size_type i = 0; i < size_; i++) {
				Traits::destroy(alloc_, data_ + i);
			}
			if (data_) {
				Traits::deallocate(alloc_, data_, capacity_);
			}

			data_ = nullptr;
			size_ = 0;
			capacity_ = 0;
			IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
			return;
		}


		T* new_data = Traits::allocate(alloc_, count);
		size_type i = 0;
		try {
			for (It it = first; it != last; ++it) {
				Traits::construct(alloc_, new_data + i, *it);
				i++;
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, new_data + j);
			}
			Traits::deallocate(alloc_, new_data, count);
			throw;
		}

		for (size_type j = 0; j < size_; j++) {
			Traits::destroy(alloc_, data_ + j);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}
		data_ = new_data;
		size_ = count;
		capacity_ = count;

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
	}

	template<typename T, typename Alloc>
	template<class It>
	requires std::input_iterator<It> && (!std::forward_iterator<It>)
	inline void vector<T, Alloc>::assign(It first, It last) {
		for (size_type i = 0; i < size_; ++i) {
			Traits::destroy(alloc_, data_ + i);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}

		data_ = nullptr;
		size_ = 0;
		capacity_ = 0;

		for (It it = first; it != last; ++it) {
			emplace_back(*it);
		}

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::assign(std::initializer_list<T> ilist) {
		assign(ilist.begin(), ilist.end());
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>& vector<T, Alloc>::operator=(const vector<T, Alloc>& other) {
		if (&other == this) return *this;

		if (other.capacity_ == 0) {
			for (size_type j = 0; j < size_; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			if (data_) {
				Traits::deallocate(alloc_, data_, capacity_);
			}

			if constexpr (Traits::propagate_on_container_copy_assignment::value) {
				alloc_ = other.alloc_;
			}

			data_ = nullptr;
			size_ = 0;
			capacity_ = 0;

			IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
				return *this;
		}

		Alloc& alloc = Traits::propagate_on_container_copy_assignment::value ? const_cast<Alloc&>(other.alloc_) : alloc_;
		T* new_data = Traits::allocate(alloc, other.capacity_);

		size_type i = 0;
		try {
			for (; i < other.size_; i++) {
				Traits::construct(alloc, new_data + i, other.data_[i]);
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc, new_data + j);
			}
			Traits::deallocate(alloc, new_data, other.capacity_);
			throw;
		}

		for (size_type j = 0; j < size_; j++) {
			Traits::destroy(alloc_, data_ + j);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}

		data_ = new_data;
		size_ = other.size_;
		capacity_ = other.capacity_;

		if constexpr (Traits::propagate_on_container_copy_assignment::value) {
			alloc_ = other.alloc_;
		}

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
		return *this;
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>& vector<T, Alloc>::copy(const vector<T, Alloc>& other) {
		if (&other == this) return *this;

		for (size_type i = 0; i < size_; i++) {
			Traits::destroy(alloc_, data_ + i);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}

		size_ = other.size_;
		capacity_ = other.capacity_;
		if constexpr (Traits::propagate_on_container_copy_assignment::value) {
			alloc_ = other.alloc_;
		}

		if (capacity_ == 0) {
			data_ = nullptr;
			IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
			return *this;
		}
		data_ = Traits::allocate(alloc_, capacity_);

		for (size_type i = 0; i < size_; i++) {
			Traits::construct(alloc_, data_ + i, other.data_[i]);
		}

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
		return *this;
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>& vector<T, Alloc>::operator=(vector<T, Alloc>&& other) 
		noexcept(Traits::propagate_on_container_move_assignment::value || Traits::is_always_equal::value)
	{
		if (&other == this) return *this;

		for (size_type i = 0; i < size_; i++) {
			Traits::destroy(alloc_, data_ + i);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}

		size_ = other.size_;
		capacity_ = other.capacity_;

		if (other.capacity_ == 0) {
			if constexpr (Traits::propagate_on_container_move_assignment::value) {
				alloc_ = std::move(other.alloc_);
			}
			data_ = nullptr;

#ifndef NDEBUG
			invalidate_all_iterators();
			for (IteratorNode<T*, T&>* node = other.head_node_; node; node = node->next_) {
				if (node->iterator_) node->iterator_->container_ = this;
			}
			for (IteratorNode<const T*, const T&>* node = other.head_node_const; node; node = node->next_) {
				if (node->iterator_) node->iterator_->container_ = this;
			}

			head_node_ = other.head_node_;
			head_node_const = other.head_node_const;
			other.head_node_ = nullptr;
			other.head_node_const = nullptr;
#endif
			return *this;
		}

		if constexpr (Traits::is_always_equal::value || Traits::propagate_on_container_move_assignment::value) {
			
			if constexpr (Traits::propagate_on_container_move_assignment::value) {
				alloc_ = std::move(other.alloc_);
			}
			data_ = other.data_;

			other.size_ = 0;
			other.capacity_ = 0;
			other.data_ = nullptr;
		}
		else {
			if (this->alloc_ == other.alloc_) {
				data_ = other.data_;

				other.size_ = 0;
				other.capacity_ = 0;
				other.data_ = nullptr;
			}
			else {
				data_ = Traits::allocate(alloc_, other.capacity_);

				for (size_type i = 0; i < other.size_; i++) {
					Traits::construct(alloc_, data_ + i, std::move(other.data_[i]));
				}

				for (size_type i = 0; i < other.size_; i++) {
					Traits::destroy(other.alloc_, other.data_ + i);
				}
				if (other.data_) {
					Traits::deallocate(other.alloc_, other.data_, other.capacity_);
				}

				other.data_ = nullptr;
				other.size_ = 0;
				other.capacity_ = 0;
			}
		}
#ifndef NDEBUG
		invalidate_all_iterators();
		for (IteratorNode<T*, T&>* node = other.head_node_; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}
		for (IteratorNode<const T*, const T&>* node = other.head_node_const; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}

		head_node_ = other.head_node_;
		head_node_const = other.head_node_const;
		other.head_node_ = nullptr;
		other.head_node_const = nullptr;
#endif
		return *this;
	}

	template<typename T, typename Alloc>
	inline vector<T, Alloc>& vector<T, Alloc>::operator=(std::initializer_list<T> list) {
		size_t size = list.size();
		
		if (size == 0) {
			for (size_type j = 0; j < size_; ++j) {
				Traits::destroy(alloc_, data_ + j);
			}
			if (data_) {
				Traits::deallocate(alloc_, data_, capacity_);
			}

			data_ = nullptr;
			size_ = 0;
			capacity_ = 0;

			IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
			return *this;
		}
		
		T* new_data = Traits::allocate(alloc_, size);
		
		size_type i = 0;
		try {
			for (const T& el : list) {
				Traits::construct(alloc_, new_data + i, el);
				i++;
			}
		}
		catch (...) {
			for (size_type j = 0; j < i; j++) {
				Traits::destroy(alloc_, new_data + j);
			}
			Traits::deallocate(alloc_, new_data, size);
			throw;
		}

		for (size_type j = 0; j < size_; j++) {
			Traits::destroy(alloc_, data_ + j);
		}
		if (data_) {
			Traits::deallocate(alloc_, data_, capacity_);
		}

		data_ = new_data;
		size_ = size;
		capacity_ = size;

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
		return *this;
	}

	template <typename T, typename Alloc>
	inline bool vector<T, Alloc>::operator!=(const vector<T, Alloc>& other) const {
		if (size_ == other.size_) {
			for (size_type i = 0; i < size_; i++) {
				if (data_[i] != other.data_[i]) return true;
			}
			return false;
		} return true;
	}

	template <typename T, typename Alloc>
	inline bool vector<T, Alloc>::operator==(const vector<T, Alloc>& other) const {
		if (size_ == other.size_) {
			for (size_type i = 0; i < size_; i++) {
				if (data_[i] != other.data_[i]) return false;
			}
			return true;
		} return false;
	}

	template <typename T, typename Alloc>
	inline bool vector<T, Alloc>::operator<=(const vector<T, Alloc>& other) const {
		size_type min_size = ((size_ < other.size_) ? size_ : other.size_);
		for (size_type i = 0; i < min_size; i++) {
			if (data_[i] < other.data_[i]) return true;
			else if (data_[i] > other.data_[i]) return false;
		}
		return size_ <= other.size_;
	}

	template <typename T, typename Alloc>
	inline bool vector<T, Alloc>::operator>=(const vector<T, Alloc>& other) const {
		size_type min_size = ((size_ < other.size_) ? size_ : other.size_);
		for (size_type i = 0; i < min_size; i++) {
			if (data_[i] > other.data_[i]) return true;
			else if (data_[i] < other.data_[i]) return false;
		}
		return size_ >= other.size_;
	}

	template <typename T, typename Alloc>
	inline bool vector<T, Alloc>::operator<(const vector<T, Alloc>& other) const {
		size_type min_size = ((size_ < other.size_) ? size_ : other.size_);
		for (size_type i = 0; i < min_size; i++) {
			if (data_[i] < other.data_[i]) return true;
			else if (data_[i] > other.data_[i]) return false;
		}
		return size_ < other.size_;
	}

	template <typename T, typename Alloc>
	inline bool vector<T, Alloc>::operator>(const vector<T, Alloc>& other) const {
		size_type min_size = ((size_ < other.size_) ? size_ : other.size_);
		for (size_type i = 0; i < min_size; i++) {
			if (data_[i] > other.data_[i]) return true;
			else if (data_[i] < other.data_[i]) return false;
		}
		return size_ > other.size_;
	}
#pragma endregion

#pragma region size
	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::resize(size_type new_size) {
		if (new_size > max_size()) throw std::length_error("vector too long");

		if (new_size <= size_) {
			for (size_type i = new_size; i < size_; i++) {
				Traits::destroy(alloc_, data_ + i);
			}
			size_ = new_size;
			return;
		}

		if (new_size > capacity_) {
			size_type new_capacity = (new_size < factor_ * capacity_ && factor_ * capacity_ <= max_size()) ? (factor_ * capacity_) : (new_size);
			reserve(new_capacity);
		}

		size_type i = size_;
		try {
			for (; i < new_size; i++) {
				Traits::construct(alloc_, data_ + i);
			}
		}
		catch (...) {
			for (size_type j = size_; j < i; j++) Traits::destroy(alloc_, data_ + j);
			throw;
		}
		size_ = new_size;
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::resize(size_type new_size, const T& val) {
		if (new_size > max_size()) throw std::length_error("vector too long");


		if (new_size <= size_) {
			for (size_type i = new_size; i < size_; i++) {
				Traits::destroy(alloc_, data_ + i);
			}
			size_ = new_size;
			return;
		}

		T val_copy = val;
		if (new_size > capacity_) {
			size_type new_capacity = (new_size < factor_ * capacity_ && factor_ * capacity_ <= max_size()) ? (factor_ * capacity_) : (new_size);
			reserve(new_capacity);
		}

		size_type i = size_;
		try {
			for (; i < new_size; i++) {
				Traits::construct(alloc_, data_ + i, val_copy);
			}
		}
		catch (...) {
			for (size_type j = size_; j < i; j++) Traits::destroy(alloc_, data_ + j);
			throw;
		}
		size_ = new_size;
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::clear() noexcept {
		for (size_type i = 0; i < size_; i++) {
			Traits::destroy(alloc_, data_ + i);
		}
		size_ = 0;

		IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
	}
#pragma endregion

#pragma region memory
	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::reserve(size_type new_capacity) {
		if (new_capacity > max_size()) throw std::length_error("vector too long");

		if (new_capacity > capacity_) {
			T* new_data = Traits::allocate(alloc_, new_capacity);
			
			size_type i = 0;
			try {
				for (; i < size_; i++) {
					Traits::construct(alloc_, new_data + i, std::move_if_noexcept(data_[i]));
				}
			}
			catch (...) {
				for (size_type j = 0; j < i; j++) {
					Traits::destroy(alloc_, new_data + j);
				}
				Traits::deallocate(alloc_, new_data, new_capacity);
				throw;
			}
			
			for (size_type j = 0; j < size_; j++) {
				Traits::destroy(alloc_, data_ + j);
			}
			if (data_) {
				Traits::deallocate(alloc_, data_, capacity_);
			}
			
			data_ = new_data;
			capacity_ = new_capacity;

			IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
		}
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::shrink_to_fit() {
		if (capacity_ > size_) {
			if (size_ == 0) {
				if (data_) Traits::deallocate(alloc_, data_, capacity_);
				data_ = nullptr;
				capacity_ = 0;
			}
			else {
				T* tmp = Traits::allocate(alloc_, size_);

				size_type i = 0;
				try {
					for (; i < size_; i++) {
						Traits::construct(alloc_, tmp + i, std::move_if_noexcept(data_[i]));
					}
				}
				catch (...) {
					for (size_type j = 0; j < i; j++) Traits::destroy(alloc_, tmp + j);
					Traits::deallocate(alloc_, tmp, size_);
					throw;
				}

				for (size_type j = 0; j < size_; j++) {
					Traits::destroy(alloc_, data_ + j);
				}
				if (data_) {
					Traits::deallocate(alloc_, data_, capacity_);
				}
				
				data_ = tmp;
				capacity_ = size_;
			}
			IF_CONFIGURATION_IS_DEBUG(invalidate_all_iterators();)
		}
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::swap(vector<T, Alloc>& other) noexcept {
#ifndef NDEBUG
		for (IteratorNode<T*, T&>* node = head_node_; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = &other;
		}
		for (IteratorNode<T*, T&>* node = other.head_node_; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}
		for (IteratorNode<const T*, const T&>* node = head_node_const; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = &other;
		}
		for (IteratorNode<const T*, const T&>* node = other.head_node_const; node; node = node->next_) {
			if (node->iterator_) node->iterator_->container_ = this;
		}
		std::swap(head_node_, other.head_node_);
		std::swap(head_node_const, other.head_node_const);
#endif

		std::swap(size_, other.size_);
		std::swap(capacity_, other.capacity_);
		std::swap(data_, other.data_);
		if constexpr (Traits::propagate_on_container_swap::value) std::swap(alloc_, other.alloc_);
	}
#pragma endregion

#pragma region insert
	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::push_back(const T& val) {
		if (size_ >= capacity_) {
			size_type new_capacity = (size_ + 1 < factor_ * capacity_ && factor_ * capacity_ <= max_size()) ? (factor_ * capacity_) : (size_ + 1);
			reserve_and_emplace(new_capacity, size_, val);
		}
		else { 
			Traits::construct(alloc_, data_ + size_, val);
			IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(size_);)
			size_++;
		}
	}

	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::push_back(T&& val) {
		if (size_ >= capacity_) {
			size_type new_capacity = (size_ + 1 < factor_ * capacity_ && factor_ * capacity_ <= max_size()) ? (factor_ * capacity_) : (size_ + 1);
			reserve_and_emplace(new_capacity, size_, std::move(val));
		}
		else {
			Traits::construct(alloc_, data_ + size_, std::move(val));
			IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(size_);)
			size_++;
		}
	}

	template<typename T, typename Alloc>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator pos, const T& val) {
		if (size_ + 1 > max_size()) throw std::length_error("vector too long");

#ifndef NDEBUG
		validity_check_from_vector_std(*this, pos);
		size_type index = pos.ptr_ - data_;
#else
		size_type index = pos - data_;
#endif
		auto generator = [this, index, &val](T* target_ptr) {
			Traits::construct(alloc_, target_ptr, val);
		};

		if (size_ >= capacity_) {
			reserve_and_insert(index, 1, generator);
		}
		else {
			if constexpr (std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
				insert_in_place(index, 1, generator);
			}
			else {
				reserve_and_insert(index, 1, generator);
			}
		}
#ifndef NDEBUG
		return { data_ + index, this };
#else
		return data_ + index;
#endif
	}

	template<typename T, typename Alloc>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator pos, T&& val) {
		if (size_ + 1 > max_size()) throw std::length_error("vector too long");

#ifndef NDEBUG
		validity_check_from_vector_std(*this, pos);
		size_type index = pos.ptr_ - data_;
#else
		size_type index = pos - data_;
#endif

		auto generator = [this, index, &val](T* target_ptr) {
			Traits::construct(alloc_, target_ptr, std::move(val));
		};

		if (size_ >= capacity_) {
			reserve_and_insert(index, 1, generator);
		}
		else {
			if constexpr (std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
				insert_in_place(index, 1, generator);
			}
			else {
				reserve_and_insert(index, 1, generator);
			}
		}
#ifndef NDEBUG
		return { data_ + index, this };
#else
		return data_ + index;
#endif
	}

	template<typename T, typename Alloc>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator pos, size_type count, const T& val) {
		if (size_ + count > max_size()) throw std::length_error("vector too long");

#ifndef NDEBUG
		validity_check_from_vector_std(*this, pos);
		size_type index = pos.ptr_ - data_;
#else
		size_type index = pos - data_;
#endif

		if (count != 0) {
			auto generator = [this, count, &val](T* target_ptr) {
				size_type i = 0;
				try {
					for (; i < count; i++) {
						Traits::construct(alloc_, target_ptr + i, val);
					}
				}
				catch (...) {
					for (size_type j = 0; j < i; j++) {
						Traits::destroy(alloc_, target_ptr + j);
					}
					throw;
				}
			};

			if (size_ + count > capacity_) {
				reserve_and_insert(index, count, generator);
			}
			else {
				if constexpr (std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
					insert_in_place(index, count, generator);
				}
				else {
					reserve_and_insert(index, count, generator);
				}
			}
		}
#ifndef NDEBUG
		return { data_ + index, this };
#else
		return data_ + index;
#endif
	}

	template<typename T, typename Alloc>
	template<class It>
	requires std::forward_iterator<It>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator pos, It first, It last) {
#ifndef NDEBUG
		validity_check_from_vector_std(*this, pos);
		size_type index = pos.ptr_ - data_;
#else
		size_type index = pos - data_;
#endif

		if (first != last) {
			size_type count = static_cast<size_t>(std::distance(first, last));
			if (size_ + count > max_size()) throw std::length_error("vector too long");

			auto generator = [this, count, first, last](T* target_ptr) mutable {
				size_type i = 0;
				try {
					for (; first != last; ++first, i++) {
						Traits::construct(alloc_, target_ptr + i, *first);
					}
				}
				catch (...) {
					for (size_type j = 0; j < i; j++) {
						Traits::destroy(alloc_, target_ptr + j);
					}
					throw;
				}
			};

			if (size_ + count > capacity_) {
				reserve_and_insert(index, count, generator);
			}
			else {
				if constexpr (std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
					insert_in_place(index, count, generator);
				}
				else {
					reserve_and_insert(index, count, generator);
				}
			}
		}
#ifndef NDEBUG
		return { data_ + index, this };
#else
		return data_ + index;
#endif
	}

	template<typename T, typename Alloc>
	template<class It>
	requires std::input_iterator<It> && (!std::forward_iterator<It>)
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator pos, It first, It last) {
#ifndef NDEBUG
		validity_check_from_vector_std(*this, pos);
		size_type index = pos.ptr_ - data_;

		size_type i = index;
		for (It it = first; it != last; ++it) {
			insert({ data_ + i, this }, *it);
			i++;
		}

		return { data_ + index, this };
#else
		size_type index = pos - data_;

		size_type i = index;
		for (It it = first; it != last; ++it) {
			insert(data_ + i, *it);
			i++;
		}

		return data_ + index;
#endif
	}

	template<typename T, typename Alloc>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::insert(iterator pos, std::initializer_list<T> list) {
		return insert(pos, list.begin(), list.end());
	}
#pragma endregion 

#pragma region erase
	template<typename T, typename Alloc>
	inline void vector<T, Alloc>::pop_back() {
		if (size_ == 0) throw std::out_of_range("Error: vector::pop_back() called on empty vector");
		IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(size_ - 1);)
		size_--;
		Traits::destroy(alloc_, data_ + size_);
	}

	template<typename T, typename Alloc>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::erase(iterator pos) {
#ifndef NDEBUG
		validity_check_from_vector_for_erase(*this, pos);
		size_type index = pos.ptr_ - data_;
#else
		size_type index = pos - data_;
#endif

		for (size_type i = index; i < size_ - 1; i++) {
			data_[i] = std::move(data_[i + 1]);
		}
		Traits::destroy(alloc_, data_ + size_ - 1);
		size_--;

		IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(index);)

#ifndef NDEBUG
			return { data_ + index, this };
#else
			return data_ + index;
#endif
	}

	template<typename T, typename Alloc>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::erase(iterator pos, size_type count) {
#ifndef NDEBUG
		validity_check_from_vector_for_erase(*this, pos);
		size_type index = pos.ptr_ - data_;
#else
		size_type index = pos - data_;
#endif

		if (count > size_ - index) throw std::invalid_argument("vector erase count exceeds remaining elements");

		if (count > 0) {
			for (size_type i = index; i < size_ - count; i++) {
				data_[i] = std::move(data_[i + count]);
			}

			for (size_type i = size_ - count; i < size_; i++) {
				Traits::destroy(alloc_, data_ + i);
			}
			size_ -= count;

			IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(index);)
		}

#ifndef NDEBUG
			return { data_ + index, this };
#else
			return data_ + index;
#endif
	}

	template<typename T, typename Alloc>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::erase(iterator first, iterator last) {
#ifndef NDEBUG
		validity_check_from_vector_for_erase(*this, first);
		validity_check_from_vector_for_erase(*this, last);
		size_type first_index = first.ptr_ - data_;
#else
		size_type first_index = first - data_;
#endif
		if (first > last) throw std::out_of_range("Error: vector::erase first and last are reversed");
		if (first < last) {
			size_type count = last - first;

			for (size_type i = first_index; i < size_ - count; i++) {
				data_[i] = std::move(data_[i + count]);
			}

			for (size_type i = size_ - count; i < size_; i++) {
				Traits::destroy(alloc_, data_ + i);
			}
			size_ -= count;

			IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(first_index);)
		}

#ifndef NDEBUG
		return { data_ + first_index, this };
#else
		return data_ + first_index;
#endif
	}
#pragma endregion

#pragma region emplace
	template<typename T, typename Alloc>
	template<typename... Args>
	inline T& vector<T, Alloc>::emplace_back(Args&&... args) {
		if (size_ >= capacity_) {
			size_type new_capacity = (size_ + 1 < factor_ * capacity_ && factor_ * capacity_ <= max_size()) ? (factor_ * capacity_) : (size_ + 1);
			reserve_and_emplace(new_capacity, size_, std::forward<Args>(args)...);
		}
		else {
			Traits::construct(alloc_, data_ + size_, std::forward<Args>(args)...);
			size_++;
			IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(size_);)
		}
		return data_[size_ - 1];
	}

	template<typename T, typename Alloc>
	template<typename... Args>
	inline typename vector<T, Alloc>::iterator vector<T, Alloc>::emplace(iterator pos, Args&&... args) {
#ifndef NDEBUG
		validity_check_from_vector_std(*this, pos);
		size_type index = pos.ptr_ - data_;
#else
		size_type index = pos - data_;
#endif 

		if (size_ >= capacity_) {
			size_type new_capacity = (capacity_ == 0) ? 1 : factor_ * capacity_;
			reserve_and_emplace(new_capacity, index, std::forward<Args>(args)...);
		}
		else {
			if (size_ > index) {
				T* tmp = Traits::allocate(alloc_, 1);
				bool tmp_el_constructed = false;

				size_type i = size_ - 1;
				bool last_el_constructed = false;
				try {
					Traits::construct(alloc_, tmp, std::forward<Args>(args)...);
					tmp_el_constructed = true;

					Traits::construct(alloc_, data_ + size_, std::move_if_noexcept(data_[size_ - 1]));
					last_el_constructed = true;
					
					for (; i > index; i--) {
						data_[i] = std::move_if_noexcept(data_[i - 1]);
					}
					data_[index] = std::move(*tmp);
				}
				catch (...) {
					if (tmp_el_constructed) {
						Traits::destroy(alloc_, tmp);

						if (last_el_constructed) {
							for (; i < size_; i++) {
								data_[i] = std::move_if_noexcept(data_[i + 1]);
							}
							Traits::destroy(alloc_, data_ + size_);
						}
					
					}
					Traits::deallocate(alloc_, tmp, 1);
					throw;
				}
				Traits::destroy(alloc_, tmp);
				Traits::deallocate(alloc_, tmp, 1);
			}
			else {
				Traits::construct(alloc_, data_ + index, std::forward<Args>(args)...);
			}
			size_++;

			IF_CONFIGURATION_IS_DEBUG(invalidate_iterators_from(index);)
		}

#ifndef NDEBUG
		return { data_ + index, this };
#else
		return data_ + index;
#endif
	}
#pragma endregion
}

#undef IF_CONFIGURATION_IS_DEBUG