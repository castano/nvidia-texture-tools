// This code is in the public domain -- castano@gmail.com

#ifndef NV_CORE_FILESYSTEM_H
#define NV_CORE_FILESYSTEM_H

#include "nvcore.h"

namespace nv
{

	namespace FileSystem
	{

		NVCORE_API bool exists(const char * path);
		NVCORE_API bool createDirectory(const char * path);
		NVCORE_API bool changeDirectory(const char * path);

	} // FileSystem namespace

} // nv namespace


#endif // NV_CORE_FILESYSTEM_H
