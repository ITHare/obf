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

#ifndef ithare_obf_kscope_extension_for_obf_h_included
#define ithare_obf_kscope_extension_for_obf_h_included

#ifdef ithare_kscope_kscope_h_included
#error IF using kscope extension, it MUST be included BEFORE kscope.h. See test/officialtest.cpp for usage example
#endif

#include <atomic>
#include "../../kscope/src/impl/kscope_injection.h"
#include "../../kscope/src/impl/kscope_literal.h"
#include "impl/obf_anti_debug.h"

#ifdef ITHARE_KSCOPE_SEED

namespace ithare { namespace kscope {//cannot really move it to ithare::obf due to *Version specializations

	constexpr size_t obf_cache_line_size = 64;//stands at least for x86/x64, and most of ARMs. 
											  //  For those platforms which have different cache line size, feel free to use #ifdefs 
											  //  to specify correct value 
	
	//const-related stuff
	template<class T, size_t N, class T2>
	constexpr size_t obf_find_idx(T (&arr)[N], T2 value) {
		for (size_t i = 0; i < N; ++i) {
			if (arr[i] == value)
				return i;
		}
		return size_t(-1);
	}
	
	template<ITHARE_KSCOPE_SEEDTPARAM seed, class T,size_t N>
	constexpr T obf_const_x(T (&excluded)[N]) {
		T candidates[6] = { 3,5,7,15,25,31 };//only odd, to allow using the same set of constants when kscope_const_odd_only flag is set
		size_t weights[6] = { 100,100,100,100,100,100 };
		for (size_t i = 0; i < N; ++i) {
			size_t found = obf_find_idx(candidates, excluded[i]);
			if (found != size_t(-1))
				weights[found] = 0;
		}
		size_t idx2 = kscope_random_from_list<seed>(weights);
		return candidates[idx2];
	}

	//alls UINT32_C constants used to generate obf_const_*, are from random.org
	constexpr uint8_t obf_const_A_excluded[] = {uint8_t(-1)};
	constexpr uint8_t OBF_CONST_A = obf_const_x<ITHARE_KSCOPE_INIT_PRNG("", UINT32_C(0x30af'dc75), UINT32_C(0x4914'086a))>(obf_const_A_excluded);
	constexpr uint8_t obf_const_B_excluded[1] = { OBF_CONST_A };
	constexpr uint8_t OBF_CONST_B = obf_const_x<ITHARE_KSCOPE_INIT_PRNG("", UINT32_C(0x2f4a'62ef), UINT32_C(0x830c'edae))>(obf_const_B_excluded);
	constexpr uint8_t obf_const_C_excluded[2] = { OBF_CONST_A, OBF_CONST_B };
	constexpr uint8_t OBF_CONST_C = obf_const_x<ITHARE_KSCOPE_INIT_PRNG("", UINT32_C(0x166a'a35e), UINT32_C(0x01f9'3034))>(obf_const_C_excluded);
	
	template<class T,ITHARE_KSCOPE_SEEDTPARAM seed,KSCOPECONSTFLAGS flags>
	constexpr static T obf_random_const(T upper_bound=0) {//has the same signature, BUT potentially different semantics compared to kscope_random_const
#ifdef ITHARE_OBF_NO_MINIMIZING_CONSTANTS
		return kscope_random_const<T,seed,flags>(upper_bound);
#else
		//using TT = typename KscopeTraits<T>::construct_from_type;
		uint8_t candidates[5] = {};
		size_t n = 0;
		if(!upper_bound || OBF_CONST_A < upper_bound)
			candidates[n++] = OBF_CONST_A;
		if(!upper_bound || OBF_CONST_B < upper_bound)
			candidates[n++] = OBF_CONST_B;
		if(!upper_bound || OBF_CONST_C < upper_bound)
			candidates[n++] = OBF_CONST_C;
		if(flags&kscope_const_zero_ok) {
			assert( !(flags&kscope_const_odd_only) );
			candidates[n++] = 0;
		}
		if(flags&kscope_const_one_ok)
			candidates[n++] = 1;

		assert(n>0);
		size_t idx = ITHARE_KSCOPE_RANDOM(seed,1,n);
		assert(idx < n);
		return candidates[idx];
#endif
	}
	
	//extended Injections

