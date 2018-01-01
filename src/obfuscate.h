#ifndef ithare_obf_obfuscate_h_included
#define ithare_obf_obfuscate_h_included

//Usage: 
//  1. Use OBF?() throughout your code to indicate the need for obfuscation
//  1a. OBFX() roughly means 'add no more than 10^(X/2) CPU cycles for obfuscation'
//      i.e. OBF2() adds up to 10 CPU cycles, OBF3() - up to 30 CPU cycles, 
//           and OBF5() - up to 300 CPU cycles
//  2. compile your code without -DITHARE_OBF_SEED for debugging and during development
//  3. compile with -DITHARE_OBF_SEED=0x<really-random-64-bit-seed>u64 for deployments (MSVC)
//  3a. GCC is not supported (yet)

#ifdef ITHARE_OBF_INTERNAL_DBG
//enable assert() in Release
//#undef NDEBUG
#endif

#include <stdint.h>
#include <inttypes.h>
#include <array>
#include <assert.h>
#include <type_traits>
#include <atomic>//for ITHARE_OBF_STRICT_MT
#include <string>//for dbgPrint() only
#include <iostream>//for dbgPrint() only

#ifdef ITHARE_OBF_INTERNAL_DBG // set of settings currently used for internal testing. DON'T rely on it!
//#define ITHARE_OBF_ENABLE_DBGPRINT
//#if 0

#define ITHARE_OBF_SEED 0x0c7dfa61a867b125ui64 //example for MSVC
#define ITHARE_OBF_INIT 
	//enables rather nasty obfuscations (including PEB-based debugger detection),
	//  but requires you to call obf_init() BEFORE ANY obf<> objects are used. 
	//  As a result - it can backfire for obfuscations-used-from-global-constructors :-(.

#define ITHARE_OBF_STRICT_MT//strict multithreading. Short story: as of now, more recommended than not.
	//Long story: in practice, with proper alignments should be MT-safe even without it, 
	//      but is formally UB, so future compilers MAY generate code which will fail under heavy MT (hard to imagine, but...) :-( 
	//      OTOH, at least for x64 generates not-too-obvious XCHG instruction (with implicit LOCK completely missed by current IDA decompiler)

//#define ITHARE_OBF_NO_ANTI_DEBUG
	//disables built-in anti-debug kinda-protections in a clean way
//#define ITHARE_OBF_DEBUG_ANTI_DEBUG_ALWAYS_FALSE
	//makes built-in anti-debugger kinda-protections to return 'not being debugged' (NOT clean, use ONLY for debugging purposes)

#define ITHARE_OBF_COMPILE_TIME_TESTS 100//supposed to affect ONLY time of compilation

//THE FOLLOWING MUST BE NOT USED FOR PRODUCTION BUILDS:
#define ITHARE_OBF_ENABLE_DBGPRINT
	//enables dbgPrint()
#endif//ITHARE_OBF_INTERNAL_DBG

#ifdef _MSC_VER
#pragma warning (disable:4307)
#define ITHARE_OBF_FORCEINLINE __forceinline
#define ITHARE_OBF_NOINLINE __declspec(noinline)
#else
#error non-MSVC compilers are not supported (yet?)
#endif

#ifdef ITHARE_OBF_SEED

#ifndef ITHARE_OBF_SCALE//#define for libraries, using OBFN() macros; 
				//  allows to promote/demote obfuscation scale of the whole library by ITHARE_OBF_SCALE
				//  ITHARE_OBF_SCALE = 1 'converts' all OBF0()'s into OBF1()'s, and so on...
				//  DOES NOT affect obfN<> without macros(!)
#define ITHARE_OBF_SCALE 0
#endif
namespace ithare {
namespace obf {
	//NAMESPACE POLICIES:
	//  EVERYTHING goes into ithare::obf namespace - except for #defines
	//  ALL #defines are prefixed with ITHARE_OBF_
	//    THE ONLY exception is OBF0()...OBF6() macros
	//      These can be disabled using ITHARE_OBF_NO_SHORT_DEFINES macro 
	//        If disabling - use equivalent ITHARE_OBF0()...ITHARE_OBF6()

	using OBFSEED = uint64_t;
	using OBFCYCLES = int32_t;//signed!

	//POTENTIALLY user-modifiable constexpr function:
	constexpr OBFCYCLES obf_exp_cycles(int exp) {
		if (exp < 0)
			return 0;
		OBFCYCLES ret = 1;
		if (exp & 1) {
			ret *= 3;
			exp -= 1;
		}
		assert((exp & 1) == 0);
		exp >>= 1;
		for (int i = 0; i < exp; ++i)
			ret *= 10;
		return ret;
	}

	//helper constexpr functions
	constexpr OBFSEED obf_compile_time_prng(OBFSEED seed, int iteration) {
		static_assert(sizeof(OBFSEED) == 8);
		assert(iteration > 0);
		OBFSEED ret = seed;
		for (int i = 0; i < iteration; ++i) {
			ret = UINT64_C(6364136223846793005) * ret + UINT64_C(1442695040888963407);//linear congruential one; TODO: replace with something crypto-strength for production use
		}
		return ret;
	}

	constexpr OBFSEED obf_seed_from_file_line(const char* file, int line) {
		OBFSEED ret = ITHARE_OBF_SEED ^ line;
		for (const char* p = file; *p; ++p)//effectively djb2 by Dan Bernstein, albeit with different initializer
			ret = ((ret << 5) + ret) + *p;
		return obf_compile_time_prng(ret, 1);//to reduce ill effects from a low-quality PRNG
	}

	template<class T, size_t N>
	constexpr T obf_compile_time_approximation(T x, std::array<T, N> xref, std::array<T, N> yref) {
		for (size_t i = 0; i < N - 1; ++i) {
			T x0 = xref[i];
			T x1 = xref[i + 1];
			if (x >= x0 && x < x1) {
				T y0 = yref[i];
				T y1 = yref[i + 1];
				return uint64_t(y0 + double(x - x0)*double(y1 - y0) / double(x1 - x0));
			}
		}
	}

	constexpr uint64_t obf_sqrt_very_rough_approximation(uint64_t x0) {
		std::array<uint64_t, 64> xref = {};
		std::array<uint64_t, 64> yref = {};
		for (size_t i = 1; i < 64; ++i) {
			uint64_t x = UINT64_C(1) << (i - 1);
			xref[i] = x * x;
			yref[i] = x;
		}
		return obf_compile_time_approximation(x0, xref, yref);
	}

	template<class MAX>
	constexpr MAX obf_weak_random(OBFSEED seed, MAX n) {
		assert(n > 0);
		OBFSEED hi = seed >> (sizeof(OBFSEED) * 4);
		return  hi % n;//biased, but for our purposes will do
	}

	template<size_t N>
	constexpr size_t obf_random_from_list(OBFSEED seed, std::array<size_t, N> weights) {
		//returns index in weights
		size_t totalWeight = 0;
		for (size_t i = 0; i < weights.size(); ++i)
			totalWeight += weights[i];
		assert(totalWeight > 0);
		size_t refWeight = obf_weak_random(seed, totalWeight);
		assert(refWeight < totalWeight);
		for (size_t i = 0; i < weights.size(); ++i) {
			if (refWeight < weights[i])
				return i;
			refWeight -= weights[i];
		}
		assert(false);
	}

	//OBF_CONST_X: constants to be used throughout this particular build
	template<class T, size_t N>
	constexpr size_t obf_find_idx_in_array(std::array<T, N> arr, T value) {
		for (size_t i = 0; i < N; ++i) {
			if (arr[i] == value)
				return i;
		}
		return size_t(-1);
	}

	template<size_t N>
	constexpr uint8_t obf_const_x(OBFSEED seed, std::array<uint8_t, N> excluded) {
		std::array<uint8_t, 6> candidates = { 3,5,7,15,25,31 };//only odd, to allow using the same set of constants in mul-by-odd injections
		std::array<size_t, 6> weights = { 100,100,100,100,100,100 };
		for (size_t i = 0; i < N; ++i) {
			size_t found = obf_find_idx_in_array(candidates, excluded[i]);
			if (found != size_t(-1))
				weights[found] = 0;
		}
		size_t idx2 = obf_random_from_list(seed, weights);
		return candidates[idx2];
	}

