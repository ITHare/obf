#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

//{ internal debugging: should go BEFORE #include obf.h
//#define ITHARE_OBF_CRYPTO_PRNG

//enable assert() in Release
//#undef NDEBUG

#define ITHARE_OBF_SEED 0x0c7dfa61a871b133
#define ITHARE_OBF_SEED2 0xdacb5ca59a237d13 

#define ITHARE_OBF_CONSISTENT_XPLATFORM_IMPLICIT_SEEDS
//---#define ITHARE_OBF_DBG_MAP --- obsolete?
//---#define ITHARE_OBF_DBG_MAP_LOG -- obsolete?
#ifndef NDEBUG
#define ITHARE_OBF_DBG_RUNTIME_CHECKS
#endif

#define ITHARE_OBF_INIT 
//enables rather nasty obfuscations (including PEB-based debugger detection),
//  but requires you to call obf_init() BEFORE ANY obf<> objects are used. 
//  As a result - it can backfire for obfuscations-used-from-global-constructors :-(.

//#define ITHARE_OBF_NO_ANTI_DEBUG
//disables built-in anti-debug kinda-protections in a clean way
#define ITHARE_OBF_DEBUG_ANTI_DEBUG_ALWAYS_FALSE
//makes built-in anti-debugger kinda-protections to return 'not being debugged' (NOT clean, use ONLY for debugging purposes)

//THE FOLLOWING MUST BE NOT USED FOR PRODUCTION BUILDS:
#define ITHARE_OBF_DBG_ENABLE_DBGPRINT
//enables dbgPrint()

//} internal debugging: should go BEFORE #include obf.h

#include "../src/obf.h"
#include <chrono>//for benchmarking

using namespace ithare::obf;

#if 0
template<class T, size_t N>
class ObfBitUint {
private:
	T val;
	static constexpr T mask = ((T)1 << N) - (T)1;
	static_assert(N < sizeof(T) * 8);
public:
	ObfBitUint() : val() {}
	ObfBitUint(T x) { val = x&mask; }
	operator T() const { assert((val&mask) == val); return val & mask; }
	ObfBitUint operator *(ObfBitUint x) const { return ObfBitUint(val * x.val); }
	ObfBitUint operator +(ObfBitUint x) { return ObfBitUint(val + x.val); }
	ObfBitUint operator -(ObfBitUint x) { return ObfBitUint(val - x.val); }
	ObfBitUint operator %(ObfBitUint x) { return ObfBitUint(val%x.val);/*TODO: double-check*/ }
	ObfBitUint operator /(ObfBitUint x) { return ObfBitUint(val / x.val); /*TODO: double-check*/ }
};
#endif

/*class MyClassName {
public:
	virtual void f() {
		std::cout << "My Class" << std::endl;
	}
};*/

/*template<class T, T C, OBFSEED seed, OBFCYCLES cycles>
constexpr obf_literal<T, C, seed, cycles> obfl(T C, OBFSEED seed, OBFCYCLES cycles) {
	return obf_literal<T, C, seed, cycles>();
}*/

#if !defined(NDEBUG) && defined(ITHARE_OBF_DBG_ENABLE_DBGPRINT)
#define DBGPRINT(x) static bool x##Printed = false;\
if(!x##Printed) { \
  std::cout << #x << std::endl;\
  x.dbgPrint(1);\
  x##Printed = true;\
}
#else
#define DBGPRINT(x) 
#endif

/*ITHARE_OBF_NOINLINE OBF6(int64_t) factorial(OBF6(size_t) x) {
    DBGPRINT(x)
	auto one = OBF5I(1);
	DBGPRINT(one)
	OBF3(int64_t) ret = one;
	DBGPRINT(ret)
	for (OBF3(size_t) i = 1; i <= x; ++i) {
		DBGPRINT(i)
		ret *= i;
	}
	return ret;
}*/

class MyException {
public:
	MyException(std::string msg)
		: message(msg) {
	}
	virtual const char* what() const {
		return message.c_str();
	}

private:
	std::string message;
};

ITHARE_OBF_NOINLINE OBF6(int64_t) factorial(OBF6(int64_t) x) {
	DBGPRINT(x)
	if (x < 0)
		throw MyException(OBF5S("Negative argument to factorial!"));
	OBF3(int64_t) ret = 1;
	DBGPRINT(ret)
	for (OBF3(int64_t) i = 1; i <= x; ++i) {
		DBGPRINT(i);
		ret *= i;
	}
	return ret;
}

