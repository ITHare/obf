#ifndef ithare_obf_common_h_included
#define ithare_obf_common_h_included

//NOT intended to be #included directly
//  #include ../obf.h instead

#include <stdint.h>
#include <inttypes.h>
#include <limits>
#include <array>
#include <assert.h>
#include <type_traits>
#include <atomic>
#include <string>//for dbgPrint() only
#include <iostream>//for dbgPrint() only
//---#include <map>//for dbg_map only

#ifdef ITHARE_OBF_DBG_RUNTIME_CHECKS
#define ITHARE_OBF_DBG_ENABLE_DBGPRINT//necessary for checks to work
#endif

#ifndef ITHARE_OBF_SEED2
#define ITHARE_OBF_SEED2 0
#endif

#ifndef ITHARE_OBF_COMPILE_TIME_TESTS
#define ITHARE_OBF_COMPILE_TIME_TESTS 100
#endif

#ifndef ITHARE_OBF_SCALE//#define for libraries, using OBFN() macros; 
				//  allows to promote/demote obfuscation scale of the whole library by ITHARE_OBF_SCALE
				//  ITHARE_OBF_SCALE = 1 'converts' all OBF0()'s into OBF1()'s, and so on...
				//  DOES NOT affect obfN<> without macros(!)
#define ITHARE_OBF_SCALE 0
#endif

#ifdef _MSC_VER
#pragma warning (disable:4307)

#define ITHARE_OBF_FORCEINLINE __forceinline
#define ITHARE_OBF_NOINLINE __declspec(noinline)

#elif defined(__clang__)

#define ITHARE_OBF_FORCEINLINE __attribute__((always_inline)) inline
#define ITHARE_OBF_NOINLINE __attribute__((noinline))

#else
#error Other compilers than MSVC and Clang are not supported (feel free to try adding GCC though)
#endif//_MSC_VER || __clang__

//regardless of ITHARE_OBF_SEED
namespace ithare {
	namespace obf {		
		template<class T,size_t N>
		constexpr size_t obf_arraysz(T(&)[N]) { return N; }

		template<class T,size_t N>
		constexpr void obf_copyarray(T(&to)[N], const T from[]) {
			for(size_t i=0; i < N; ++i ) {
				to[i] = from[i];
			}
		}
		template<class T,size_t N>
		constexpr void obf_zeroarray(T(&to)[N]) {
			for(size_t i=0; i < N; ++i ) {
				to[i] = 0;
			}
		}
		template<class T,size_t N>
		struct ObfArrayWrapper {
			T arr[N];
		};

		constexpr size_t obf_strlen(const char* s) {
			for (size_t ret = 0; ; ++ret, ++s)
				if (*s == 0)
					return ret;
		}
		
		enum class obf_endian//along the lines of p0463r1, to be replaced with std::endian
		{
#ifdef _MSC_VER
#if defined(_M_IX86) || defined(_M_X64)
			little = 0x22d7,//from random.org :-), to prevent relying on specific values
			big    = 0xe72d,//from random.org
			native = little
//x86/x64
#else
#error "Endianness not defined yet"
#endif 

//_MSC_VER
#else
			little = __ORDER_LITTLE_ENDIAN__,
			big    = __ORDER_BIG_ENDIAN__,
			native = __BYTE_ORDER__,
#endif
		};

		using OBFFLAGS = uint64_t;
		constexpr OBFFLAGS obf_flag_is_constexpr = 0x01;
		
		struct obf_private_constructor_tag {
		};
		
		//type helpers
		template<bool which, class T, class T2> struct obf_select_type;
		template<class T, class T2>
		struct obf_select_type<true, T, T2> {
			using type = T;
		};
		template<class T, class T2>
		struct obf_select_type<false, T, T2> {
			using type = T2;
		};

		template<class T, class T2> struct obf_larger_type {
			static_assert(std::is_integral<T>::value);
			static_assert(std::is_integral<T2>::value);
			constexpr static bool ts = std::is_signed<T>::value;
			constexpr static bool t2s = std::is_signed<T2>::value;
			static_assert(ts == t2s);//'larger_type' is undefined for different sign-ness
			constexpr static bool which = sizeof(T) > sizeof(T2);
			using type = typename obf_select_type<which, T, T2>::type;

			static_assert(sizeof(type) >= sizeof(T));
			static_assert(sizeof(type) >= sizeof(T2));
		};

		static_assert(sizeof(int)==sizeof(unsigned));
		template<class T>
		struct obf_integral_promotion {//see issue #2 for description of the approximation of C++ promotion rules
			static_assert(std::is_integral<T>::value);
			using type = typename obf_select_type< (sizeof(T) < sizeof(int)), int, T >::type;

