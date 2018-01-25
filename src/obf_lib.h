#ifndef ithare_obf_lib_h_included
#define ithare_obf_lib_h_included

#include "obf.h"

namespace ithare {
	namespace obf {
		ITHARE_OBF_DECLARELIBFUNC_WITHEXTRA(class T, class T2, size_t N)
		void obf_copyarray(T(&to)[N], const T2 from[]) {
			if constexpr((obfflags&obf_flag_is_constexpr) ||
				!std::is_same<decltype(from[0]), decltype(to[0])>::value ||
				!std::is_trivially_copyable<decltype(from[0])>::value || obf_avoid_memxxx) {
				for (ITHARE_OBFLIB(size_t) i = 0; i < N; ++i) {
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
			if constexpr((obfflags&obf_flag_is_constexpr) ||
				!std::is_integral<decltype(to[0])>::value || obf_avoid_memxxx) {
				for (size_t i = 0; i < N; ++i) {
					to[i] = 0;
				}
			}
			else
				memset(to, 0, sizeof(T)*N);
		}
	}//namespace obf
}//namespace ithare 

#endif //ithare_obf_lib_h_included
