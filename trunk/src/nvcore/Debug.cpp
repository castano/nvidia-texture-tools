// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "Debug.h"
#include "StrLib.h" // StringBuilder

// Extern
#if NV_OS_WIN32 //&& NV_CC_MSVC
#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   include <windows.h>
#   include <direct.h>
#   if NV_CC_MSVC
#       include <crtdbg.h>
#       if _MSC_VER < 1300
#           define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#           include <dbghelp.h> // must be XP version of file
//          include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"
#       else
// VC7: ships with updated headers
#           include <dbghelp.h>
#       endif
#   endif
#   pragma comment(lib,"dbghelp.lib")
#endif

#if NV_OS_XBOX
#    include <Xtl.h>
#    ifdef _DEBUG
#        include <xbdm.h>
#    endif //_DEBUG
#endif //NV_OS_XBOX

#if !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)
#   include <signal.h>
#endif

#if NV_OS_LINUX || NV_OS_DARWIN || NV_OS_FREEBSD
#   include <unistd.h> // getpid
#endif

#if NV_OS_LINUX && defined(HAVE_EXECINFO_H)
#   include <execinfo.h> // backtrace
#   if NV_CC_GNUC // defined(HAVE_CXXABI_H)
#       include <cxxabi.h>
#   endif
#endif

#if NV_OS_DARWIN || NV_OS_FREEBSD
#   include <sys/types.h>
#   include <sys/sysctl.h> // sysctl
#   include <sys/ucontext.h>
#   if defined(HAVE_EXECINFO_H) // only after OSX 10.5
#       include <execinfo.h> // backtrace
#       if NV_CC_GNUC // defined(HAVE_CXXABI_H)
#           include <cxxabi.h>
#       endif
#   endif
#endif

using namespace nv;

namespace 
{

    static MessageHandler * s_message_handler = NULL;
    static AssertHandler * s_assert_handler = NULL;

    static bool s_sig_handler_enabled = false;

#if NV_OS_WIN32 && NV_CC_MSVC

    // Old exception filter.
    static LPTOP_LEVEL_EXCEPTION_FILTER s_old_exception_filter = NULL;

#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)

    // Old signal handlers.
    struct sigaction s_old_sigsegv;
    struct sigaction s_old_sigtrap;
    struct sigaction s_old_sigfpe;
    struct sigaction s_old_sigbus;

#endif


#if NV_OS_WIN32 && NV_CC_MSVC

    static bool writeMiniDump(EXCEPTION_POINTERS * pExceptionInfo)
    {
        // create the file
        HANDLE hFile = CreateFileA("crash.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            nvDebug("*** Failed to create dump file.\n");
            return false;
        }

        MINIDUMP_EXCEPTION_INFORMATION ExInfo;
        ExInfo.ThreadId = ::GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExceptionInfo;
        ExInfo.ClientPointers = NULL;

        // write the dump
        BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL) != 0;
        CloseHandle(hFile);

        if (ok == FALSE) {
            nvDebug("*** Failed to save dump file.\n");
            return false;
        }

        nvDebug("\nDump file saved.\n");

        return true;
    }

    static bool hasStackTrace() {
        return true;
    }

    /*static NV_NOINLINE int backtrace(void * trace[], int maxcount) {
	
        // In Windows XP and Windows Server 2003, the sum of the FramesToSkip and FramesToCapture parameters must be less than 63.
        int xp_maxcount = min(63-1, maxcount);

        int count = RtlCaptureStackBackTrace(1, xp_maxcount, trace, NULL);
        nvDebugCheck(count <= maxcount);

        return count;
    }*/

    static NV_NOINLINE int backtraceWithSymbols(CONTEXT * ctx, void * trace[], int maxcount, int skip = 0) {
		
        // Init the stack frame for this function
        STACKFRAME64 stackFrame = { 0 };

    #if NV_CPU_X86_64
        DWORD dwMachineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = ctx->Rip;
        stackFrame.AddrFrame.Offset = ctx->Rbp;
        stackFrame.AddrStack.Offset = ctx->Rsp;
    #elif NV_CPU_X86
        DWORD dwMachineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = ctx->Eip;
        stackFrame.AddrFrame.Offset = ctx->Ebp;
        stackFrame.AddrStack.Offset = ctx->Esp;
    #else
        #error "Platform not supported!"
    #endif
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;

        // Walk up the stack
        const HANDLE hThread = GetCurrentThread();
        const HANDLE hProcess = GetCurrentProcess();
        int i;
        for (i = 0; i < maxcount; i++)
        {
            // walking once first makes us skip self
            if (!StackWalk64(dwMachineType, hProcess, hThread, &stackFrame, ctx, NULL, &SymFunctionTableAccess64, &SymGetModuleBase64, NULL)) {
                break;
            }

            /*if (stackFrame.AddrPC.Offset == stackFrame.AddrReturn.Offset || stackFrame.AddrPC.Offset == 0) {
                break;
            }*/

            if (i >= skip) {
                trace[i - skip] = (PVOID)stackFrame.AddrPC.Offset;
            }
        }

        return i - skip;
    }