			static_assert(sizeof(type) >= sizeof(T));
			static_assert(sizeof(type) >= sizeof(int));
		};

		template<class T, class T2>
		struct obf_integral_operator_promoconv {//see issue #2 for description of the approximation of C++ promotion/operator conversion rules
			using TPROMOTED = typename obf_integral_promotion<T>::type;
			using T2PROMOTED = typename obf_integral_promotion<T2>::type;
			static_assert(sizeof(TPROMOTED) >= sizeof(T));
			static_assert(sizeof(T2PROMOTED) >= sizeof(T2));
			static constexpr bool ts = std::is_signed<TPROMOTED>::value;
			static constexpr bool t2s = std::is_signed<T2PROMOTED>::value;
			
			using type = typename obf_select_type< ts == t2s, 
										  typename obf_select_type< (sizeof(TPROMOTED) >= sizeof(T2PROMOTED)), TPROMOTED,T2PROMOTED>::type,
										  typename obf_select_type< ts ,
														   typename obf_select_type< (sizeof(TPROMOTED) > sizeof(T2PROMOTED)), TPROMOTED, T2PROMOTED >::type,
														   typename obf_select_type< (sizeof(T2PROMOTED) > sizeof(TPROMOTED)), T2PROMOTED, TPROMOTED >::type
														 >::type
										>::type;
			
			static_assert(sizeof(type) >= sizeof(TPROMOTED));
			static_assert(sizeof(type) >= sizeof(T2PROMOTED));
		};
		
		class ObfSint128 {//larger than the largest
			int64_t hi;
			uint64_t lo;
			
			public:
			constexpr ObfSint128(uint64_t x) : hi(0), lo(x) {
			}
			constexpr ObfSint128(int64_t x) : hi(x<0?int64_t(-1):0), lo(x) {
			}
			constexpr ObfSint128(uint32_t x) : ObfSint128(uint64_t(x)) {
			}
			constexpr ObfSint128(int32_t x) : ObfSint128(int64_t(x)) {
			}
			constexpr ObfSint128(uint16_t x) : ObfSint128(uint64_t(x)) {
			}
			constexpr ObfSint128(int16_t x) : ObfSint128(int64_t(x)) {
			}
			constexpr ObfSint128(uint8_t x) : ObfSint128(uint64_t(x)) {
			}
			constexpr ObfSint128(int8_t x) : ObfSint128(int64_t(x)) {
			}
			
			constexpr bool operator <=(const ObfSint128& other) const {
				return cmp(other) <= 0;
			}
			constexpr bool operator >=(const ObfSint128& other) const {
				return cmp(other) >= 0;
			}
			
			private:
			constexpr int cmp(const ObfSint128& other) const {
				if( hi < other.hi )
					return -1;
				if( hi > other.hi )
					return 1;
					
				int ret = 0;
				if( lo < other.lo )
					ret = -1;
				if( lo > other.lo )
					ret = 1;
				return hi >= 0 ? ret : -ret;
			}
		};

		template<class T, class TC, TC C>
		constexpr bool obf_integral_operator_literal_cast_is_safe() {
			return ObfSint128(C) <= ObfSint128(std::numeric_limits<T>::max()) && 
					   ObfSint128(C) >= ObfSint128(std::numeric_limits<T>::min());
		}
		
	}//namespace obf
}//namespace ithare

#ifdef ITHARE_OBF_SEED

namespace ithare {
	namespace obf {
		//NAMESPACE POLICIES:
		//  EVERYTHING goes into ithare::obf namespace - except for #defines
		//  ALL #defines are prefixed with ITHARE_OBF_
		//    THE ONLY exception is OBF0()...OBF6() macros
		//      These can be disabled using ITHARE_OBF_NO_SHORT_DEFINES macro 
		//        If disabling - use equivalent ITHARE_OBF0()...ITHARE_OBF6()

		using OBFCYCLES = int32_t;//signed!
		using OBFLEVEL = int8_t;//signed! 

		constexpr OBFLEVEL obf_addlevel(OBFLEVEL base, OBFLEVEL diff) {
			return base < 0 ? base : base + diff;
		}
		
		constexpr OBFFLAGS obf_flag_cross_platform_only = 0x02; 
		
		using OBFINJECTIONCAPS = uint64_t;//injection capability flags
		constexpr OBFINJECTIONCAPS obf_injection_has_add_mod_max_value_ex = 0x01;
		

	}//namespace obf
}//namespace ithare

#endif //ITHARE_OBF_SEED

#endif //ithare_obf_common_h_included
