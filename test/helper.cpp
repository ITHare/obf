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
std::string setup() {
	return std::string("#!/bin/sh");
}
std::string cleanup() {
	return std::string("rm obftemp");	
}
#elif defined(_WIN32)
#include <windows.h>

std::string replace_string(std::string subject, std::string search,//adapted from https://stackoverflow.com/a/14678964
	std::string replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}
std::string buildRelease(std::string defines_) {
	std::string defines = replace_string(defines_, " -D", " /D");
	return std::string("cl /permissive- /GS /GL /W3 /Gy /Zc:wchar_t /Gm- /O2 /sdl /Zc:inline /fp:precise /DNDEBUG /D_CONSOLE /D_UNICODE /DUNICODE /errorReport:prompt /WX- /Zc:forScope /GR- /Gd /Oi /MT /EHsc /nologo /diagnostics:classic /std:c++17 /cgthreads1") + defines + " ..\\official.cpp";
		//string is copy-pasted from Rel-NoPDB config
}
std::string buildDebug(std::string defines_) {
	std::string defines = replace_string(defines_, " -D", " /D");
	return std::string("cl /permissive- /GS /W3 /Zc:wchar_t /ZI /Gm /Od /sdl /Zc:inline /fp:precise /D_DEBUG /D_CONSOLE /D_UNICODE /DUNICODE /errorReport:prompt /WX- /Zc:forScope /RTC1 /Gd /MDd /EHsc /nologo /diagnostics:classic /std:c++17 /cgthreads1") + defines + " ..\\official.cpp";
		//string is copy-pasted from Debug config
}
std::string genRandom64() {
	static HCRYPTPROV prov = NULL;
	if (!prov) {
		BOOL ok = CryptAcquireContext(&prov,NULL,NULL,PROV_RSA_FULL,0);
		if (!ok) {
			std::cout << "CryptAcquireContext() returned error, aborting" << std::endl;
			abort();
		}
	};
	uint8_t buf[8];
	BOOL ok = CryptGenRandom(prov, sizeof(buf), buf);
	if (!ok) {
		std::cout << "Problems calling CryptGenRandom(), aborting" << std::endl;
		abort();
	}

	char buf2[sizeof(buf) * 2 + 1];
	for (size_t i = 0; i < sizeof(buf); ++i) {
		assert(buf[i] <= 255);
		sprintf(&buf2[i * 2], "%02x", buf[i]);
	}
	return std::string(buf2);
}
std::string exitCheck(bool expectok = true) {
	static int nextlabel = 1;
	if (expectok) {
		auto ret = std::string("IF NOT ERRORLEVEL 1 GOTO LABEL") + std::to_string(nextlabel)
			+ "\nEXIT /B\n:LABEL" + std::to_string(nextlabel);
		nextlabel++;
		return ret;
	}
	else {
		auto ret = std::string("IF ERRORLEVEL 1 GOTO LABEL") + std::to_string(nextlabel)
			+ "\nEXIT /B\n:LABEL" + std::to_string(nextlabel);
		nextlabel++;
		return ret;
	}
}
std::string echo(std::string s) {
	return std::string("ECHO " + s);
}
std::string run() {
	return std::string("official.exe");
}
std::string checkObfuscation(bool obfuscated) {//very weak heuristics, but still better than nothing
	return "";//sorry, nothing for now for Windows :-(
}
std::string cleanup() {
	return std::string("del official.exe");
}
std::string setup() {
	return "@ECHO OFF";
}
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

void buildCheckRunCheckx2(std::string cmd,bool obfuscated=true) {
	buildCheckRunCheck(cmd,obfuscated);
	buildCheckRunCheck(cmd+" -DITHARE_OBF_TEST_NO_NAMESPACE",obfuscated);
}

void genDefineTests() {
	std::cout << echo( std::string("=== -Define Test 1/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildDebug(""),false);
	std::cout << echo( std::string("=== -Define Test 2/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildRelease(""),false);
	std::cout << echo( std::string("=== -Define Test 3/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildDebug(genSeed()));
	std::cout << echo( std::string("=== -Define Test 4/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildRelease(genSeed()));
	std::cout << echo( std::string("=== -Define Test 5/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildDebug(genSeeds()));
	std::cout << echo( std::string("=== -Define Test 6/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildRelease(genSeeds()));
	std::cout << echo( std::string("=== -Define Test 7/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildDebug(genSeeds()+" -DITHARE_OBF_DBG_RUNTIME_CHECKS"));
	std::cout << echo( std::string("=== -Define Test 8/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildRelease(genSeeds()+" -DITHARE_OBF_DBG_RUNTIME_CHECKS"));
	std::cout << echo( std::string("=== -Define Test 9/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildDebug(genSeeds()+" -DITHARE_OBF_CRYPTO_PRNG"));
	std::cout << echo( std::string("=== -Define Test 10/10 ===" ) ) << std::endl;
	buildCheckRunCheckx2(buildRelease(genSeeds()+" -DITHARE_OBF_CRYPTO_PRNG"));
}

void genRandomTests(size_t n) {
	for(size_t i=0; i < n; ++i ) {
		std::string extra;
		if( i%3 == 0 )//every third, non-exclusive
			extra += " -DITHARE_OBF_DBG_RUNTIME_CHECKS";
		std::cout << echo( std::string("=== Random Test ") + std::to_string(i+1) + "/" + std::to_string(n) + " ===" ) << std::endl;
		std::string defines = genSeeds()+" -DITHARE_OBF_INIT -DITHARE_OBF_CONSISTENT_XPLATFORM_IMPLICIT_SEEDS"+extra;
		if( i%4 == 0 ) 
			buildCheckRunCheck(buildDebug(defines));
		else
			buildCheckRunCheck(buildRelease(defines));
	}
}

void genSetup() {
	std::cout << setup() << std::endl;
}

void genCleanup() {
	std::cout << cleanup() << std::endl;
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
		
	genSetup();
	if(!nodefinetests)
		genDefineTests();

	genRandomTests(nrandom);
	genCleanup();
}
