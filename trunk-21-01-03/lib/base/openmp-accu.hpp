// 2010 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include <lib/base/Math.hpp>
#include <boost/serialization/split_free.hpp>
#include <unistd.h>

#ifdef YADE_OPENMP
#include "omp.h"
#endif

namespace yade { // Cannot have #include directive inside.

// due to the memcpy(…) invocation below only POD types are compatible with multithreaded accumulator.
// Later we might find a correct replacement, e.g. something like following line:
//     std::copy_n((T*)oldChunk, (nCL * CLS) / sizeof(T), chunks[th]);
// instead of memcpy(…)
// but it hasn't been properly tested yet.
#if defined(YADE_OPENMP) and (YADE_REAL_BIT <= 128)
// O(1) access container which stores data in contiguous chunks of memory
// each chunk belonging to one thread
template <typename T> class OpenMPArrayAccumulator {
	int             CLS;      // cache line size
	size_t          nThreads; // number of threads
	int             perCL;    // number of elements fitting inside cache line
	std::vector<T*> chunks;   /* array with pointers to the chunks of memory we have allocated; each item for one thread
	                             Each pointer *T inside chunks[th] is to be interpreted as an old C-style array with sz elements in there
	                             They are accessed with chunks[th][i], where always th<nThreads and i<sz */
	size_t          sz;       // current number of elements
	size_t          nCL;      // current number of allocated cache lines
	int nCL_for_N(size_t n) { return n / perCL + (n % perCL == 0 ? 0 : 1); } // return number of cache lines to allocate for given number of elements
public:
	OpenMPArrayAccumulator()
	        : CLS(sysconf(_SC_LEVEL1_DCACHE_LINESIZE) > 0 ? sysconf(_SC_LEVEL1_DCACHE_LINESIZE) : 64)
	        , nThreads(omp_get_max_threads())
	        , perCL(CLS / sizeof(T))
	        , chunks(nThreads, nullptr)
	        , sz(0)
	        , nCL(0)
	{
	}
	OpenMPArrayAccumulator(size_t n)
	        : CLS(sysconf(_SC_LEVEL1_DCACHE_LINESIZE) > 0 ? sysconf(_SC_LEVEL1_DCACHE_LINESIZE) : 64)
	        , nThreads(omp_get_max_threads())
	        , perCL(CLS / sizeof(T))
	        , chunks(nThreads, nullptr)
	        , sz(0)
	        , nCL(0)
	{
		resize(n);
	}
	// change number of elements (per each thread)
	void resize(size_t n)
	{
		if (n == sz)
			return; // nothing to do
		size_t nCL_new = nCL_for_N(n);
		if (nCL_new > nCL) {
			for (size_t th = 0; th < nThreads; th++) {
				void* oldChunk = (void*)chunks[th];
				int   succ     = posix_memalign((void**)(&chunks[th]), /*alignment*/ CLS, /*size*/ nCL_new * CLS);
				if (succ != 0)
					throw std::runtime_error("OpenMPArrayAccumulator: posix_memalign failed to allocate memory.");
				if (oldChunk) { // initialized to NULL initially, that must not be copied and freed
#if (YADE_REAL_BIT > 64)
#warning                                                                                                                                                       \
        "OpenMPArrayAccumulator hasn't been tested on high precision Real type. And it is disabled by default in CMakeLists.txt. It is possible that this memcpy may work for long double and float128, but not higher types. So better to replace it too."
#endif
					memcpy(/*dest*/ (void*)chunks[th], /*src*/ oldChunk, nCL * CLS); // preserve old data
					free(oldChunk);                                                  // deallocate old storage
				}
				nCL = nCL_new;
			}
		}
		// if nCL_new<nCL, do not deallocate memory
		// if nCL_new==nCL, only update sz
		// reset items that were added
		for (size_t s = sz; s < n; s++) {
			for (size_t th = 0; th < nThreads; th++)
				chunks[th][s] = ZeroInitializer<T>();
		}
		sz = n;
	}
	// clear (does not deallocate storage, anyway)
	void clear() { resize(0); }
	// return number of elements
	size_t size() const { return sz; }
	// get value of one element, by summing contributions of all threads
	T operator[](size_t ix) const { return get(ix); }
	T get(size_t ix) const
	{
		T ret(ZeroInitializer<T>());
		for (size_t th = 0; th < nThreads; th++)
			ret += chunks[th][ix];
		return ret;
	}
	// set value of one element; all threads are reset except for the 0th one, which assumes that value
	void set(size_t ix, const T& val)
	{
		for (size_t th = 0; th < nThreads; th++)
			chunks[th][ix] = (th == 0 ? val : ZeroInitializer<T>());
	}
	// reset one element to ZeroInitializer
	void add(size_t ix, const T& diff) { chunks[omp_get_thread_num()][ix] += diff; }
	void reset(size_t ix) { set(ix, ZeroInitializer<T>()); }
	// get all stored data, organized first by index, then by threads; only used for debugging
	std::vector<std::vector<T>> getPerThreadData() const
	{
		std::vector<std::vector<T>> ret;
		for (size_t ix = 0; ix < sz; ix++) {
			std::vector<T> vi;
			for (size_t th = 0; th < nThreads; th++)
				vi.push_back(chunks[th][ix]);
			ret.push_back(vi);
		}
		return ret;
	}
};

/* Class accumulating results of type T in parallel sections. Summary value (over all threads) can be read or reset in non-parallel sections only.

#. update value, useing the += operator.
#. Get value using implicit conversion to T (in non-parallel sections only)
#. Reset value by calling reset() (in non-parallel sections only)

Storage of data is aligned to cache line size, no false sharing should occur (but some space is wasted, OTOH)
This will currently not compile for non-POSIX systems, as we use sysconf and posix_memalign.

*/
template <typename T> class OpenMPAccumulator {
	// in the ctor, assume 64 bytes (arbitrary, but safe) if sysconf does not report anything meaningful
	// that might happen on newer proc models not yet reported by sysconf (?)
	// e.g. https://lists.launchpad.net/yade-dev/msg06294.html
	// where zero was reported, leading to FPU exception at startup
	int   CLS; // cache line size
	int   nThreads;
	int   eSize; // size of an element, computed from cache line size and sizeof(T)
	char* data;  // use void* rather than T*, since with T* the pointer arithmetics has sizeof(T) as unit, which is confusing; char* takes one byte
public:
	// initialize storage with _zeroValue, depending on number of threads
	OpenMPAccumulator()
	        : CLS(sysconf(_SC_LEVEL1_DCACHE_LINESIZE) > 0 ? sysconf(_SC_LEVEL1_DCACHE_LINESIZE) : 64)
	        , nThreads(omp_get_max_threads())
	        , eSize(CLS * (sizeof(T) / CLS + (sizeof(T) % CLS == 0 ? 0 : 1)))
	{
		int succ = posix_memalign(/*where allocated*/ (void**)&data, /*alignment*/ CLS, /*size*/ nThreads * eSize);
		if (succ != 0)
			throw std::runtime_error("OpenMPAccumulator: posix_memalign failed to allocate memory.");
		reset();
	}
	~OpenMPAccumulator() { free((void*)data); }
	// lock-free addition
	void operator+=(const T& val) { *((T*)(data + omp_get_thread_num() * eSize)) += val; }
	void operator-=(const T& val) { *((T*)(data + omp_get_thread_num() * eSize)) -= val; }
	// return summary value; must not be used concurrently
	operator T() const { return get(); }
	// reset to zeroValue; must NOT be used concurrently
	void reset()
	{
		for (int i = 0; i < nThreads; i++)
			*(T*)(data + i * eSize) = ZeroInitializer<T>();
	}
	// this can be used to get the value from python, something like
	// .def_readonly("myAccu",&OpenMPAccumulator::get,"documentation")
	T get() const
	{
		T ret(ZeroInitializer<T>());
		for (int i = 0; i < nThreads; i++)
			ret += *(T*)(data + i * eSize);
		return ret;
	}
	void set(const T& value)
	{
		reset(); /* set value for the 0th thread */
		*(T*)(data) = value;
	}
	// only useful for debugging
	std::vector<T> getPerThreadData() const
	{
		std::vector<T> ret;
		for (int i = 0; i < nThreads; i++)
			ret.push_back(*(T*)(data + i * eSize));
		return ret;
	}
};

/* OpenMP implementation of std::vector. 
 * Very minimal functionality, which is required by Yade
 */
template <typename T> class OpenMPVector {
	std::vector<std::vector<T>> vals;
	size_t                      sizeV;

public:
	OpenMPVector()
	{
		sizeV = omp_get_max_threads();
		vals.resize(sizeV);
	};
	void   push_back(const T& val) { vals[omp_get_thread_num()].push_back(val); };
	size_t size() const
	{
		size_t sumSize = 0;
		for (size_t i = 0; i < sizeV; i++) {
			sumSize += vals[i].size();
		}
		return sumSize;
	}

	size_t size(size_t t) const
	{
		if (t >= sizeV) {
			std::cerr << ("Index is out of range.") << std::endl;
			exit(EXIT_FAILURE);
		} else {
			return vals[t].size();
		}
	}

	size_t sizeT() { return sizeV; }

	T operator[](size_t ix) const
	{
		if (ix >= size()) {
			std::cerr << ("Index is out of range.") << std::endl;
			exit(EXIT_FAILURE);
		} else {
			size_t t = 0;
			while (ix >= vals[t].size()) {
				ix -= vals[t].size();
				t += 1;
			}
			return vals[t][ix];
		}
	}

	void clear()
	{
		for (size_t i = 0; i < sizeV; i++) {
			vals[i].clear();
		}
	}
};
#else
template <typename T> class OpenMPArrayAccumulator {
	std::vector<T> data;

public:
	OpenMPArrayAccumulator() { }
	OpenMPArrayAccumulator(size_t n) { resize(n); }
	void                        resize(size_t s) { data.resize(s, ZeroInitializer<T>()); }
	void                        clear() { data.clear(); }
	size_t                      size() const { return data.size(); }
	T                           operator[](size_t ix) const { return get(ix); }
	T                           get(size_t ix) const { return data[ix]; }
	void                        add(size_t ix, const T& diff) { data[ix] += diff; }
	void                        set(size_t ix, const T& val) { data[ix] = val; }
	void                        reset(size_t ix) { data[ix] = ZeroInitializer<T>(); }
	std::vector<std::vector<T>> getPerThreadData() const
	{
		std::vector<std::vector<T>> ret;
		for (size_t ix = 0; ix < data.size(); ix++) {
			std::vector<T> vi;
			vi.push_back(data[ix]);
			ret.push_back(vi);
		}
		return ret;
	}
};

// single-threaded version of the accumulator above
template <typename T> class OpenMPAccumulator {
	T data;

public:
	void operator+=(const T& val) { data += val; }
	void operator-=(const T& val) { data -= val; }
	     operator T() const { return get(); }
	void reset() { data = ZeroInitializer<T>(); }
	T    get() const { return data; }
	void set(const T& val) { data = val; }
	// debugging only
	std::vector<T> getPerThreadData() const
	{
		std::vector<T> ret;
		ret.push_back(data);
		return ret;
	}
};

template <typename T> using OpenMPVector = std::vector<T>;
#endif

}; // namespace yade

