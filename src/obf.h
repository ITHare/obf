#ifndef ithare_obf_obfuscate_h_included
#define ithare_obf_obfuscate_h_included

#include "impl/obf_common.h"
#include "impl/obf_prng.h"

//Usage: 
//  1. Use OBF?() throughout your code to indicate the need for obfuscation
//  1a. OBFX() roughly means 'add no more than 10^(X/2) CPU cycles for obfuscation of this variable'
//      i.e. OBF2() adds up to 10 CPU cycles, OBF3() - up to 30 CPU cycles, 
//           and OBF5() - up to 300 CPU cycles
//  1b. To obfuscate literals, use OBF?I() (for integral literals) and OBF?S() (for string literals)
//  2. compile your code without -DITHARE_OBF_SEED for debugging and during development
//  3. compile with -DITHARE_OBF_SEED=0x<really-random-64-bit-seed> for deployments

#ifdef ITHARE_OBF_INTERNAL_DBG
//enable assert() in Release
//#undef NDEBUG
#endif

//#include <atomic>//for ITHARE_OBF_STRICT_MT

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

		using OBFCYCLES = int32_t;//signed!

/* - obsolete?
#ifdef ITHARE_OBF_DBG_MAP
#ifdef ITHARE_OBF_DBG_MAP_LOG
#define ITHARE_OBF_DBG_MAP_ADD(where,dbg_map, from,to) 	do {\
		if constexpr(!InjectionRequirements::is_constexpr)\
			std::cout << "MAP_ADD @" << where << "(seed=" << obf_dbgPrintSeed<seed>() << "): " << from << "=>" << to << std::endl; \
		} while(false)
#define ITHARE_OBF_DBG_MAP_CHECK(where,dbg_map,from,to,whoToPrint) do {\
			if constexpr(!InjectionRequirements::is_constexpr) {\
				std::cout << "MAP_CHECK @" << where << "(seed=" << obf_dbgPrintSeed<seed>() << "): " << from << "=>" << to << std::endl; \
			}\
		} while(false)
#define ITHARE_OBF_DBG_CHECK_LITERAL(where, val, c,whoToPrint) do {\
			if constexpr(!InjectionRequirements::is_constexpr) {\
				if (val != c) {\
					std::cout << "DBG_CHECK_LITERAL ERROR @" << where << ": " << val << "!=" << c << std::endl; \
					whoToPrint dbgPrint(); \
					abort(); \
				}\
			}\
		}while (false)
#else
#define ITHARE_OBF_DEFINE_DBG_MAP
#define ITHARE_OBF_DBG_MAP_ADD(where,dbg_map, from,to) 	do {\
		if constexpr(!InjectionRequirements::is_constexpr)\
			dbg_map.insert(std::make_pair(from,to));\
		} while(false)
#define ITHARE_OBF_DBG_MAP_CHECK(where,dbg_map,from,to,whoToPrint) do {\
			if constexpr(!InjectionRequirements::is_constexpr) {\
				auto found = dbg_map.find(from); \
				if (found == dbg_map.end() ) {\
					std::cout << "DBG_MAP ERROR @" << where << ": " << from << " NOT FOUND" << std::endl;\
					whoToPrint dbgPrint();\
					abort();\
				}\
				auto dbgVal = (*found).second;\
				if (dbgVal != to) {\
					std::cout << "DBG_MAP ERROR @" << where << ": " << to << "!=" << dbgVal << std::endl; \
				}\
			}\
		} while(false)
#define ITHARE_OBF_DBG_CHECK_LITERAL(where, val, c,whoToPrint) do {\
			if constexpr(!InjectionRequirements::is_constexpr) {\
				if (val != c) {\
					std::cout << "DBG_CHECK_LITERAL ERROR @" << where << ": " << val << "!=" << c << std::endl; \
					whoToPrint dbgPrint(); \
					abort(); \
				}\
			}\
		}while (false)
#endif//!_LOG
#else
#define ITHARE_OBF_DBG_MAP_ADD(where,dbg_map, from,to)
#define ITHARE_OBF_DBG_MAP_CHECK(where,dbg_map,from,to,whoToPrint)
//#define ITHARE_OBF_DBG_CHECK_LITERAL(where, val, c,whoToPrint)
#endif
*/

#ifdef ITHARE_OBF_DBG_RUNTIME_CHECKS
#define ITHARE_OBF_DBG_ENABLE_DBGPRINT//necessary for checks to work
#define ITHARE_OBF_DBG_ASSERT_SURJECTION(where,x,y) do {\
			if (surjection(y) != x) {\
				std::cout << "DBG_ASSERT_SURJECTION FAILED @" << where << ": injection(" << x << ")=" << y << " but surjection(" << y << ") = " << surjection(y) << " != " << x << std::endl; \
				dbgPrint(); \
				abort(); \
			}\
		} while(false)
#define ITHARE_OBF_DBG_CHECK_LITERAL(where, val, c) do {\
			if (val.value() != c) {\
				std::cout << "DBG_CHECK_LITERAL ERROR @" << where << ": " << val.value() << "!=" << c << std::endl; \
				val.dbgCheck();\
				dbgPrint(); \
				abort(); \
			}\
		}while (false)