	//version last+1: injection over lower half /*CHEAP!*/
	template<class T, class Context>
	struct ObfInjectionAdditionalVersion1Descr {
		static constexpr KSCOPECYCLES own_min_injection_cycles = 3;
		static constexpr KSCOPECYCLES own_min_surjection_cycles = 3;
		static constexpr KSCOPECYCLES own_min_cycles = KscopeSimpleInjectionHelper<Context>::descriptor_own_min_cycles(own_min_injection_cycles,own_min_surjection_cycles);
		static constexpr KscopeDescriptor descr =
			KscopeTraits<T>::has_half_type ?
			KscopeDescriptor(own_min_cycles, 100/*it's cheap, but doesn't obfuscate the whole thing well => let's use it mostly for lower-cycle stuff*/) :
			KscopeDescriptor(nullptr);
	};

	template <class T, class Context, class InjectionRequirements, ITHARE_KSCOPE_SEEDTPARAM seed, KSCOPECYCLES cycles>
	class KscopeInjectionVersion<ITHARE_KSCOPE_LAST_STOCK_INJECTION+1, T, Context, InjectionRequirements, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static constexpr KSCOPECYCLES availCycles = cycles - ObfInjectionAdditionalVersion1Descr<T,Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		struct RecursiveInjectionRequirements : public InjectionRequirements {
			static constexpr size_t exclude_version = ITHARE_KSCOPE_LAST_STOCK_INJECTION+1;
		};
		using halfT = typename KscopeTraits<T>::HalfT;

		constexpr static size_t split[] = { 200 /*RecursiveInjection*/, 100 /*LoInjection*/ };
		static constexpr auto splitCycles = kscope_random_split<ITHARE_KSCOPE_NEW_PRNG(seed, 1)>(availCycles, split);

		static constexpr KSCOPECYCLES cycles_rInj = splitCycles.arr[0];
		static constexpr KSCOPECYCLES cycles_lo = splitCycles.arr[1];
		static_assert(cycles_rInj + cycles_lo <= availCycles);

		using RecursiveInjection = KscopeInjection<T, Context, RecursiveInjectionRequirements,ITHARE_KSCOPE_NEW_PRNG(seed, 2), cycles_rInj + Context::context_cycles>;
		using return_type = typename RecursiveInjection::return_type;

	public:
		struct LoInjectionRequirements : public InjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool only_bijections = true;
		};

		constexpr static size_t splitLo[] = { 100 /*Context*/, 100 /*Injection*/ };
		static constexpr auto splitCyclesLo = kscope_random_split<ITHARE_KSCOPE_NEW_PRNG(seed, 3)>(cycles_lo, splitLo);
		static constexpr KSCOPECYCLES cycles_loCtx = splitCyclesLo.arr[0];
		static constexpr KSCOPECYCLES cycles_loInj = splitCyclesLo.arr[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using LoContext = typename KscopeRecursiveContext < halfT, Context, ITHARE_KSCOPE_NEW_PRNG(seed, 4), cycles_loCtx>::intermediate_context_type;
		using LoInjection = KscopeInjection<halfT, LoContext,  LoInjectionRequirements,ITHARE_KSCOPE_NEW_PRNG(seed, 5), cycles_loInj + LoContext::context_cycles>;
		static_assert(sizeof(typename LoInjection::return_type) == sizeof(halfT));//only_bijections

		template<ITHARE_KSCOPE_SEEDTPARAM seedc,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE constexpr static T local_injection(T x) {
			halfT lo0 = halfT(x);
			typename LoInjection::return_type lo1 = LoInjection::template injection<ITHARE_KSCOPE_NEW_PRNG(seedc, 1),flags>(lo0);
			//halfT lo = *reinterpret_cast<halfT*>(&lo1);//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			halfT lo = halfT(lo1);
			T y = x - T(lo0) + lo;
			return y;
		}
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE constexpr static return_type injection(T x) {
			ITHARE_KSCOPE_DECLAREPRNG_INFUNC seedc = ITHARE_KSCOPE_COMBINED_PRNG(seed,seed2);
			T y = local_injection<seedc,flags>(x);
			ITHARE_KSCOPE_DBG_ASSERT_SURJECTION_LOCAL("<ITHARE_KSCOPE_LAST_STOCK_INJECTION+1>", x, y);
			return_type ret = RecursiveInjection::template injection<ITHARE_KSCOPE_NEW_PRNG(seedc,2),flags>(y);
			//ITHARE_KSCOPE_DBG_ASSERT_SURJECTION_RECURSIVE("<ITHARE_KSCOPE_LAST_STOCK_INJECTION+1>", x, ret);
			return ret;
		}

		template<ITHARE_KSCOPE_SEEDTPARAM seedc,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE constexpr static T local_surjection(T y) {
			halfT lo0 = halfT(y);
			halfT lo = LoInjection::template surjection<ITHARE_KSCOPE_NEW_PRNG(seedc,4),flags>(/* *reinterpret_cast<typename LoInjection::return_type*>(&lo0)*/ typename LoInjection::return_type(lo0));//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return y - T(lo0) + lo;
		}
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE constexpr static T surjection(return_type yy) {
			ITHARE_KSCOPE_DECLAREPRNG_INFUNC seedc = ITHARE_KSCOPE_COMBINED_PRNG(seed, seed2);
			T y = RecursiveInjection::template surjection<ITHARE_KSCOPE_NEW_PRNG(seedc, 3),flags>(yy);
			return local_surjection<seedc,flags>(y);
		}

		static constexpr KSCOPEINJECTIONCAPS injection_caps = 0;

#ifdef ITHARE_KSCOPE_DBG_ENABLE_DBGPRINT
		static void dbg_print(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "KscopeInjectionVersion<ITHARE_KSCOPE_LAST_STOCK_INJECTION+1="<< (ITHARE_KSCOPE_LAST_STOCK_INJECTION+1) <<"/*injection(halfT)*/," << kscope_dbg_print_t<T>() << "," << kscope_dbg_print_seed<seed>() << "," << cycles << ">" << std::endl;
			LoInjection::dbg_print(offset + 1, "Lo:");
			RecursiveInjection::dbg_print(offset + 1, "Recursive:");
		}
#endif
	};

}} //namespace ithare::kscope
	
