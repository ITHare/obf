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

#ifndef ithare_obf_kscope_extension_h_included
#define ithare_obf_kscope_extension_h_included

#ifdef ithare_kscope_kscope_h_included
#error IF using kscope extension, it MUST be included BEFORE kscope.h. See test/officialtest.cpp for usage example
#endif

#include "../../kscope/src/impl/kscope_injection.h"

#ifdef ITHARE_KSCOPE_SEED

namespace ithare { namespace kscope {

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

#endif //ITHARE_KSCOPE_SEED

#endif //ithare_obf_kscope_extension_h_included