#else
#define ITHARE_OBF_DBG_ASSERT_SURJECTION(where,x,y)
#define ITHARE_OBF_DBG_CHECK_LITERAL(where, val, c)
#endif//ITHARE_OBF_DBG_RUNTIME_CHECKS

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
		constexpr size_t obf_strlen(const char* s) {
			for (size_t ret = 0; ; ++ret, ++s)
				if (*s == 0)
					return ret;
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
			assert(false);
			return T(0);
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

		template<ITHARE_OBF_SEEDTPARAM seed, size_t N>
		constexpr size_t obf_random_from_list(std::array<size_t, N> weights) {
			//returns index in weights
			size_t totalWeight = 0;
			for (size_t i = 0; i < weights.size(); ++i)
				totalWeight += weights[i];
			assert(totalWeight > 0);
			assert(uint32_t(totalWeight) == totalWeight);
			size_t refWeight = ITHARE_OBF_RANDOM(seed, 1, uint32_t(totalWeight));
			assert(refWeight < totalWeight);
			for (size_t i = 0; i < weights.size(); ++i) {
				if (refWeight < weights[i])
					return i;
				refWeight -= weights[i];
			}
			assert(false);
			return size_t(-1);
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

		template<ITHARE_OBF_SEEDTPARAM seed, size_t N>
		constexpr uint8_t obf_const_x(std::array<uint8_t, N> excluded) {
			std::array<uint8_t, 6> candidates = { 3,5,7,15,25,31 };//only odd, to allow using the same set of constants in mul-by-odd injections
			std::array<size_t, 6> weights = { 100,100,100,100,100,100 };
			for (size_t i = 0; i < N; ++i) {
				size_t found = obf_find_idx_in_array(candidates, excluded[i]);
				if (found != size_t(-1))
					weights[found] = 0;
			}
			size_t idx2 = obf_random_from_list<seed>(weights);
			return candidates[idx2];
		}

		//XOR-ed constants are merely random numbers with no special meaning
		constexpr std::array<uint8_t, 0> obf_const_A_excluded = {};
		constexpr uint8_t OBF_CONST_A = obf_const_x<ITHARE_OBF_INIT_PRNG(__FILE__, __LINE__, __COUNTER__)>(obf_const_A_excluded);
		constexpr std::array<uint8_t, 1> obf_const_B_excluded = { OBF_CONST_A };
		constexpr uint8_t OBF_CONST_B = obf_const_x<ITHARE_OBF_INIT_PRNG(__FILE__, __LINE__, __COUNTER__)>(obf_const_B_excluded);
		constexpr std::array<uint8_t, 2> obf_const_C_excluded = { OBF_CONST_A, OBF_CONST_B };
		constexpr uint8_t OBF_CONST_C = obf_const_x<ITHARE_OBF_INIT_PRNG(__FILE__, __LINE__, __COUNTER__)>(obf_const_C_excluded);

		template<ITHARE_OBF_SEEDTPARAM seed, class T, size_t N>
		constexpr T obf_random_const(std::array<T, N> lst) {
			return lst[ITHARE_OBF_RANDOM(seed, 1, N)];
		}

		struct ObfDescriptor {
			bool is_recursive;
			OBFCYCLES min_cycles;
			uint32_t weight;

			constexpr ObfDescriptor(bool is_recursive_, OBFCYCLES min_cycles_, uint32_t weight_)
				: is_recursive(is_recursive_), min_cycles(min_cycles_), weight(weight_) {
			}
		};

		template<ITHARE_OBF_SEEDTPARAM seed, size_t N>
		constexpr size_t obf_random_obf_from_list(OBFCYCLES cycles, std::array<ObfDescriptor, N> descr, size_t exclude_version = size_t(-1)) {
			//returns index in descr
			size_t sz = descr.size();
			std::array<size_t, N> nr_weights = {};
			std::array<size_t, N> r_weights = {};
			size_t sum_r = 0;
			size_t sum_nr = 0;
			for (size_t i = 0; i < sz; ++i) {
				if (i != exclude_version && cycles >= descr[i].min_cycles) {
					if (descr[i].is_recursive) {
						r_weights[i] = descr[i].weight;
						sum_r += r_weights[i];
					}
					else {
						nr_weights[i] = descr[i].weight;
						sum_nr += nr_weights[i];
					}
				}
			}
			if (sum_r)
				return obf_random_from_list<seed>(r_weights);
			else {
				assert(sum_nr > 0);
				return obf_random_from_list<seed>(nr_weights);
			}
		}

		template<ITHARE_OBF_SEEDTPARAM seed, size_t N>
		constexpr std::array<OBFCYCLES, N> obf_random_split(OBFCYCLES cycles, std::array<ObfDescriptor, N> elements) {
			//size_t totalWeight = 0;
			assert(cycles >= 0);
			OBFCYCLES mins = 0;
			for (size_t i = 0; i < N; ++i) {
				assert(elements[i].min_cycles == 0);//mins NOT to be used within calls to obf_random_split 
												  //  (it not a problem with this function, but they tend to cause WAY too much trouble in injection_version<> code
				mins += elements[i].min_cycles;
				//totalWeight += elements[i].weight;
			}
			OBFCYCLES leftovers = cycles - mins;
			assert(leftovers >= 0);
			std::array<OBFCYCLES, N> ret = {};
			size_t totalWeight = 0;
			for (size_t i = 0; i < N; ++i) {
				if (elements[i].weight > 0)
					ret[i] = OBFCYCLES(ITHARE_OBF_RANDOM(seed, int(i + 1), elements[i].weight)) + 1;//'+1' is to avoid "all-zeros" case
				else
					ret[i] = 0;
				totalWeight += ret[i];
			}
			size_t totalWeight2 = 0;
			double q = double(leftovers) / double(totalWeight);
			for (size_t i = 0; i < N; ++i) {
				ret[i] = elements[i].min_cycles + OBFCYCLES(double(ret[i]) * double(q));
				assert(ret[i] >= elements[i].min_cycles);
				totalWeight2 += ret[i];
			}
			assert(OBFCYCLES(totalWeight2) <= mins + leftovers);
			return ret;
		}

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
			constexpr static bool which = sizeof(T) > sizeof(T2);
			using type = typename obf_select_type<which, T, T2>::type;
		};

		//ObfTraits<>
		template<class T>
		struct ObfTraits;

		template<>
		struct ObfTraits<uint64_t> {
			static constexpr bool is_built_in = true;
			static std::string type_name() { return "uint64_t"; }
			using signed_type = int64_t;
			using literal_type = uint64_t;

			static constexpr bool has_half_type = true;
			using HalfT = uint32_t;
			using UintT = typename obf_larger_type<uint64_t, unsigned>::type;//UintT is a type to cast to, to avoid idiocies like uint16_t*uint16_t being promoted to signed(!) int, and then overflowing to cause UB
			static constexpr size_t nbits = 64;
		};

		template<>
		struct ObfTraits<uint32_t> {
			static constexpr bool is_built_in = true;
			static std::string type_name() { return "uint32_t"; }
			using signed_type = int32_t;
			using literal_type = uint32_t;

			static constexpr bool has_half_type = true;
			using HalfT = uint16_t;
			using UintT = typename obf_larger_type<uint32_t, unsigned>::type;
			static constexpr size_t nbits = 32;
		};

		template<>
		struct ObfTraits<uint16_t> {
			static constexpr bool is_built_in = true;
			static std::string type_name() { return "uint16_t"; }
			using signed_type = int16_t;
			using literal_type = uint16_t;

			static constexpr bool has_half_type = true;
			using HalfT = uint8_t;
			using UintT = typename obf_larger_type<uint16_t, unsigned>::type;
			static constexpr size_t nbits = 16;
		};

		template<>
		struct ObfTraits<uint8_t> {
			static constexpr bool is_built_in = true;
			static std::string type_name() { return "uint8_t"; }
			using signed_type = int8_t;
			using literal_type = uint8_t;

			static constexpr bool has_half_type = false;
			using UintT = typename obf_larger_type<uint8_t, unsigned>::type;
			static constexpr size_t nbits = 8;
		};

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
	//dbgPrint helpers
	template<class T>
	std::string obf_dbgPrintT() {
		return ObfTraits<T>::type_name();
	}

	//fwd decl
	template<size_t N>
	class ObfBitUint;

	template<class T>
	struct ObfPrintC {
		using type = T;
	};
	template<>
	struct ObfPrintC<uint8_t> {
		using type = int;
	};
	template<size_t N>
	struct ObfPrintC<ObfBitUint<N>> {
		using type = typename ObfPrintC<typename ObfBitUint<N>::T>::type;
	};

	template<class T>
	typename ObfPrintC<T>::type obf_dbgPrintC(T c) {
		return typename ObfPrintC<T>::type(c);
	}
#endif

		constexpr size_t obf_smallest_uint_size(size_t n) {
			assert(n <= 64);
			if (n <= 8)
				return 8;
			else if (n <= 16)
				return 16;
			else if (n <= 32)
				return 32;
			else {
				assert(n <= 64);
				return 64;
			}
		}

		template<size_t N>
		struct obf_uint_by_size;
		template<>
		struct obf_uint_by_size<8> { using type = uint8_t; };
		template<>
		struct obf_uint_by_size<16> { using type = uint16_t; };
		template<>
		struct obf_uint_by_size<32> { using type = uint32_t; };
		template<>
		struct obf_uint_by_size<64> { using type = uint64_t; };

		template<class T>
		constexpr T obf_mask(size_t n) {
			assert(n <= sizeof(T) * 8);
			if (n == sizeof(T) * 8)
				return T(-1);
			else
				return (T(1) << n ) - T(1);
		}

		template<size_t N_>
		class ObfBitUint {
		public:
			static constexpr size_t N = N_;
			using T = typename obf_uint_by_size<obf_smallest_uint_size(N)>::type;
			static_assert(N <= sizeof(T) * 8);

		private:
			static constexpr T mask = obf_mask<T>(N);

		public:
			constexpr ITHARE_OBF_FORCEINLINE ObfBitUint() : val(0) {}
			constexpr ITHARE_OBF_FORCEINLINE ObfBitUint(T x) : val(x & mask) {}
			//constexpr ObfBitUint(const ObfBitUint& other) : val(other.val) {}
			//constexpr ObfBitUint(const volatile ObfBitUint& other) : val(other.val) {}
			constexpr ITHARE_OBF_FORCEINLINE operator T() const { assert((val&mask) == val); return val & mask; }

			constexpr ITHARE_OBF_FORCEINLINE ObfBitUint operator *(ObfBitUint x) const { return ObfBitUint(val * x.val); }
			constexpr ITHARE_OBF_FORCEINLINE ObfBitUint operator +(ObfBitUint x) const { return ObfBitUint(val + x.val); }
			constexpr ITHARE_OBF_FORCEINLINE ObfBitUint operator -(ObfBitUint x) const { return ObfBitUint(val - x.val); }
			constexpr ITHARE_OBF_FORCEINLINE ObfBitUint operator %(ObfBitUint x) const { return ObfBitUint(val%x.val); }
			constexpr ITHARE_OBF_FORCEINLINE ObfBitUint operator /(ObfBitUint x) const { return ObfBitUint(val / x.val); }

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0,const char* prefix="") {
			std::cout << std::string(offset, ' ') << prefix << "ObfBitUint<" << N << ">: mask =" << obf_dbgPrintC<T>(mask) << std::endl;
		}
#endif

		private:
			T val;
		};

		template<size_t N_>
		class ObfBitSint {
		public:
			static constexpr size_t N = N_;
			using UT = typename obf_uint_by_size<obf_smallest_uint_size(N)>::type;
			using T = typename std::make_signed<UT>::type;
			static_assert(N <= sizeof(T) * 8);
			static_assert(sizeof(T) == sizeof(typename ObfBitUint<N_>::T));

		private:
			static constexpr UT high = UT(UT(1) << N);
			static constexpr UT mask = obf_mask<UT>(N);

		public:
			constexpr ObfBitSint() : val(0) {}
			constexpr ObfBitSint(T x) : val(UT(x)&mask) {}
			constexpr operator T() const { return T(val & mask); }

			constexpr ObfBitSint operator -() const { return ObfBitSint(val^high); }

		private:
			UT val;
		};

		template<size_t N>
		struct ObfTraits<ObfBitUint<N>> {
		private:
			using TT = ObfBitUint<N>;
		public:
			static constexpr bool is_built_in = false;
			static std::string type_name() {
				return std::string("ObfBitUint<") + std::to_string(N) + ">";
			}
			using signed_type = ObfBitSint<N>;
			using literal_type = typename TT::T;

			static constexpr bool has_half_type = false;
			using UintT = typename obf_larger_type<typename TT::T, unsigned>::type;
			static constexpr size_t nbits = N;
		};

	//forward declarations
	template<class T, class Context, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles,class InjectionRequirements>
	class obf_injection;
	template<class T, T C, class Context, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_literal_ctx;
	template<class T_, T_ C_, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_literal;

	//ObfRecursiveContext
	template<class T, class Context, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct ObfRecursiveContext;

	//injection-with-constant - building block both for injections and for literals
	template <size_t which, class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
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

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<0, T, Context, InjectionRequirements, seed, cycles> {
		using Traits = ObfTraits<T>;

		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version0_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

	public:
		using return_type = T;
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			return_type ret = Context::final_injection(x);
			ITHARE_OBF_DBG_ASSERT_SURJECTION("<0>", x, ret);
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			return Context::final_surjection(y);
		}

		static constexpr bool has_add_mod_max_value_ex = false;

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0,const char* prefix="") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<0/*identity*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">: availCycles=" << availCycles << std::endl;
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

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<1, T, Context, InjectionRequirements, seed, cycles> {
		using Traits = ObfTraits<T>;
	public:
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version1_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		struct RecursiveInjectionRequirements {
			static constexpr size_t exclude_version = 1;
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = InjectionRequirements::only_bijections;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};

		using RecursiveInjection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 1), availCycles+Context::context_cycles,RecursiveInjectionRequirements>;
		using return_type = typename RecursiveInjection::return_type;
		static constexpr std::array<T, 5> consts = { 0, 1, OBF_CONST_A, OBF_CONST_B, OBF_CONST_C };
		constexpr static T C = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 2)>(consts);
		static constexpr bool neg = C == 0 ? true : ITHARE_OBF_RANDOM(seed, 3, 2) == 0;
		using ST = typename Traits::signed_type;

#ifdef ITHARE_OBF_DEFINE_DBG_MAP
		static std::map<T,T> dbg_map;
		static std::map<T,return_type> dbg_map_r;
#endif

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			if constexpr(neg) {
				ST sx = ST(x);
				auto y = T(-sx) + C;
				//ITHARE_OBF_DBG_MAP_ADD("<1>/ret",dbg_map, x,y);
				return_type ret = RecursiveInjection::injection(y);
				//ITHARE_OBF_DBG_MAP_ADD("<1>/r",dbg_map_r, y, ret);
				ITHARE_OBF_DBG_ASSERT_SURJECTION("<1>/a",x,ret);
				return ret;
			}
			else {
				T y = x + C;
				//ITHARE_OBF_DBG_MAP_ADD("<1>/ret",dbg_map, x,y);
				return_type ret = RecursiveInjection::injection(y);
				//ITHARE_OBF_DBG_MAP_ADD("<1>/r",dbg_map_r, y, ret);
				ITHARE_OBF_DBG_ASSERT_SURJECTION("<1>/b", x, ret);
				return ret;
			}
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			T yy0 = RecursiveInjection::surjection(y);
			//ITHARE_OBF_DBG_MAP_CHECK("<1>/r", dbg_map_r, yy0,y,RecursiveInjection::);
			T yy = yy0-C;
			if constexpr(neg) {
				ST syy = ST(yy);
				T ret = T(-syy);
				//ITHARE_OBF_DBG_MAP_CHECK("<1>/ret(a)", dbg_map, ret,yy0,);
				return ret;
			}
			else {
				//ITHARE_OBF_DBG_MAP_CHECK("<1>/ret(b)", dbg_map, yy,yy0,);
				return yy;
			}
		}

		static constexpr bool has_add_mod_max_value_ex = false;
		/*@@! ITHARE_OBF_FORCEINLINE constexpr static T injected_add_mod_max_value_ex(T base,T x) {
			if constexpr(RecursiveInjection::has_add_mod_max_value_ex) {
				if constexpr(neg) {
					//return injection(surjection(base) + x);
					//return RecursiveInjection::injection(RecursiveInjection::surjection(base) - x);
					return RecursiveInjection::injected_add_mod_max_value_ex(base, -ST(x));
				}
				else {
					//auto noShortcut = RecursiveInjection::injection(RecursiveInjection::surjection(base) + x);
					auto ret = RecursiveInjection::injected_add_mod_max_value_ex(base, x);
					//assert(ret == noShortcut);
					return ret;
				}
			}
			else {
				if constexpr(neg) {
					//return injection(surjection(base) + x);
					return RecursiveInjection::injection(RecursiveInjection::surjection(base) - x);
				}
				else {
					return RecursiveInjection::injection(RecursiveInjection::surjection(base) + x);
				}
			}
			//return base + x;//sic! - no C involved
		}*/

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<1/*add mod 2^N*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">: C=" << obf_dbgPrintC<T>(C) << " neg=" << neg << std::endl;
			RecursiveInjection::dbgPrint(offset + 1);
		}
#endif
	};

#ifdef ITHARE_OBF_DEFINE_DBG_MAP
	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	std::map<T, T> obf_injection_version<1, T, Context, InjectionRequirements, seed, cycles>::dbg_map = {};
	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	std::map<T, typename obf_injection_version<1, T, Context, InjectionRequirements, seed, cycles>::return_type> obf_injection_version<1, T, Context, InjectionRequirements, seed, cycles>::dbg_map_r = {};
#endif

	//helper for Feistel-like: randomized_non_reversible_function 
	template<size_t which, class T, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version;
	//IMPORTANT: Feistel-like non-reversible functions SHOULD be short, to avoid creating code 'signatures'
	//  Therefore, currently we're NOT using any recursions here

	struct obf_randomized_non_reversible_function_version0_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(false, 0, 100);
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version<0, T, seed, cycles> {
		constexpr ITHARE_OBF_FORCEINLINE T operator()(T x) {
			return x;
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<0/*identity*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
		}
#endif		
	};

	struct obf_randomized_non_reversible_function_version1_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 3, 100);
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version<1,T,seed,cycles> {
		using UintT = typename ObfTraits<T>::UintT;
		constexpr ITHARE_OBF_FORCEINLINE T operator()(T x) {
			return UintT(x)*UintT(x);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<1/*x^2*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
		}
#endif		
	};

	struct obf_randomized_non_reversible_function_version2_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 7, 100);
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function_version<2, T, seed, cycles> {
		using ST = typename std::make_signed<T>::type;
		constexpr ITHARE_OBF_FORCEINLINE T operator()(T x) {
			ST sx = ST(x);
			return T(sx < 0 ? -sx : sx);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<2/*abs*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
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

	template<class T, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct obf_randomized_non_reversible_function {
		constexpr static std::array<ObfDescriptor, 3> descr{
			obf_randomized_non_reversible_function_version0_descr::descr,
			obf_randomized_non_reversible_function_version1_descr::descr,
			obf_randomized_non_reversible_function_version2_descr::descr,
		};
		constexpr static size_t max_cycles_that_make_sense = obf_max_min_descr(descr);
		constexpr static size_t which = obf_random_obf_from_list<ITHARE_OBF_NEW_PRNG(seed, 1)>(cycles, descr);
		using FType = obf_randomized_non_reversible_function_version<which, T, seed, cycles>;
		constexpr ITHARE_OBF_FORCEINLINE T operator()(T x) {
			return FType()(x);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_randomized_non_reversible_function<" << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">: which=" << which << std::endl;
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
			ObfTraits<T>::has_half_type ?
			ObfDescriptor(true, own_min_cycles, 100) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<2, T, Context, InjectionRequirements, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version2_descr<T,Context>::own_min_cycles;
		static_assert(availCycles >= 0);
		constexpr static std::array<ObfDescriptor, 2> split {
			ObfDescriptor(true,0,100),//f() 
			ObfDescriptor(true,0,100)//RecursiveInjection
		};
		static constexpr auto splitCycles = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 1)>(availCycles, split);
		static constexpr OBFCYCLES cycles_f0 = splitCycles[0];
		static constexpr OBFCYCLES cycles_rInj0 = splitCycles[1];
		static_assert(cycles_f0 + cycles_rInj0 <= availCycles);

		//doesn't make sense to use more than max_cycles_that_make_sense cycles for f...
		static constexpr OBFCYCLES max_cycles_that_make_sense = obf_randomized_non_reversible_function<T, ITHARE_OBF_DUMMY_PRNG, 0>::max_cycles_that_make_sense;
		static constexpr OBFCYCLES delta_f = cycles_f0 > max_cycles_that_make_sense ? cycles_f0 - max_cycles_that_make_sense : 0;
		static constexpr OBFCYCLES cycles_f = cycles_f0 - delta_f;
		static constexpr OBFCYCLES cycles_rInj = cycles_rInj0 + delta_f;
		static_assert(cycles_f + cycles_rInj <= availCycles);

		struct RecursiveInjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = InjectionRequirements::only_bijections;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};

		using RecursiveInjection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 2), cycles_rInj+ Context::context_cycles,RecursiveInjectionRequirements>;
		using return_type = typename RecursiveInjection::return_type;

		using halfT = typename ObfTraits<T>::HalfT;
		using FType = obf_randomized_non_reversible_function<halfT, ITHARE_OBF_NEW_PRNG(seed, 3), cycles_f>;

		constexpr static int halfTBits = sizeof(halfT) * 8;
		//constexpr static T mask = ((T)1 << halfTBits) - 1;
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			T lo = x >> halfTBits;
			//T hi = (x & mask) + f((halfT)lo);
			T hi = x + f((halfT)lo);
			return_type ret = RecursiveInjection::injection((hi << halfTBits) + lo);
			ITHARE_OBF_DBG_ASSERT_SURJECTION("<2>", x, ret);
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y_) {
			T y = RecursiveInjection::surjection(y_);
			halfT hi = y >> halfTBits;
			T lo = y;
			//T z = (hi - f((halfT)lo)) & mask;
			halfT z = (hi - f((halfT)lo));
			return z + (lo << halfTBits);
		}

		static constexpr bool has_add_mod_max_value_ex = false;

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<2/*kinda-Feistel*/,"<<obf_dbgPrintT<T>()<<"," << obf_dbgPrintSeed<seed>() << "," << cycles << ">:" 
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
			ObfTraits<T>::has_half_type ?
			ObfDescriptor(true, own_min_cycles, 100) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<3, T, Context, InjectionRequirements, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		//TODO:split-join based on union
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version3_descr<T, Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		using halfT = typename ObfTraits<T>::HalfT;
		constexpr static int halfTBits = sizeof(halfT) * 8;

		constexpr static std::array<ObfDescriptor, 3> split{
			ObfDescriptor(true,0,200),//RecursiveInjection
			ObfDescriptor(true,0,100),//LoInjection
			ObfDescriptor(true,0,100),//HiInjection
		};
		static constexpr auto splitCycles = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 1)>(availCycles, split);
		static constexpr OBFCYCLES cycles_rInj = splitCycles[0];
		static constexpr OBFCYCLES cycles_lo = splitCycles[1];
		static constexpr OBFCYCLES cycles_hi = splitCycles[2];
		static_assert(cycles_rInj + cycles_lo + cycles_hi <= availCycles);

		struct RecursiveInjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = InjectionRequirements::only_bijections;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};
		using RecursiveInjection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 2), cycles_rInj+ Context::context_cycles, RecursiveInjectionRequirements>;
		using return_type = typename RecursiveInjection::return_type;

		struct LoHiInjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = true;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};

		constexpr static std::array<ObfDescriptor, 2> splitLo {
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesLo = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 2)>(cycles_lo, splitLo);
		static constexpr OBFCYCLES cycles_loCtx = splitCyclesLo[0];
		static constexpr OBFCYCLES cycles_loInj = splitCyclesLo[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using LoContext = typename ObfRecursiveContext < halfT, Context, ITHARE_OBF_NEW_PRNG(seed, 3), cycles_loCtx>::intermediate_context_type;
		using LoInjection = obf_injection<halfT, LoContext, ITHARE_OBF_NEW_PRNG(seed, 4), cycles_loInj+LoContext::context_cycles, LoHiInjectionRequirements>;
		static_assert(sizeof(typename LoInjection::return_type) == sizeof(halfT));//bijections ONLY

		constexpr static std::array<ObfDescriptor, 2> splitHi{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesHi = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 5)>(cycles_hi, splitHi);
		static constexpr OBFCYCLES cycles_hiCtx = splitCyclesHi[0];
		static constexpr OBFCYCLES cycles_hiInj = splitCyclesHi[1];
		static_assert(cycles_hiCtx + cycles_hiInj <= cycles_hi);
		using HiContext = typename ObfRecursiveContext<halfT, Context, ITHARE_OBF_NEW_PRNG(seed, 6), cycles_hiCtx>::intermediate_context_type;
		using HiInjection = obf_injection<halfT, HiContext, ITHARE_OBF_NEW_PRNG(seed, 7), cycles_hiInj+HiContext::context_cycles, LoHiInjectionRequirements>;
		static_assert(sizeof(typename HiInjection::return_type) == sizeof(halfT));//bijections ONLY

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			halfT lo = x >> halfTBits;
			typename LoInjection::return_type lo1 = LoInjection::injection(lo);
			lo = halfT(lo1);// *reinterpret_cast<halfT*>(&lo1);//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			halfT hi = (halfT)x;
			typename HiInjection::return_type hi1 = HiInjection::injection(hi);
			hi = halfT(hi1);// *reinterpret_cast<halfT*>(&hi1);//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return_type ret = RecursiveInjection::injection((T(hi) << halfTBits) + T(lo));
			ITHARE_OBF_DBG_ASSERT_SURJECTION("<3>", x, ret);
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y_) {
			auto y = RecursiveInjection::surjection(y_);
			halfT hi0 = y >> halfTBits;
			halfT lo0 = (halfT)y;
			halfT hi = HiInjection::surjection(/* *reinterpret_cast<typename HiInjection::return_type*>(&hi0)*/typename HiInjection::return_type(hi0));//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			halfT lo = LoInjection::surjection(/**reinterpret_cast<typename LoInjection::return_type*>(&lo0)*/ typename LoInjection::return_type(lo0));//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return T(hi) + (T(lo) << halfTBits);
		}

		static constexpr bool has_add_mod_max_value_ex = false;

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<3/*split-join*/,"<<obf_dbgPrintT<T>()<<"," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
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

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<4, T, Context, InjectionRequirements, seed, cycles> {
		using Traits = ObfTraits<T>;
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version4_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		struct RecursiveInjectionRequirements {
			static constexpr size_t exclude_version = 4;
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = InjectionRequirements::only_bijections;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};

	public:
		using RecursiveInjection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 1), availCycles+Context::context_cycles,RecursiveInjectionRequirements>;
		using return_type = typename RecursiveInjection::return_type;
		//constexpr static T C = (T)(obf_gen_const<T>(obf_compile_time_prng(seed, 2)) | 1);
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T C = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 2)>(consts);
		static_assert((C & 1) == 1);
		constexpr static T CINV0 = obf_mul_inverse_mod2n(C);
		static_assert((T)(C*CINV0) == (T)1);
		constexpr static typename Traits::literal_type CINV = CINV0;

		using literal = typename Context::template literal<typename Traits::literal_type, CINV, ITHARE_OBF_NEW_PRNG(seed, 3)>::type;
			//using CINV in injection to hide literals a bit better...

