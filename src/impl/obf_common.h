#ifndef ithare_obf_common_h_included
#define ithare_obf_common_h_included

//NOT intended to be #included directly
//  #include ../obf.h instead

#include <stdint.h>
#include <inttypes.h>
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
		void obf_copyarray(T(&to)[N], const T from[]) {
			for(size_t i=0; i < N; ++i ) {
				to[i] = from[i];
			}
		}

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
		
		constexpr OBFFLAGS obf_flag_cross_platform_only = 0x02; 
		
		using OBFINJECTIONCAPS = uint64_t;//injection capability flags
		constexpr OBFINJECTIONCAPS obf_injection_has_add_mod_max_value_ex = 0x01;
		

	}//namespace obf
}//namespace ithare

#endif //ITHARE_OBF_SEED

#endif //ithare_obf_common_h_included