#define ITHARE_KSCOPE_ADDITIONAL_INJECTION_DESCRIPTOR_LIST \
	ObfInjectionAdditionalVersion1Descr<T,Context>::descr,	

namespace ithare { namespace kscope {//cannot really move it to ithare::obf due to *Version specializations

	//version last+1: aliased pointers
	struct ObfLiteralAdditionalVersion1Descr {
		static constexpr KscopeDescriptor descr = KscopeDescriptor(20, 100);
	};

	template<class T>//TODO: randomize contents of the function
	ITHARE_KSCOPE_NOINLINE T obf_aliased_zero(T* x, T* y) {
		*x = 0;
		*y = 1;
		return *x;
	}

	template<class T, ITHARE_KSCOPE_SEEDTPARAM seed>
	struct KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+1,T,seed> {
		using Traits = KscopeTraits<T>;
		constexpr static KSCOPECYCLES context_cycles = ObfLiteralAdditionalVersion1Descr::descr.min_cycles;

		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE static constexpr T final_surjection(T y) {
			if constexpr(flags&kscope_flag_is_constexpr)
				return y;
			else {
				T x, yy;
				T z = obf_aliased_zero(&x, &yy);
				return y - z;
			}
		}
#ifdef ITHARE_KSCOPE_DBG_ENABLE_DBGPRINT
		static void dbg_print(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "KscopeLiteralContext_version<ITHARE_KSCOPE_LAST_STOCK_LITERAL+1="<< (ITHARE_KSCOPE_LAST_STOCK_LITERAL+1) <<"/*func with aliased pointers*/," << kscope_dbg_print_t<T>() << "," << kscope_dbg_print_seed<seed>() << ">:" << std::endl;
		}
#endif
	};

	//version last+2: Naive System-Dependent Anti-Debugging
	struct ObfLiteralAdditionalVersion2Descr {//NB: to ensure 100%-compatible generation across platforms, probabilities MUST NOT depend on the platform, directly or indirectly
#if defined(ITHARE_OBF_INIT) && !defined(ITHARE_OBF_NO_ANTI_DEBUG) && !defined(ITHARE_OBF_NO_IMPLICIT_ANTI_DEBUG)
		static constexpr KscopeDescriptor descr = KscopeDescriptor(10, 100);
#else
		static constexpr KscopeDescriptor descr = KscopeDescriptor(nullptr);
#endif
	};

	template<class T, ITHARE_KSCOPE_SEEDTPARAM seed>
	struct KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+2,T,seed> {
		using Traits = KscopeTraits<T>;
		constexpr static KSCOPECYCLES context_cycles = ObfLiteralAdditionalVersion2Descr::descr.min_cycles;

		constexpr static T CC = obf_random_const<T,ITHARE_KSCOPE_NEW_PRNG(seed, 1),0>();
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE constexpr static T final_surjection(T y) {
			if constexpr(flags&kscope_flag_is_constexpr)
				return y - CC;
			else
				return y - CC * T(1 + ithare::obf::ObfNaiveSystemSpecific<void>::zero_if_not_being_debugged());
		}

#ifdef ITHARE_KSCOPE_DBG_ENABLE_DBGPRINT
		static void dbg_print(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+2="<< (ITHARE_KSCOPE_LAST_STOCK_LITERAL+2) <<"/*Naive System-Dependent Anti-Debug*/," << kscope_dbg_print_t<T>() << "," << kscope_dbg_print_seed<seed>() << ">: CC=" << kscope_dbg_print_c<T>(CC) << std::endl;
		}
#endif
	};
	
