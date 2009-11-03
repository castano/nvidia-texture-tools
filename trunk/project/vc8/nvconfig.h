#ifndef NV_CONFIG
#define NV_CONFIG

//#cmakedefine HAVE_UNISTD_H
#define HAVE_STDARG_H
//#cmakedefine HAVE_SIGNAL_H
//#cmakedefine HAVE_EXECINFO_H
#define HAVE_MALLOC_H

#if !defined(_M_X64)
//#define HAVE_PNG
//#define HAVE_JPEG
//#define HAVE_TIFF
#define HAVE_FREEIMAGE
#endif

//#define HAVE_ATITC
//#define HAVE_D3DX
//#define HAVE_SQUISH
//#define HAVE_STB


#endif // NV_CONFIG
