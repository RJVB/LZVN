#if defined(__linux__) || defined(__linux)

#include <sys/types.h>

#include "FastCompression.h"

// on Linux symbols aren't prefixed with an underscore so we need to provide interface functions
// that call the prefixed functions from the assembler files. This also solves (potential) issues
// where information necessary at runtime isn't available for symbols from assembler files bundled
// in a shared library, e.g.
// ld: warning: type and size of dynamic symbol `lzvn_decode' are not defined
// This causes a SIGSEGV when trying to call the function.

extern size_t _lzvn_encode(void * dst, size_t dst_size, const void * src, size_t src_size, void * work_space);
extern size_t _lzvn_decode(void * dst, size_t dst_size, const void * src, size_t src_size);
extern size_t _lzvn_encode_work_size(void);

size_t lzvn_encode(void * dst, size_t dst_size, const void * src, size_t src_size, void * work_space)
{
    return _lzvn_encode(dst, dst_size, src,src_size, work_space);
}

size_t lzvn_decode(void * dst, size_t dst_size, const void * src, size_t src_size)
{
    return _lzvn_decode(dst, dst_size, src, src_size);
}

size_t lzvn_encode_work_size(void)
{
    return _lzvn_encode_work_size();
}

#elif defined(WIN32) || defined(__windows__) || defined(_MSC_VER)

// provide a DLLMain function for MSWindows builds
// acts as a stub that enables shared lib soversion/soname functionality in CMake

#include <windows.h>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{ BOOL ret = TRUE;
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// init here as far as necessary
			break;
		case DLL_PROCESS_DETACH:
			break;
        default:
            // noop
            break;
	}
	return ret;
}

#endif //!windows