	//version last+3: Time-Based Anti-Debugging
	struct ObfLiteralAdditionalVersion3Descr {//NB: to ensure 100%-compatible generation across platforms, probabilities MUST NOT depend on the platform, directly or indirectly
#if defined(ITHARE_OBF_INIT) && !defined(ITHARE_OBF_NO_ANTI_DEBUG) && !defined(ITHARE_OBF_NO_IMPLICIT_ANTI_DEBUG)
		static constexpr KscopeDescriptor descr = KscopeDescriptor(15, 100);//may involve reading from std::atomic<>
#else
		static constexpr KscopeDescriptor descr = KscopeDescriptor(nullptr);
#endif
	};

	template<class T, ITHARE_KSCOPE_SEEDTPARAM seed>
	struct KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+3,T,seed> {
		using Traits = KscopeTraits<T>;
		constexpr static KSCOPECYCLES context_cycles = ObfLiteralAdditionalVersion3Descr::descr.min_cycles;

		constexpr static T CC = obf_random_const<T,ITHARE_KSCOPE_NEW_PRNG(seed, 1),0>();
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE constexpr static T final_surjection(T y) {
			if constexpr(flags&kscope_flag_is_constexpr)
				return y - CC;
			else
				return y - CC * T(1 + ithare::obf::ObfNonBlockingCodeStaticData<void>::zero_if_not_being_debugged());
		}

#ifdef ITHARE_KSCOPE_DBG_ENABLE_DBGPRINT
		static void dbg_print(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+3="<< (ITHARE_KSCOPE_LAST_STOCK_LITERAL+3) <<"/*ObfNonBlockingCode()*/," << kscope_dbg_print_t<T>() << "," << kscope_dbg_print_seed<seed>() << ">: CC=" << kscope_dbg_print_c<T>(CC) << std::endl;
		}
#endif
	};
	
	//version last+4: global var-with-invariant
	//using template to move static data to header...
	template<class Dummy>
	struct ObfGlobalVarUpdateTlsCounter {
		static thread_local uint32_t access_count;
	};
	template<class Dummy>
	thread_local uint32_t ObfGlobalVarUpdateTlsCounter<Dummy>::access_count = 0;

	template<class T>
	struct ObfLiteralAdditionalVersion4Descr {
		static constexpr KscopeDescriptor descr = 
			KscopeTraits<T>::is_built_in ? //MIGHT be lifted if we adjust maths
			KscopeDescriptor(15, 100)//'15 cycles' is an estimate for AMORTIZED time; see comments within final_surjection() function below
			: KscopeDescriptor(nullptr);
			//TODO: another literal version, moving the whole invariant to thread_local (tricky as we don't want to increase the number of thread_local vars too much)
	};

	template<class T, ITHARE_KSCOPE_SEEDTPARAM seed>
	struct KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+4, T, seed> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static KSCOPECYCLES context_cycles = ObfLiteralAdditionalVersion4Descr<T>::descr.min_cycles;

		static constexpr T PREMODRNDCONST = obf_random_const<T,ITHARE_KSCOPE_NEW_PRNG(seed, 2),0>();//TODO: check which constants we want here
		static constexpr T PREMODMASK = (T(1) << (sizeof(T) * 4)) - 1;
		static constexpr T PREMOD = PREMODRNDCONST & PREMODMASK;
		static constexpr T MOD = PREMOD == 0 ? 100 : PREMOD;//remapping 'bad value' 0 to 'something'
		static constexpr T CC = T(ITHARE_KSCOPE_RANDOM(seed, 2,MOD));

		static constexpr T MAXMUL1 = T(-1)/MOD;
		static constexpr uint64_t MAXMUL1_ADJUSTED0 = MAXMUL1;// obf_sqrt_very_rough_approximation(MAXMUL1); TODO
		static_assert(MAXMUL1_ADJUSTED0 < T(-1));
		static constexpr T MAXMUL1_ADJUSTED = (T)MAXMUL1_ADJUSTED0;
		static constexpr T MUL1 = MAXMUL1 > 2 ? 1+(ITHARE_KSCOPE_RANDOM_UINT32(seed, 2)%MAXMUL1_ADJUSTED) : 1;//TODO: check if uint32_t is enough
		static constexpr T DELTA = MUL1 * MOD;
		static_assert(DELTA / MUL1 == MOD);//overflow check

