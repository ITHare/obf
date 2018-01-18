#include "lest.hpp"
#include "../src/obf.h"
using namespace ithare::obf;


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
	//DBGPRINT(x)
	if (x < 0)
		throw MyException(OBF5S("Negative argument to factorial!"));
	OBF3(int64_t) ret = 1;
	//DBGPRINT(ret)
	for (OBF3(int64_t) i = 1; i <= x; ++i) {
		//DBGPRINT(i);
		ret *= i;
	}
	return ret;
}

const lest::test spec[] = {
	CASE("factorial(18)") {
		EXPECT( factorial(18) == UINT64_C(6402373705728000));
	},
	CASE("factorial(19)") {
		EXPECT( factorial(19) == UINT64_C(121645100408832000));
	},
	CASE("factorial(20)") {
		EXPECT( factorial(20) == UINT64_C(2432902008176640000));
	},
	CASE("factorial(19)") {
		EXPECT( factorial(21) == UINT64_C(14197454024290336768));//with wrap-around(!)
	},
};

int main(int argc, char** argv) {
	obf_init();
	return lest::run(spec,argc,argv);
}
