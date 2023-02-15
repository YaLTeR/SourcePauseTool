#pragma once

/*
* A work-in-progress RAII wrapper for materials, textures, and probably DWrite objects when we get text. The
* REF_MGR class should be stateless, default constructable, and have const Release()/AddRef() methods that take in
* type T or T& as a param.
*/
template<class T, typename REF_MGR, REF_MGR refMgr = REF_MGR{}>
struct AutoRefPtr
{
	AutoRefPtr() : _ptr(nullptr) {}

	// This doesn't add a ref since ptr must already have a ref to exist, but it does mean that you don't want to
	// create two instances of this from the same ptr; using the copy ctor is fine since that'll add a ref.
	AutoRefPtr(const T _ptr) : _ptr(_ptr) {}

	AutoRefPtr(const AutoRefPtr& other) : _ptr(other._ptr)
	{
		if (_ptr)
			refMgr.AddRef(_ptr);
	}

	AutoRefPtr(AutoRefPtr&& rhs) : _ptr(rhs._ptr)
	{
		rhs._ptr = nullptr;
	}

	AutoRefPtr& operator=(T ptr)
	{
		if (_ptr != ptr)
		{
			if (ptr)
				refMgr.AddRef(ptr);
			if (_ptr)
				refMgr.Release(_ptr);
			_ptr = ptr;
		}
		return *this;
	}

	AutoRefPtr& operator=(const AutoRefPtr rhs)
	{
		return *this = rhs._ptr;
	}

	operator T() const
	{
		return _ptr;
	}

	T operator->() const
	{
		return _ptr;
	}

	void Release()
	{
		if (_ptr)
			refMgr.Release(_ptr);
	}

	~AutoRefPtr()
	{
		Release();
	}

	T _ptr;
};
