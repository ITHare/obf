// ConsoleApplication1.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#define ITHARE_OBF_ENABLE_DBGPRINT
//#define ITHARE_OBF_NO_SHORT_DEFINES - use to avoid name clashes with OBF(); would require using ITHARE_OBF macros instead of OBF ones
#include "..\..\src\obfuscate.h"

using namespace ithare::obf;

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

#ifndef NDEBUG
#define ENABLE_DBGPRINT
#endif

/*template<class T, T C, OBFSEED seed, OBFCYCLES cycles>
constexpr obf_literal<T, C, seed, cycles> obfl(T C, OBFSEED seed, OBFCYCLES cycles) {
	return obf_literal<T, C, seed, cycles>();
}*/

#ifdef ENABLE_DBGPRINT
#define DBGPRINT(x) static bool x##Printed = false;\
if(!x##Printed) { \
  std::cout << #x << std::endl;\
  x.dbgPrint(1);\
  x##Printed = true;\
}
#else
#define DBGPRINT(x) 
#endif

ITHARE_OBF_NOINLINE OBF6(int64_t) factorial(OBF6(size_t) x) {
    DBGPRINT(x)
	auto one = OBF5L(1);
	DBGPRINT(one)
	OBF3(int64_t) ret = one;
	DBGPRINT(ret)
	for (OBF3(size_t) i = 1; i <= x; ++i) {
		DBGPRINT(i)
		ret *= i;
	}
	return ret;
}

/*template<class T,size_t N,OBFSEED seed,OBFCYCLES cycles>
class obf_string_literal {
public:
	constexpr obf_string_literal(const std::array<T,N-1>& )
};*/

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

template<size_t N>
constexpr std::array<char, N> obf_str(const char* s) {
	std::array<char, N> ret = {};
	for (size_t i = 0; i < N; ++i) {
		ret[i] = s[i] + 'a';
	}
	return ret;
}

template<size_t N>
std::string deobf_str(std::array<char, N> c) {
	std::string ret;
	for (size_t i = 0; i < N; ++i)
		ret.push_back(c[i] - 'a');
	return ret;
}

constexpr size_t obf_strlen(const char* s) {
	for (size_t ret = 0; ; ++ret, ++s)
		if (*s == 0)
			return ret;
}

template<char... C>
struct const_str {
	static constexpr size_t origSz = sizeof...(C);
	static constexpr char const str[sizeof...(C)] = { C... };
	static constexpr size_t sz = obf_strlen(str);
	static constexpr auto const strC = obf_str<sz>(str);

	static std::array<char, sz> c;//TODO: volatile
	std::string value() const {
		return deobf_str(c);
	}
};

template<char... C>
std::array<char, const_str<C...>::sz> const_str<C...>::c = strC;

/*constexpr uint64_t obf_string_hash(const char* s) {//TODO: replace with crypto-level hash; now - djb2 by Bernstein
	uint64_t ret = 5381;
	for (const char* p = file; *p; ++p)
		ret = ((ret << 5) + ret) + *p;
	return ret;
}*/

/*template<char C>
struct ttt {
	static constexpr char c = char(C + 1);

	char value() const { return c;  }
};*/

#define OBFS(lit) const_str<(sizeof(lit)>0?lit[0]:'\0'),(sizeof(lit)>1?lit[1]:'\0'),(sizeof(lit)>2?lit[2]:'\0'),(sizeof(lit)>3?lit[3]:'\0'),\
							(sizeof(lit)>4?lit[4]:'\0'),(sizeof(lit)>5?lit[5]:'\0'),(sizeof(lit)>6?lit[6]:'\0'),(sizeof(lit)>7?lit[7]:'\0'),\
							(sizeof(lit)>8?lit[8]:'\0'),(sizeof(lit)>9?lit[9]:'\0'),(sizeof(lit)>10?lit[10]:'\0'),(sizeof(lit)>11?lit[11]:'\0'),\
							(sizeof(lit)>12?lit[12]:'\0'),(sizeof(lit)>13?lit[13]:'\0'),(sizeof(lit)>14?lit[14]:'\0'),(sizeof(lit)>15?lit[15]:'\0'),\
							(sizeof(lit)>16?lit[16]:'\0'),(sizeof(lit)>17?lit[17]:'\0'),(sizeof(lit)>18?lit[18]:'\0'),(sizeof(lit)>19?lit[19]:'\0'),\
							(sizeof(lit)>20?lit[20]:'\0'),(sizeof(lit)>21?lit[21]:'\0'),(sizeof(lit)>22?lit[22]:'\0'),(sizeof(lit)>23?lit[23]:'\0'),\
							(sizeof(lit)>24?lit[24]:'\0'),(sizeof(lit)>25?lit[25]:'\0'),(sizeof(lit)>26?lit[26]:'\0'),(sizeof(lit)>27?lit[27]:'\0'),\
							(sizeof(lit)>28?lit[28]:'\0'),(sizeof(lit)>29?lit[29]:'\0'),(sizeof(lit)>30?lit[30]:'\0'),(sizeof(lit)>31?lit[31]:'\0')\
							>().value()
//TODO: OBFS_LONG(lit)

int main(int argc, char** argv) {
#ifndef NDEBUG
	freopen("ConsoleApplication1.log", "w", stdout);
#endif
	std::cout << OBFS("Hello!") << std::endl;
	//constexpr int sz = GetArrLength("hello!");
	//std::cout << sz << std::endl;
	//std::cout << const_str<'H', 'e', 'l', 'l', 'o'>().value() << std::endl;
	//std::cout << obf_string_literal("Hello, obfuscation!") << std::endl;

	obf_init();
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
	std::cout << argc / 5 << std::endl;

	obf_dbgPrint();
	//std::string s = obf_literal<decltype(""), "",0, 1>();
	//std::string s = deobfuscate<seed,cycles>(constexpr obfuscate<seed,XYZ>("Long string which makes lots of sense"));
	if (argc <= 1)
		return 0;
	int x = atoi(argv[1]);
	auto f = factorial(x);
	DBGPRINT(f)
	int64_t ff = f;
	std::cout << "factorial(" << x << ") = " << ff << std::endl;

	return 0;
}
