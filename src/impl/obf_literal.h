#ifndef ithare_obf_literal_h_included
#define ithare_obf_literal_h_included

//NOT intended to be #included directly
//  #include ../obf.h instead

//NB: does NOT contain obf_literal<> itself (it is kinda-public and resides in ../obf.h)

#include "obf_common.h"
#include "obf_prng.h"
#include "obf_injection.h"

//NB: principles for cross-platform obfuscations, laid out in obf_injection.h, _seem_ to be UNNECESSARY for literals
//  Current implementation still complies with them, but it _seems_ that if necessary, we can ignore them for literals

#ifdef ITHARE_OBF_SEED

namespace ithare {
	namespace obf {

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

		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
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
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
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

		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
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

	//version 3: System-Dependent Anti-Debugging
	struct obf_literal_context_version3_descr {//NB: to ensure 100%-compatible generation across platforms, probabilities MUST NOT depend on the platform, directly or indirectly
#if defined(ITHARE_OBF_INIT) && !defined(ITHARE_OBF_NO_ANTI_DEBUG)
		static constexpr ObfDescriptor descr = ObfDescriptor(true, 10, 100);
#else
		static constexpr ObfDescriptor descr = ObfDescriptor(false, 0, 0);
#endif
	};

#ifdef _MSC_VER
#include <intrin.h>

	//moving globals into header (along the lines of https://stackoverflow.com/a/27070265)
	template<class Dummy>
	struct obf_literal_context_version3_globals {
		static volatile uint8_t* obf_peb;
	};

	template<class Dummy>
	volatile uint8_t* obf_literal_context_version3_globals<Dummy>::obf_peb = nullptr;

	inline void obf_init_literal_context_version3() {//TODO/decide: ?should we obfuscate this function itself?
#ifdef _WIN64
		constexpr auto offset = 0x60;
		obf_literal_context_version3_globals<void>::obf_peb = (uint8_t*)__readgsqword(offset);
#else
		constexpr auto offset = 0x30;
		obf_literal_context_version3_globals<void>::obf_peb = (uint8_t*)__readfsdword(offset);
#endif
		return;
	}

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<3,T,seed> {
		using Traits = ObfTraits<T>;
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version3_descr::descr.min_cycles;

		//static constexpr T CC = obf_gen_const<T>(ITHARE_OBF_NEW_PRNG(seed, 1));
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T CC = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 1)>(consts);
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
#ifdef ITHARE_OBF_DEBUG_ANTI_DEBUG_ALWAYS_FALSE
			return y - CC;
#else
			return y - CC * T(1 + obf_literal_context_version3_globals<void>::obf_peb[2]);
#endif
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<3/*PEB*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << ">: CC=" << obf_dbgPrintC<T>(CC) << std::endl;
		}
#endif
	};
//_MSC_VER
#elif defined(__APPLE_CC__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <mach/task.h>
#include <mach/mach_init.h>
	
	//moving globals into header (along the lines of https://stackoverflow.com/a/27070265)
	template<class Dummy>
	struct obf_literal_context_version3_globals {
		static volatile uint64_t obf_kp_proc_p_flag;
		static volatile mach_port_t obf_mach_port;
	};

	template<class Dummy>
	volatile uint64_t obf_literal_context_version3_globals<Dummy>::obf_kp_proc_p_flag = 0;
	template<class Dummy>
	volatile mach_port_t obf_literal_context_version3_globals<Dummy>::obf_mach_port = MACH_PORT_NULL;

	inline void obf_init_literal_context_version3() {//TODO/decide: ?should we obfuscate this function itself?
		
		//#1. Detecting PTRACE: adaptation from https://developer.apple.com/library/content/qa/qa1361/_index.html
		int                 junk;
		int                 mib[4];
		struct kinfo_proc   info;
		size_t              size;

		// Initialize the flags so that, if sysctl fails for some bizarre 
		// reason, we get a predictable result.

		info.kp_proc.p_flag = 0;

		// Initialize mib, which tells sysctl the info we want, in this case
		// we're looking for information about a specific process ID.

		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PID;
		mib[3] = getpid();

		// Call sysctl.

		size = sizeof(info);
		junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
		assert(junk == 0);

		// We're being debugged if the P_TRACED flag is set.

		//return ( (info.kp_proc.p_flag & P_TRACED) != 0 );		
		obf_literal_context_version3_globals<void>::obf_kp_proc_p_flag = info.kp_proc.p_flag;//we'll check for P_TRACED a bit later
		//std::cout << "p_flag: 0x" << std::hex << obf_literal_context_version3_globals<void>::obf_kp_proc_p_flag << std::endl;
		
		//#2. Detecting Mach: adaptation from https://zgcoder.net/ramblings/osx-debugger-detection.html

		mach_msg_type_number_t count = 0;
		exception_mask_t masks[EXC_TYPES_COUNT];
		mach_port_t ports[EXC_TYPES_COUNT];
		exception_behavior_t behaviors[EXC_TYPES_COUNT];
		thread_state_flavor_t flavors[EXC_TYPES_COUNT];
		exception_mask_t mask = EXC_MASK_ALL & ~(EXC_MASK_RESOURCE | EXC_MASK_GUARD);

		kern_return_t result = task_get_exception_ports(mach_task_self(), mask, masks, &count, ports, behaviors, flavors);
		if (result == KERN_SUCCESS)
		{
			for (mach_msg_type_number_t portIndex = 0; portIndex < count; portIndex++)
			{
				obf_literal_context_version3_globals<void>::obf_mach_port = ports[portIndex];
				if (MACH_PORT_VALID(obf_literal_context_version3_globals<void>::obf_mach_port))
					return;//leaving obf_mach_port to a value which will return MACH_PORT_VALID(obf_mach_port) as true
			}
		}
	}

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<3,T,seed> {
		using Traits = ObfTraits<T>;
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version3_descr::descr.min_cycles;

		//static constexpr T CC = obf_gen_const<T>(ITHARE_OBF_NEW_PRNG(seed, 1));
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T CC = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 1)>(consts);
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
#ifdef ITHARE_OBF_DEBUG_ANTI_DEBUG_ALWAYS_FALSE
			return y - CC;
#else
			///std::cout << "p_flag|P_TRACED: 0x" << std::hex << (obf_literal_context_version3_globals<void>::obf_kp_proc_p_flag&P_TRACED) << std::endl;
			return y - CC * T(1 + (obf_literal_context_version3_globals<void>::obf_kp_proc_p_flag & P_TRACED) + MACH_PORT_VALID(obf_literal_context_version3_globals<void>::obf_mach_port));
#endif
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<3/*PTRACE+MACH*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << ">: CC=" << obf_dbgPrintC<T>(CC) << std::endl;
		}
#endif
	};
//__APPLE_CC__
#else
#warning No support for anti-debugging for this platform (yet) - defaulting to simple read-volatile (copy of version<1> above)  
	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	struct ObfLiteralContext_version<3,T,seed> {
		using Traits = ObfTraits<T>;
		constexpr static OBFCYCLES context_cycles = obf_literal_context_version1_descr::descr.min_cycles;

		//static constexpr T CC = obf_gen_const<T>(ITHARE_OBF_NEW_PRNG(seed, 1));
		static constexpr std::array<T, 3> consts = { OBF_CONST_A,OBF_CONST_B,OBF_CONST_C };
		constexpr static T CC = obf_random_const<ITHARE_OBF_NEW_PRNG(seed, 1)>(consts);
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static T final_surjection(T y) {
			return y - T(c);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "ObfLiteralContext_version<3/*system-dependent defaulting to read-volatile*/," << obf_dbgPrintT<T>() << "," << obf_dbgPrintSeed<seed>() << ">: CC=" << obf_dbgPrintC<T>(CC) << std::endl;
		}
#endif
	private:
		static /*volatile*/ T c;//TODO! - return back volatile
	};

	template<class T, ITHARE_OBF_SEEDTPARAM seed>
	/*volatile*/ T ObfLiteralContext_version<3, T, seed>::c = CC;
	
	inline void obf_init_literal_context_version3() {
	}
	