	//XOR-ed constants are merely random numbers with no special meaning
	constexpr std::array<uint8_t, 0> obf_const_A_excluded = {};
	constexpr uint8_t OBF_CONST_A = obf_const_x(obf_compile_time_prng(ITHARE_OBF_SEED^UINT64_C(0xcec4b8ea4b89a1a9),1), obf_const_A_excluded);
	constexpr std::array<uint8_t, 1> obf_const_B_excluded = { OBF_CONST_A };
	constexpr uint8_t OBF_CONST_B = obf_const_x(obf_compile_time_prng(ITHARE_OBF_SEED^UINT64_C(0x5eec23716fa1d0aa),1), obf_const_B_excluded);
	constexpr std::array<uint8_t, 2> obf_const_C_excluded = { OBF_CONST_A, OBF_CONST_B };
	constexpr uint8_t OBF_CONST_C = obf_const_x(obf_compile_time_prng(ITHARE_OBF_SEED^UINT64_C(0xfb2de18f982a2d55),1), obf_const_C_excluded);

	template<class T, size_t N>
	constexpr T obf_random_const(OBFSEED seed, std::array<T, N> lst) {
		return lst[obf_weak_random(seed, N)];
	}


	struct ObfDescriptor {
		bool is_recursive;
		OBFCYCLES min_cycles;
		size_t weight;

		constexpr ObfDescriptor(bool is_recursive_, OBFCYCLES min_cycles_, size_t weight_)
			: is_recursive(is_recursive_), min_cycles(min_cycles_), weight(weight_) {
		}
	};

	template<size_t N>
	constexpr size_t obf_random_obf_from_list(OBFSEED seed, OBFCYCLES cycles, std::array<ObfDescriptor, N> descr,size_t exclude_version=size_t(-1)) {
		//returns index in descr
		size_t sz = descr.size();
		std::array<size_t, N> nr_weights = {};
		std::array<size_t, N> r_weights = {};
		size_t sum_r = 0;
		size_t sum_nr = 0;
		for (size_t i = 0; i < sz; ++i) {
			if (i != exclude_version && cycles >= descr[i].min_cycles)
				if (descr[i].is_recursive) {
					r_weights[i] = descr[i].weight;
					sum_r += r_weights[i];
				}
				else {
					nr_weights[i] = descr[i].weight;
					sum_nr += nr_weights[i];
				}
		}
		if (sum_r)
			return obf_random_from_list(seed, r_weights);
		else {
			assert(sum_nr > 0);
			return obf_random_from_list(seed, nr_weights);
		}
	}

	template<size_t N>
	constexpr std::array<OBFCYCLES, N> obf_random_split(OBFSEED seed, OBFCYCLES cycles, std::array<ObfDescriptor, N> elements) {
		//size_t totalWeight = 0;
		assert(cycles >= 0);
		OBFCYCLES mins = 0;
		for (size_t i = 0; i < N; ++i) {
			assert(elements[i].min_cycles==0);//mins NOT to be used within calls to obf_random_split 
											  //  (it not a problem with this function, but they tend to cause WAY too much trouble in injection_version<> code
			mins += elements[i].min_cycles;
			//totalWeight += elements[i].weight;
		}
		OBFCYCLES leftovers = cycles - mins;
		assert(leftovers >= 0);
		std::array<OBFCYCLES, N> ret = {};
		size_t totalWeight = 0;
		for (size_t i = 0; i < N; ++i) {
			ret[i] = OBFCYCLES(obf_weak_random(obf_compile_time_prng(seed, int(i+1)), elements[i].weight)) + 1;//'+1' is to avoid "all-zeros" case
			totalWeight += ret[i];
		}
		size_t totalWeight2 = 0;
		double q = double(leftovers) / double(totalWeight);
		for (size_t i = 0; i < N; ++i) {
			ret[i] = elements[i].min_cycles+OBFCYCLES(double(ret[i]) * double(q));
			assert(ret[i] >= elements[i].min_cycles);
			totalWeight2 += ret[i];
		}
		assert(OBFCYCLES(totalWeight2) <= mins + leftovers);
		return ret;
	}

	//type helpers
	//obf_half_size_int<>
	//TODO: obf_traits<>, including obf_traits<>::half_size_int
	template<class T>
	struct obf_half_size_int;

	template<>
	struct obf_half_size_int<uint16_t> {
		using value_type = uint8_t;
	};

	template<>
	struct obf_half_size_int<int16_t> {
		using value_type = int8_t;
	};

	template<>
	struct obf_half_size_int<uint32_t> {
		using value_type = uint16_t;
	};

	template<>
	struct obf_half_size_int<int32_t> {
		using value_type = int16_t;
	};

	template<>
	struct obf_half_size_int<uint64_t> {
		using value_type = uint32_t;
	};

	template<>
	struct obf_half_size_int<int64_t> {
		using value_type = int32_t;
	};

	//forward declarations
	struct ObfDefaultInjectionContext {
		static constexpr size_t exclude_version = size_t(-1);
	};
	template<class T, class Context, OBFSEED seed, OBFCYCLES cycles,class InjectionContext>
	class obf_injection;
	template<class T, T C, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_literal_ctx;
	template<class T_, T_ C_, OBFSEED seed, OBFCYCLES cycles>
	class obf_literal;

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
	//dbgPrint helpers
	template<class T>
	std::string obf_dbgPrintT() {
		return std::string("T(sizeof=") + std::to_string(sizeof(T)) + ")";
	}

	template<class T>
	struct ObfPrintC {
		using type = T;
	};
	template<>
	struct ObfPrintC<uint8_t> {
		using type = int;
	};

	template<class T>
	typename ObfPrintC<T>::type obf_dbgPrintC(T c) {
		return ObfPrintC<T>::type(c);
	}
#endif

	//ObfRecursiveContext
	template<class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	struct ObfRecursiveContext;

	//injection-with-constant - building block both for injections and for literals
	template <size_t which, class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version;

	//version 0: identity
	template<class Context>
	struct obf_injection_version0_descr {
		//cannot make it a part of class obf_injection_version<0, T, C, seed, cycles>,
		//  as it would cause infinite recursion in template instantiation
		static constexpr OBFCYCLES own_min_injection_cycles = 0;
		static constexpr OBFCYCLES own_min_surjection_cycles = 0;
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr = ObfDescriptor(false, own_min_cycles, 1);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<0, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);

		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version0_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

	public:
		using return_type = T;
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			return Context::final_injection(x);
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			return Context::final_surjection(y);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0,const char* prefix="") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<0/*identity*/," << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">: availCycles=" << availCycles << std::endl;
			Context::dbgPrint(offset + 1);
		}
#endif
	};

	//version 1: add mod 2^n
	template<class Context>
	struct obf_injection_version1_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 1;
		static constexpr OBFCYCLES own_min_surjection_cycles = 1;
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr = ObfDescriptor(true, own_min_cycles, 100);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<1, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version1_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		struct RecursiveInjectionContext {
			static constexpr size_t exclude_version = 1;
		};

		using RecursiveInjection = obf_injection<T, Context, obf_compile_time_prng(seed, 1), availCycles+Context::context_cycles,RecursiveInjectionContext>;
		using return_type = typename RecursiveInjection::return_type;
		static constexpr std::array<T, 5> consts = { 0,1,OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T C = obf_random_const<T>(obf_compile_time_prng(seed, 2), consts);
		static constexpr bool neg = C == 0 ? true : obf_weak_random(obf_compile_time_prng(seed, 3),2) == 0;
		using ST = typename std::make_signed<T>::type;
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			if constexpr(neg) {
				ST sx = ST(x);
				return RecursiveInjection::injection(T(-sx) + C);
			}
			else
				return RecursiveInjection::injection(x + C);
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			T yy = RecursiveInjection::surjection(y) - C;
			if constexpr(neg) {
				ST syy = ST(yy);
				return T(-syy);
			}
			else
				return yy;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<1/*add mod 2^N*/," << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">: C=" << obf_dbgPrintC(C) << " neg=" << neg << std::endl;
			RecursiveInjection::dbgPrint(offset + 1);
		}
