/*

Copyright (c) 2018, ITHare.com
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

//Usage: 
//  1. Use OBF?() throughout your code to indicate the need for obfuscation
//  1a. OBFX() roughly means 'add no more than 10^(X/2) CPU cycles for obfuscation of this variable'
//      i.e. OBF2() adds up to 10 CPU cycles, OBF3() - up to 30 CPU cycles, 
//           and OBF5() - up to 300 CPU cycles
//  1b. To obfuscate literals, use OBF?I() (for integral literals) and OBF?S() (for string literals)
//  1c. See ../test/official.cpp for examples
//  2. compile your code without -DITHARE_OBF_SEED for debugging and during development
//  3. compile with -DITHARE_OBF_SEED=0x<really-random-64-bit-seed> for deployments

// LIST OF supported defines (expected to be specified via -D command-line option):
// MAIN SWITCH:
//   ITHARE_OBF_SEED=0x<some-random-64-bit-number>
//     if not specified - no obfuscation happens
// TODO: add ITHARE_OBF_DISABLED (to work around some compiler's performance hit even when no ITHARE_OBF_SEED is specified)  
//
// COMMON DEFINES:
//   ITHARE_OBF_SEED2=0x<some-random-64-bit-number>
//	 ITHARE_OBF_NO_MINIMIZING_CONSTANTS (by default, we're "minimizing" number of constants used for obfuscation 
//                                       - to prevent creation of "signatures")
//   ITHARE_OBF_NO_ANTI_DEBUG
//   ITHARE_OBF_NO_IMPLICIT_ANTI_DEBUG (disables using anti debug in generated obfuscations, but still allows to read it)
//   ITHARE_OBF_NO_AUTO_INIT (disables automated call to obf_init() via constructor, so you can call it manually, 
//							  ensuring proper order of initialization. Wrong order of calls shouldn't crash the program, 
//							  but some anti-debug protections may be disabled before obf_init() is called)
//   ITHARE_OBF_NO_SHORT_DEFINES (define to avoid polluting macro name space with short OBFI*() etc. macros 
//								  - and use full ITHARE_OBF_INT*() etc. macros instead)
//
// DEBUG-ONLY; MUST NOT be used in production
//   ITHARE_OBF_COMPILE_TIME_TESTS
//   ITHARE_OBF_DBG_ENABLE_DBGPRINT
//   ITHARE_OBF_DBG_RUNTIME_CHECKS
//   ITHARE_OBF_DBG_ANTI_DEBUG_ALWAYS_FALSE (to disable anti-debug - use ITHARE_OBF_NO_ANTI_DEBUG or 

//ithare::obf naming conventions are the same as those of kscope, in particular: 
//  functions: 
//    obf_function()
//  classes:
//    ObfClass
//  primitive types:
//    OBFTYPE
//  macros:
//    ITHARE_OBF_MACRO

#ifndef ithare_obf_obf_h_included
#define ithare_obf_obf_h_included

//to reduce confusion for end-users ("which macro to use - *_OBF one or *_KSCOPE one") 
//  we'll  provide *_OBF macro counterparts for all the macros 

//re-mapping of input *_OBF macros to *_KSCOPE ones 
#ifdef ITHARE_OBF_SEED
#define ITHARE_KSCOPE_SEED ITHARE_OBF_SEED
#endif 
#ifdef ITHARE_OBF_SEED2
#define ITHARE_KSCOPE_SEED2 ITHARE_OBF_SEED2
#endif
#ifdef ITHARE_OBF_DBG_RUNTIME_CHECKS
#define ITHARE_KSCOPE_DBG_RUNTIME_CHECKS ITHARE_OBF_DBG_RUNTIME_CHECKS
#endif
#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
#define ITHARE_KSCOPE_DBG_ENABLE_DBGPRINT ITHARE_OBF_DBG_ENABLE_DBGPRINT
#endif
#ifdef ITHARE_OBF_ENABLE_AUTO_DBGPRINT
#define ITHARE_KSCOPE_ENABLE_AUTO_DBGPRINT ITHARE_OBF_ENABLE_AUTO_DBGPRINT
#endif
#ifdef ITHARE_OBF_COMPILE_TIME_TESTS
#define ITHARE_KSCOPE_COMPILE_TIME_TESTS ITHARE_OBF_COMPILE_TIME_TESTS
#endif

#include "kscope_extension_for_obf.h"
#include "../../kscope/src/kscope.h"

#define ITHARE_OBF_FORCEINLINE ITHARE_KSCOPE_FORCEINLINE
#define ITHARE_OBF_NOINLINE ITHARE_KSCOPE_NOINLINE

#define ITHARE_OBF_INT0 ITHARE_KSCOPE_INT0
#define ITHARE_OBF_INT1 ITHARE_KSCOPE_INT1
#define ITHARE_OBF_INT2 ITHARE_KSCOPE_INT2
#define ITHARE_OBF_INT3 ITHARE_KSCOPE_INT3
#define ITHARE_OBF_INT4 ITHARE_KSCOPE_INT4
#define ITHARE_OBF_INT5 ITHARE_KSCOPE_INT5
#define ITHARE_OBF_INT6 ITHARE_KSCOPE_INT6

#define ITHARE_OBF_INTLIT0 ITHARE_KSCOPE_INTLIT0
#define ITHARE_OBF_INTLIT1 ITHARE_KSCOPE_INTLIT1
#define ITHARE_OBF_INTLIT2 ITHARE_KSCOPE_INTLIT2
#define ITHARE_OBF_INTLIT3 ITHARE_KSCOPE_INTLIT3
#define ITHARE_OBF_INTLIT4 ITHARE_KSCOPE_INTLIT4
#define ITHARE_OBF_INTLIT5 ITHARE_KSCOPE_INTLIT5
#define ITHARE_OBF_INTLIT6 ITHARE_KSCOPE_INTLIT6

//INTLIT?I: INTLIT?I casted to integer (to use literals in function calls)
/*
#define ITHARE_OBF_INTLIT0I ITHARE_KSCOPE_INTLIT0I
#define ITHARE_OBF_INTLIT1I ITHARE_KSCOPE_INTLIT1I
#define ITHARE_OBF_INTLIT2I ITHARE_KSCOPE_INTLIT2I
#define ITHARE_OBF_INTLIT3I ITHARE_KSCOPE_INTLIT3I
#define ITHARE_OBF_INTLIT4I ITHARE_KSCOPE_INTLIT4I
#define ITHARE_OBF_INTLIT5I ITHARE_KSCOPE_INTLIT5I
#define ITHARE_OBF_INTLIT6I ITHARE_KSCOPE_INTLIT6I
*/

