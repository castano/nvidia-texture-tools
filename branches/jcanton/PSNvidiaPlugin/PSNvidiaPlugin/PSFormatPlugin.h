//Copyright (c) 2006, Industrial Light & Magic, a division of Lucasfilm
//
//Entertainment Company Ltd.  Portions contributed and copyright held by
//
//others as indicated.  All rights reserved.
//
// 
//
//Redistribution and use in source and binary forms, with or without
//
//modification, are permitted provided that the following conditions are
//
//met:
//
// 
//
//    * Redistributions of source code must retain the above
//
//      copyright notice, this list of conditions and the following
//
//      disclaimer.
//
// 
//
//    * Redistributions in binary form must reproduce the above
//
//      copyright notice, this list of conditions and the following
//
//      disclaimer in the documentation and/or other materials provided with
//
//      the distribution.
//
// 
//
//    * Neither the name of Industrial Light & Magic nor the names of
//
//      any other contributors to this software may be used to endorse or
//
//      promote products derived from this software without specific prior
//
//      written permission.
//
// 
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//
//IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//
//PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//
//EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//
//PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//
//PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//
//LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// ===========================================================================
//	PSFormatPlugin.h			
// ===========================================================================

#pragma once

#include "PSFormatGlobals.h"

#include "PIAbout.h"


//-------------------------------------------------------------------------------
//	PSFormatPlugin
//-------------------------------------------------------------------------------
//
//	Base class for a Photoshop File Format plugin.
//

class PSFormatPlugin
{
	public:
	
			//-------------------------------------------------------------------
			//	Constructor / Destructor
			//-------------------------------------------------------------------
	
									PSFormatPlugin			();
		virtual						~PSFormatPlugin			();


			//-------------------------------------------------------------------
			//	Run - main function called from plug-ins main entry point
			//-------------------------------------------------------------------
	
				void				Run						(short				inSelector,
															 FormatRecord*		inFormatRecord,
															 long*				inData,
															 short*				outResult);
		
	
	protected:


			//-------------------------------------------------------------------
			//	Convenience routines for making globals as painless
			//	as possible (not very painless, though)
			//-------------------------------------------------------------------
				
				void				AllocateGlobals			(FormatRecord*		inFormatRecord,
															 long*				inData,
															 short*				outResult);



			//-------------------------------------------------------------------
			//	Override hooks - subclasses should override as many of these
			//	as they need to, and disregard the rest
			//-------------------------------------------------------------------
		
		virtual int					GlobalsSize				();
		virtual void				InitGlobals				();

		virtual void				DoAbout					(AboutRecord*       inAboutRec);
			
		virtual void				DoReadPrepare			();
		virtual void				DoReadStart				();
		virtual void				DoReadContinue			();
		virtual void				DoReadFinish			();
			
		virtual void				DoOptionsPrepare		();
		virtual void				DoOptionsStart			();
		virtual void				DoOptionsContinue		();
		virtual void				DoOptionsFinish			();
			
		virtual void				DoEstimatePrepare		();
		virtual void				DoEstimateStart			();
		virtual void				DoEstimateContinue		();
		virtual void				DoEstimateFinish		();
				
		virtual void				DoWritePrepare			();
		virtual void				DoWriteStart			();
		virtual void				DoWriteContinue			();
		virtual void				DoWriteFinish			();
			
		virtual void				DoFilterFile			();


		//-------------------------------------------------------------------
		//	Globals - valid upon entry into every override hook
		//	except DoAbout().  May actually be a pointer to your
		//	subclass of PSFormatGlobals.
		//-------------------------------------------------------------------

		PSFormatGlobals*			mGlobals;
		short*						mResult;
		FormatRecord*				mFormatRec;
};
