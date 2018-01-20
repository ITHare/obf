#ifndef ithare_obf_anti_debug_h_included
#define ithare_obf_anti_debug_h_included

//NOT intended to be #included directly
//  #include ../obf.h instead

#if defined(ITHARE_OBF_SEED) && !defined(ITHARE_OBF_NO_ANTI_DEBUG)

#include "obf_common.h"
#include "obf_prng.h"

#if defined(_MSC_VER) && ( defined(_M_IX86) || defined(_M_X64))  	
#define ITHARE_OBF_TIME_TYPE uint64_t
#define ITHARE_OBF_TIME_NOW() __rdtsc()
#define ITHARE_OBF_TIME_NON_BLOCKING_THRESHOLD (UINT64_C(4'000'000'000)*60) //4GHz * 60 seconds is a Damn Lot(tm); if frequency is lower - it is even safer
#elif defined(__clang__) && (defined(__x86_64__)||defined(__i386__))
#include <x86intrin.h>
#define ITHARE_OBF_TIME_TYPE uint64_t
#define ITHARE_OBF_TIME_NOW() __rdtsc()
#define ITHARE_OBF_TIME_NON_BLOCKING_THRESHOLD (UINT64_C(4'000'000'000)*60) //4GHz * 60 seconds is a Damn Lot(tm)
#else
#warning "Time-based protection is not supported yet for this platform (executable will work, but without time-based protection)"
#define ITHARE_OBF_TIME_TYPE unsigned //we don't really need it
#define ITHARE_OBF_TIME_NOW() 0
#define ITHARE_OBF_TIME_NON_BLOCKING_THRESHOLD 1
#endif

namespace ithare {
	namespace obf {

//using template to move static data to header...
template<class Dummy>
struct ObfNonBlockingCodeStaticData {
	static std::atomic<uint64_t> violation_count;
	
	template<ITHARE_OBF_SEEDTPARAM seed2>
	ITHARE_OBF_FORCEINLINE static uint64_t zero_if_not_being_debugged() {
		return violation_count.load();
	} 
};
template<class Dummy>
std::atomic<uint64_t> ObfNonBlockingCodeStaticData<Dummy>::violation_count = 0;

constexpr int obf_bit_upper_bound(uint64_t x) {
	uint64_t test = 1;
	for(int i=0; i < 64; ++i, test <<= 1 ) {
		if( test > x )
			return i;
	}
	assert(false);
	return 63;
}

class ObfNonBlockingCode {//to be used ONLY on-stack
	ITHARE_OBF_TIME_TYPE started;

	public:
	ITHARE_OBF_FORCEINLINE ObfNonBlockingCode() {
		started = ITHARE_OBF_TIME_NOW();
	}
	ITHARE_OBF_FORCEINLINE ~ObfNonBlockingCode() {
		ITHARE_OBF_TIME_TYPE delta = ITHARE_OBF_TIME_NOW() - started;
		constexpr int threshold_bits = obf_bit_upper_bound(ITHARE_OBF_TIME_NON_BLOCKING_THRESHOLD);
		delta >>= threshold_bits;//expected to be zero at this point
		ObfNonBlockingCodeStaticData<void>::violation_count += delta;
	}
	
	//trying to prevent accidental non-stack uses; not bulletproof, but better than nothing
	ObfNonBlockingCode(const ObfNonBlockingCode&) = delete;
	ObfNonBlockingCode& operator =(const ObfNonBlockingCode&) = delete;
	ObfNonBlockingCode(const ObfNonBlockingCode&&) = delete;
	ObfNonBlockingCode& operator =(const ObfNonBlockingCode&&) = delete;
	static void* operator new(size_t) = delete;
	static void* operator new[](size_t) = delete;
};

  }//namespace obf
}//namespace ithare

#undef ITHARE_OBF_TIME_TYPE
#undef ITHARE_OBF_TIME_NOW
#undef ITHARE_OBF_TIME_NON_BLOCKING_THRESHOLD

#else //ITHARE_OBF_SEED && !ITHARE_OBF_NO_ANTI_DEBUG
namespace ithare {
	namespace obf {

class ObfNonBlockingCode {//to be used ONLY on-stack
	public:
	ObfNonBlockingCode() {
	}
	~ObfNonBlockingCode() {
	}
	
	//trying to prevent accidental non-stack uses; not bulletproof, but better than nothing
	ObfNonBlockingCode(const ObfNonBlockingCode&) = delete;
	ObfNonBlockingCode& operator =(const ObfNonBlockingCode&) = delete;
	ObfNonBlockingCode(const ObfNonBlockingCode&&) = delete;
	ObfNonBlockingCode& operator =(const ObfNonBlockingCode&&) = delete;
	static void* operator new(size_t) = delete;
	static void* operator new[](size_t) = delete;
};

  }//namespace obf
}//namespace ithare

#endif //ITHARE_OBF_SEED && !ITHARE_OBF_NO_ANTI_DEBUG

#endif //ithare_obf_anti_debug_h_included