#define ITHARE_OBF_STRLIT0 ITHARE_KSCOPE_STRLIT0
#define ITHARE_OBF_STRLIT1 ITHARE_KSCOPE_STRLIT1
#define ITHARE_OBF_STRLIT2 ITHARE_KSCOPE_STRLIT2
#define ITHARE_OBF_STRLIT3 ITHARE_KSCOPE_STRLIT3
#define ITHARE_OBF_STRLIT4 ITHARE_KSCOPE_STRLIT4
#define ITHARE_OBF_STRLIT5 ITHARE_KSCOPE_STRLIT5
#define ITHARE_OBF_STRLIT6 ITHARE_KSCOPE_STRLIT6

//#define ITHARE_OBF_INT_CONSTEXPR ITHARE_KSCOPE_INT_CONSTEXPR
#define ITHARE_OBF_INTNULLPTR ITHARE_KSCOPE_INTNULLPTR 

#define ITHARE_OBF_CALL0 ITHARE_KSCOPE_CALL0
#define ITHARE_OBF_CALL1 ITHARE_KSCOPE_CALL1
#define ITHARE_OBF_CALL2 ITHARE_KSCOPE_CALL2
#define ITHARE_OBF_CALL3 ITHARE_KSCOPE_CALL3
#define ITHARE_OBF_CALL4 ITHARE_KSCOPE_CALL4
#define ITHARE_OBF_CALL5 ITHARE_KSCOPE_CALL5
#define ITHARE_OBF_CALL6 ITHARE_KSCOPE_CALL6
#define ITHARE_OBF_CALL_AS_CONSTEXPR ITHARE_KSCOPE_CALL_AS_CONSTEXPR

#define ITHARE_OBF_VALUE ITHARE_KSCOPE_VALUE
#define ITHARE_OBF_ARRAY_OF_SAME_TYPE_AS ITHARE_KSCOPE_ARRAY_OF_SAME_TYPE_AS
#define ITHARE_OBF_PTR_OF_SAME_TYPE_AS ITHARE_KSCOPE_PTR_OF_SAME_TYPE_AS

#define ITHARE_OBF_DBGPRINT ITHARE_KSCOPE_DBGPRINT

#ifndef ITHARE_OBF_NO_SHORT_DEFINES

#define OBFI0 ITHARE_OBF_INT0
#define OBFI1 ITHARE_OBF_INT1
#define OBFI2 ITHARE_OBF_INT2
#define OBFI3 ITHARE_OBF_INT3
#define OBFI4 ITHARE_OBF_INT4
#define OBFI5 ITHARE_OBF_INT5
#define OBFI6 ITHARE_OBF_INT6

#define OBFI0L ITHARE_OBF_INTLIT0
#define OBFI1L ITHARE_OBF_INTLIT1
#define OBFI2L ITHARE_OBF_INTLIT2
#define OBFI3L ITHARE_OBF_INTLIT3
#define OBFI4L ITHARE_OBF_INTLIT4
#define OBFI5L ITHARE_OBF_INTLIT5
#define OBFI6L ITHARE_OBF_INTLIT6

/*
#define OBFI0LI ITHARE_OBF_INTLIT0I
#define OBFI1LI ITHARE_OBF_INTLIT1I
#define OBFI2LI ITHARE_OBF_INTLIT2I
#define OBFI3LI ITHARE_OBF_INTLIT3I
#define OBFI4LI ITHARE_OBF_INTLIT4I
#define OBFI5LI ITHARE_OBF_INTLIT5I
#define OBFI6LI ITHARE_OBF_INTLIT6I
*/

#define OBFS0L ITHARE_OBF_STRLIT0
#define OBFS1L ITHARE_OBF_STRLIT1
#define OBFS2L ITHARE_OBF_STRLIT2
#define OBFS3L ITHARE_OBF_STRLIT3
#define OBFS4L ITHARE_OBF_STRLIT4
#define OBFS5L ITHARE_OBF_STRLIT5
#define OBFS6L ITHARE_OBF_STRLIT6

//#define OBFICE ITHARE_OBF_INT_CONSTEXPR
#define OBFINULLPTR ITHARE_OBF_INTNULLPTR

#define OBFCALL6 ITHARE_OBF_CALL6
#define OBFCALLCE ITHARE_OBF_CALL_AS_CONSTEXPR

#define OBFVAL ITHARE_OBF_VALUE
#define OBFSTARR ITHARE_OBF_ARRAY_OF_SAME_TYPE_AS
#define OBFSTPTR ITHARE_KSCOPE_PTR_OF_SAME_TYPE_AS

#endif //ITHARE_OBF_NO_SHORT_DEFINES

#endif//ithare_obf_obf_h_included
