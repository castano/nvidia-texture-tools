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
//	DDSFormatPlugin.h
// ===========================================================================

#pragma once

#include "PSFormatPlugin.h"
#include "DDSFormatGlobals.h"

#include "PIDefines.h"
#include "PITypes.h"

//-------------------------------------------------------------------------------
//	EXRFormatPlugin
//-------------------------------------------------------------------------------

class DDSFormatPlugin : public PSFormatPlugin
{
	public:
	
						DDSFormatPlugin			();
		virtual			~DDSFormatPlugin		();
	
	
	protected:

		// access to our Globals struct
	
 		inline DDSFormatGlobals* Globals		()
 		{
 			return (DDSFormatGlobals*) mGlobals;
 		}

		
		// PSFormatPlugin Overrides
				
		virtual int		GlobalsSize				();
		virtual void	InitGlobals				();

		virtual void	DoAbout					(AboutRecord* inAboutRec);
			
		virtual void	DoReadPrepare			();
		virtual void	DoReadStart				();
		virtual void	DoReadContinue			();
		virtual void	DoReadFinish			();
			
		//virtual void	DoOptionsStart			();
		//virtual void	DoEstimateStart			();
 		//virtual void	DoWriteStart			();
};


//-------------------------------------------------------------------------------
//	Main entry point
//-------------------------------------------------------------------------------

DLLExport MACPASCAL 
void PluginMain 			(const short 		inSelector,
			  	             FormatRecord*		inFormatRecord,
				             long*				inData,
				             short*				outResult);
