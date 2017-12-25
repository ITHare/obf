#include "stdafx.h"
#include "obfuscate.h"

#ifdef _MSC_VER
#include <intrin.h>
namespace obf {
	volatile uint8_t* obf_peb = nullptr;
	static int obf_nInits = 0;

	int __cdecl obf_preMain(void) {
		constexpr auto offset = 0x30;
		obf_peb = (uint8_t*)__readfsdword(offset);
		++obf_nInits;
		return 0;
	}
}
/* didn't find a way to avoid eliminating it in Release :-( */
#pragma section(".CRT$XIC",long,read)
__declspec(allocate(".CRT$XIC")) static auto obfinit = obf::obf_preMain;
#pragma data_seg()

#endif