#ifdef ITHARE_OBF_DEFINE_DBG_MAP
		static std::map<T,T> dbg_map;
		static std::map<T,return_type> dbg_map_r;
#endif

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			auto lit = literal();
			ITHARE_OBF_DBG_CHECK_LITERAL("<4>",lit, CINV0);
			auto y = typename Traits::UintT(x) * typename Traits::UintT(lit.value());
			//ITHARE_OBF_DBG_MAP_ADD("<4>/ret",dbg_map, x,y);
			return_type ret = RecursiveInjection::injection(y);
			//ITHARE_OBF_DBG_MAP_ADD("<4>/r",dbg_map_r, y, ret);
			ITHARE_OBF_DBG_ASSERT_SURJECTION("<4>", x, ret);
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			T x = RecursiveInjection::surjection(y);
			//ITHARE_OBF_DBG_MAP_CHECK("<4>/r", dbg_map_r, x, y,RecursiveInjection::);
			T ret = x * C;
			//ITHARE_OBF_DBG_MAP_CHECK("<4>/ret", dbg_map, ret,x, );
			return ret;
		}

		static constexpr bool has_add_mod_max_value_ex = false;

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<4/*mul odd mod 2^N*/,"<<obf_dbgPrintT<T>()<<"," << obf_dbgPrintSeed<seed>() << "," << cycles << ">: C=" << obf_dbgPrintC<T>(C) << " CINV=" << obf_dbgPrintC<T>(CINV) << std::endl;
			//std::cout << std::string(offset, ' ') << " literal:" << std::endl;
			literal::dbgPrint(offset + 1,"literal:");
			//std::cout << std::string(offset, ' ') << " Recursive:" << std::endl;
			RecursiveInjection::dbgPrint(offset + 1,"Recursive:");
		}
#endif
	};

#ifdef ITHARE_OBF_DEFINE_DBG_MAP
	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	std::map<T, T> obf_injection_version<4, T, Context, InjectionRequirements, seed, cycles>::dbg_map = {};
	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	std::map<T, typename obf_injection_version<4, T, Context, InjectionRequirements, seed, cycles>::return_type> obf_injection_version<4, T, Context, InjectionRequirements, seed, cycles>::dbg_map_r = {};
