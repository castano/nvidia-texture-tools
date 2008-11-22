// This code is in the public domain -- castano@gmail.com

#include "FileSystem.h"
#include <nvcore/nvcore.h>

#if NV_OS_WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
//#include <unistd.h>
#endif

using namespace nv;


bool FileSystem::exists(const char * path)
{
//   stat buf;
//   stat(path, &buf);
    return false;
}

bool FileSystem::createDirectory(const char * path)
{
#if NV_OS_WIN32
    return _mkdir(path) != -1;
#else
    return mkdir(path, 0777) != -1;
#endif
}
