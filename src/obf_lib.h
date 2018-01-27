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

#ifndef ithare_obf_lib_h_included
#define ithare_obf_lib_h_included

#include "obf.h"

namespace ithare {
	namespace obf {
		ITHARE_OBF_DECLARELIBFUNC_WITHEXTRA(class T, class T2, size_t N)
		void obf_copyarray(T(&to)[N], const T2 from[]) { 
			ITHARE_OBF_DBGPRINTLIBFUNCNAME("obf_copyarray");//no 'X' as there is no need to print func name if there is no other stuff to be printed
			if constexpr((obfflags&obf_flag_is_constexpr) ||
				!std::is_same<decltype(from[0]), decltype(to[0])>::value ||
				!std::is_trivially_copyable<decltype(from[0])>::value || obf_avoid_memxxx) {
				auto n = ITHARE_OBFILIBF(N); ITHARE_OBF_DBGPRINTLIB(n);//naming literal as variable just to enable printing it
				for (ITHARE_OBFLIBF(size_t) i = 0; i < n; ++i) { ITHARE_OBF_DBGPRINTLIB(i); 
					to[i] = from[i];
				}
			}
			else {
				assert(sizeof(T) == sizeof(T2));
				memcpy(to, from, sizeof(T));
			}
		}
		ITHARE_OBF_DECLARELIBFUNC_WITHEXTRA(class T, size_t N)
		void obf_zeroarray(T(&to)[N]) {
			ITHARE_OBF_DBGPRINTLIBFUNCNAME("obf_zeroarray");//no 'X'
			if constexpr((obfflags&obf_flag_is_constexpr) ||
				!std::is_integral<decltype(to[0])>::value || obf_avoid_memxxx) {
				auto n = ITHARE_OBFILIBF(N); ITHARE_OBF_DBGPRINTLIB(n);//naming literal as variable just to enable printing it
				for (ITHARE_OBFLIBF(size_t) i = 0; i < n; ++i) { ITHARE_OBF_DBGPRINTLIB(i);
					to[i] = 0;
				}
			}
			else
				memset(to, 0, sizeof(T)*N);
		}
	}//namespace obf
}//namespace ithare 

#endif //ithare_obf_lib_h_included