#endif
	};

	//helper for Feistel-like: randomized_non_reversible_function 
	template<size_t which, class T, OBFSEED seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version;
	//IMPORTANT: Feistel-like non-reversible functions SHOULD be short, to avoid creating code 'signatures'
	//  Therefore, currently we're NOT using any recursions here

	struct obf_randomized_non_reversible_function_version0_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(false, 0, 100);
	};

	template<class T, OBFSEED seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version<0, T, seed, cycles> {
		ITHARE_OBF_FORCEINLINE T operator()(T x) {
			return x;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<0/*identity*/," << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">" << std::endl;
		}
#endif		
	};

	struct obf_randomized_non_reversible_function_version1_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 3, 100);
	};

	template<class T, OBFSEED seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version<1,T,seed,cycles> {
		ITHARE_OBF_FORCEINLINE T operator()(T x) {
			return x*x;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<1/*x^2*/," << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">" << std::endl;
		}
#endif		
	};

	struct obf_randomized_non_reversible_function_version2_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 7, 100);
	};

	template<class T, OBFSEED seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version<2, T, seed, cycles> {
		using ST = typename std::make_signed<T>::type;
		ITHARE_OBF_FORCEINLINE T operator()(T x) {
			ST sx = ST(x);
			return T(sx < 0 ? -sx : sx);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<2/*abs*/," << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">" << std::endl;
		}
#endif		
	};

	template<size_t N>
	constexpr OBFCYCLES obf_max_min_descr(std::array<ObfDescriptor,N> descr) {
		OBFCYCLES ret = 0;
		for (size_t i = 0; i < N; ++i) {
			OBFCYCLES mn = descr[i].min_cycles;
			if (ret < mn)
				ret = mn;
		}
		return ret;
	}

	template<class T, OBFSEED seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function {
		constexpr static std::array<ObfDescriptor, 3> descr{
			obf_randomized_non_reversible_function_version0_descr::descr,
			obf_randomized_non_reversible_function_version1_descr::descr,
			obf_randomized_non_reversible_function_version2_descr::descr,
		};
		constexpr static size_t max_cycles_that_make_sense = obf_max_min_descr(descr);
		constexpr static size_t which = obf_random_obf_from_list(obf_compile_time_prng(seed, 1), cycles, descr);
		using FType = obf_randomized_non_reversible_function_version<which, T, seed, cycles>;
		ITHARE_OBF_FORCEINLINE T operator()(T x) {
			return FType()(x);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<" << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">: which=" << which << std::endl;
			FType::dbgPrint(offset + 1);
		}
#endif		
	};

	//version 2: kinda-Feistel round
	template<class T, class Context>
	struct obf_injection_version2_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 7;
		static constexpr OBFCYCLES own_min_surjection_cycles = 7;
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr =
			sizeof(T) > 1 ?
			ObfDescriptor(true, own_min_cycles, 100) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<2, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version2_descr<T,Context>::own_min_cycles;
		static_assert(availCycles >= 0);
		constexpr static std::array<ObfDescriptor, 2> split {
			ObfDescriptor(true,0,100),//f() 
			ObfDescriptor(true,0,100)//RecursiveInjection
		};
		static constexpr auto splitCycles = obf_random_split(obf_compile_time_prng(seed, 1), availCycles, split);
		static constexpr OBFCYCLES cycles_f0 = splitCycles[0];
		static constexpr OBFCYCLES cycles_rInj0 = splitCycles[1];
		static_assert(cycles_f0 + cycles_rInj0 <= availCycles);

		//doesn't make sense to use more than max_cycles_that_make_sense cycles for f...
		static constexpr OBFCYCLES max_cycles_that_make_sense = obf_randomized_non_reversible_function<T, 0, 0>::max_cycles_that_make_sense;
		static constexpr OBFCYCLES delta_f = cycles_f0 > max_cycles_that_make_sense ? cycles_f0 - max_cycles_that_make_sense : 0;
		static constexpr OBFCYCLES cycles_f = cycles_f0 - delta_f;
		static constexpr OBFCYCLES cycles_rInj = cycles_rInj0 + delta_f;
		static_assert(cycles_f + cycles_rInj <= availCycles);

		using RecursiveInjection = obf_injection<T, Context, obf_compile_time_prng(seed, 2), cycles_rInj+ Context::context_cycles,ObfDefaultInjectionContext>;
		using return_type = typename RecursiveInjection::return_type;

		using halfT = typename obf_half_size_int<T>::value_type;
		using FType = obf_randomized_non_reversible_function<halfT, obf_compile_time_prng(seed, 3), cycles_f>;

		constexpr static int halfTBits = sizeof(halfT) * 8;
		//constexpr static T mask = ((T)1 << halfTBits) - 1;
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			T lo = x >> halfTBits;
			//T hi = (x & mask) + f((halfT)lo);
			T hi = x + f((halfT)lo);
			return RecursiveInjection::injection((hi << halfTBits) + lo);
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y_) {
			T y = RecursiveInjection::surjection(y_);
			halfT hi = y >> halfTBits;
			T lo = y;
			//T z = (hi - f((halfT)lo)) & mask;
			halfT z = (hi - f((halfT)lo));
			return z + (lo << halfTBits);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<2/*kinda-Feistel*/,"<<obf_dbgPrintT<T>()<<"," << seed << "," << cycles << ">:" 
				" availCycles=" << availCycles << " cycles_f=" << cycles_f << " cycles_rInj=" << cycles_rInj << std::endl;
			//auto splitCyclesRT = obf_random_split(obf_compile_time_prng(seed, 1), availCycles, split);
			//std::cout << std::string(offset, ' ') << " f():" << std::endl;
			FType::dbgPrint(offset + 1,"f():");
			//std::cout << std::string(offset, ' ') << " Recursive:" << std::endl;
			RecursiveInjection::dbgPrint(offset + 1,"Recursive:");
		}