#endif//_MSC_VER || __APPLE_CC__

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

		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x + CC;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
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

		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			return x;
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
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

		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static constexpr T final_injection(T x) {
			ITHARE_OBF_DECLAREPRNG_INFUNC seedc = ITHARE_OBF_COMBINED_PRNG(seed,seed2);
			return WhichType::template final_injection<seedc>(x);
		}
		template<ITHARE_OBF_SEEDTPARAM seed2>
		ITHARE_OBF_FORCEINLINE static /*non-constexpr*/ T final_surjection(T y) {
			ITHARE_OBF_DECLAREPRNG_INFUNC seedc = ITHARE_OBF_COMBINED_PRNG(seed,seed2);
			return WhichType::template final_surjection<seedc>(y);
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

		/*constexpr static OBFCYCLES literal_cycles = 0;
		template<class T2, T2 C, ITHARE_OBF_SEEDTPARAM seed2>
		struct literal {
			using type = obf_literal_ctx<T2, C, ObfZeroLiteralContext<T2>, seed2, literal_cycles>;
		};*/

		struct InjectionRequirements {
			static constexpr size_t exclude_version = size_t(-1);
			static constexpr bool is_constexpr = true;
			static constexpr bool only_bijections = false;
			static constexpr bool no_physical_size_increase = false;
		};
		using Injection = obf_injection<T, Context, ITHARE_OBF_NEW_PRNG(seed, 1), cycles,InjectionRequirements>;
	public:
		ITHARE_OBF_FORCEINLINE constexpr obf_literal_ctx() : val(Injection::template injection<ITHARE_OBF_NEW_PRNG(seed, 2)>(C)) {
		}
		ITHARE_OBF_FORCEINLINE constexpr T value() const {
			return Injection::template surjection<ITHARE_OBF_NEW_PRNG(seed, 3)>(val);
		}

#ifdef ITHARE_OBF_DBG_ENABLE_DBGPRINT
		static void dbgPrint(size_t offset = 0, const char* prefix = "") {
			std::cout << std::string(offset, ' ') << prefix << "obf_literal_ctx<" << obf_dbgPrintT<T>() << "," << obf_dbgPrintC<T>(C) << "," << obf_dbgPrintSeed<seed>() << "," << cycles << ">" << std::endl;
			Injection::dbgPrint(offset + 1);
		}
		static void dbgCheck() {
			typename Injection::return_type c = Injection::template injection<seed>(C);
			T cc = Injection::template surjection<seed>(c);
			assert(cc == C);
		}
#endif
	private:
		typename Injection::return_type val;
	};
  } //namespace obf
} //namespace ithare

#endif //ITHARE_OBF_SEED

#endif //ithare_obf_literal_h_included
