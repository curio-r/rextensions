#pragma once

template<typename Class>
struct BaseProxy : public Class
{
	typedef Class ProxyClass;

	template<typename Func>
	struct MethodRef
	{
	private:

		Func _Ptr;

	public:

		template<typename Type>
		inline void operator=(Type Ptr)
		{
			_Ptr = *reinterpret_cast<Func *>(&Ptr);
		}

		template<typename ... Types>
		auto operator()(Class *_Context, Types ... _Args) -> decltype((_Context->*_Ptr)(_Args ...))
		{
			return (_Context->*_Ptr)(_Args ...);
		}
	};
};

template<typename Class>
struct ClassProxy : public BaseProxy<Class>
{
protected:

	Class *_Instance;

public:

	//ClassProxy<Class>& operator=(Class * instance)
	//{
	//	Instance = instance;
	//
	//	return *this;
	//}

	inline Class* operator->() const
	{
		return _Instance;
	}

	inline operator Class *()
	{
		return _Instance;
	}
};

template<typename Class>
struct ClassRefProxy : BaseProxy<Class>
{
protected:

	Class **_Instance;

public:

	inline Class * operator*()
	{
		return *_Instance;
	}

	inline operator Class **()
	{
		return _Instance;
	}

	inline Class * operator->() const
	{
		return *_Instance;
	}
};
