#include "../obf.h"

#ifdef _MSC_VER
#include <intrin.h>
namespace ithare {
	namespace obf {
		volatile uint8_t* obf_peb = nullptr;
		static int obf_nInits = 0;

#ifdef ITHARE_OBF_SEED //otherwise - it is completely inlined cpp-less
		void obf_init() {
#ifdef _WIN64
			constexpr auto offset = 0x60;
			obf_peb = (uint8_t*)__readgsqword(offset);
#else
			constexpr auto offset = 0x30;
			obf_peb = (uint8_t*)__readfsdword(offset);
#endif
			++obf_nInits;
			return;
		}
#endif
	}//namespace obf
}//namespace ithare

/* didn't find a way to avoid eliminating it in Release :-( 
#pragma section(".CRT$XIC",long,read)
__declspec(allocate(".CRT$XIC")) static auto obfinit = obf::obf_preMain;
#pragma data_seg()*/

#else//_MSC_VER
namespace ithare {
	namespace obf {
		static int obf_nInits = 0;

#ifdef ITHARE_OBF_SEED //otherwise - it is completely inlined cpp-less
		void obf_init() {
			++obf_nInits;
			return;
		}
#endif
	}//namespace obf
}//namespace ithare

#endif