// boost serialization
BOOST_SERIALIZATION_SPLIT_FREE(yade::OpenMPAccumulator<int>);
BOOST_SERIALIZATION_SPLIT_FREE(yade::OpenMPAccumulator<::yade::Real>);
BOOST_SERIALIZATION_SPLIT_FREE(yade::OpenMPArrayAccumulator<::yade::Real>);

namespace boost {
namespace serialization {
	template <class Archive> void save(Archive& ar, const yade::OpenMPAccumulator<int>& a, unsigned int /*version*/)
	{
		int value = a.get();
		ar& BOOST_SERIALIZATION_NVP(value);
	}
	template <class Archive> void load(Archive& ar, yade::OpenMPAccumulator<int>& a, unsigned int /*version*/)
	{
		int value;
		ar& BOOST_SERIALIZATION_NVP(value);
		a.set(value);
	}
	template <class Archive> void save(Archive& ar, const yade::OpenMPAccumulator<::yade::Real>& a, unsigned int /*version*/)
	{
		::yade::Real value = a.get();
		ar&          BOOST_SERIALIZATION_NVP(value);
	}
	template <class Archive> void load(Archive& ar, yade::OpenMPAccumulator<::yade::Real>& a, unsigned int /*version*/)
	{
		::yade::Real value;
		ar&          BOOST_SERIALIZATION_NVP(value);
		a.set(value);
	}
	template <class Archive> void save(Archive& ar, const yade::OpenMPArrayAccumulator<::yade::Real>& a, unsigned int /*version*/)
	{
		size_t size = a.size();
		ar&    BOOST_SERIALIZATION_NVP(size);
		for (size_t i = 0; i < size; i++) {
			::yade::Real item(a.get(i));
			ar&          boost::serialization::make_nvp(("item" + boost::lexical_cast<std::string>(i)).c_str(), item);
		}
	}
	template <class Archive> void load(Archive& ar, yade::OpenMPArrayAccumulator<::yade::Real>& a, unsigned int /*version*/)
	{
		size_t size;
		ar&    BOOST_SERIALIZATION_NVP(size);
		a.resize(size);
		for (size_t i = 0; i < size; i++) {
			::yade::Real item;
			ar&          boost::serialization::make_nvp(("item" + boost::lexical_cast<std::string>(i)).c_str(), item);
			a.set(i, item);
		}
	}

}
}
