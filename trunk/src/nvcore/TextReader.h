// This code is in the public domain -- castano@gmail.com

#ifndef NV_CORE_TEXTREADER_H
#define NV_CORE_TEXTREADER_H

#include <nvcore/Containers.h>
#include <nvcore/Stream.h>

namespace nv
{

/// Text reader.
class NVCORE_CLASS TextReader {
public:
	
	/// Ctor.
	TextReader(Stream * stream) : m_stream(stream), m_text(512) {
		nvCheck(stream != NULL);
		nvCheck(stream->isLoading());
	}
	
	char peek();
	char read();
	
	const char *readToEnd();

	// Returns a temporary string.
	const char * readLine(); 

private:
	Stream * m_stream;
	Array<char> m_text;
};

} // nv namespace

#endif // NV_CORE_TEXTREADER_H