#pragma warning(push)
#pragma warning(disable:4748)
    static NV_NOINLINE int backtrace(void * trace[], int maxcount) {
        CONTEXT ctx = { 0 };
#if NV_CPU_X86 && !NV_CPU_X86_64
        ctx.ContextFlags = CONTEXT_CONTROL;
        _asm {
             call x
          x: pop eax
             mov ctx.Eip, eax
             mov ctx.Ebp, ebp
             mov ctx.Esp, esp
        }
#else
        RtlCaptureContext(&ctx); // Not implemented correctly in x86.
#endif

        return backtraceWithSymbols(&ctx, trace, maxcount, 1);
    }
#pragma warning(pop)

    static NV_NOINLINE void printStackTrace(void * trace[], int size, int start=0)
    {
        HANDLE hProcess = GetCurrentProcess();
    	
        nvDebug( "\nDumping stacktrace:\n" );

	    // Resolve PC to function names
	    for (int i = start; i < size; i++)
	    {
		    // Check for end of stack walk
		    DWORD64 ip = (DWORD64)trace[i];
		    if (ip == NULL)
			    break;

		    // Get function name
		    #define MAX_STRING_LEN	(512)
		    unsigned char byBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_STRING_LEN] = { 0 };
		    IMAGEHLP_SYMBOL64 * pSymbol = (IMAGEHLP_SYMBOL64*)byBuffer;
		    pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		    pSymbol->MaxNameLength = MAX_STRING_LEN;

		    DWORD64 dwDisplacement;
    		
		    if (SymGetSymFromAddr64(hProcess, ip, &dwDisplacement, pSymbol))
		    {
			    pSymbol->Name[MAX_STRING_LEN-1] = 0;
    			
			    /*
			    // Make the symbol readable for humans
			    UnDecorateSymbolName( pSym->Name, lpszNonUnicodeUnDSymbol, BUFFERSIZE, 
				    UNDNAME_COMPLETE | 
				    UNDNAME_NO_THISTYPE |
				    UNDNAME_NO_SPECIAL_SYMS |
				    UNDNAME_NO_MEMBER_TYPE |
				    UNDNAME_NO_MS_KEYWORDS |
				    UNDNAME_NO_ACCESS_SPECIFIERS );
			    */
    			
			    // pSymbol->Name
			    const char * pFunc = pSymbol->Name;
    			
			    // Get file/line number
			    IMAGEHLP_LINE64 theLine = { 0 };
			    theLine.SizeOfStruct = sizeof(theLine);

			    DWORD dwDisplacement;
			    if (!SymGetLineFromAddr64(hProcess, ip, &dwDisplacement, &theLine))
			    {
				    nvDebug("unknown(%08X) : %s\n", (uint32)ip, pFunc);
			    }
			    else
			    {
				    /*
				    const char* pFile = strrchr(theLine.FileName, '\\');
				    if ( pFile == NULL ) pFile = theLine.FileName;
				    else pFile++;
				    */
				    const char * pFile = theLine.FileName;
    				
				    int line = theLine.LineNumber;
    				
				    nvDebug("%s(%d) : %s\n", pFile, line, pFunc);
			    }
		    }
	    }
    }


    // Write mini dump and print stack trace.
    static LONG WINAPI topLevelFilter(EXCEPTION_POINTERS * pExceptionInfo)
    {
        void * trace[64];
        
        int size = backtraceWithSymbols(pExceptionInfo->ContextRecord, trace, 64);
        printStackTrace(trace, size, 0);

        writeMiniDump(pExceptionInfo);

        return EXCEPTION_CONTINUE_SEARCH;
    }

#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H) // NV_OS_LINUX || NV_OS_DARWIN

