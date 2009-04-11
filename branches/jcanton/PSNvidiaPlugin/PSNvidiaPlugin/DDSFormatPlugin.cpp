//Copyright (c) 2008, Javier Cantón Ferrero <javiuniversidad@gmail.com>
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

// ===========================================================================
//	DDSFormatPlugin.cpp
// ===========================================================================

#include "DDSFormatPlugin.h"

#include "PITypes.h"
#include "ADMBasic.h"

#include <cassert>

#include <qt/QApplication.h>
#include <qt/QPushButton.h>
#include "QtHelpers.h"

//-------------------------------------------------------------------------------
//	EXRFormatPlugin
//-------------------------------------------------------------------------------

DDSFormatPlugin::DDSFormatPlugin ()
{
	// nothing to do
}

//-------------------------------------------------------------------------------
//	~EXRFormatPlugin
//-------------------------------------------------------------------------------

DDSFormatPlugin::~DDSFormatPlugin ()
{
	// nothing to do
}

//-------------------------------------------------------------------------------
//	DoAbout
//-------------------------------------------------------------------------------

void DDSFormatPlugin::DoAbout (AboutRecord* inAboutRec)
{
	//QtHelpers::QtIntialize();

	//QWidget *window = new QWidget();
	//window->resize(320,240);
	//window->show();
	//
	//QPushButton *button = new QPushButton("Qt Button", window);
	//button->move(100, 100);
	//button->show();

	//HWND hwnd = (HWND)((PlatformData*)inAboutRec->platformData)->hwnd;
	//SetParent(window->winId(), hwnd);

	//qApp->exec();

	//QtHelpers::QtShutdown();

	//Using Adobe Dialog Manager (ADM)
 	if (inAboutRec != NULL && inAboutRec->sSPBasic != NULL)
   {
      ADMBasicSuite6* basicSuite = NULL;
        
 		inAboutRec->sSPBasic->AcquireSuite (kADMBasicSuite, kADMBasicSuiteVersion6, (const void**) &basicSuite);

        if (basicSuite != NULL)
        {                      
            basicSuite->MessageAlert ("Nvidia texture tool\n\n"
                                      "Plug-in by Ignacio Castaño, Javier Cantón\n"
                                      "www.nvidia.com");

            inAboutRec->sSPBasic->ReleaseSuite (kADMBasicSuite, kADMBasicSuiteVersion6);
            basicSuite = NULL;
        }
    }
}

//-------------------------------------------------------------------------------
//	Main entry point
//-------------------------------------------------------------------------------

DLLExport MACPASCAL 
void PluginMain
(
	const short 		inSelector,
	FormatRecord*		inFormatRecord,
	long*				inData,
	short*				outResult
)
{

	// configure resampling based on host's capabilities
	
	/*if (inSelector != formatSelectorAbout)
	{
		ConfigureLimits (inFormatRecord);
	}
	*/

	// create and run the plug-in

	//try
	//{
		DDSFormatPlugin		plugin;
			
		plugin.Run (inSelector, inFormatRecord, inData, outResult);
	//}
	//
	//
	//// catch out-of-memory exception
	//
	//catch (const std::bad_alloc&)
	//{
	//	*outResult = memFullErr;
	//}
	//
	//
	//// catch an exception that provides an error string
	//
	//catch (const std::exception& ex)
	//{
	//	if (inSelector == formatSelectorAbout || ex.what() == NULL)
	//	{
	//		*outResult = formatCannotRead;
	//	}
	//	else
	//	{
	//		memcpy (& (*inFormatRecord->errorString)[1], ex.what(), strlen (ex.what()));
	//		(*inFormatRecord->errorString)[0] = strlen (ex.what());
	//		*outResult = errReportString;
	//	}
	//}
	//
	//
	//// catch any other exception (we don't want to throw back into the host)
	//
	//catch (...)
	//{
	//	*outResult = formatCannotRead;
	//}
}