#endif

	//version 5: split (w/o join)
	template<class T, class Context>
	struct obf_injection_version5_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 3;
		static constexpr OBFCYCLES own_min_surjection_cycles = 3;
		static constexpr OBFCYCLES own_min_cycles = 2*Context::context_cycles /* have to allocate context_cycles for BOTH branches */ + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr =
			ObfTraits<T>::has_half_type ?
			ObfDescriptor(true, own_min_cycles, 100) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<5, T, Context, InjectionRequirements, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		using halfT = typename ObfTraits<T>::HalfT;
		constexpr static int halfTBits = sizeof(halfT) * 8;

		struct RecursiveInjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = InjectionRequirements::only_bijections;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};

		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version5_descr<T,Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		constexpr static std::array<ObfDescriptor, 2> split{
			ObfDescriptor(true,0,100),//LoInjection
			ObfDescriptor(true,0,100),//HiInjection
		};
		static constexpr auto splitCycles = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 1)>(availCycles, split);
		static constexpr OBFCYCLES cycles_lo = splitCycles[0];
		static constexpr OBFCYCLES cycles_hi = splitCycles[1];
		static_assert(cycles_lo + cycles_hi <= availCycles);

		constexpr static std::array<ObfDescriptor, 2> splitLo{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesLo = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 2)>(cycles_lo, splitLo);
		static constexpr OBFCYCLES cycles_loCtx = splitCyclesLo[0];
		static constexpr OBFCYCLES cycles_loInj = splitCyclesLo[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using RecursiveLoContext = typename ObfRecursiveContext<halfT, Context, ITHARE_OBF_NEW_PRNG(seed, 3), cycles_loCtx+Context::context_cycles>::recursive_context_type;
		using RecursiveInjectionLo = obf_injection<halfT, RecursiveLoContext, ITHARE_OBF_NEW_PRNG(seed, 4), cycles_loInj+ RecursiveLoContext::context_cycles,RecursiveInjectionRequirements>;

		constexpr static std::array<ObfDescriptor, 2> splitHi{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesHi = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 5)>(cycles_hi, splitHi);
		static constexpr OBFCYCLES cycles_hiCtx = splitCyclesHi[0];
		static constexpr OBFCYCLES cycles_hiInj = splitCyclesHi[1];
		static_assert(cycles_hiCtx + cycles_hiInj <= cycles_hi);
		using RecursiveHiContext = typename ObfRecursiveContext<halfT, Context, ITHARE_OBF_NEW_PRNG(seed, 6), cycles_hiCtx+Context::context_cycles>::recursive_context_type;
		using RecursiveInjectionHi = obf_injection < halfT, RecursiveHiContext, ITHARE_OBF_NEW_PRNG(seed, 7), cycles_hiInj+ RecursiveHiContext::context_cycles,RecursiveInjectionRequirements > ;

		struct return_type {
			typename RecursiveInjectionLo::return_type lo;
			typename RecursiveInjectionHi::return_type hi;

			constexpr return_type(typename RecursiveInjectionLo::return_type lo_, typename RecursiveInjectionHi::return_type hi_)
				: lo(lo_), hi(hi_) {
			}
			constexpr return_type(T x) 
			: lo(halfT(x)), hi(halfT(x>>halfTBits)){
			}
			constexpr operator T() {
				halfT lo1 = halfT(lo);
				halfT hi1 = halfT(hi);
				return ( T(hi1) << halfTBits ) + T(lo1);
			}
		};
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			return_type ret{ RecursiveInjectionLo::injection((halfT)x), RecursiveInjectionHi::injection(x >> halfTBits) };
			ITHARE_OBF_DBG_ASSERT_SURJECTION("<5>", x, ret);
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y_) {
			halfT hi = RecursiveInjectionHi::surjection(y_.hi);
			halfT lo = RecursiveInjectionLo::surjection(y_.lo);
			return (T)lo + ((T)hi << halfTBits);
		}

		static constexpr bool has_add_mod_max_value_ex = false;

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<5/*split*/,"<<obf_dbgPrintT<T>()<<"," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
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
			ObfTraits<T>::has_half_type ?
			ObfDescriptor(true, own_min_cycles, 100/*it's cheap, but doesn't obfuscate the whole thing well => let's use it mostly for lower-cycle stuff*/) :
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<6, T, Context, InjectionRequirements, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version6_descr<T,Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		struct RecursiveInjectionRequirements {
			static constexpr size_t exclude_version = 6;
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = InjectionRequirements::only_bijections;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};
		using halfT = typename ObfTraits<T>::HalfT;

		constexpr static std::array<ObfDescriptor, 2> split{
			ObfDescriptor(true,0,200),//RecursiveInjection
			ObfDescriptor(true,0,100),//LoInjection
		};
		static constexpr auto splitCycles = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 1)>(availCycles, split);
		static constexpr OBFCYCLES cycles_rInj = splitCycles[0];
		static constexpr OBFCYCLES cycles_lo = splitCycles[1];
		static_assert(cycles_rInj + cycles_lo <= availCycles);

		using RecursiveInjection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 2), cycles_rInj + Context::context_cycles, RecursiveInjectionRequirements>;
		using return_type = typename RecursiveInjection::return_type;

	public:
		struct LoInjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = true;
			static constexpr bool no_physical_size_increase = InjectionRequirements::no_physical_size_increase;
		};

		constexpr static std::array<ObfDescriptor, 2> splitLo{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesLo = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 3)>(cycles_lo, splitLo);
		static constexpr OBFCYCLES cycles_loCtx = splitCyclesLo[0];
		static constexpr OBFCYCLES cycles_loInj = splitCyclesLo[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using LoContext = typename ObfRecursiveContext < halfT, Context, ITHARE_OBF_NEW_PRNG(seed, 4), cycles_loCtx>::intermediate_context_type;
		using LoInjection = obf_injection<halfT, LoContext, ITHARE_OBF_NEW_PRNG(seed, 5), cycles_loInj + LoContext::context_cycles, LoInjectionRequirements>;
		static_assert(sizeof(typename LoInjection::return_type) == sizeof(halfT));//bijections ONLY

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			halfT lo0 = halfT(x);
			typename LoInjection::return_type lo1 = LoInjection::injection(lo0);
			//halfT lo = *reinterpret_cast<halfT*>(&lo1);//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			halfT lo = halfT(lo1);
			return_type ret = RecursiveInjection::injection(x - T(lo0) + lo);
			ITHARE_OBF_DBG_ASSERT_SURJECTION("<6>", x, ret);
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type yy) {
			T y = RecursiveInjection::surjection(yy);
			halfT lo0 = halfT(y);
			halfT lo = LoInjection::surjection(/* *reinterpret_cast<typename LoInjection::return_type*>(&lo0)*/ typename LoInjection::return_type(lo0));//relies on static_assert(sizeof(return_type)==sizeof(halfT)) above
			return y - T(lo0) + lo;
		}

		static constexpr bool has_add_mod_max_value_ex = false;

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<6/*injection(halfT)*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
			LoInjection::dbgPrint(offset + 1, "Lo:");
			RecursiveInjection::dbgPrint(offset + 1, "Recursive:");
		}
#endif
	};

	//version 7: split into two ObfBitUints (w/o join)
	template<class T, class Context, class InjectionRequirements>
	struct obf_injection_version7_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 15;
		static constexpr OBFCYCLES own_min_surjection_cycles = 15;
		static constexpr OBFCYCLES own_min_cycles = 2 * Context::context_cycles /* have to allocate context_cycles for BOTH branches */ + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr =
			!InjectionRequirements::only_bijections && !InjectionRequirements::no_physical_size_increase && ObfTraits<T>::nbits >= 2 ?
			ObfDescriptor(true, own_min_cycles, 100)
			: 
			ObfDescriptor(false, 0, 0);
	};

	template <class T, class Context, class InjectionRequirements, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<7, T, Context, InjectionRequirements, seed, cycles> {
		using Traits = ObfTraits<T>;
	public:
		static constexpr size_t loBits = ITHARE_OBF_RANDOM(seed, 1, Traits::nbits-1) + 1;
		static_assert(loBits > 0);
		static_assert(loBits < Traits::nbits);
		static constexpr size_t hiBits = Traits::nbits - loBits;
		using TypeLo = ObfBitUint<loBits>;
		using TypeHi = ObfBitUint<hiBits>;

		struct RecursiveInjectionRequirements {
			static constexpr size_t exclude_version = T(-1);
			static constexpr bool is_constexpr = InjectionRequirements::is_constexpr;
			static constexpr bool only_bijections = InjectionRequirements::only_bijections;
			static constexpr bool no_physical_size_increase = true;
		};

		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version7_descr<T, Context,InjectionRequirements>::own_min_cycles;
		static_assert(availCycles >= 0);

		constexpr static std::array<ObfDescriptor, 2> split{
			ObfDescriptor(true,0,100),//LoInjection
			ObfDescriptor(true,0,100),//HiInjection
		};
		static constexpr auto splitCycles = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 2)>(availCycles, split);
		static constexpr OBFCYCLES cycles_lo = splitCycles[0];
		static constexpr OBFCYCLES cycles_hi = splitCycles[1];
		static_assert(cycles_lo + cycles_hi <= availCycles);

		constexpr static std::array<ObfDescriptor, 2> splitLo{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesLo = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 2)>(cycles_lo, splitLo);
		static constexpr OBFCYCLES cycles_loCtx = splitCyclesLo[0];
		static constexpr OBFCYCLES cycles_loInj = splitCyclesLo[1];
		static_assert(cycles_loCtx + cycles_loInj <= cycles_lo);
		using RecursiveLoContext = typename ObfRecursiveContext<TypeLo, Context, ITHARE_OBF_NEW_PRNG(seed, 3), cycles_loCtx + Context::context_cycles>::recursive_context_type;
		using RecursiveInjectionLo = obf_injection<TypeLo, RecursiveLoContext, ITHARE_OBF_NEW_PRNG(seed, 4), cycles_loInj + RecursiveLoContext::context_cycles, RecursiveInjectionRequirements>;

		constexpr static std::array<ObfDescriptor, 2> splitHi{
			ObfDescriptor(true,0,100),//Context
			ObfDescriptor(true,0,100)//Injection
		};
		static constexpr auto splitCyclesHi = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 5)>(cycles_hi, splitHi);
		static constexpr OBFCYCLES cycles_hiCtx = splitCyclesHi[0];
		static constexpr OBFCYCLES cycles_hiInj = splitCyclesHi[1];
		static_assert(cycles_hiCtx + cycles_hiInj <= cycles_hi);
		using RecursiveHiContext = typename ObfRecursiveContext<TypeHi, Context, ITHARE_OBF_NEW_PRNG(seed, 6), cycles_hiCtx + Context::context_cycles>::recursive_context_type;
		using RecursiveInjectionHi = obf_injection <TypeHi, RecursiveHiContext, ITHARE_OBF_NEW_PRNG(seed, 7), cycles_hiInj + RecursiveHiContext::context_cycles, RecursiveInjectionRequirements >;

		struct return_type {
			typename RecursiveInjectionLo::return_type lo;
			typename RecursiveInjectionHi::return_type hi;

			constexpr return_type(typename RecursiveInjectionLo::return_type lo_, typename RecursiveInjectionHi::return_type hi_)
				: lo(lo_), hi(hi_) {
			}
			constexpr return_type(T x)
				: lo(TypeLo(x)), hi(TypeHi(x >> loBits)) {
			}
			constexpr operator T() {
				TypeLo lo1 = TypeLo(lo);
				TypeHi hi1 = TypeHi(hi);
				return (T(hi1) << loBits) + T(lo1);
			}
		};

