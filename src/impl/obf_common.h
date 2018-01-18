#ifndef ithare_obf_common_h_included
#define ithare_obf_common_h_included

//NOT intended to be #included directly
//  #include ../obf.h instead

#include <stdint.h>
#include <inttypes.h>
#include <array>
#include <assert.h>
#include <type_traits>
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

#endif//ithare_obf_common_h_included
