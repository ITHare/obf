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
#include <map>//for dbg_map only

#ifdef ITHARE_OBF_INTERNAL_DBG // set of settings currently used for internal testing. DON'T rely on it!
//#define ITHARE_OBF_ENABLE_DBGPRINT
//#if 0

//#define ITHARE_OBF_CRYPTO_PRNG

#define ITHARE_OBF_SEED 0x0c7dfa61a871b133
#define ITHARE_OBF_SEED2 0xdacb5ca59a237d13 

#define ITHARE_OBF_CONSISTENT_DEBUG_RELEASE
//#define ITHARE_OBF_DBG_MAP
//#define ITHARE_OBF_DBG_MAP_LOG

#define ITHARE_OBF_INIT 
//enables rather nasty obfuscations (including PEB-based debugger detection),
//  but requires you to call obf_init() BEFORE ANY obf<> objects are used. 
//  As a result - it can backfire for obfuscations-used-from-global-constructors :-(.

//#define ITHARE_OBF_NO_ANTI_DEBUG
//disables built-in anti-debug kinda-protections in a clean way
#define ITHARE_OBF_DEBUG_ANTI_DEBUG_ALWAYS_FALSE
//makes built-in anti-debugger kinda-protections to return 'not being debugged' (NOT clean, use ONLY for debugging purposes)

//THE FOLLOWING MUST BE NOT USED FOR PRODUCTION BUILDS:
#define ITHARE_OBF_ENABLE_DBGPRINT
//enables dbgPrint()
#endif//ITHARE_OBF_INTERNAL_DBG

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