#if defined(HAVE_EXECINFO_H)

    static bool hasStackTrace() {
#if NV_OS_DARWIN
        return backtrace != NULL;
#else
        return true;
#endif
    }

    static void printStackTrace(void * trace[], int size, int start=0) {
        char ** string_array = backtrace_symbols(trace, size);

        nvDebug( "\nDumping stacktrace:\n" );
        for(int i = start; i < size-1; i++ ) {
#       if NV_CC_GNUC // defined(HAVE_CXXABI_H)
            // @@ Write a better parser for the possible formats.
            char * begin = strchr(string_array[i], '(');
            char * end = strrchr(string_array[i], '+');
            char * module = string_array[i];

            if (begin == 0 && end != 0) {
                *(end - 1) = '\0';
                begin = strrchr(string_array[i], ' ');
                module = NULL; // Ignore module.
            }

            if (begin != 0 && begin < end) {
                int stat;
                *end = '\0';
                *begin = '\0';
                char * name = abi::__cxa_demangle(begin+1, 0, 0, &stat);
                if (module == NULL) {
                    if (name == NULL || stat != 0) {
                        nvDebug( "  In: '%s'\n", begin+1 );
                    }
                    else {
                        nvDebug( "  In: '%s'\n", name );
                    }
                }
                else {
                    if (name == NULL || stat != 0) {
                        nvDebug( "  In: [%s] '%s'\n", module, begin+1 );
                    }
                    else {
                        nvDebug( "  In: [%s] '%s'\n", module, name );
                    }
                }
                free(name);
            }
            else {
                nvDebug( "  In: '%s'\n", string_array[i] );
            }
#       else
            nvDebug( "  In: '%s'\n", string_array[i] );
#       endif
        }
        nvDebug("\n");

        free(string_array);
    }

#endif // defined(HAVE_EXECINFO_H)

    static void * callerAddress(void * secret)
    {
#if NV_OS_DARWIN
#  if defined(_STRUCT_MCONTEXT)
#    if NV_CPU_PPC
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *) ucp->uc_mcontext->__ss.__srr0;
#    elif NV_CPU_X86_64
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *) ucp->uc_mcontext->__ss.__rip;
#    elif NV_CPU_X86
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *) ucp->uc_mcontext->__ss.__eip;
#    elif NV_CPU_ARM
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *) ucp->uc_mcontext->__ss.__pc;
#    else
#      error "Unknown CPU"
#    endif
#  else
#    if NV_CPU_PPC
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *) ucp->uc_mcontext->ss.srr0;
#    elif NV_CPU_X86
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *) ucp->uc_mcontext->ss.eip;
#    else
#      error "Unknown CPU"
#    endif
#  endif
#elif NV_OS_FREEBSD
#  if NV_CPU_X86_64
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *)ucp->uc_mcontext.mc_rip;
#  elif NV_CPU_X86
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *)ucp->uc_mcontext.mc_eip;
#    else
#      error "Unknown CPU"
#    endif
#else
#  if NV_CPU_X86_64
        // #define REG_RIP REG_INDEX(rip) // seems to be 16
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *)ucp->uc_mcontext.gregs[REG_RIP];
#  elif NV_CPU_X86
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *)ucp->uc_mcontext.gregs[14/*REG_EIP*/];
#  elif NV_CPU_PPC
        ucontext_t * ucp = (ucontext_t *)secret;
        return (void *) ucp->uc_mcontext.regs->nip;
#    else
#      error "Unknown CPU"
#    endif
#endif

        // How to obtain the instruction pointers in different platforms, from mlton's source code.
        // http://mlton.org/
        // OpenBSD && NetBSD
        // ucp->sc_eip
        // FreeBSD:
        // ucp->uc_mcontext.mc_eip
        // HPUX:
        // ucp->uc_link
        // Solaris:
        // ucp->uc_mcontext.gregs[REG_PC]
        // Linux hppa:
        // uc->uc_mcontext.sc_iaoq[0] & ~0x3UL
        // Linux sparc:
        // ((struct sigcontext*) secret)->sigc_regs.tpc
        // Linux sparc64:
        // ((struct sigcontext*) secret)->si_regs.pc

        // potentially correct for other archs:
        // Linux alpha: ucp->m_context.sc_pc
        // Linux arm: ucp->m_context.ctx.arm_pc
        // Linux ia64: ucp->m_context.sc_ip & ~0x3UL
        // Linux mips: ucp->m_context.sc_pc
        // Linux s390: ucp->m_context.sregs->regs.psw.addr
    }

    static void nvSigHandler(int sig, siginfo_t *info, void *secret)
    {
        void * pnt = callerAddress(secret);

        // Do something useful with siginfo_t
        if (sig == SIGSEGV) {
            if (pnt != NULL) nvDebug("Got signal %d, faulty address is %p, from %p\n", sig, info->si_addr, pnt);
            else nvDebug("Got signal %d, faulty address is %p\n", sig, info->si_addr);
        }
        else if(sig == SIGTRAP) {
            nvDebug("Breakpoint hit.\n");
        }
        else {
            nvDebug("Got signal %d\n", sig);
        }

#if defined(HAVE_EXECINFO_H)
        if (hasStackTrace()) // in case of weak linking
        {
            void * trace[64];
            int size = backtrace(trace, 64);

            if (pnt != NULL) {
                // Overwrite sigaction with caller's address.
                trace[1] = pnt;
            }

            printStackTrace(trace, size, 1);
        }
#endif // defined(HAVE_EXECINFO_H)

        exit(0);
    }

