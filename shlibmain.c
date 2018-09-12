// shlibmain.c
// provides a DLLMain function for MSWindows builds
// acts as a stub that enables shared lib soversion/soname functionality in CMake

#if defined(WIN32) || defined(__windows__) || defined(_MSC_VER)

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

