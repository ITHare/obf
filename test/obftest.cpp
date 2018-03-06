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

#include "../../kscope/test/lest.hpp"
#include "../src/obf.h"

#ifdef ITHARE_OBF_TEST_NO_NAMESPACE
using namespace ithare::obf;
using namespace ithare::obf::tls;
#else
#define ITOBF ithare::obf::
#endif

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

ITHARE_OBF_NOINLINE OBFI6(uint64_t) factorial(OBFI6(int64_t) x) {
	//DBGPRINT(x)
	if (x < 0)
		throw MyException(OBFS5L("Negative argument to factorial!"));
	OBFI3(int64_t) ret = 1;
	//DBGPRINT(ret)
	for (OBFI3(int64_t) i = 1; i <= x; ++i) {
		//DBGPRINT(i);
		ret *= i;
	}
	return ret;
}

/*
class Benchmark {
	std::chrono::high_resolution_clock::time_point start;

public:
	Benchmark() {
		start = std::chrono::high_resolution_clock::now();
	}
	int64_t us() {
		auto stop = std::chrono::high_resolution_clock::now();
		auto length = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
		return (int64_t)length.count();
	}
};*/

#define NBENCH 1000

#ifdef __GNUC__ //warnings in lest.hpp - can only disable :-(
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#ifdef _MSC_VER //warnings in lest.hpp - can only disable :-(
#pragma warning(push)
#pragma warning(disable:4100)
#endif

const lest::test module[] = {
	CASE("obf::factorial()",) {
		auto f = factorial(17); ITHARE_OBF_DBGPRINT(f);
		EXPECT(f==UINT64_C(355687428096000));
		EXPECT( factorial(18) == UINT64_C(6402373705728000));
		EXPECT( factorial(19) == UINT64_C(121645100408832000));
		EXPECT( factorial(20) == UINT64_C(2432902008176640000));
		EXPECT( factorial(21) == UINT64_C(14197454024290336768));//with wrap-around(!)
	},
	/*
	CASE("benchmarks") {
#if !defined(ITHARE_OBF_ENABLE_AUTO_DBGPRINT) || ITHARE_OBF_ENABLE_AUTO_DBGPRINT == 2//excluding platform-specific stuff to avoid spurious changes to obftemp.txt with -DITHARE_OBF_ENABLE_AUTO_DBGPRINT
		{
			Benchmark bm0;
			for (size_t i = 0; i < NBENCH; ++i) {
				uint8_t inp[16] = { 0 };
				uint8_t out[16] = { 0 };
				test_chacha_cipher0(inp, out);
			}
			std::cout << "chacha_cipher():" << ( bm0.us() * 1000 / NBENCH ) << " nanoseconds" << std::endl; 
		}
		{
			Benchmark bm1;
			for (size_t i = 0; i < NBENCH; ++i) {
				uint8_t inp[16] = { 0 };
				uint8_t out[16] = { 0 };
				test_chacha_cipher1(inp, out);
			}
			std::cout << "OBF3(chacha_cipher)():" << (bm1.us() * 1000 / NBENCH) << " nanoseconds" << std::endl;
		}
		{
			Benchmark bmrfc;
			for (size_t i = 0; i < NBENCH; ++i) {
				uint8_t inp[16] = { 0 };
				uint8_t out[16] = { 0 };
				test_chacha_cipher_rfc(inp, out);
			}
			std::cout << "OBF4(chacha_cipher)():" << (bmrfc.us() * 1000 / NBENCH) << " nanoseconds" << std::endl;
		}
#endif
	}*/
};

/*
int main(int argc, char** argv) {
#if !defined(ITHARE_OBF_ENABLE_AUTO_DBGPRINT) || ITHARE_OBF_ENABLE_AUTO_DBGPRINT == 2//excluding platform-specific stuff to avoid spurious changes to obftemp.txt with -DITHARE_OBF_ENABLE_AUTO_DBGPRINT
		std::cout << "__cplusplus=" << __cplusplus << std::endl;
#ifdef _MSC_VER
		std::cout << "_MSC_VER=" << _MSC_VER << " _MSC_FULL_VER=" << _MSC_FULL_VER << " _MSC_BUILD=" << _MSC_BUILD << std::endl;
#endif
#ifdef __GNUC__
		std::cout << "__GNUC__=" << __GNUC__ << " __GNUC_MINOR__=" << __GNUC_MINOR__ << " __GNUC_PATCHLEVEL__=" << __GNUC_PATCHLEVEL__ << std::endl;
#endif
#ifdef __clang__
		std::string modifier = "";
#ifdef __apple_build_version__
		modifier = "__apple_build_version__: ";
#endif
		std::cout << modifier << "__clang_major__=" << __clang_major__ << " __clang_minor__=" << 	__clang_minor__ << " __clang_patchlevel__=" << __clang_patchlevel__ << std::endl;
#endif
		std::cout << "sizeof(void*) == " << sizeof(void*) << std::endl;
#endif
#ifdef ITHARE_OBF_SEED
		std::cout << "ITHARE_OBF_SEED=" << std::hex << ITHARE_OBF_SEED << std::dec << std::endl;
#endif
#ifdef ITHARE_OBF_SEED2
		std::cout << "ITHARE_OBF_SEED2=" << std::hex << ITHARE_OBF_SEED2 << std::dec << std::endl;
#endif

	ITOBF obf_init();
	{
		ITOBF ObfNonBlockingCode obf_nb_guard;	
		int err = lest::run(spec,argc,argv);
		if(err)
			return err;
	}//~ObfNonBlockingCode() is called here 
	return lest::run(spec,argc,argv);//running once again, but with a chance for ~ObfNonBlockingCode() to kick in
}*/

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif


extern lest::tests& specification();

MODULE( specification(), module )