#endif // defined(HAVE_SIGNAL_H)



#if NV_OS_WIN32 //&& NV_CC_MSVC

    /** Win32 assert handler. */
    struct Win32AssertHandler : public AssertHandler 
    {
        // Flush the message queue. This is necessary for the message box to show up.
        static void flushMessageQueue()
        {
            MSG msg;
            while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
                //if( msg.message == WM_QUIT ) break;
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

        // Assert handler method.
        virtual int assertion( const char * exp, const char * file, int line, const char * func/*=NULL*/ )
        {
            int ret = NV_ABORT_EXIT;

            StringBuilder error_string;
            if( func != NULL ) {
                error_string.format( "*** Assertion failed: %s\n    On file: %s\n    On function: %s\n    On line: %d\n ", exp, file, func, line );
            }
            else {
                error_string.format( "*** Assertion failed: %s\n    On file: %s\n    On line: %d\n ", exp, file, line );
            }
            nvDebug( error_string.str() );

            if (debug::isDebuggerPresent()) {
                return NV_ABORT_DEBUG;
            }

            flushMessageQueue();
            int action = MessageBoxA(NULL, error_string.str(), "Assertion failed", MB_ABORTRETRYIGNORE|MB_ICONERROR);
            switch( action ) {
            case IDRETRY:
                ret = NV_ABORT_DEBUG;
                break;
            case IDIGNORE:
                ret = NV_ABORT_IGNORE;
                break;
            case IDABORT:
            default:
                ret = NV_ABORT_EXIT;
                break;
            }
            /*if( _CrtDbgReport( _CRT_ASSERT, file, line, module, exp ) == 1 ) {
                return NV_ABORT_DEBUG;
            }*/

            if( ret == NV_ABORT_EXIT ) {
                 // Exit cleanly.
                throw "Assertion failed";
            }

            return ret;
        }
    };
#elif NV_OS_XBOX

    /** Xbox360 assert handler. */
    struct Xbox360AssertHandler : public AssertHandler 
    {
        // Assert handler method.
        virtual int assertion( const char * exp, const char * file, int line, const char * func/*=NULL*/ )
        {
            int ret = NV_ABORT_EXIT;

            StringBuilder error_string;
            if( func != NULL ) {
                error_string.format( "*** Assertion failed: %s\n    On file: %s\n    On function: %s\n    On line: %d\n ", exp, file, func, line );
                nvDebug( error_string.str() );
            }
            else {
                error_string.format( "*** Assertion failed: %s\n    On file: %s\n    On line: %d\n ", exp, file, line );
                nvDebug( error_string.str() );
            }

            if (debug::isDebuggerPresent()) {
                return NV_ABORT_DEBUG;
            }

            if( ret == NV_ABORT_EXIT ) {
                 // Exit cleanly.
                throw "Assertion failed";
            }

            return ret;
        }
    };
#else

    /** Unix assert handler. */
    struct UnixAssertHandler : public AssertHandler
    {
        // Assert handler method.
        virtual int assertion(const char * exp, const char * file, int line, const char * func)
        {
            if( func != NULL ) {
                nvDebug( "*** Assertion failed: %s\n    On file: %s\n    On function: %s\n    On line: %d\n ", exp, file, func, line );
            }
            else {
                nvDebug( "*** Assertion failed: %s\n    On file: %s\n    On line: %d\n ", exp, file, line );
            }

#if _DEBUG
            if (debug::isDebuggerPresent()) {
                return NV_ABORT_DEBUG;
            }
#endif

#if defined(HAVE_EXECINFO_H)
            if (hasStackTrace())
            {
                void * trace[64];
                int size = backtrace(trace, 64);
                printStackTrace(trace, size, 2);
            }
#endif

            // Exit cleanly.
            throw "Assertion failed";
        }
    };

#endif

} // namespace


