//HELPER file for testing, generates randomized command lines for a testing session
//  Using .cpp to avoid writing the same logic twice in *nix .sh and Win* .bat

#include <iostream>
#include <string.h>
#include <string>
#include <stdio.h>
#include <assert.h>

#if defined(__APPLE_CC__) || defined(__linux__)
std::string buildRelease(std::string defines) {
	return std::string("gcc -O3 -DNDEBUG ") + defines + " -o obftemp -std=c++1z -lstdc++ ../official.cpp";
}
std::string buildDebug(std::string defines) {
	return std::string("gcc ") + defines + " -o obftemp -std=c++1z -lstdc++ ../official.cpp";
}
std::string genRandom64() {
	static FILE* frnd = fopen("/dev/urandom","rb");
	if(frnd==0) {
		std::cout << "Cannot open /dev/urandom, aborting" << std::endl;
		abort();
	}
	uint8_t buf[8];
	size_t rd = fread(buf,sizeof(buf),1,frnd);
	if( rd != 1 ) {
		std::cout << "Problems reading from /dev/urandom, aborting" << std::endl;
		abort();
	}

	char buf2[sizeof(buf)*2+1];
	for(size_t i=0; i < sizeof(buf) ; ++i) {
		assert(buf[i]<=255);
		sprintf(&buf2[i*2],"%02x",buf[i]);
	}
	return std::string(buf2);
}
std::string exitCheck(bool expectok = true) {
	if( expectok )
		return std::string("if [ ! $? -eq 0 ]; then\n  exit 1\nfi");
	else
		return std::string("if [ ! $? -ne 0 ]; then\n  exit 1\nfi");
}
std::string echo(std::string s) {
	return std::string("echo " + s);
}
std::string run() {
	return std::string("./obftemp");
}
std::string checkObfuscation(bool obfuscated) {//very weak heuristics, but still better than nothing
	std::string ret = std::string("strings obftemp | grep Negative");//referring to string "Negative value of factorial()"
	return ret + "\n" + exitCheck(!obfuscated);
}
std::string cleanup() {
	return std::string("rm obftemp");	
}
#elif defined(WIN32)
#error //for now
#else
#error "Unrecognized platform for randomized testing"
#endif 

std::string genSeed() {
	return std::string(" -DITHARE_OBF_SEED=0x") + genRandom64();
}

std::string genSeeds() {
	return genSeed() + " -DITHARE_OBF_SEED2=0x" + genRandom64();
}

int usage() {
	std::cout << "Usage:" << std::endl;
	std::cout << "helper [-nodefinetests] <nrandomtests>" << std::endl; 
	return 1;
}

void issueCommand(std::string cmd) {
	std::cout << echo(cmd) << std::endl;
	std::cout << cmd << std::endl;
}

void buildCheckRunCheck(std::string cmd,bool obfuscated=true) {
	issueCommand(cmd);
	std::cout << exitCheck() << std::endl;
	std::cout << checkObfuscation(obfuscated) << std::endl;
	issueCommand(run());
	std::cout << exitCheck() << std::endl;
}

void genDefineTests() {
	std::cout << echo( std::string("=== -Define Test 1/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildDebug(""),false);
	std::cout << echo( std::string("=== -Define Test 2/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildRelease(""),false);
	std::cout << echo( std::string("=== -Define Test 3/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildDebug(genSeed()));
	std::cout << echo( std::string("=== -Define Test 4/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildRelease(genSeed()));
	std::cout << echo( std::string("=== -Define Test 5/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildDebug(genSeeds()));
	std::cout << echo( std::string("=== -Define Test 6/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildRelease(genSeeds()));
	std::cout << echo( std::string("=== -Define Test 7/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildDebug(genSeeds()+" -DITHARE_OBF_DBG_RUNTIME_CHECKS"));
	std::cout << echo( std::string("=== -Define Test 8/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildRelease(genSeeds()+" -DITHARE_OBF_DBG_RUNTIME_CHECKS"));
	std::cout << echo( std::string("=== -Define Test 9/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildDebug(genSeeds()+" -DITHARE_OBF_CRYPTO_PRNG"));
	std::cout << echo( std::string("=== -Define Test 10/10 ===" ) ) << std::endl;
	buildCheckRunCheck(buildRelease(genSeeds()+" -DITHARE_OBF_CRYPTO_PRNG"));
}

void genRandomTests(size_t n) {
	for(size_t i=0; i < n; ++i ) {
		std::string extra;
		if( i%3 == 0 )//every third, non-exclusive
			extra += " -DITHARE_OBF_DBG_RUNTIME_CHECKS";
		std::cout << echo( std::string("=== Random Test ") + std::to_string(i+1) + "/" + std::to_string(n) + " ===" ) << std::endl;
		std::string defines = genSeeds()+" -DITHARE_OBF_INIT -ITHARE_OBF_CONSISTENT_XPLATFORM_IMPLICIT_SEEDS"+extra;
		if( i%4 == 0 ) 
			buildCheckRunCheck(buildDebug(defines));
		else
			buildCheckRunCheck(buildRelease(defines));
	}
}

void genCleanup() {
	issueCommand(cleanup());
}

int main(int argc, char** argv) {
	bool nodefinetests = false;
	
	int argcc = 1;
	while(argcc<argc) {
		if(strcmp(argv[argcc],"-nodefinetests") == 0) {
			nodefinetests = true;
			argcc++;
		}
		//other options go here
		else
			break;
	}
	
	if(argcc >= argc)
		return usage();
	char* end;
	unsigned long nrandom = strtoul(argv[argcc],&end,10);
	if(*end!=0)
		return usage();
		
	if(!nodefinetests)
		genDefineTests();

	genRandomTests(nrandom);
	genCleanup();
}