#endif

	private:
		ITHARE_OBF_FORCEINLINE static constexpr halfT f(halfT x) {
			return FType()(x);
		}
	};

	//version 3: split-join
	template<class T,class Context>
	struct obf_injection_version3_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 7;
		static constexpr OBFCYCLES own_min_surjection_cycles = 7;
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr =
			sizeof(T) > 1 ?
			ObfDescriptor(true, own_min_cycles, 100) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<3, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		//TODO:split-join based on union
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version3_descr<T, Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		using halfT = typename obf_half_size_int<T>::value_type;
		constexpr static int halfTBits = sizeof(halfT) * 8;

		constexpr static std::array<ObfDescriptor, 3> split{
			ObfDescriptor(true,0,200),//RecursiveInjection
			ObfDescriptor(true,0,100),//LoInjection
			ObfDescriptor(true,0,100),//HiInjection
		};
		static constexpr auto splitCycles = obf_random_split(obf_compile_time_prng(seed, 1), availCycles, split);
		static constexpr OBFCYCLES cycles_rInj = splitCycles[0];
		static constexpr OBFCYCLES cycles_lo = splitCycles[1];
		static constexpr OBFCYCLES cycles_hi = splitCycles[2];
		static_assert(cycles_rInj + cycles_lo + cycles_hi <= availCycles);

		using RecursiveInjection = obf_injection<T, Context, obf_compile_time_prng(seed, 2), cycles_rInj+ Context::context_cycles, ObfDefaultInjectionContext>;
		using return_type = typename RecursiveInjection::return_type;

		constexpr static std::array<ObfDescriptor, 2> splitLo {
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesLo = obf_random_split(obf_compile_time_prng(seed, 2), cycles_lo, splitLo);
		static constexpr OBFCYCLES cycles_loCtx = splitCyclesLo[0];
		static constexpr OBFCYCLES cycles_loInj = splitCyclesLo[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using LoContext = typename ObfRecursiveContext < halfT, Context, obf_compile_time_prng(seed, 3), cycles_loCtx>::intermediate_context_type;
		using LoInjection = obf_injection<halfT, LoContext, obf_compile_time_prng(seed, 4), cycles_loInj+LoContext::context_cycles, ObfDefaultInjectionContext>;
		static_assert(sizeof(LoInjection::return_type) == sizeof(halfT));//bijections ONLY; TODO: enforce

		constexpr static std::array<ObfDescriptor, 2> splitHi{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesHi = obf_random_split(obf_compile_time_prng(seed, 5), cycles_hi, splitHi);
		static constexpr OBFCYCLES cycles_hiCtx = splitCyclesHi[0];
		static constexpr OBFCYCLES cycles_hiInj = splitCyclesHi[1];
		static_assert(cycles_hiCtx + cycles_hiInj <= cycles_hi);
		using HiContext = typename ObfRecursiveContext<halfT, Context, obf_compile_time_prng(seed, 6), cycles_hiCtx>::intermediate_context_type;
		using HiInjection = obf_injection<halfT, HiContext, obf_compile_time_prng(seed, 7), cycles_hiInj+HiContext::context_cycles, ObfDefaultInjectionContext>;
		static_assert(sizeof(HiInjection::return_type) == sizeof(halfT));//bijections ONLY; TODO: enforce

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			halfT lo = x >> halfTBits;
			typename LoInjection::return_type lo1 = LoInjection::injection(lo);
			lo = *reinterpret_cast<halfT*>(&lo1);//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			halfT hi = (halfT)x;
			typename HiInjection::return_type hi1 = HiInjection::injection(hi);
			hi = *reinterpret_cast<halfT*>(&hi1);//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return RecursiveInjection::injection((T(hi) << halfTBits) + T(lo));
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y_) {
			auto y = RecursiveInjection::surjection(y_);
			halfT hi0 = y >> halfTBits;
			halfT lo0 = (halfT)y;
			halfT hi = HiInjection::surjection(*reinterpret_cast<typename HiInjection::return_type*>(&hi0));//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			halfT lo = LoInjection::surjection(*reinterpret_cast<typename LoInjection::return_type*>(&lo0));//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return T(hi) + (T(lo) << halfTBits);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<3/*split-join*/,"<<obf_dbgPrintT<T>()<<"," << seed << "," << cycles << ">" << std::endl;
			//std::cout << std::string(offset, ' ') << " Lo:" << std::endl;
			LoInjection::dbgPrint(offset + 1,"Lo:");
			//std::cout << std::string(offset, ' ') << " Hi:" << std::endl;
			HiInjection::dbgPrint(offset + 1,"Hi:");
			//std::cout << std::string(offset, ' ') << " Recursive:" << std::endl;
			RecursiveInjection::dbgPrint(offset + 1,"Recursive:");
		}
#endif
	};
	
	//version 4: multiply by odd
	template< class T >
	constexpr T obf_mul_inverse_mod2n(T num) {//extended GCD, intended to be used in compile-time only
											  //by Dmytro Ivanchykhin
		assert(num & 1);
		T num0 = num;
		T x = 0, lastx = 1, y = 1, lasty = 0;
		T q=0, temp1=0, temp2=0, temp3=0;
		T mod = 0;

		// zero step: do some tricks to avoid overflowing
		// note that initially mod is power of 2 that does not fit to T
		if (num == T(mod - T(1)))
			return num;
		q = T((T(mod - num)) / num) + T(1);
		temp1 = (T(T(T(mod - T(2))) % num) + T(2)) % num;
		mod = num;
		num = temp1;

		temp2 = x;
		x = lastx - T(q * x);
		lastx = temp2;

		temp3 = y;
		y = lasty - T(q * y);
		lasty = temp3;

		while (num != 0) {
			q = mod / num;
			temp1 = mod % num;
			mod = num;
			num = temp1;

			temp2 = x;
			x = lastx - T(q * x);
			lastx = temp2;

			temp3 = y;
			y = lasty - T(q * y);
			lasty = temp3;
		}
		assert(T(num0*lasty) == T(1));
		return lasty;
	}

	template<class Context>
	struct obf_injection_version4_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 3 + Context::literal_cycles;
		static constexpr OBFCYCLES own_min_surjection_cycles = 3;
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr = ObfDescriptor(true, own_min_cycles, 100);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<4, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version4_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		struct RecursiveInjectionContext {
			static constexpr size_t exclude_version = 4;
		};

	public:
		using RecursiveInjection = obf_injection<T, Context, obf_compile_time_prng(seed, 1), availCycles+Context::context_cycles,RecursiveInjectionContext>;
		using return_type = typename RecursiveInjection::return_type;
		//constexpr static T C = (T)(obf_gen_const<T>(obf_compile_time_prng(seed, 2)) | 1);
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T C = obf_random_const<T>(obf_compile_time_prng(seed, 2), consts);
		static_assert((C & 1) == 1);
		constexpr static T CINV = obf_mul_inverse_mod2n(C);
		static_assert((T)(C*CINV) == (T)1);

		using literal = typename Context::template literal<T, CINV, obf_compile_time_prng(seed, 3)>::type;

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			return RecursiveInjection::injection(x * literal().value());//using CINV in injection to hide literals a bit better...
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			return RecursiveInjection::surjection(y) * C;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<4/*mul odd mod 2^N*/,"<<obf_dbgPrintT<T>()<<"," << seed << "," << cycles << ">: C=" << obf_dbgPrintC(C) << " CINV=" << obf_dbgPrintC(CINV) << std::endl;
			//std::cout << std::string(offset, ' ') << " literal:" << std::endl;
			literal::dbgPrint(offset + 1,"literal:");
			//std::cout << std::string(offset, ' ') << " Recursive:" << std::endl;
			RecursiveInjection::dbgPrint(offset + 1,"Recursive:");
		}
