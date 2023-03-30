#pragma once

#ifndef OE

#include <vector>

/*
* A slice of a vector. The primary purpose of this is to create a stack-like pool of shared memory for dynamic and
* debug meshes. A valid slice can only be created at the end of a vector, and there it can be edited mostly like a
* normal vector. Upon being destroyed the vector will shrink to whatever size it was before the slice was used. I
* only bothered to wrap the common vector methods plus a couple additional ones.
*/
template<typename T>
struct VectorSlice
{
	std::vector<T>* vec;
	size_t off;
	size_t len;

	VectorSlice() = default;

	VectorSlice(VectorSlice&) = delete;

	VectorSlice(VectorSlice&& other) : vec(other.vec), off(other.off), len(other.len)
	{
		other._clear();
	}

	VectorSlice(std::vector<T>& vec) : vec(&vec), off(vec.size()), len(0) {}

	inline VectorSlice& operator=(VectorSlice&& other)
	{
		if (this != &other)
		{
			memcpy(this, &other, sizeof VectorSlice<T>);
			other._clear();
		}
		return *this;
	}

	inline void _clear()
	{
		vec = nullptr;
		off = len = 0;
	}

	inline void assign_to_end(std::vector<T>& _vec)
	{
		Assert(!vec);
		vec = &_vec;
		off = _vec.size();
		len = 0;
	}

	inline void _verify_at_end() const
	{
		Assert(off + len == vec->size());
	}

	inline void pop()
	{
		if (vec)
		{
			_verify_at_end();
			vec->resize(vec->size() - len);
			_clear();
		}
	}

	inline std::vector<T>::iterator begin() const
	{
		return vec->begin() + off;
	}

	inline std::vector<T>::iterator end() const
	{
		return vec->begin() + off + len;
	}

	inline std::vector<T>::reverse_iterator rbegin() const
	{
		return vec->rbegin();
	}

	inline std::vector<T>::reverse_iterator rend() const
	{
		return std::vector<T>::reverse_iterator(begin());
	}

	template<class _It>
	inline void add_range(_It first, _It last)
	{
		_verify_at_end();
		len += std::distance(first, last);
		vec->insert(vec->end(), first, last);
	}

	/*inline void reserve_extra(size_t n)
	{
		_verify_at_end();
		if (vec->capacity() < off + len + n)
			vec->reserve(SmallestPowerOfTwoGreaterOrEqual(off + len + n));
	}*/

	inline void resize(size_t n)
	{
		_verify_at_end();
		vec->resize(off + n);
		len = n;
	}

	/*template<class _Pr>
	inline void erase_if(_Pr pred)
	{
		_verify_at_end();
		auto it = std::remove_if(begin(), end(), pred);
		auto removed = std::distance(it, end());
		vec->erase(it, end());
		len -= removed;
	}*/

	inline void push_back(const T& elem)
	{
		_verify_at_end();
		len++;
		vec->push_back(elem);
	}

	template<class... Params>
	inline decltype(auto) emplace_back(Params&&... params)
	{
		_verify_at_end();
		len++;
		return vec->emplace_back(std::forward<Params>(params)...);
	}

	inline T& operator[](size_t i) const
	{
		return vec->at(off + i);
	}

	inline bool empty() const
	{
		return len == 0;
	}

	inline size_t size() const
	{
		return len;
	}

	~VectorSlice()
	{
		pop();
	}
};

#endif