		static constexpr T MAXMUL2 = T(-1) / DELTA;
		static constexpr T MUL2 = MAXMUL2 > 2 ? 1+(ITHARE_KSCOPE_RANDOM_UINT32(seed, 3)% MAXMUL2) : 1;//TODO: check if uint32_t is enough
		static constexpr T DELTAMOD = MUL2 * MOD;
		static_assert(DELTAMOD / MUL2 == MOD);//overflow check

		static constexpr T PREMUL3 = ITHARE_KSCOPE_RANDOM_UINT32(seed, 4)% MUL2;//TODO: check if uint32_t is enough
		static constexpr T MUL3 = PREMUL3 > 0 ? PREMUL3 : 1;
		static constexpr T CC0 = ( CC + MUL3 * MOD ) % DELTAMOD;

		static_assert((CC0 + DELTA) % MOD == CC);
		static constexpr bool test_n_iterations(T x, int n) {
			assert(x%MOD == CC);
			if (n == 0)
				return true;
			T newC = (x + DELTA) % DELTAMOD;
			assert(newC%MOD == CC);
			return test_n_iterations(newC,n-1);
		}
		static_assert(test_n_iterations(CC0, ITHARE_KSCOPE_COMPILE_TIME_TESTS));//test only

		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_KSCOPE_SEEDTPARAM seed2,KSCOPEFLAGS flags>
		ITHARE_KSCOPE_FORCEINLINE static constexpr T final_surjection(T y) {
			if constexpr(flags&kscope_flag_is_constexpr) {
				return y - CC;
			}
			else {
				//{MT-related
				//  we do NOT want to write on each access to reduce potential for cache thrashing
				//  what we're trying to do, is limiting the number of writes while keeping reading every time
				//  in the worst case, write can incur a penalty of ~100 cycles, but if we're doing it only one out 15 times -
				//  amortized penalty reduces to 100/15 ~= 7 cycles (NB: cost of branch misprediction is also amortized). 
				auto access_count = ++ObfGlobalVarUpdateTlsCounter<void>::access_count;
				if((access_count&0xf)==0) {//every 15th time; TODO - obfuscate 0xf
					T newC = (statdata.c+DELTA)%DELTAMOD;
					statdata.c = newC;//NB: read-modify-write is not really atomic as a whole, but for our purposes we don't care 
				}
				//}MT-related
				assert(statdata.c%MOD == CC);
				return y - (statdata.c%MOD);
			}
		}

#ifdef ITHARE_KSCOPE_DBG_ENABLE_DBGPRINT
		static void dbg_print(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<ITHARE_KSCOPE_LAST_STOCK_LITERAL+4="<<(ITHARE_KSCOPE_LAST_STOCK_LITERAL+4)<<"/*global volatile var-with-invariant*/," << kscope_dbg_print_t<T>() << "," << kscope_dbg_print_seed<seed>() << ">: CC=" << kscope_dbg_print_c<T>(CC) << std::endl;
		}
#endif
	private:
		union StaticData {//to reduce potential for cache false sharing 
						  //NB: if migrating c into thread_local, DON'T do it (doesn't make any sense for thread_local) 
			public:
			std::atomic<T> c;//TODO: randomize position within cache line
			uint8_t filler[obf_cache_line_size];
			
			constexpr StaticData(T t) 
			: c(t)	{
			}
		};
		static_assert(sizeof(StaticData)==obf_cache_line_size);//not really a strict requirement, but very nice to have, and seems to stand
		static StaticData statdata;
	};

	template<class T, ITHARE_KSCOPE_SEEDTPARAM seed>
	alignas(obf_cache_line_size) typename KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+4, T, seed>::StaticData KscopeLiteralContextVersion<ITHARE_KSCOPE_LAST_STOCK_LITERAL+4, T, seed>::statdata = {CC0};
	
}} //namespace ithare::kscope

#define ITHARE_KSCOPE_ADDITIONAL_LITERAL_DESCRIPTOR_LIST \
	ObfLiteralAdditionalVersion1Descr::descr,\
	ObfLiteralAdditionalVersion2Descr::descr,\
	ObfLiteralAdditionalVersion3Descr::descr,\
	ObfLiteralAdditionalVersion4Descr<T>::descr
		

#endif //ITHARE_KSCOPE_SEED

#endif //ithare_obf_kscope_extension_for_obf_h_included