#endif
	};

	//version 5: split (w/o join)
	template<class T, class Context>
	struct obf_injection_version5_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 3;
		static constexpr OBFCYCLES own_min_surjection_cycles = 3;
		static constexpr OBFCYCLES own_min_cycles = 2*Context::context_cycles /* have to allocate context_cycles for BOTH branches */ + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr =
			sizeof(T) > 1 ?
			ObfDescriptor(true, own_min_cycles, 100) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<5, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		using halfT = typename obf_half_size_int<T>::value_type;
		constexpr static int halfTBits = sizeof(halfT) * 8;

		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version5_descr<T,Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		constexpr static std::array<ObfDescriptor, 2> split{
			ObfDescriptor(true,0,100),//LoInjection
			ObfDescriptor(true,0,100),//HiInjection
		};
		static constexpr auto splitCycles = obf_random_split(obf_compile_time_prng(seed, 1), availCycles, split);
		static constexpr OBFCYCLES cycles_lo = splitCycles[0];
		static constexpr OBFCYCLES cycles_hi = splitCycles[1];
		static_assert(cycles_lo + cycles_hi <= availCycles);

		constexpr static std::array<ObfDescriptor, 2> splitLo{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesLo = obf_random_split(obf_compile_time_prng(seed, 2), cycles_lo, splitLo);
		static constexpr OBFCYCLES cycles_loCtx = splitCyclesLo[0];
		static constexpr OBFCYCLES cycles_loInj = splitCyclesLo[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using RecursiveLoContext = typename ObfRecursiveContext<halfT, Context, obf_compile_time_prng(seed, 3), cycles_loCtx+Context::context_cycles>::recursive_context_type;
		using RecursiveInjectionLo = obf_injection<halfT, RecursiveLoContext, obf_compile_time_prng(seed, 4), cycles_loInj+ RecursiveLoContext::context_cycles,ObfDefaultInjectionContext>;

		constexpr static std::array<ObfDescriptor, 2> splitHi{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesHi = obf_random_split(obf_compile_time_prng(seed, 5), cycles_hi, splitHi);
		static constexpr OBFCYCLES cycles_hiCtx = splitCyclesHi[0];
		static constexpr OBFCYCLES cycles_hiInj = splitCyclesHi[1];
		static_assert(cycles_hiCtx + cycles_hiInj <= cycles_hi);
		using RecursiveHiContext = typename ObfRecursiveContext<halfT, Context, obf_compile_time_prng(seed, 6), cycles_hiCtx+Context::context_cycles>::recursive_context_type;
		using RecursiveInjectionHi = obf_injection < halfT, RecursiveHiContext, obf_compile_time_prng(seed, 7), cycles_hiInj+ RecursiveHiContext::context_cycles,ObfDefaultInjectionContext > ;

		struct return_type {
			typename RecursiveInjectionLo::return_type lo;
			typename RecursiveInjectionHi::return_type hi;
		};
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			return_type ret{ RecursiveInjectionLo::injection((halfT)x), RecursiveInjectionHi::injection(x >> halfTBits) };
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y_) {
			halfT hi = RecursiveInjectionHi::surjection(y_.hi);
			halfT lo = RecursiveInjectionLo::surjection(y_.lo);
			return (T)lo + ((T)hi << halfTBits);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<5/*split*/,"<<obf_dbgPrintT<T>()<<"," << seed << "," << cycles << ">" << std::endl;
			//std::cout << std::string(offset, ' ') << " Lo:" << std::endl;
			RecursiveInjectionLo::dbgPrint(offset + 1,"Lo:");
			//std::cout << std::string(offset, ' ') << " Hi:" << std::endl;
			RecursiveInjectionHi::dbgPrint(offset + 1,"Hi:");
		}
#endif
	};

	//version 6: injection over lower half /*CHEAP!*/
	template<class T, class Context>
	struct obf_injection_version6_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 3;
		static constexpr OBFCYCLES own_min_surjection_cycles = 3;
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr =
			sizeof(T) > 1 ?
			ObfDescriptor(true, own_min_cycles, 100/*it's cheap, but doesn't obfuscate the whole thing well => let's use it mostly for lower-cycle stuff*/) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<6, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version6_descr<T,Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		struct RecursiveInjectionContext {
			static constexpr size_t exclude_version = 6;
		};
		using halfT = typename obf_half_size_int<T>::value_type;

		constexpr static std::array<ObfDescriptor, 2> split{
			ObfDescriptor(true,0,200),//RecursiveInjection
			ObfDescriptor(true,0,100),//LoInjection
		};
		static constexpr auto splitCycles = obf_random_split(obf_compile_time_prng(seed, 1), availCycles, split);
		static constexpr OBFCYCLES cycles_rInj = splitCycles[0];
		static constexpr OBFCYCLES cycles_lo = splitCycles[1];
		static_assert(cycles_rInj + cycles_lo <= availCycles);

		using RecursiveInjection = obf_injection<T, Context, obf_compile_time_prng(seed, 2), cycles_rInj + Context::context_cycles, ObfDefaultInjectionContext>;
		using return_type = typename RecursiveInjection::return_type;

	public:
		constexpr static std::array<ObfDescriptor, 2> splitLo{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesLo = obf_random_split(obf_compile_time_prng(seed, 3), cycles_lo, splitLo);
		static constexpr OBFCYCLES cycles_loCtx = splitCyclesLo[0];
		static constexpr OBFCYCLES cycles_loInj = splitCyclesLo[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using LoContext = typename ObfRecursiveContext < halfT, Context, obf_compile_time_prng(seed, 4), cycles_loCtx>::intermediate_context_type;
		using LoInjection = obf_injection<halfT, LoContext, obf_compile_time_prng(seed, 5), cycles_loInj + LoContext::context_cycles, ObfDefaultInjectionContext>;
		static_assert(sizeof(LoInjection::return_type) == sizeof(halfT));//bijections ONLY; TODO: enforce

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			halfT lo0 = halfT(x);
			typename LoInjection::return_type lo1 = LoInjection::injection(lo0);
			halfT lo = *reinterpret_cast<halfT*>(&lo1);//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return RecursiveInjection::injection(x - T(lo0) + lo);
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type yy) {
			T y = RecursiveInjection::surjection(yy);
			halfT lo0 = halfT(y);
			halfT lo = LoInjection::surjection(*reinterpret_cast<typename LoInjection::return_type*>(&lo0));//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return y - T(lo0) + lo;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<6/*injection(halfT)*/," << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">" << std::endl;
			LoInjection::dbgPrint(offset + 1, "Lo:");
			RecursiveInjection::dbgPrint(offset + 1, "Recursive:");
		}
#endif
	};

	#if 0 //COMMENTED OUT - TOO OBVIOUS IN DECOMPILE :-(
	//version 7: 1-bit rotation 
	template<class Context>
	struct obf_injection_version7_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 5;//relying on compiler generating cmovns etc.
		static constexpr OBFCYCLES own_min_surjection_cycles = 5;//relying on compiler generating cmovns etc.
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr = ObfDescriptor(true, own_min_cycles, 100);
	};

	template <class T, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_injection_version<7, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version7_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		using RecursiveInjection = obf_injection<T, Context, obf_compile_time_prng(seed, 1), availCycles + Context::context_cycles, ObfDefaultInjectionContext>;
		using return_type = typename RecursiveInjection::return_type;
		using ST = typename std::make_signed<T>::type;
		static constexpr T highbit = T(1) << (sizeof(T) * 8 - 1);
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			if ((x % 2) == 0)
				x = x >> 1;
			else
				x = ( x >> 1 ) + highbit;
			return RecursiveInjection::injection(x);
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			T yy = RecursiveInjection::surjection(y);
			ST syy = ST(yy);
			if (syy < 0)
				return yy + yy + 1;
			else
				return yy+yy;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<7/*1-bit rotation*/," << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">" << std::endl;
			RecursiveInjection::dbgPrint(offset + 1);
		}
#endif
	};
#endif//#if 0

	//obf_injection: combining obf_injection_version
	template<class T, class Context, OBFSEED seed, OBFCYCLES cycles,class InjectionContext>
	class obf_injection {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static std::array<ObfDescriptor, 7> descr{
			obf_injection_version0_descr<Context>::descr,
			obf_injection_version1_descr<Context>::descr,
			obf_injection_version2_descr<T,Context>::descr,
			obf_injection_version3_descr<T,Context>::descr,
			obf_injection_version4_descr<Context>::descr,
			obf_injection_version5_descr<T,Context>::descr,
			obf_injection_version6_descr<T,Context>::descr,
			//obf_injection_version7_descr<Context>::descr,
		};
		constexpr static size_t which = obf_random_obf_from_list(obf_compile_time_prng(seed, 1), cycles, descr,InjectionContext::exclude_version);
		using WhichType = obf_injection_version<which, T, Context, seed, cycles>;

	public:
		using return_type = typename WhichType::return_type;
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			return WhichType::injection(x);
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			return WhichType::surjection(y);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			size_t dbgWhich = obf_random_obf_from_list(obf_compile_time_prng(seed, 1), cycles, descr);
			std::cout << std::string(offset, ' ') << prefix << "obf_injection<"<<obf_dbgPrintT<T>()<<"," << seed << "," << cycles << ">: which=" << which << " dbgWhich=" << dbgWhich << std::endl;
			//std::cout << std::string(offset, ' ') << " Context:" << std::endl;
			Context::dbgPrint(offset + 1,"Context:");
			//std::cout << std::string(offset, ' ') << " Version:" << std::endl;
			WhichType::dbgPrint(offset + 1);
		}
#endif
	};

	//ObfLiteralContext
	template<size_t which, class T, OBFSEED seed>
	struct ObfLiteralContext_version;
	//forward declaration:
	template<class T, OBFSEED seed, OBFCYCLES cycles>
	class ObfLiteralContext;

	//version 0: identity
	struct obf_literal_context_version0_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(false, 0, 1);
	};

	template<class T,OBFSEED seed>
	struct ObfLiteralContext_version<0,T,seed> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version0_descr::descr.min_cycles;

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static constexpr T final_surjection(T y) {
			return y;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<0/*identity*/," << obf_dbgPrintT<T>() << ">" << std::endl;
		}
#endif
	};

	//version 1: global volatile constant
	struct obf_literal_context_version1_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 6, 100);
	};

	template<class T, OBFSEED seed>
	struct ObfLiteralContext_version<1,T,seed> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version1_descr::descr.min_cycles;

		//static constexpr T CC = obf_gen_const<T>(obf_compile_time_prng(seed, 1));
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T CC = obf_random_const<T>(obf_compile_time_prng(seed, 1), consts);
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
			return y - c;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<1/*global volatile*/," << obf_dbgPrintT<T>() << "," << seed << ">: CC=" << obf_dbgPrintC(CC) << std::endl;
		}
#endif
	private:
		static volatile T c;
	};

	template<class T, OBFSEED seed>
	volatile T ObfLiteralContext_version<1, T, seed>::c = CC;

	//version 2: aliased pointers
	struct obf_literal_context_version2_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 20, 100);
	};

	template<class T>//TODO: randomize contents of the function
	ITHARE_OBF_NOINLINE T obf_aliased_zero(T* x, T* y) {
		*x = 0;
		*y = 1;
		return *x;
	}

	template<class T, OBFSEED seed>
	struct ObfLiteralContext_version<2,T,seed> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version2_descr::descr.min_cycles;

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static /*non-constexpr*/ T final_surjection(T y) {
			T x, yy;
			T z = obf_aliased_zero(&x, &yy);
			return y - z;
		}
#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<2/*func with aliased pointers*/," << obf_dbgPrintT<T>() << "," << seed << ">:" << std::endl;
		}