#ifdef ITHARE_OBF_DEFINE_DBG_MAP
		inline static std::map<T, TypeLo> dbg_map_lo = {};
		inline static std::map<T, TypeHi> dbg_map_hi = {};
		inline static std::map<TypeLo,typename RecursiveInjectionLo::return_type> dbg_map_rlo = {};
		inline static std::map<TypeHi, typename RecursiveInjectionHi::return_type> dbg_map_rhi = {};
#endif

		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			TypeLo lo = TypeLo(typename TypeLo::T(x));
			TypeHi hi = TypeHi(typename TypeHi::T(x >> loBits));
			//ITHARE_OBF_DBG_MAP_ADD("<7>/lo",dbg_map_lo, x, lo);
			//ITHARE_OBF_DBG_MAP_ADD("<7>/hi",dbg_map_hi, x, hi);
			return_type ret{ RecursiveInjectionLo::injection(lo),
				RecursiveInjectionHi::injection(hi) };
			//ITHARE_OBF_DBG_MAP_ADD("<7>/rlo",dbg_map_rlo, lo, ret.lo);
			//ITHARE_OBF_DBG_MAP_ADD("<7>/rhi",dbg_map_rhi, hi, ret.hi);
			ITHARE_OBF_DBG_ASSERT_SURJECTION("<7>", x, ret);
			return ret;
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y_) {
			TypeHi hi = RecursiveInjectionHi::surjection(y_.hi);
			//ITHARE_OBF_DBG_MAP_CHECK("<7>/rhi",dbg_map_rhi, hi, y_.hi, RecursiveInjectionHi::);
			TypeLo lo = RecursiveInjectionLo::surjection(y_.lo);
			//ITHARE_OBF_DBG_MAP_CHECK("<7>/rlo", dbg_map_rlo, lo , y_.lo, RecursiveInjectionLo::);

			T ret = T(lo) + T(T(hi) << loBits);
			//ITHARE_OBF_DBG_MAP_CHECK("<7>/hi", dbg_map_hi, ret, T(hi), );
			//ITHARE_OBF_DBG_MAP_CHECK("<7>/lo", dbg_map_lo, ret, T(lo), );
			return ret;
		}

		static constexpr bool has_add_mod_max_value_ex = false;

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<7/*split into ObfBitUint<>*/," << obf_dbgPrintT<T>() << ", Context, InjectionRequirements, " << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
			Context::dbgPrint(offset + 1, "Context:");
			TypeLo::dbgPrint(offset + 1, "TypeLo:");
			RecursiveInjectionLo::dbgPrint(offset + 1, "Lo:");
			TypeHi::dbgPrint(offset + 1, "TypeHi:");
			RecursiveInjectionHi::dbgPrint(offset + 1, "Hi:");
		}
#endif
	};



	#if 0 //COMMENTED OUT - TOO OBVIOUS IN DECOMPILE :-(; if using - rename into "version 8"
	//version 7: 1-bit rotation 
	template<class Context>
	struct obf_injection_version7_descr {
		static constexpr OBFCYCLES own_min_injection_cycles = 5;//relying on compiler generating cmovns etc.
		static constexpr OBFCYCLES own_min_surjection_cycles = 5;//relying on compiler generating cmovns etc.
		static constexpr OBFCYCLES own_min_cycles = Context::context_cycles + Context::calc_cycles(own_min_injection_cycles, own_min_surjection_cycles);
		static constexpr ObfDescriptor descr = ObfDescriptor(true, own_min_cycles, 100);
	};

	template <class T, class Context, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_injection_version<7, T, Context, seed, cycles> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
	public:
		static constexpr OBFCYCLES availCycles = cycles - obf_injection_version7_descr<Context>::own_min_cycles;
		static_assert(availCycles >= 0);

		using RecursiveInjection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 1), availCycles + Context::context_cycles, RecursiveInjectionRequirements>;
		using return_type = typename RecursiveInjection::return_type;
		using ST = typename std::make_signed<T>::type;
		static constexpr T highbit = T(1) << (ObfTraits<T>::nbits - 1);
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

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_injection_version<7/*1-bit rotation*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
			RecursiveInjection::dbgPrint(offset + 1);
		}
#endif
	};
#endif//#if 0

	//obf_injection: combining obf_injection_version
	template<class T, class Context, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles,class InjectionRequirements>
	class obf_injection {
		static_assert(std::is_same<T, typename Context::Type>::value);
		using Traits = ObfTraits<T>;
		constexpr static std::array<ObfDescriptor, 8> descr{
			obf_injection_version0_descr<Context>::descr,
			obf_injection_version1_descr<Context>::descr,
			obf_injection_version2_descr<T,Context>::descr,
			obf_injection_version3_descr<T,Context>::descr,
			obf_injection_version4_descr<Context>::descr,
			obf_injection_version5_descr<T,Context>::descr,
			obf_injection_version6_descr<T,Context>::descr,
			obf_injection_version7_descr<T,Context,InjectionRequirements>::descr,
		};
		constexpr static size_t which = obf_random_obf_from_list<ITHARE_OBF_NEW_PRNG(seed, 1)>(cycles, descr,InjectionRequirements::exclude_version);
		static_assert(which >= 0 && which < descr.size());
		using WhichType = obf_injection_version<which, T, Context, InjectionRequirements, seed, cycles>;

	public:
		using return_type = typename WhichType::return_type;
		ITHARE_OBF_FORCEINLINE constexpr static return_type injection(T x) {
			return WhichType::injection(x);
		}
		ITHARE_OBF_FORCEINLINE constexpr static T surjection(return_type y) {
			return WhichType::surjection(y);
		}

	public:
		static constexpr bool has_add_mod_max_value_ex = WhichType::has_add_mod_max_value_ex;
		ITHARE_OBF_FORCEINLINE constexpr static T injected_add_mod_max_value_ex(T base, T x) {
			return WhichType::injected_add_mod_max_value_ex(base,x);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			size_t dbgWhich = obf_random_obf_from_list<ITHARE_OBF_NEW_PRNG(seed, 1)>(cycles, descr);
			std::cout << std::string(offset, ' ') << prefix << "obf_injection<"<<obf_dbgPrintT<T>()<<"," << obf_dbgPrintSeed<seed>() << "," << cycles << ">: which=" << which << " dbgWhich=" << dbgWhich << std::endl;
			//std::cout << std::string(offset, ' ') << " Context:" << std::endl;
			Context::dbgPrint(offset + 1,"Context:");
			//std::cout << std::string(offset, ' ') << " Version:" << std::endl;
			WhichType::dbgPrint(offset + 1);
		}
#endif
	};

	//ObfLiteralContext
	template<size_t which, class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version;
	//forward declaration:
	template<class T, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class ObfLiteralContext;

	//version 0: identity
	struct obf_literal_context_version0_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(false, 0, 1);
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<0,T,seed> {
		using Traits = ObfTraits<T>;
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version0_descr::descr.min_cycles;

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static constexpr T final_surjection(T y) {
			return y;
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<0/*identity*/," << obf_dbgPrintT<T>() << ">" << std::endl;
		}
#endif
	};

	//version 1: global volatile constant
	struct obf_literal_context_version1_descr {
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 6, 100);
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<1,T,seed> {
		using Traits = ObfTraits<T>;
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version1_descr::descr.min_cycles;

		//static constexpr T CC = obf_gen_const<T>(ITHARE_OBF_NEW_PRNG(seed, 1));
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T CC = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 1)>(consts);
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
			return y - T(c);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<1/*global volatile*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << ">: CC=" << obf_dbgPrintC<T>(CC) << std::endl;
		}
#endif
	private:
		static /*volatile*/ T c;//TODO! - return back volatile
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	/*volatile*/ T ObfLiteralContext_version<1, T, seed>::c = CC;

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

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<2,T,seed> {
		using Traits = ObfTraits<T>;
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version2_descr::descr.min_cycles;

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static /*non-constexpr*/ T final_surjection(T y) {
			T x, yy;
			T z = obf_aliased_zero(&x, &yy);
			return y - z;
		}
#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<2/*func with aliased pointers*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << ">:" << std::endl;
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

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<3,T,seed> {
		using Traits = ObfTraits<T>;
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version3_descr::descr.min_cycles;

		//static constexpr T CC = obf_gen_const<T>(ITHARE_OBF_NEW_PRNG(seed, 1));
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T CC = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 1)>(consts);
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
#ifdef ITHARE_OBF_DEBUG_ANTI_DEBUG_ALWAYS_FALSE
			return y - CC;
#else
			return y - CC * T(1 + obf_peb[2]);
#endif
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<3/*PEB*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << ">: CC=" << obf_dbgPrintC<T>(CC) << std::endl;
		}
#endif
	};
#endif

	//version 4: global var-with-invariant
	template<class T>
	struct obf_literal_context_version4_descr {
		static constexpr ObfDescriptor descr = 
			ObfTraits<T>::is_built_in ? 
			ObfDescriptor(true, 100, 100)
			: ObfDescriptor(false,0,0);//MIGHT be lifted if we remove strange games with sizeof etc.
			//yes, it is up 100+ cycles now (due to worst-case MT caching issues)
			//TODO: move invariant to thread_local, something else?
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<4, T, seed> {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version4_descr<T>::descr.min_cycles;

		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		static constexpr T PREMODRNDCONST = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 2)>(consts);//TODO: check which constants we want
		static constexpr T PREMODMASK = (T(1) << (sizeof(T) * 4)) - 1;
		static constexpr T PREMOD = PREMODRNDCONST & PREMODMASK;
		static constexpr T MOD = PREMOD == 0 ? 100 : PREMOD;//remapping 'bad value' 0 to 'something'
		static constexpr T CC = ITHARE_OBF_RANDOM(seed, 2,MOD);

		static constexpr T MAXMUL1 = T(-1)/MOD;
		static constexpr auto MAXMUL1_ADJUSTED0 = obf_sqrt_very_rough_approximation(MAXMUL1);
		static_assert(MAXMUL1_ADJUSTED0 < T(-1));
		static constexpr T MAXMUL1_ADJUSTED = (T)MAXMUL1_ADJUSTED0;
		static constexpr T MUL1 = MAXMUL1 > 2 ? 1+ITHARE_OBF_RANDOM(seed, 2, MAXMUL1_ADJUSTED) : 1;
		static constexpr T DELTA = MUL1 * MOD;
		static_assert(DELTA / MUL1 == MOD);//overflow check

		static constexpr T MAXMUL2 = T(-1) / DELTA;
		static constexpr T MUL2 = MAXMUL2 > 2 ? 1+ITHARE_OBF_RANDOM(seed, 3, MAXMUL2) : 1;
		static constexpr T DELTAMOD = MUL2 * MOD;
		static_assert(DELTAMOD / MUL2 == MOD);//overflow check

		static constexpr T PREMUL3 = ITHARE_OBF_RANDOM(seed, 4, MUL2);
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

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<4/*global volatile var-with-invariant*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << ">: CC=" << obf_dbgPrintC<T>(CC) << std::endl;
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
	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	std::atomic<T> ObfLiteralContext_version<4, T, seed>::c = CC0;
#else
	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	volatile T ObfLiteralContext_version<4, T, seed>::c = CC0;
#endif

	//ObfZeroLiteralContext
	template<class T>
	struct ObfZeroLiteralContext {
		//same as ObfLiteralContext_version<0,...> but with additional stuff to make it suitable for use as Context parameter to injections
		using Type = T;
		constexpr static OBFCYCLES context_cycles = 0;
		constexpr static OBFCYCLES calc_cycles(OBFCYCLES inj, OBFCYCLES surj) {
			return surj;//for literals, ONLY surjection costs apply in runtime (as injection applies in compile-time)
		}
		constexpr static OBFCYCLES literal_cycles = 0;
		template<class T2,T2 C, ITHARE_OBF_SEEDTPARAM seed>
		struct literal {
			using type = obf_literal_ctx<T2, C, ObfZeroLiteralContext<T2>, seed, literal_cycles>;
		};

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static constexpr T final_surjection(T y) {
			return y;
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfZeroContext<" << obf_dbgPrintT<T>() << ">" << std::endl;
		}
#endif
	};
	template<class T, class T0, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct ObfRecursiveContext<T, ObfZeroLiteralContext<T0>, seed, cycles> {
		using recursive_context_type = ObfZeroLiteralContext<T>;
		using intermediate_context_type = ObfZeroLiteralContext<T>;
	};

	//ObfLiteralContext
	template<class T, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class ObfLiteralContext {
		using Traits = ObfTraits<T>;
		constexpr static std::array<ObfDescriptor, 5> descr{
			obf_literal_context_version0_descr::descr,
			obf_literal_context_version1_descr::descr,
			obf_literal_context_version2_descr::descr,
			obf_literal_context_version3_descr::descr,
			obf_literal_context_version4_descr<T>::descr,
		};
		constexpr static size_t which = obf_random_obf_from_list<ITHARE_OBF_NEW_PRNG(seed, 1)>(cycles, descr);
		using WhichType = ObfLiteralContext_version<which, T, seed>;

	public:
		using Type = T;
		constexpr static OBFCYCLES context_cycles = WhichType::context_cycles;
		constexpr static OBFCYCLES calc_cycles(OBFCYCLES inj, OBFCYCLES surj) {
			return surj;//for literals, ONLY surjection costs apply in runtime (as injection applies in compile-time)
		}

		constexpr static OBFCYCLES literal_cycles = 0;
		template<class T2, T2 C, ITHARE_OBF_SEEDTPARAM seed2>
		struct literal {
			using type = obf_literal_ctx<T2, C, ObfZeroLiteralContext<T2>, seed2, literal_cycles>;
		};

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return WhichType::final_injection(x);
		}
		ITHARE_OBF_FORCEINLINE static /*non-constexpr*/ T final_surjection(T y) {
			return WhichType::final_surjection(y);
		}


	public:
#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			size_t dbgWhich = obf_random_obf_from_list<ITHARE_OBF_NEW_PRNG(seed, 1)>(cycles, descr);
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext<" << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">: which=" << which << " dbgWhich=" << dbgWhich << std::endl;
			WhichType::dbgPrint(offset + 1);
		}
