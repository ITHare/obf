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

//TEST GENERATOR. Generates randomized command lines for a testing session
//  Using C++ to avoid writing the same logic twice in *nix .sh and Win* .bat

#include "../../kscope/test/randomtestgen.h"

static const char* obf_randomtest_files[] = { "../obftest.cpp", nullptr };

inline bool starts_with(std::string a,std::string b) {
	return a.compare(0,b.length(),b) == 0;
}

class ObfTestEnvironment : public KscopeTestEnvironment {
	public:
	virtual std::string test_src_dir() override { return  src_dir_prefix + "../../../kscope/test/"; }
	//virtual std::string file_list() override { return KscopeTestEnvironment::file_list() + make_file_list(obf_randomtest_files,src_dir_prefix); }

	std::string obf_define(std::string s) {
		if(starts_with(s,"SEED=")
		   ||starts_with(s,"SEED2=")
		   ||s=="CONSISTENT_XPLATFORM_IMPLICIT_SEEDS"
		   ||s=="CRYPTO_PRNG"
		   ||s=="DBG_RUNTIME_CHECKS"
		   ||s=="DBG_ENABLE_DBGPRINT"
		   ||s=="ENABLE_AUTO_DBGPRINT"
		   ||s=="COMPILE_TIME_TESTS"
		   )
			return "ITHARE_OBF_" + s;
		else
			return "";
	}
#ifdef __GNUC__ //includes clang
#ifdef __apple_build_version__
	static constexpr const char* lopt_extra ="";//no -latomic needed or possible for Apple Clang
#else
	static constexpr const char* lopt_extra = " -latomic";
#endif

	virtual MultiString build_release(MultiString defines,std::string opts) override {
		std::string kscopedefs = "";
		std::string obfdefs = "";
		for(std::string s:defines) {
			kscopedefs += " -DITHARE_KSCOPE_" + s;
			if(obf_define(s)!="")
				obfdefs += " -D" + obf_define(s);
		}
		std::string cpplist = KscopeTestEnvironment::file_list() + make_file_list(obf_randomtest_files,src_dir_prefix);
		std::string objlist0 = replace_string(cpplist,".cpp",".o");
		std::string objlist1 = replace_string(objlist0,test_src_dir(),"");
		std::string objlist = replace_string(objlist1,"../","");
		return MultiString{
			"$CXX -c" + compiler_options_release() + " -DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + kscopedefs + opts + KscopeTestEnvironment::file_list(),
			"$CXX -c" + compiler_options_release() + " -DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + obfdefs    + opts + make_file_list(obf_randomtest_files,src_dir_prefix),
			"$CXX" + linker_options_release() + lopt_extra + opts + objlist
			};
	}
	virtual MultiString build_debug(MultiString defines,std::string opts) override {
		std::string kscopedefs = "";
		std::string obfdefs = "";
		for(std::string s:defines) {
			kscopedefs += " -DITHARE_KSCOPE_" + s;
			if(obf_define(s)!="")
				obfdefs += " -D" + obf_define(s);
		}
		std::string cpplist = KscopeTestEnvironment::file_list() + make_file_list(obf_randomtest_files,src_dir_prefix);
		std::string objlist0 = replace_string(cpplist,".cpp",".o");
		std::string objlist1 = replace_string(objlist0,test_src_dir(),"");
		std::string objlist = replace_string(objlist1,"../","");
		return MultiString{
			"$CXX -c" + compiler_options_debug() + " -DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + kscopedefs + opts + KscopeTestEnvironment::file_list(),
			"$CXX -c" + compiler_options_debug() + " -DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + obfdefs    + opts + make_file_list(obf_randomtest_files,src_dir_prefix),
			"$CXX" + linker_options_debug() + lopt_extra + opts + objlist
			};
	}
#elif defined(_MSC_VER)
	virtual MultiString build_release(MultiString defines,std::string opts) {
		std::string kscopedefs = "";
		std::string obfdefs = "";
		for(std::string s:defines) {
			kscopedefs += " /DITHARE_KSCOPE_" + s;
			if(obf_define(s)!="")
				obfdefs += " /D" + obf_define(s);
		}
		std::string cpplist = KscopeTestEnvironment::file_list() + make_file_list(obf_randomtest_files,src_dir_prefix);
		std::string objlist0 = replace_string(cpplist,".cpp",".obj");
		std::string objlist1 = replace_string(objlist0,test_src_dir(),"");
		std::string objlist = replace_string(objlist1,"../","");
		return MultiString{
			"cl /c" + compiler_options_release() + " /DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + kscopedefs + opts + KscopeTestEnvironment::file_list(),
			"cl /c" + compiler_options_release() + " /DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + obfdefs + opts + make_file_list(obf_randomtest_files,src_dir_prefix),
			"cl " + linker_options_release() + opts + objlist
			};
	}
	virtual MultiString build_debug(MultiString defines,std::string opts) {
		std::string kscopedefs = "";
		std::string obfdefs = "";
		for(std::string s:defines) {
			kscopedefs += " /DITHARE_KSCOPE_" + s;
			if(obf_define(s)!="")
				obfdefs += " /D" + obf_define(s);
		}
		std::string cpplist = KscopeTestEnvironment::file_list() + make_file_list(obf_randomtest_files,src_dir_prefix);
		std::string objlist0 = replace_string(cpplist,".cpp",".obj");
		std::string objlist1 = replace_string(objlist0,test_src_dir(),"");
		std::string objlist = replace_string(objlist1,"../","");
		return MultiString{
			"cl /c" + compiler_options_debug() + " /DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + kscopedefs + opts + KscopeTestEnvironment::file_list(),
			"cl /c" + compiler_options_debug() + " /DITHARE_KSCOPE_TEST_EXTENSION=\"../../obf/src/kscope_extension_for_obf.h\"" + obfdefs + opts + make_file_list(obf_randomtest_files,src_dir_prefix),
			"cl " + linker_options_debug() + opts + objlist
			};
	}
#endif
	
#if defined(__APPLE_CC__) || defined(__linux__)
	virtual std::string check_exe(int nseeds,config cfg,Flags flags) override {
		bool obfuscated = nseeds != 0;

		//automated check: very weak heuristics, but still better than nothing
		std::string cmp = std::string("strings randomtest | grep \"Negative argument\"");//referring to string "Negative argument to factorial()" 
		if(flags&flag_auto_dbg_print) {//result is unclear
			return cmp;
		}
		else {
			cmp += "\n" + exit_check(cmp, !obfuscated);
			if (obfuscated) {
				std::string cp = "cp randomtest randomtest-obf";
				cmp += "\n" + cp + "\n" + exit_check(cp);
			}
			return cmp;
		}
	}
#elif defined(_MSC_VER)
	virtual std::string check_exe(int nseeds, config cfg, Flags flags) override {
		bool obfuscated = nseeds != 0;
		//no automated check for Windows at the moment :-(
		if ((flags&flag_auto_dbg_print)==0 && obfuscated & cfg==config::release) {
			//copying for automated check
			std::string cp = "copy randomtest.exe randomtest-obf.exe";
			cp += "\n" + exit_check(cp);
			return cp;
		}
		return "";
	}
#endif
};

class ObfTestGenerator : public KscopeTestGenerator {
	public:
	using KscopeTestGenerator::KscopeTestGenerator;//inheriting
	virtual std::string project_name() override {
		return "obf";
	}
};

int main(int argc, char** argv) {
	ObfTestEnvironment oenv;
	ObfTestGenerator ogen(oenv);
	return almost_main(oenv,ogen,argc,argv);
}
