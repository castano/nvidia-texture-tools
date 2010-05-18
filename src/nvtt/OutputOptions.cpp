// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "OutputOptions.h"

using namespace nvtt;


static void defaultBeginImageCallback(int size, int width, int height, int depth, int face, int miplevel, void * userData) {
	nvtt::OutputHandler * outputHandler = (nvtt::OutputHandler *)userData;
	if (outputHandler != NULL) outputHandler->beginImage(size, width, height, depth, face, miplevel);
}

static bool defaultOutputCallback(const void * data, int size, void * userData) {
	nvtt::OutputHandler * outputHandler = (nvtt::OutputHandler *)userData;
	if (outputHandler != NULL) return outputHandler->writeData(data, size);
}

static void defaultErrorCallback(Error e, void * userData) {
	nvtt::ErrorHandler * errorHandler = (nvtt::ErrorHandler *)userData;
	if (errorHandler != NULL) errorHandler->error(e);
}


OutputOptions::OutputOptions() : m(*new OutputOptions::Private())
{
	reset();
}

OutputOptions::~OutputOptions()
{
	// Cleanup output handler.
	setOutputHandler(NULL);

	delete &m;
}

/// Set default output options.
void OutputOptions::reset()
{
	m.fileName.reset();

	m.outputHandler = NULL;
	m.errorHandler = NULL;

	m.beginImageCallback = defaultBeginImageCallback;
	m.outputCallback = defaultOutputCallback;
	m.errorCallback = defaultErrorCallback;

	m.outputHeader = true;
	m.container = Container_DDS;
}


/// Set output file name.
void OutputOptions::setFileName(const char * fileName)
{
	m.fileName = fileName; // @@ Do we need to record filename?
	m.outputHandler = NULL;

	DefaultOutputHandler * oh = new DefaultOutputHandler(fileName);
	if (!oh->stream.isError())
	{
		m.outputHandler = oh;
	}
	m.outputCallback = defaultOutputCallback;
}

/// Set output handler.
void OutputOptions::setOutputHandler(OutputHandler * outputHandler)
{
	if (!m.fileName.isNull())
	{
		delete m.outputHandler;
		m.fileName.reset();
	}
	m.outputHandler = outputHandler;
	m.outputCallback = defaultOutputCallback;
}

/// Set error handler.
void OutputOptions::setErrorHandler(ErrorHandler * errorHandler)
{
	m.errorHandler = errorHandler;
}

/// Set output header.
void OutputOptions::setOutputHeader(bool outputHeader)
{
	m.outputHeader = outputHeader;
}

/// Set container.
void OutputOptions::setContainer(Container container)
{
	m.container = container;
}


bool OutputOptions::Private::hasValidOutputHandler() const
{
	if (!fileName.isNull())
	{
		return outputHandler != NULL;
	}
	
	return true;
}

void OutputOptions::Private::beginImage(int size, int width, int height, int depth, int face, int miplevel) const
{
	beginImageCallback(size, width, height, depth, face, miplevel, outputHandler);
}

bool OutputOptions::Private::writeData(const void * data, int size) const
{
	return outputCallback(data, size, outputHandler);
}

void OutputOptions::Private::error(Error e) const
{
	errorCallback(e, errorHandler);
}