#endif
	};

	template<class T, class T0, ITHARE_OBF_SEEDTPARAM seed, ITHARE_OBF_SEEDTPARAM seed0, OBFCYCLES cycles0,OBFCYCLES cycles>
	struct ObfRecursiveContext<T, ObfLiteralContext<T0, seed0,cycles0>, seed, cycles> {
		using recursive_context_type = ObfLiteralContext<T, ITHARE_OBF_NEW_PRNG(seed, 1),cycles>;//@@
		using intermediate_context_type = typename ithare::obf::ObfLiteralContext<T, ITHARE_OBF_NEW_PRNG(seed, 2), cycles>;//whenever cycles is low (which is very often), will fallback to version0
	};

	//obf_literal
	template<class T, T C, class Context, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_literal_ctx {
		static_assert(std::is_integral<T>::value);
		static_assert(std::is_unsigned<T>::value);

		struct InjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = true;
			static constexpr bool only_bijections = false;
			static constexpr bool no_physical_size_increase = false;
		};
		using Injection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 1), cycles,InjectionRequirements>;
	public:
		ITHARE_OBF_FORCEINLINE constexpr obf_literal_ctx() : val(Injection::injection(C)) {
		}
		ITHARE_OBF_FORCEINLINE constexpr T value() const {
			return Injection::surjection(val);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_literal_ctx<" << obf_dbgPrintT<T>() << "," << obf_dbgPrintC<T>(C) << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
			Injection::dbgPrint(offset + 1);
		}
		static void dbgCheck() {
			typename Injection::return_type c = Injection::injection(C);
			T cc = Injection::surjection(c);
			assert(cc == C);
		}
#endif
	private:
		typename Injection::return_type val;
	};

	//IMPORTANT: ANY API CHANGES MUST BE MIRRORED in obf_literal_dbg<>
	template<class T_, T_ C_, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_literal {
		static_assert(std::is_integral<T_>::value);
		using T = typename std::make_unsigned<T_>::type;//from this point on, unsigned only
		static constexpr T C = (T)C_;

		using Context = ObfLiteralContext<T, ITHARE_OBF_NEW_PRNG(seed, 1),cycles>;
		struct InjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = true;
			static constexpr bool only_bijections = false;
			static constexpr bool no_physical_size_increase = false;
		};
		using Injection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 2), cycles,InjectionRequirements>;
	public:
		ITHARE_OBF_FORCEINLINE constexpr obf_literal() : val(Injection::injection(C)) {
		}
		ITHARE_OBF_FORCEINLINE T value() const {
			return Injection::surjection(val);
		}
		ITHARE_OBF_FORCEINLINE operator T() const {
			return value();
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_literal<"<<obf_dbgPrintT<T>()<<"," << C << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
			Injection::dbgPrint(offset + 1);
		}