/*ITHARE_OBF_NOINLINE int64_t factorial(int64_t x) {
	if (x < 0)
		throw MyException("Negative argument to factorial!");
	int64_t ret = 1;
	for (int64_t i = 1; i <= x; ++i) {
		ret *= i;
	}
	return ret;
}*/

/*template<class T,size_t N>
constexpr std::array<typename std::remove_const<T>::type,N-1> obf_calc_obf_string_literal(T (&lit)[N]) {
	std::array<typename std::remove_const<T>::type, N-1> ret = {};
	for (size_t i = 0; i < N-1; ++i) {
		ret[i] = lit[i]+'a';
	}
	return ret;
}

template<class T, size_t N>
constexpr std::string obf_string_literal(T(&lit)[N]) {
	static auto constexpr C = obf_calc_obf_string_literal(lit);
	static volatile auto c = C;

	T ret0[N-1];
	for (size_t i = 0; i < N-1; ++i) {
		ret0[i] = x[i] - 'a';
	}

	return std::string(ret0, N - 1);
};

constexpr auto STRLIT = obf_calc_obf_string_literal("Hello, obfuscation!");
auto strLit = STRLIT;*/

class Benchmark {
	std::chrono::high_resolution_clock::time_point start;

public:
	Benchmark() {
		start = std::chrono::high_resolution_clock::now();
	}
	int32_t us() {
		auto stop = std::chrono::high_resolution_clock::now();
		auto length = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
		return (int32_t)length.count();
	}
};

int main(int argc, char** argv) {
#ifndef NDEBUG
	freopen("ConsoleApplication1.log", "w", stdout);
#endif
	printf("%s %d\n", __FILE__, __LINE__);
	//auto xxx = OBFS_("Hello!")::str_obf();
	//auto hello = OBF5S("Hello, cruel MOG world! :-(");
	//DBGPRINT(hello)
	//std::cout << hello.value() << std::endl;
	//constexpr int sz = GetArrLength("hello!");
	//std::cout << sz << std::endl;
	//std::cout << const_str<'H', 'e', 'l', 'l', 'o'>().value() << std::endl;
	//std::cout << obf_string_literal("Hello, obfuscation!") << std::endl;

	obf_init();
	//obf_injection_version<7/*split into ObfBitUint<>*/, uint32_t, 4675287498475271348, 189> inj;
	/*ObfBitUint<size_t, 31> x = 12832197;
	auto y = obf_mul_inverse_mod2n(x);
	assert(y*x == 1);
	using Lit0 = obf_literal < size_t, 123, OBFUSCATE_SEED+0, 500>;
	Lit0::dbgPrint();
	Lit0 c;
	//using inj = obf_injection_with_constant<uint32_t, OBFUSCATE_SEED, 8>;
	//printf("%d, %d, %d\n",inj::C, inj::injection(123,inj::C), inj::surjection(inj::injection(123,inj::C),inj::C));
	//std::cout << c.value() << std::endl;
	//constexpr static const char* loc = OBF_LOCATION;
	//std::cout << loc;
	//constexpr static OBFSEED seed = obf_seed_from_file_line(LOCATION, 0);
	//obf_var<size_t, 0, obf_exp_cycles(OBFSCALE + 0)> var(c.value());
	//OBFCYCLES c0 = obf_exp_cycles(0);
	OBF0(size_t) var(c.value());
	var.dbgPrint();
	OBF0(size_t) var2(c.value());
	var2.dbgPrint();
	var = var.value() + 1;
	std::cout << var.value() << std::endl;
	return 0;*/
	//int n = obf_mul_inverse_mod2n(0x66666667U);
	//std::cout << std::hex << n << std::endl;
	//std::cout << argc / 5 << std::endl;

	//obf_dbgPrint();
	//std::string s = obf_literal<decltype(""), "",0, 1>();
	//std::string s = deobfuscate<seed,cycles>(constexpr obfuscate<seed,XYZ>("Long string which makes lots of sense"));
	if (argc <= 1)
		return 0;
	ObfNonBlockingCode obf_nb_guard;
	int x = atoi(argv[1]);
	int64_t total = 0;
	int64_t f = 0;
	try {
		Benchmark bm;
		for (int i = 0; i < 1000; ++i) {
			auto ff = factorial(x);
			DBGPRINT(ff)
			f = ff;
			//DBGPRINT(f)
			total += f;
		}
		std::cout << "factorial(" << x << ") = " << f << ", avg " << bm.us() << "ns per call" << std::endl;
		std::cout << "dummy (to avoid optimizing out)=" << total << std::endl;
	}
	catch (MyException& x) {
		std::cout << "exception:" << x.what() << std::endl;
	}
	//MyClassName mc;
	//mc.f();
	return 0;
}