#endif
	};

	//version 3: Windows/PEB
	struct obf_literal_context_version3_descr {
#if defined(_MSC_VER) && defined(ITHARE_OBF_INIT) && !defined(ITHARE_OBF_NO_ANTI_DEBUG)
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 10, 100);
#else
		static constexpr ObfDescriptor descr = ObfDescriptor(false, 0, 0);
#endif
	};

#ifdef _MSC_VER
	extern volatile uint8_t* obf_peb;

	template<class T, OBFSEED seed>
	struct ObfLiteralContext_version<3,T,seed> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version3_descr::descr.min_cycles;

		//static constexpr T CC = obf_gen_const<T>(obf_compile_time_prng(seed, 1));
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T CC = obf_random_const<T>(obf_compile_time_prng(seed, 1), consts);
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
#ifdef ITHARE_OBF_DEBUG_ANTI_DEBUG_ALWAYS_FALSE
			return y - CC;
#else
			return y - CC * (1 + obf_peb[2]);
#endif
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<3/*PEB*/," << obf_dbgPrintT<T>() << "," << seed << ">: CC=" << obf_dbgPrintC(CC) << std::endl;
		}
#endif
	};
#endif

	//version 4: global var-with-invariant
	struct obf_literal_context_version4_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 100, 100);
			//yes, it is up 100+ cycles now (due to worst-case MT caching issues)
			//TODO: move invariant to thread_local, something else?
	};

	template<class T, OBFSEED seed>
	struct ObfLiteralContext_version<4, T, seed> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version4_descr::descr.min_cycles;

		static constexpr T PREMODRNDCONST = obf_gen_const<T>(obf_compile_time_prng(seed, 1));
		static constexpr T PREMODMASK = (T(1) << (sizeof(T) * 4)) - 1;
		static constexpr T PREMOD = PREMODRNDCONST & PREMODMASK;
		static constexpr T MOD = PREMOD == 0 ? 100 : PREMOD;//remapping 'bad value' 0 to 'something'
		static constexpr T CC = obf_weak_random(obf_compile_time_prng(seed, 2),MOD);

		static constexpr T MAXMUL1 = T(-1)/MOD;
		static constexpr auto MAXMUL1_ADJUSTED0 = obf_sqrt_very_rough_approximation(MAXMUL1);
		static_assert(MAXMUL1_ADJUSTED0 < T(-1));
		static constexpr T MAXMUL1_ADJUSTED = (T)MAXMUL1_ADJUSTED0;
		static constexpr T MUL1 = MAXMUL1 > 2 ? 1+obf_weak_random(obf_compile_time_prng(seed, 2), MAXMUL1_ADJUSTED) : 1;
		static constexpr T DELTA = MUL1 * MOD;
		static_assert(DELTA / MUL1 == MOD);//overflow check

		static constexpr T MAXMUL2 = T(-1) / DELTA;
		static constexpr T MUL2 = MAXMUL2 > 2 ? 1+obf_weak_random(obf_compile_time_prng(seed, 3), MAXMUL2) : 1;
		static constexpr T DELTAMOD = MUL2 * MOD;
		static_assert(DELTAMOD / MUL2 == MOD);//overflow check

		static constexpr T PREMUL3 = obf_weak_random(obf_compile_time_prng(seed, 3), MUL2);
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
		static_assert(test_n_iterations(CC0, ITHARE_OBF_COMPILE_TIME_TESTS));//test only

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
			//{MT-related:
			T newC = (c+DELTA)%DELTAMOD;
			c = newC;
			//}MT-related
			assert(c%MOD == CC);
			return y - (c%MOD);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<4/*global volatile var-with-invariant*/," << obf_dbgPrintT<T>() << "," << seed << ">: CC=" << obf_dbgPrintC(CC) << std::endl;
		}
#endif
	private:
#ifdef ITHARE_OBF_STRICT_MT
		static std::atomic<T> c;
#else
		static volatile T c;
#endif
	};

#ifdef ITHARE_OBF_STRICT_MT
	template<class T, OBFSEED seed>
	std::atomic<T> ObfLiteralContext_version<4, T, seed>::c = CC0;
#else
	template<class T, OBFSEED seed>
	volatile T ObfLiteralContext_version<4, T, seed>::c = CC0;
#endif

	//ObfZeroLiteralContext
	template<class T>
	struct ObfZeroLiteralContext {
		//same as ObfLiteralContext_version<0,...> but with additional stuff to make it suitable for use as Context parameter to injections
		constexpr static OBFCYCLES context_cycles = 0;
		constexpr static OBFCYCLES calc_cycles(OBFCYCLES inj, OBFCYCLES surj) {
			return surj;//for literals, ONLY surjection costs apply in runtime (as injection applies in compile-time)
		}
		constexpr static OBFCYCLES literal_cycles = 0;
		template<class T,T C,OBFSEED seed>
		struct literal {
			using type = obf_literal_ctx<T, C, ObfZeroLiteralContext<T>, seed, literal_cycles>;
		};

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static constexpr T final_surjection(T y) {
			return y;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfZeroContext<" << obf_dbgPrintT<T>() << ">" << std::endl;
		}
#endif
	};
	template<class T, class T0, OBFSEED seed, OBFCYCLES cycles>
	struct ObfRecursiveContext<T, ObfZeroLiteralContext<T0>, seed, cycles> {
		using recursive_context_type = ObfZeroLiteralContext<T>;
		using intermediate_context_type = ObfZeroLiteralContext<T>;
	};

	//ObfLiteralContext
	template<class T, OBFSEED seed, OBFCYCLES cycles>
	class ObfLiteralContext {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static std::array<ObfDescriptor, 5> descr{
			obf_literal_context_version0_descr::descr,
			obf_literal_context_version1_descr::descr,
			obf_literal_context_version2_descr::descr,
			obf_literal_context_version3_descr::descr,
			obf_literal_context_version4_descr::descr,
		};
		constexpr static size_t which = obf_random_obf_from_list(obf_compile_time_prng(seed, 1), cycles, descr);
		using WhichType = ObfLiteralContext_version<which, T, seed>;

	public:
		constexpr static OBFCYCLES context_cycles = WhichType::context_cycles;
		constexpr static OBFCYCLES calc_cycles(OBFCYCLES inj, OBFCYCLES surj) {
			return surj;//for literals, ONLY surjection costs apply in runtime (as injection applies in compile-time)
		}

		constexpr static OBFCYCLES literal_cycles = 0;
		template<class T, T C, OBFSEED seed>
		struct literal {
			using type = obf_literal_ctx<T, C, ObfZeroLiteralContext<T>, seed, literal_cycles>;
		};

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return WhichType::final_injection(x);
		}
		ITHARE_OBF_FORCEINLINE static /*non-constexpr*/ T final_surjection(T y) {
			return WhichType::final_surjection(y);
		}


	public:
#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			size_t dbgWhich = obf_random_obf_from_list(obf_compile_time_prng(seed, 1), cycles, descr);
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext<" << obf_dbgPrintT<T>() << "," << seed << "," << cycles << ">: which=" << which << " dbgWhich=" << dbgWhich << std::endl;
			WhichType::dbgPrint(offset + 1);
		}
#endif
	};

	template<class T, class T0, OBFSEED seed, OBFSEED seed0, OBFCYCLES cycles0,OBFCYCLES cycles>
	struct ObfRecursiveContext<T, ObfLiteralContext<T0, seed0,cycles0>, seed, cycles> {
		using recursive_context_type = ObfLiteralContext<T, obf_compile_time_prng(seed, 1),cycles>;//@@
		using intermediate_context_type = typename ObfLiteralContext<T, obf_compile_time_prng(seed, 2), cycles>;//whenever cycles is low (which is very often), will fallback to version0
	};

	//obf_literal
	template<class T, T C, class Context, OBFSEED seed, OBFCYCLES cycles>
	class obf_literal_ctx {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);

		using Injection = obf_injection<T, Context, obf_compile_time_prng(seed, 1), cycles,ObfDefaultInjectionContext>;
	public:
		ITHARE_OBF_FORCEINLINE constexpr obf_literal_ctx() : val(Injection::injection(C)) {
		}
		ITHARE_OBF_FORCEINLINE T value() const {
			return Injection::surjection(val);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_literal_ctx<" << obf_dbgPrintT<T>() << "," << obf_dbgPrintC(C) << "," << seed << "," << cycles << ">" << std::endl;
			Injection::dbgPrint(offset + 1);
		}