#endif
	private:
		typename Injection::return_type val;
	};

	//ObfVarContext
	template<class T, ITHARE_OBF_SEEDTPARAM seed,OBFCYCLES cycles>
	struct ObfVarContext {
		using Type = T;
		constexpr static OBFCYCLES context_cycles = 0;
		constexpr static OBFCYCLES calc_cycles(OBFCYCLES inj, OBFCYCLES surj) {
			return inj + surj;//for variables, BOTH injection and surjection are executed in runtime
		}

		constexpr static OBFCYCLES literal_cycles = std::min(cycles/2,50);//TODO: justify (or define?)
		template<class T2, T2 C, ITHARE_OBF_SEEDTPARAM seed2>
		struct literal {
			using LiteralContext = ObfLiteralContext<T2, seed, literal_cycles>;
			using type = obf_literal_ctx<T2, C, LiteralContext, seed2, literal_cycles>;
		};

		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		ITHARE_OBF_FORCEINLINE static constexpr T final_surjection(T y) {
			return y;
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfVarContext<" << obf_dbgPrintT<T>() << ">" << std::endl;
		}
#endif
	};
	template<class T, class T0, ITHARE_OBF_SEEDTPARAM seed0, OBFCYCLES cycles0, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	struct ObfRecursiveContext<T, ObfVarContext<T0,seed0,cycles0>, seed, cycles> {
		using recursive_context_type = ObfVarContext<T,seed,cycles>;
		using intermediate_context_type = ObfVarContext<T,seed,cycles>;
	};

	//obf_var
	//IMPORTANT: ANY API CHANGES MUST BE MIRRORED in obf_var_dbg<>
	template<class T_, ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles>
	class obf_var {
		static_assert(std::is_integral<T_>::value);
		using T = typename std::make_unsigned<T_>::type;//from this point on, unsigned only
		//using TTraits = ObfTraits<T>;

		using Context = ObfVarContext<T, ITHARE_OBF_NEW_PRNG(seed, 1), cycles>;
		struct InjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = false;
			static constexpr bool only_bijections = false;
			static constexpr bool no_physical_size_increase = false;
		};
		using Injection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 2), cycles, InjectionRequirements>;

	public:
		ITHARE_OBF_FORCEINLINE obf_var(T_ t) : val(Injection::injection(T(t))) {
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var(obf_var<T2, seed2, cycles2> t) : val(Injection::injection(T(T_(t.value())))) {//TODO: randomized injection implementation
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var(obf_literal<T2, C2, seed2, cycles2> t) : val(Injection::injection(T(T_(t.value())))) {//TODO: randomized injection implementation
		}
		ITHARE_OBF_FORCEINLINE obf_var& operator =(T_ t) {
			val = Injection::injection(T(t));//TODO: different implementations of the same injection in different contexts
			return *this;
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator =(obf_var<T2, seed2, cycles2> t) {
			val = Injection::injection(T(T_(t.value())));//TODO: different implementations of the same injection in different contexts
			return *this;
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator =(obf_literal<T2, C2, seed2, cycles2> t) {
			val = Injection::injection(T(T_(t.value())));//TODO: different implementations of the same injection in different contexts
			return *this;
		}
		ITHARE_OBF_FORCEINLINE T_ value() const {
			return T_(Injection::surjection(val));
		}

		ITHARE_OBF_FORCEINLINE operator T_() const { return value(); }
		ITHARE_OBF_FORCEINLINE obf_var& operator ++() { 
			if constexpr(Injection::has_add_mod_max_value_ex) {
				//auto noShortcut = Injection::injection(T(value()+1));
				val = Injection::injected_add_mod_max_value_ex(val,1);
				//assert(val == noShortcut);
			}
			else {
				*this = value() + 1; 
			}
			return *this;
		}
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

		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator <(obf_var<T2, seed2, cycles2> t) {
			return value() < t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator >(obf_var<T2, seed2, cycles2> t) {
			return value() > t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator ==(obf_var<T2, seed2, cycles2> t) {
			return value() == t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator !=(obf_var<T2, seed2, cycles2> t) {
			return value() != t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator <=(obf_var<T2, seed2, cycles2> t) {
			return value() <= t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator >=(obf_var<T2, seed2, cycles2> t) {
			return value() >= t.value();
		}

		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator <(obf_literal<T2, C2, seed2, cycles2> t) {
			return value() < t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator >(obf_literal<T2, C2, seed2, cycles2> t) {
			return value() > t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator ==(obf_literal<T2, C2, seed2, cycles2> t) {
			return value() == t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator !=(obf_literal<T2, C2, seed2, cycles2> t) {
			return value() != t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator <=(obf_literal<T2, C2, seed2, cycles2> t) {
			return value() <= t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE bool operator >=(obf_literal<T2, C2, seed2, cycles2> t) {
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

		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator +=(obf_var<T2, seed2, cycles2> t) {
			return *this += t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator -=(obf_var<T2, seed2, cycles2> t) {
			return *this -= t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator *=(obf_var<T2, seed2, cycles2> t) {
			return *this *= t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator /=(obf_var<T2, seed2, cycles2> t) {
			return *this /= t.value();
		}
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator %=(obf_var<T2, seed2, cycles2> t) {
			return *this %= t.value();
		}

		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator +=(obf_literal<T2, C2, seed2, cycles2> t) {
			return *this += t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator -=(obf_literal<T2, C2, seed2, cycles2> t) {
			return *this -= t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator *=(obf_literal<T2, C2, seed2, cycles2> t) {
			return *this *= t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator /=(obf_literal<T2, C2, seed2, cycles2> t) {
			return *this /= t.value();
		}
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var& operator %=(obf_literal<T2, C2, seed2, cycles2> t) {
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
		
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator +(obf_var<T2,seed2,cycles2> t) { return obf_var(value() + t.value()); }
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator -(obf_var<T2, seed2, cycles2> t) { return obf_var(value() - t.value()); }
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator *(obf_var<T2, seed2, cycles2> t) { return obf_var(value() * t.value()); }
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator /(obf_var<T2, seed2, cycles2> t) { return obf_var(value() / t.value()); }
		template<class T2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator %(obf_var<T2, seed2, cycles2> t) { return obf_var(value() % t.value()); }

		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator +(obf_literal<T2, C2, seed2, cycles2> t) { return obf_var(value() + t.value()); }
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator -(obf_literal<T2, C2, seed2, cycles2> t) { return obf_var(value() - t.value()); }
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator *(obf_literal<T2, C2, seed2, cycles2> t) { return obf_var(value() * t.value()); }
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator /(obf_literal<T2, C2, seed2, cycles2> t) { return obf_var(value() / t.value()); }
		template<class T2, T2 C2, ITHARE_OBF_SEEDTPARAM seed2, OBFCYCLES cycles2>
		ITHARE_OBF_FORCEINLINE obf_var operator %(obf_literal<T2, C2, seed2, cycles2> t) { return obf_var(value() % t.value()); }

		//TODO: bitwise

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_var<" << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() <<","<<cycles<<">" << std::endl;
			Injection::dbgPrint(offset+1);
		}
#endif

	private:
		typename Injection::return_type val;
	};

	//IMPORTANT: ANY API CHANGES MUST BE MIRRORED in obf_str_literal_dbg<>
	template<ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles, char... C>//TODO! - wchar_t
	struct obf_str_literal {
		//TODO: consider using different contexts beyond current (effectively global var)
		static_assert(sizeof(char) == 1);
		static constexpr size_t origSz = sizeof...(C);
		static constexpr char const str[sizeof...(C)+1] = { C...,'\0' };
		static constexpr size_t sz = obf_strlen(str);
		static_assert(sz > 0);
		static_assert(sz <= 32);
		static constexpr size_t sz4 = (sz+3)/ 4;
		static_assert(sz4 > 0);
		static_assert(sz4 <= 8);//corresponds to max literal = 32, TODO: more later
		static constexpr uint32_t FILLER = uint32_t(ITHARE_OBF_RANDOM_UINT32(seed,1));

		constexpr static std::array<ObfDescriptor, 8> split{
			ObfDescriptor(true,0,100),//Injection0
			ObfDescriptor(true,0,sz4>1?100:0),//Injection1
			ObfDescriptor(true,0,sz4>2 ? 100 : 0),//Injection2
			ObfDescriptor(true,0,sz4>3 ? 100 : 0),//Injection3
			ObfDescriptor(true,0,sz4>4 ? 100 : 0),//Injection4
			ObfDescriptor(true,0,sz4>5 ? 100 : 0),//Injection5
			ObfDescriptor(true,0,sz4>6 ? 100 : 0),//Injection6
			ObfDescriptor(true,0,sz4>7 ? 100 : 0),//Injection7
		};
		static constexpr auto splitCycles = obf_random_split<ITHARE_OBF_NEW_PRNG(seed, 2)>(cycles, split);
		static constexpr OBFCYCLES split0 = splitCycles[0];
		static constexpr OBFCYCLES split1 = splitCycles[1];
		static constexpr OBFCYCLES split2 = splitCycles[2];
		static constexpr OBFCYCLES split3 = splitCycles[3];
		static constexpr OBFCYCLES split4 = splitCycles[4];
		static constexpr OBFCYCLES split5 = splitCycles[5];
		static constexpr OBFCYCLES split6 = splitCycles[6];
		static constexpr OBFCYCLES split7 = splitCycles[7];

		struct InjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = true;
			static constexpr bool only_bijections = true;
			static constexpr bool no_physical_size_increase = false;
		};

		using Injection0 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 3), std::max(split0,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection0::return_type) == sizeof(uint32_t));//MUST be bijection
		using Injection1 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 4), std::max(split1,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection1::return_type) == sizeof(uint32_t));//MUST be bijection
		using Injection2 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 5), std::max(split2,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection2::return_type) == sizeof(uint32_t));//MUST be bijection
		using Injection3 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 6), std::max(split3,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection3::return_type) == sizeof(uint32_t));//MUST be bijection
		using Injection4 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 7), std::max(split4,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection4::return_type) == sizeof(uint32_t));//MUST be bijection
		using Injection5 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 8), std::max(split5,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection5::return_type) == sizeof(uint32_t));//MUST be bijection
		using Injection6 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 9), std::max(split6,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection6::return_type) == sizeof(uint32_t));//MUST be bijection
		using Injection7 = obf_injection<uint32_t, ObfZeroLiteralContext<uint32_t>, ITHARE_OBF_NEW_PRNG(seed, 10), std::max(split7,2), InjectionRequirements>;
		static_assert(sizeof(typename Injection7::return_type) == sizeof(uint32_t));//MUST be bijection

		ITHARE_OBF_FORCEINLINE static constexpr uint32_t little_endian4(const char* str, size_t offset) {//TODO: BIG-ENDIAN
			//replacement for non-constexpr return *(uint32_t*)(str + offset);
			return str[offset] | (uint32_t(str[offset + 1]) << 8) | (uint32_t(str[offset + 2]) << 16) | (uint32_t(str[offset + 3]) << 24);
		}
		ITHARE_OBF_FORCEINLINE static constexpr uint32_t last4(char const str[origSz], size_t offset, uint32_t filler) {
			assert(origSz > offset);
			size_t delta = origSz - offset;
			assert(delta <= 3);
			char buf[4] = {};
			size_t i = 0;
			for (; i < delta; ++i) {
				buf[i] = str[origSz + i];
			}
			for (; i < 4; ++i) {
				buf[i] = char(filler);
				filler >>= 8;
			}
			return little_endian4(buf,0);
		}
		ITHARE_OBF_FORCEINLINE static constexpr uint32_t get4(char const str[origSz], size_t offset) {
			assert(offset < origSz);
			if (offset + 4 < origSz)
				return little_endian4(str, offset);
			else
				return last4(str, offset,FILLER);
		}
		ITHARE_OBF_FORCEINLINE static constexpr std::array<uint32_t, sz4> str_obf() {
			std::array<uint32_t, sz4> ret = {};
			ret[0] = Injection0::injection(get4(str,0));
			if constexpr(sz4 > 1)
				ret[1] = Injection1::injection(get4(str, 4));
			if constexpr(sz4 > 2)
				ret[2] = Injection2::injection(get4(str, 8));
			if constexpr(sz4 > 3)
				ret[3] = Injection3::injection(get4(str, 12));
			if constexpr(sz4 > 4)
				ret[4] = Injection4::injection(get4(str, 16));
			if constexpr(sz4 > 5)
				ret[5] = Injection5::injection(get4(str, 20));
			if constexpr(sz4 > 6)
				ret[6] = Injection6::injection(get4(str, 24));
			if constexpr(sz4 > 7)
				ret[7] = Injection7::injection(get4(str, 28));
			return ret;
		}

		static constexpr std::array<uint32_t, sz4> strC = str_obf();

		static std::array<uint32_t, sz4> c;//TODO: volatile
		ITHARE_OBF_FORCEINLINE std::string value() const {
			char buf[sz4 * 4];
			*(uint32_t*)(buf + 0) = Injection0::surjection(c[0]);
			if constexpr(sz4 > 1)
				*(uint32_t*)(buf + 4) = Injection1::surjection(c[1]);
			if constexpr(sz4 > 2)
				*(uint32_t*)(buf + 8) = Injection2::surjection(c[2]);
			if constexpr(sz4 > 3)
				*(uint32_t*)(buf + 12) = Injection3::surjection(c[3]);
			if constexpr(sz4 > 4)
				*(uint32_t*)(buf + 16) = Injection4::surjection(c[4]);
			if constexpr(sz4 > 5)
				*(uint32_t*)(buf + 20) = Injection5::surjection(c[5]);
			if constexpr(sz4 > 6)
				*(uint32_t*)(buf + 24) = Injection6::surjection(c[6]);
			if constexpr(sz4 > 7)
				*(uint32_t*)(buf + 28) = Injection7::surjection(c[7]);
			return std::string(buf,sz);
		}
		ITHARE_OBF_FORCEINLINE operator std::string() const {
			return value();
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_str_literal<'" << str << "'," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
			Injection0::dbgPrint(offset + 1, "Injection0:");
			if constexpr(sz4 > 1)
				Injection1::dbgPrint(offset+1,"Injection1:");
			if constexpr(sz4 > 2)
				Injection2::dbgPrint(offset + 1, "Injection2:");
			if constexpr(sz4 > 3)
				Injection3::dbgPrint(offset + 1, "Injection3:");
			if constexpr(sz4 > 4)
				Injection4::dbgPrint(offset + 1, "Injection4:");
			if constexpr(sz4 > 5)
				Injection5::dbgPrint(offset + 1, "Injection5:");
			if constexpr(sz4 > 6)
				Injection6::dbgPrint(offset + 1, "Injection6:");
			if constexpr(sz4 > 7)
				Injection7::dbgPrint(offset + 1, "Injection7:");
		}
#endif
	};

	template<ITHARE_OBF_SEEDTPARAM seed, OBFCYCLES cycles, char... C>
	std::array<uint32_t, obf_str_literal<seed,cycles,C...>::sz4> obf_str_literal<seed,cycles,C...>::c = strC;

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

	//external functions
	void obf_init();
#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
	inline void obf_dbgPrint() {
		std::cout << "OBF_CONST_A=" << int(OBF_CONST_A) << " OBF_CONST_B=" << int(OBF_CONST_B) << " OBF_CONST_C=" << int(OBF_CONST_C) << std::endl;
		//auto c = obf_const_x(obf_compile_time_prng(ITHARE_OBF_SEED^UINT64_C(0xfb2de18f982a2d55), 1), obf_const_C_excluded);
	}
#endif

}//namespace obf
}//namespace ithare

 //macros; DON'T belong to the namespace...
#define ITHARE_OBFS_HELPER(seed,cycles,s) obf_str_literal<seed,cycles,(sizeof(s)>0?s[0]:'\0'),(sizeof(s)>1?s[1]:'\0'),(sizeof(s)>2?s[2]:'\0'),(sizeof(s)>3?s[3]:'\0'),\
							(sizeof(s)>4?s[4]:'\0'),(sizeof(s)>5?s[5]:'\0'),(sizeof(s)>6?s[6]:'\0'),(sizeof(s)>7?s[7]:'\0'),\
							(sizeof(s)>8?s[8]:'\0'),(sizeof(s)>9?s[9]:'\0'),(sizeof(s)>10?s[10]:'\0'),(sizeof(s)>11?s[11]:'\0'),\
							(sizeof(s)>12?s[12]:'\0'),(sizeof(s)>13?s[13]:'\0'),(sizeof(s)>14?s[14]:'\0'),(sizeof(s)>15?s[15]:'\0'),\
							(sizeof(s)>16?s[16]:'\0'),(sizeof(s)>17?s[17]:'\0'),(sizeof(s)>18?s[18]:'\0'),(sizeof(s)>19?s[19]:'\0'),\
							(sizeof(s)>20?s[20]:'\0'),(sizeof(s)>21?s[21]:'\0'),(sizeof(s)>22?s[22]:'\0'),(sizeof(s)>23?s[23]:'\0'),\
							(sizeof(s)>24?s[24]:'\0'),(sizeof(s)>25?s[25]:'\0'),(sizeof(s)>26?s[26]:'\0'),(sizeof(s)>27?s[27]:'\0'),\
							(sizeof(s)>28?s[28]:'\0'),(sizeof(s)>29?s[29]:'\0'),(sizeof(s)>30?s[30]:'\0'),(sizeof(s)>31?s[31]:'\0'),\
							(sizeof(s)>32?s[32]:'\0')/*one extra to generate an error if we're over*/>

//direct use of __LINE__ doesn't count as constexpr in MSVC - don't ask why...
//  AND we DO want to align other compilers with MSVC at least for ITHARE_OBF_CONSISTENT_XPLATFORM_IMPLICIT_SEEDS

//along the lines of https://stackoverflow.com/questions/19343205/c-concatenating-file-and-line-macros:
#define ITHARE_OBF_S1(x) #x
#define ITHARE_OBF_S2(x) ITHARE_OBF_S1(x)
#define ITHARE_OBF_LOCATION __FILE__ " : " ITHARE_OBF_S2(__LINE__)

#define ITHARE_OBF0(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0)>
#define ITHARE_OBF1(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1)>
#define ITHARE_OBF2(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2)>
#define ITHARE_OBF3(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3)>
#define ITHARE_OBF4(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4)>
#define ITHARE_OBF5(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5)>
#define ITHARE_OBF6(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6)>

#define ITHARE_OBF0I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0)>()
#define ITHARE_OBF1I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1)>()
#define ITHARE_OBF2I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2)>()
#define ITHARE_OBF3I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3)>()
#define ITHARE_OBF4I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4)>()
#define ITHARE_OBF5I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5)>()
#define ITHARE_OBF6I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6)>()

#define ITHARE_OBF0S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0),s)()
#define ITHARE_OBF1S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1),s)()
#define ITHARE_OBF2S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2),s)()
#define ITHARE_OBF3S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3),s)()
#define ITHARE_OBF4S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4),s)()
#define ITHARE_OBF5S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5),s)()
#define ITHARE_OBF6S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(ITHARE_OBF_LOCATION,0,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6),s)()

/*#else//_MSC_VER
#define ITHARE_OBF0(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0)>
#define ITHARE_OBF1(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1)>
#define ITHARE_OBF2(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2)>
#define ITHARE_OBF3(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3)>
#define ITHARE_OBF4(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4)>
#define ITHARE_OBF5(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5)>
#define ITHARE_OBF6(type) ithare::obf::obf_var<type,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6)>

#define ITHARE_OBF0I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0)>()
#define ITHARE_OBF1I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1)>()
#define ITHARE_OBF2I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2)>()
#define ITHARE_OBF3I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3)>()
#define ITHARE_OBF4I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4)>()
#define ITHARE_OBF5I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5)>()
#define ITHARE_OBF6I(c) obf_literal<decltype(c),c,ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6)>()

#define ITHARE_OBF0S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+0),s)().value()
#define ITHARE_OBF1S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+1),s)().value()
#define ITHARE_OBF2S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+2),s)().value()
#define ITHARE_OBF3S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+3),s)().value()
#define ITHARE_OBF4S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+4),s)().value()
#define ITHARE_OBF5S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+5),s)().value()
#define ITHARE_OBF6S(s) ITHARE_OBFS_HELPER(ITHARE_OBF_INIT_PRNG(__FILE__,__LINE__,__COUNTER__),ithare::obf::obf_exp_cycles((ITHARE_OBF_SCALE)+6),s)().value()
#endif*/

#else//ITHARE_OBF_SEED
namespace ithare {
	namespace obf {
#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		constexpr size_t obf_strlen(const char* s) {
			for (size_t ret = 0; ; ++ret, ++s)
				if (*s == 0)
					return ret;
		}

		//dbgPrint helpers
		template<class T>
		std::string obf_dbgPrintT() {
			return std::string("T(sizeof=") + std::to_string(sizeof(T)) + ")";
		}

		inline void obf_dbgPrint() {
		}
#endif

		//obf_literal_dbg
		//IMPORTANT: ANY API CHANGES MUST BE MIRRORED in obf_literal<>
		template<class T, T C>
		class obf_literal_dbg {
			static_assert(std::is_integral<T>::value);

		public:
			constexpr obf_literal_dbg() : val(C) {
			}
			T value() const {
				return val;
			}
			operator T() const {
				return value();
			}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
			static void dbgPrint(size_t offset = 0, const char* prefix = "") {
				std::cout << std::string(offset, ' ') << prefix << "obf_literal<" << obf_dbgPrintT<T>() << "," << C << std::endl;
			}
#endif
		private:
			T val;
		};

		//obf_var_dbg
		//IMPORTANT: ANY API CHANGES MUST BE MIRRORED in obf_var<>
		template<class T>
		class obf_var_dbg {
			static_assert(std::is_integral<T>::value);

		public:
			obf_var_dbg(T t) : val(t) {
			}
			template<class T2>
			obf_var_dbg(obf_var_dbg<T2> t) : val(T(t.value())) {
			}
			template<class T2,T2 C2>
			obf_var_dbg(obf_literal_dbg<T2,C2> t) : val(T(t.value())) {
			}
			obf_var_dbg& operator =(T t) {
				val = t;
				return *this;
			}
			template<class T2>
			obf_var_dbg& operator =(obf_var_dbg<T2> t) {
				val = T(t.value());
				return *this;
			}
			template<class T2, T2 C2>
			obf_var_dbg& operator =(obf_literal_dbg<T2,C2> t) {
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

			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE bool operator <(obf_literal_dbg<T2, C2> t) {
				return value() < t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE bool operator >(obf_literal_dbg<T2, C2> t) {
				return value() > t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE bool operator ==(obf_literal_dbg<T2, C2> t) {
				return value() == t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE bool operator !=(obf_literal_dbg<T2, C2> t) {
				return value() != t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE bool operator <=(obf_literal_dbg<T2, C2> t) {
				return value() <= t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE bool operator >=(obf_literal_dbg<T2, C2> t) {
				return value() > t.value();
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
			}
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

			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg& operator +=(obf_literal_dbg<T2, C2> t) {
				return *this += t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg& operator -=(obf_literal_dbg<T2, C2> t) {
				return *this -= t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg& operator *=(obf_literal_dbg<T2, C2> t) {
				return *this *= t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg& operator /=(obf_literal_dbg<T2, C2> t) {
				return *this /= t.value();
			}
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg& operator %=(obf_literal_dbg<T2, C2> t) {
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

			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg operator +(obf_literal_dbg<T2, C2> t) { return obf_var_dbg(value() + t.value()); }
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg operator -(obf_literal_dbg<T2, C2> t) { return obf_var_dbg(value() - t.value()); }
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg operator *(obf_literal_dbg<T2, C2> t) { return obf_var_dbg(value() * t.value()); }
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg operator /(obf_literal_dbg<T2, C2> t) { return obf_var_dbg(value() / t.value()); }
			template<class T2, T2 C2>
			ITHARE_OBF_FORCEINLINE obf_var_dbg operator %(obf_literal_dbg<T2, C2> t) { return obf_var_dbg(value() % t.value()); }

			//TODO: bitwise

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
			static void dbgPrint(size_t offset = 0,const char* prefix="") {
				std::cout << std::string(offset, ' ') << prefix << "obf_var_dbg<" << obf_dbgPrintT<T>() << ">" << std::endl;
			}
#endif

		private:
			typename T val;
		};

		inline void obf_init() {
		}

		//IMPORTANT: ANY API CHANGES MUST BE MIRRORED in obf_str_literal
		template<char... C>
		struct obf_str_literal_dbg {
			static constexpr size_t origSz = sizeof...(C);
			static constexpr char const str[sizeof...(C)+1] = { C...,'\0'};
			static constexpr size_t sz = obf_strlen(str);
			static_assert(sz > 0);
			static_assert(sz <= 32);

			ITHARE_OBF_FORCEINLINE std::string value() const {
				return std::string(str, origSz);
			}
			ITHARE_OBF_FORCEINLINE operator std::string() const {
				return value();
			}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
			static void dbgPrint(size_t offset = 0, const char* prefix = "") {
				std::cout << std::string(offset, ' ') << prefix << "obf_str_literal_dbg<'" << str << "'>" << std::endl;
			}
#endif
		};

	}//namespace obf
}//namespace ithare

#define ITHARE_OBF0(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF1(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF2(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF3(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF4(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF5(type) ithare::obf::obf_var_dbg<type>
#define ITHARE_OBF6(type) ithare::obf::obf_var_dbg<type>

#define ITHARE_OBF0I(c) obf_literal_dbg<decltype(c),c>()
#define ITHARE_OBF1I(c) obf_literal_dbg<decltype(c),c>()
#define ITHARE_OBF2I(c) obf_literal_dbg<decltype(c),c>()
#define ITHARE_OBF3I(c) obf_literal_dbg<decltype(c),c>()
#define ITHARE_OBF4I(c) obf_literal_dbg<decltype(c),c>()
#define ITHARE_OBF5I(c) obf_literal_dbg<decltype(c),c>()
#define ITHARE_OBF6I(c) obf_literal_dbg<decltype(c),c>()

#define ITHARE_OBFS_DBG_HELPER(s) obf_str_literal_dbg<(sizeof(s)>0?s[0]:'\0'),(sizeof(s)>1?s[1]:'\0'),(sizeof(s)>2?s[2]:'\0'),(sizeof(s)>3?s[3]:'\0'),\
							(sizeof(s)>4?s[4]:'\0'),(sizeof(s)>5?s[5]:'\0'),(sizeof(s)>6?s[6]:'\0'),(sizeof(s)>7?s[7]:'\0'),\
							(sizeof(s)>8?s[8]:'\0'),(sizeof(s)>9?s[9]:'\0'),(sizeof(s)>10?s[10]:'\0'),(sizeof(s)>11?s[11]:'\0'),\
							(sizeof(s)>12?s[12]:'\0'),(sizeof(s)>13?s[13]:'\0'),(sizeof(s)>14?s[14]:'\0'),(sizeof(s)>15?s[15]:'\0'),\
							(sizeof(s)>16?s[16]:'\0'),(sizeof(s)>17?s[17]:'\0'),(sizeof(s)>18?s[18]:'\0'),(sizeof(s)>19?s[19]:'\0'),\
							(sizeof(s)>20?s[20]:'\0'),(sizeof(s)>21?s[21]:'\0'),(sizeof(s)>22?s[22]:'\0'),(sizeof(s)>23?s[23]:'\0'),\
							(sizeof(s)>24?s[24]:'\0'),(sizeof(s)>25?s[25]:'\0'),(sizeof(s)>26?s[26]:'\0'),(sizeof(s)>27?s[27]:'\0'),\
							(sizeof(s)>28?s[28]:'\0'),(sizeof(s)>29?s[29]:'\0'),(sizeof(s)>30?s[30]:'\0'),(sizeof(s)>31?s[31]:'\0'),\
							(sizeof(s)>32?s[32]:'\0')/*one extra to generate an error if we're over*/>

#define ITHARE_OBF0S(s) ITHARE_OBFS_DBG_HELPER(s)()
#define ITHARE_OBF1S(s) ITHARE_OBFS_DBG_HELPER(s)()
#define ITHARE_OBF2S(s) ITHARE_OBFS_DBG_HELPER(s)()
#define ITHARE_OBF3S(s) ITHARE_OBFS_DBG_HELPER(s)()
#define ITHARE_OBF4S(s) ITHARE_OBFS_DBG_HELPER(s)()
#define ITHARE_OBF5S(s) ITHARE_OBFS_DBG_HELPER(s)()
#define ITHARE_OBF6S(s) ITHARE_OBFS_DBG_HELPER(s)()

#endif //ITHARE_OBF_SEED

#ifndef ITHARE_OBF_NO_SHORT_DEFINES//#define to avoid polluting global namespace w/o prefix
#define OBF0 ITHARE_OBF0
#define OBF1 ITHARE_OBF1
#define OBF2 ITHARE_OBF2
#define OBF3 ITHARE_OBF3
#define OBF4 ITHARE_OBF4
#define OBF5 ITHARE_OBF5
#define OBF6 ITHARE_OBF6

#define OBF0I ITHARE_OBF0I
#define OBF1I ITHARE_OBF1I
#define OBF2I ITHARE_OBF2I
#define OBF3I ITHARE_OBF3I
#define OBF4I ITHARE_OBF4I
#define OBF5I ITHARE_OBF5I
#define OBF6I ITHARE_OBF6I

#define OBF0S ITHARE_OBF0S
#define OBF1S ITHARE_OBF1S
#define OBF2S ITHARE_OBF2S
#define OBF3S ITHARE_OBF3S
#define OBF4S ITHARE_OBF4S
#define OBF5S ITHARE_OBF5S
#define OBF6S ITHARE_OBF6S
#endif

#endif//ithare_obf_obfuscate_h_included
