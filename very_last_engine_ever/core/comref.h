#ifndef COMREF_H
#define COMREF_H

#include <cassert>

template <typename T>
class ComRef {
public:
	ComRef() : p(NULL)
	{
	}

	explicit ComRef(T *p) : p(p)
	{
		if (p)
			p->AddRef();
	}

	ComRef(const ComRef<T>& t) :
	    p(t.p)
	{
		if (t.p)
			t.p->AddRef();
	}

	const ComRef<T> &operator =(const ComRef<T>& t)
	{
		if (t != p) {
			if (t)
				t->AddRef();
			if (p)
				p->Release();
			p = t;
		}
		return *this;
	}

	virtual ~ComRef()
	{
		if (p)
			p->Release();
	}

	void attachRef(T *p)
	{
		if (this->p)
			this->p->Release();
		this->p = p;
	}

	operator T*() const
	{
		return p;
	}

	T *operator ->() const
	{
		return p;
	}

	T **operator &()
	{
		return &p;
	}

	T *p;
};

#endif // COMREF_H