#endif
	private:
		typename Injection::return_type val;
	};

	template<class T_, T_ C_, OBFSEED seed, OBFCYCLES cycles>
	class obf_literal {
		static_assert(std::is_integral<T_>::value);
		using T = typename std::make_unsigned<T_>::type;//from this point on, unsigned only
		static constexpr T C = (T)C_;

		using Context = ObfLiteralContext<T, obf_compile_time_prng(seed, 1),cycles>;
		using Injection = obf_injection<T, Context, obf_compile_time_prng(seed, 2), cycles,ObfDefaultInjectionContext>;
	public:
		ITHARE_OBF_FORCEINLINE constexpr obf_literal() : val(Injection::injection(C)) {
		}
		ITHARE_OBF_FORCEINLINE T value() const {
			return Injection::surjection(val);
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_literal<"<<obf_dbgPrintT<T>()<<"," << C << "," << seed << "," << cycles << ">" << std::endl;
			Injection::dbgPrint(offset + 1);
		}
#endif
	private:
		typename Injection::return_type val;
	};

	//ObfVarContext
	template<class T,OBFSEED seed,OBFCYCLES cycles>
	struct ObfVarContext {
		constexpr static OBFCYCLES context_cycles = 0;
		constexpr static OBFCYCLES calc_cycles(OBFCYCLES inj, OBFCYCLES surj) {
			return inj + surj;//for variables, BOTH injection and surjection are executed in runtime
		}

		constexpr static OBFCYCLES literal_cycles = 50;//TODO: justify (or define?)
		using LiteralContext = ObfLiteralContext<T, seed, literal_cycles>;
		template<class T, T C, OBFSEED seed>
		struct literal {
			using type = obf_literal_ctx<T, C, LiteralContext, seed, literal_cycles>;
		};

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static constexpr T final_surjection(T y) {
			return y;
		}

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfVarContext<" << obf_dbgPrintT<T>() << ">" << std::endl;
		}
