// 2005 Olivier Galizzi, Janek Kozicki
// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include <mutex>

#define FRIEND_SINGLETON(Class) friend class Singleton<Class>;
// use to instantiate the self static member.
#define SINGLETON_SELF(Class) template <> Class* Singleton<Class>::self = NULL;
// one singleton cannot access another in its constructor.
namespace {
std::mutex singleton_constructor_mutex;
}
template <class T> class Singleton {
protected:
	static T* self; // must not be method-local static variable, since it gets created in different translation units multiple times.
public:
	static T& instance()
	{
		if (!self) {
			const std::lock_guard<std::mutex> lock(singleton_constructor_mutex);
			if (!self)
				self = new T;
		}
		return *self;
	}
};
