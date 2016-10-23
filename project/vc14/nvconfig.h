#ifndef NV_CONFIG
#define NV_CONFIG

//#cmakedefine HAVE_UNISTD_H
#define HAVE_STDARG_H
//#cmakedefine HAVE_SIGNAL_H
//#cmakedefine HAVE_EXECINFO_H
#define HAVE_MALLOC_H

#if defined(_OPENMP)
#define HAVE_OPENMP
#endif

#define HAVE_STBIMAGE
/*#if !defined(_M_X64)
//#define HAVE_FREEIMAGE
#define HAVE_PNG
#define HAVE_JPEG
#define HAVE_TIFF
#endif*/

#endif // NV_CONFIG