#endif
	};
	template<class T, class T0, OBFSEED seed0, OBFCYCLES cycles0, OBFSEED seed, OBFCYCLES cycles>
	struct ObfRecursiveContext<T, ObfVarContext<T0,seed0,cycles0>, seed, cycles> {
		using recursive_context_type = ObfVarContext<T,seed,cycles>;
		using intermediate_context_type = ObfVarContext<T,seed,cycles>;
	};

	//obf_var
	template<class T_, OBFSEED seed, OBFCYCLES cycles>
	class obf_var {
		static_assert(std::is_integral<T_>::value);
		using T = typename std::make_unsigned<T_>::type;//from this point on, unsigned only

		using Context = ObfVarContext<T, obf_compile_time_prng(seed, 1), cycles>;
		using Injection = obf_injection<T, Context, obf_compile_time_prng(seed, 2), cycles, ObfDefaultInjectionContext>;

	public:
		ITHARE_OBF_FORCEINLINE obf_var(T_ t) : val(Injection::injection(T(t))) {
		}
		template<class T2,OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var(obf_var<T2, seed2, cycles2> t) : val(Injection::injection(T(T_(t.value())))) {//TODO: randomized injection implementation
		}//TODO: template<obf_literal>
		ITHARE_OBF_FORCEINLINE obf_var& operator =(T_ t) {
			val = Injection::injection(T(t));//TODO: randomized injection implementation
			return *this;
		}
		template<class T2,OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator =(obf_var<T2, seed2, cycles2> t) {
			val = Injection::injection(T(T_(t.value())));//TODO: randomized injection implementation
			return *this;
		}//TODO: template<obf_literal>
		ITHARE_OBF_FORCEINLINE T_ value() const {
			return T_(Injection::surjection(val));
		}

		ITHARE_OBF_FORCEINLINE operator T_() const { return value(); }
		ITHARE_OBF_FORCEINLINE obf_var& operator ++() { *this = value() + 1; return *this; }
		ITHARE_OBF_FORCEINLINE obf_var& operator --() { *this = value() - 1; return *this; }
		ITHARE_OBF_FORCEINLINE obf_var operator++(int) { obf_var ret = obf_var(value());  *this = value() + 1; return ret; }
		ITHARE_OBF_FORCEINLINE obf_var operator--(int) { obf_var ret = obf_var(value());  *this = value() + 1; return ret; }

		template<class T2>
		ITHARE_OBF_FORCEINLINE bool operator <(T2 t) { return value() < t; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE bool operator >(T2 t) { return value() > t; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE bool operator ==(T2 t) { return value() == t; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE bool operator !=(T2 t) { return value() != t; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE bool operator <=(T2 t) { return value() <= t; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE bool operator >=(T2 t) { return value() >= t; }

		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator <(obf_var<T2, seed2, cycles2> t) {
			return value() < t.value();
		}//TODO: template<obf_literal>(for ALL comparisons)
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator >(obf_var<T2, seed2, cycles2> t) {
			return value() > t.value();
		}
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator ==(obf_var<T2, seed2, cycles2> t) {
			return value() == t.value();
		}
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator !=(obf_var<T2, seed2, cycles2> t) {
			return value() != t.value();
		}
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator <=(obf_var<T2, seed2, cycles2> t) {
			return value() <= t.value();
		}
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator >=(obf_var<T2, seed2, cycles2> t) {
			return value() >= t.value();
		}

		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var& operator +=(T2 t) { *this = value() + t; return *this; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var& operator -=(T2 t) { *this = value() - t; return *this; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var& operator *=(T2 t) { *this = value() * t; return *this; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var& operator /=(T2 t) { *this = value() / t; return *this; }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var& operator %=(T2 t) { *this = value() % t; return *this; }

		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator +=(obf_var<T2, seed2, cycles2> t) {
			return *this += t.value();
		}//TODO: template<obf_literal>(for ALL ?= operations)
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator -=(obf_var<T2, seed2, cycles2> t) {
			return *this -= t.value();
		}
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator *=(obf_var<T2, seed2, cycles2> t) {
			return *this *= t.value();
		}
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator /=(obf_var<T2, seed2, cycles2> t) {
			return *this /= t.value();
		}
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator %=(obf_var<T2, seed2, cycles2> t) {
			return *this %= t.value();
		}

		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var operator +(T2 t) { return obf_var(value()+t); }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var operator -(T2 t) { return obf_var(value() - t); }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var operator *(T2 t) { return obf_var(value() * t); }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var operator /(T2 t) { return obf_var(value() / t); }
		template<class T2>
		ITHARE_OBF_FORCEINLINE obf_var operator %(T2 t) { return obf_var(value() % t); }
		
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>//TODO: template<obf_literal_dbg>(for ALL binary operations)
		ITHARE_OBF_FORCEINLINE obf_var operator +(obf_var<T2,seed2,cycles2> t) { return obf_var(value() + t.value()); }
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator -(obf_var<T2, seed2, cycles2> t) { return obf_var(value() - t.value()); }
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator *(obf_var<T2, seed2, cycles2> t) { return obf_var(value() * t.value()); }
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator /(obf_var<T2, seed2, cycles2> t) { return obf_var(value() / t.value()); }
		template<class T2, OBFSEED seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator %(obf_var<T2, seed2, cycles2> t) { return obf_var(value() % t.value()); }

		//TODO: bitwise

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_var<" << obf_dbgPrintT<T>() << "," << seed <<","<<cycles<<">" << std::endl;
			Injection::dbgPrint(offset+1);
		}
#endif

	private:
		typename Injection::return_type val;
	};

	//external functions
	extern int obf_preMain();
	inline void obf_init() {
		obf_preMain();
	}
#ifdef ITHARE_OBF_ENABLE_DBGPRINT
	inline void obf_dbgPrint() {
		std::cout << "OBF_CONST_A=" << int(OBF_CONST_A) << " OBF_CONST_B=" << int(OBF_CONST_B) << " OBF_CONST_C=" << int(OBF_CONST_C) << std::endl;
		//auto c = obf_const_x(obf_compile_time_prng(ITHARE_OBF_SEED^UINT64_C(0xfb2de18f982a2d55), 1), obf_const_C_excluded);
	}
#endif

	//USER-LEVEL:
	/*think about it further //  obfN<> templates
	template<class T,OBFSEED seed>
	class obf0 {
		using Base = obf_var<T, seed, obf_exp_cycles(0)>;

	public:
		obf0() : val() {}
		obf0(T x) : val(x) {}
		obf0 operator =(T x) { val = x; return *this; }
		operator T() const { return val.value(); }

	private:
		Base val;
	};*/

}//namespace obf
}//namespace ithare

 //macros; DON'T belong to the namespace...
#ifdef _MSC_VER
 //direct use of __LINE__ doesn't count as constexpr in MSVC - don't ask why...

 //along the lines of https://stackoverflow.com/questions/19343205/c-concatenating-file-and-line-macros:
#define ITHARE_OBF_S1(x) #x
#define ITHARE_OBF_S2(x) ITHARE_OBF_S1(x)
#define ITHARE_OBF_LOCATION __FILE__ " : " ITHARE_OBF_S2(__LINE__)

#define ITHARE_OBF0(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(ITHARE_OBF_LOCATION,0),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0)>
#define ITHARE_OBF1(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(ITHARE_OBF_LOCATION,0),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1)>
#define ITHARE_OBF2(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(ITHARE_OBF_LOCATION,0),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2)>
#define ITHARE_OBF3(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(ITHARE_OBF_LOCATION,0),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3)>
#define ITHARE_OBF4(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(ITHARE_OBF_LOCATION,0),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4)>
#define ITHARE_OBF5(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(ITHARE_OBF_LOCATION,0),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5)>
#define ITHARE_OBF6(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(ITHARE_OBF_LOCATION,0),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6)>
#else//_MSC_VER
#define ITHARE_OBF0(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(__FILE__,__LINE__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0)>
#define ITHARE_OBF1(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(__FILE__,__LINE__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1)>
#define ITHARE_OBF2(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(__FILE__,__LINE__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2)>
#define ITHARE_OBF3(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(__FILE__,__LINE__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3)>
#define ITHARE_OBF4(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(__FILE__,__LINE__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4)>
#define ITHARE_OBF5(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(__FILE__,__LINE__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5)>
#define ITHARE_OBF6(type) ithare::obf::obf_var<type,ithare::obf::obf_seed_from_file_line(__FILE__,__LINE__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6)>
#endif

#else//ITHARE_OBF_SEED
namespace ithare {
	namespace obf {
#ifdef ITHARE_OBF_ENABLE_DBGPRINT
		//dbgPrint helpers
		template<class T>
		std::string obf_dbgPrintT() {
			return std::string("T(sizeof=") + std::to_string(sizeof(T)) + ")";
		}

		inline void obf_dbgPrint() {
		}
#endif

		//obf_var_dbg
		template<class T>
		class obf_var_dbg {
			static_assert(std::is_integral<T>::value);

		public:
			obf_var_dbg(T t) : val(t) {
			}
			template<class T, class T2>
			obf_var_dbg(obf_var_dbg<T2> t) : val(T(t.value())) {
			}
			obf_var_dbg& operator =(T t) {
				val = t;
				return *this;
			}
			template<class T, class T2>
			obf_var_dbg& operator =(obf_var_dbg<T2> t) {
				val = T(t.value());
				return *this;
			}
			T value() const {
				return val;
			}

			operator T() const { return value(); }
			obf_var_dbg& operator ++() { *this = value() + 1; return *this; }
			obf_var_dbg& operator --() { *this = value() - 1; return *this; }
			obf_var_dbg operator++(int) { obf_var_dbg ret = obf_var_dbg(value());  *this = value() + 1; return ret; }
			obf_var_dbg operator--(int) { obf_var_dbg ret = obf_var_dbg(value());  *this = value() + 1; return ret; }

			template<class T2>
			bool operator <(T2 t) { return value() < t; }
			template<class T2>
			bool operator >(T2 t) { return value() > t; }
			template<class T2>
			bool operator ==(T2 t) { return value() == t; }
			template<class T2>
			bool operator !=(T2 t) { return value() != t; }
			template<class T2>
			bool operator <=(T2 t) { return value() <= t; }
			template<class T2>
			bool operator >=(T2 t) { return value() >= t; }

			template<class T2>
			bool operator <(obf_var_dbg<T2> t) {
				return value() < t.value();
			}//TODO: template<obf_literal_dbg>(for ALL comparisons)
			template<class T2>
			bool operator >(obf_var_dbg<T2> t) {
				return value() > t.value();
			}
			template<class T2>
			bool operator ==(obf_var_dbg<T2> t) {
				return value() == t.value();
			}
			template<class T2>
			bool operator !=(obf_var_dbg<T2> t) {
				return value() != t.value();
			}
			template<class T2>
			bool operator <=(obf_var_dbg<T2> t) {
				return value() <= t.value();
			}
			template<class T2>
			bool operator >=(obf_var_dbg<T2> t) {
				return value() >= t.value();
			}

			template<class T2>
			obf_var_dbg& operator +=(T2 t) { *this = value() + t; return *this; }
			template<class T2>
			obf_var_dbg& operator -=(T2 t) { *this = value() - t; return *this; }
			template<class T2>
			obf_var_dbg& operator *=(T2 t) { *this = value() * t; return *this; }
			template<class T2>
			obf_var_dbg& operator /=(T2 t) { *this = value() / t; return *this; }
			template<class T2>
			obf_var_dbg& operator %=(T2 t) { *this = value() % t; return *this; }

			template<class T2>
			obf_var_dbg& operator +=(obf_var_dbg<T2> t) {
				return *this += t.value();
			}//TODO: template<obf_literal_dbg>(for ALL ?= operations)
			template<class T2>
			obf_var_dbg& operator -=(obf_var_dbg<T2> t) {
				return *this -= t.value();
			}
			template<class T2>
			obf_var_dbg& operator *=(obf_var_dbg<T2> t) {
				return *this *= t.value();
			}
			template<class T2>
			obf_var_dbg& operator /=(obf_var_dbg<T2> t) {
				return *this /= t.value();
			}
			template<class T2>
			obf_var_dbg& operator %=(obf_var_dbg<T2> t) {
				return *this %= t.value();
			}

			template<class T2>
			obf_var_dbg operator +(T2 t) { return obf_var_dbg(value() + t); }
			template<class T2>
			obf_var_dbg operator -(T2 t) { return obf_var_dbg(value() - t); }
			template<class T2>
			obf_var_dbg operator *(T2 t) { return obf_var_dbg(value() * t); }
			template<class T2>
			obf_var_dbg operator /(T2 t) { return obf_var_dbg(value() / t); }
			template<class T2>
			obf_var_dbg operator %(T2 t) { return obf_var_dbg(value() % t); }

			template<class T2>//TODO: template<obf_literal_dbg>(for ALL binary operations)
			obf_var_dbg operator +(obf_var_dbg<T2> t) { return obf_var_dbg(value() + t.value()); }
			template<class T2>
			obf_var_dbg operator -(obf_var_dbg<T2> t) { return obf_var_dbg(value() - t.value()); }
			template<class T2>
			obf_var_dbg operator *(obf_var_dbg<T2> t) { return obf_var_dbg(value() * t.value()); }
			template<class T2>
			obf_var_dbg operator /(obf_var_dbg<T2> t) { return obf_var_dbg(value() / t.value()); }
			template<class T2>
			obf_var_dbg operator %(obf_var_dbg<T2> t) { return obf_var_dbg(value() % t.value()); }

			//TODO: bitwise

#ifdef ITHARE_OBF_ENABLE_DBGPRINT
			static void dbgPrint(size_t offset = 0) {
				std::cout << std::string(offset, ' ') << "obf_var_dbg<" << obf_dbgPrintT<T>() << ">" << std::endl;
			}
#endif

		private:
			typename T val;
		};

		inline void obf_init() {
		}

	}//namespace obf
}//namespace ithare

#define ITHARE_OBF0(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF1(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF2(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF3(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF4(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF5(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF6(type) ithare::obf::obf_var_dbg<type>

#endif //ITHARE_OBF_SEED

#ifndef ITHARE_OBF_NO_SHORT_DEFINES//#define to avoid polluting global namespace w/o prefix
#define OBF0 ITHARE_OBF0
#define OBF1 ITHARE_OBF1
#define OBF2 ITHARE_OBF2
#define OBF3 ITHARE_OBF3
#define OBF4 ITHARE_OBF4
#define OBF5 ITHARE_OBF5
#define OBF6 ITHARE_OBF6
#endif

#endif//ithare_obf_obfuscate_h_included