/// Handle assertion through the assert handler.
int nvAbort(const char * exp, const char * file, int line, const char * func/*=NULL*/)
{
#if NV_OS_WIN32 //&& NV_CC_MSVC
    static Win32AssertHandler s_default_assert_handler;
#elif NV_OS_XBOX
    static Xbox360AssertHandler s_default_assert_handler;
#else
    static UnixAssertHandler s_default_assert_handler;
#endif

    if (s_assert_handler != NULL) {
        return s_assert_handler->assertion( exp, file, line, func );
    }
    else {
        return s_default_assert_handler.assertion( exp, file, line, func );
    }
}


/// Shows a message through the message handler.
void NV_CDECL nvDebugPrint(const char *msg, ...)
{
    va_list arg;
    va_start(arg,msg);
    if (s_message_handler != NULL) {
        s_message_handler->log( msg, arg );
    }
    va_end(arg);
}


/// Dump debug info.
void debug::dumpInfo()
{
#if (NV_OS_WIN32 && NV_CC_MSVC) || (defined(HAVE_SIGNAL_H) && defined(HAVE_EXECINFO_H))
    if (hasStackTrace())
    {
        void * trace[64];
        int size = backtrace(trace, 64);
        printStackTrace(trace, size, 1);
    }
#endif
}


/// Set the debug message handler.
void debug::setMessageHandler(MessageHandler * message_handler)
{
    s_message_handler = message_handler;
}

/// Reset the debug message handler.
void debug::resetMessageHandler()
{
    s_message_handler = NULL;
}

/// Set the assert handler.
void debug::setAssertHandler(AssertHandler * assert_handler)
{
    s_assert_handler = assert_handler;
}

/// Reset the assert handler.
void debug::resetAssertHandler()
{
    s_assert_handler = NULL;
}


/// Enable signal handler.
void debug::enableSigHandler()
{
    nvCheck(s_sig_handler_enabled != true);
    s_sig_handler_enabled = true;

#if NV_OS_WIN32 && NV_CC_MSVC

    s_old_exception_filter = ::SetUnhandledExceptionFilter( topLevelFilter );

    // SYMOPT_DEFERRED_LOADS make us not take a ton of time unless we actual log traces
    SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_LOAD_LINES|SYMOPT_UNDNAME);

    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
        DWORD error = GetLastError();
        nvDebug("SymInitialize returned error : %d\n", error);
    }

#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)

    // Install our signal handler
    struct sigaction sa;
    sa.sa_sigaction = nvSigHandler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;

    sigaction(SIGSEGV, &sa, &s_old_sigsegv);
    sigaction(SIGTRAP, &sa, &s_old_sigtrap);
    sigaction(SIGFPE, &sa, &s_old_sigfpe);
    sigaction(SIGBUS, &sa, &s_old_sigbus);

#endif
}

/// Disable signal handler.
void debug::disableSigHandler()
{
    nvCheck(s_sig_handler_enabled == true);
    s_sig_handler_enabled = false;

#if NV_OS_WIN32 && NV_CC_MSVC

    ::SetUnhandledExceptionFilter( s_old_exception_filter );
    s_old_exception_filter = NULL;

    SymCleanup(GetCurrentProcess());

#elif !NV_OS_WIN32 && defined(HAVE_SIGNAL_H)

    sigaction(SIGSEGV, &s_old_sigsegv, NULL);
    sigaction(SIGTRAP, &s_old_sigtrap, NULL);
    sigaction(SIGFPE, &s_old_sigfpe, NULL);
    sigaction(SIGBUS, &s_old_sigbus, NULL);

#endif
}


bool debug::isDebuggerPresent()
{
#if NV_OS_WIN32
    HINSTANCE kernel32 = GetModuleHandleA("kernel32.dll");
    if (kernel32) {
        FARPROC IsDebuggerPresent = GetProcAddress(kernel32, "IsDebuggerPresent");
        if (IsDebuggerPresent != NULL && IsDebuggerPresent()) {
            return true;
        }
    }
    return false;
#elif NV_OS_XBOX
#ifdef _DEBUG
    return DmIsDebuggerPresent() == TRUE;
#else
    return false;
#endif
#elif NV_OS_DARWIN
    int mib[4];
    struct kinfo_proc info;
    size_t size;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();
    size = sizeof(info);
    info.kp_proc.p_flag = 0;
    sysctl(mib,4,&info,&size,NULL,0);
    return ((info.kp_proc.p_flag & P_TRACED) == P_TRACED);
#else
    // if ppid != sid, some process spawned our app, probably a debugger. 
    return getsid(getpid()) != getppid();
#endif
}
