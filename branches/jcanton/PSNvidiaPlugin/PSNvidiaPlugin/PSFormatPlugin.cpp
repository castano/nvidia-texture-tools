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
//	PSFormatPlugin.cp 			
// ===========================================================================

#include "PSFormatPlugin.h"

#include "PIFormat.h"

#include <string.h>



//-------------------------------------------------------------------------------
//	PSFormatPlugin
//-------------------------------------------------------------------------------

PSFormatPlugin::PSFormatPlugin ()
{
	mGlobals	=	NULL;
	mFormatRec	=	NULL;
	mResult		=	NULL;
}

//-------------------------------------------------------------------------------
//	~PSFormatPlugin
//-------------------------------------------------------------------------------

PSFormatPlugin::~PSFormatPlugin ()
{

}

//-------------------------------------------------------------------------------
//	Run
//-------------------------------------------------------------------------------

void PSFormatPlugin::Run 
(
	short 			inSelector,
 	FormatRecord* 	inFormatRecord,
 	long* 			inData,
 	short* 			outResult
)
{
	// plug-in's main routine
	// does the work of setting up the globals, and then
	// calls the appropriate override hook
	if (inSelector == formatSelectorAbout)
	{
		// format record isn't valid, so can't set up globals
		// just show about box
	
		DoAbout ((AboutRecord*) inFormatRecord);
	}
	else
	{
		// set up globals
	
		mResult 	= 	outResult;
		mFormatRec 	=	inFormatRecord;
		
		AllocateGlobals (inFormatRecord, inData, outResult);
	
		if (mGlobals == NULL)
		{
			*outResult = memFullErr;
			return;
		}
	
		
		// handle selector through override hooks
	
		switch (inSelector)
		{
			case formatSelectorFilterFile:			DoFilterFile();			break;

			case formatSelectorReadPrepare:			DoReadPrepare();		break;
			case formatSelectorReadStart:			DoReadStart();			break;
			case formatSelectorReadContinue:		DoReadContinue();		break;
			case formatSelectorReadFinish:			DoReadFinish();			break;

			case formatSelectorOptionsPrepare:		DoOptionsPrepare();		break;
			case formatSelectorOptionsStart:		DoOptionsStart();		break;
			case formatSelectorOptionsContinue:		DoOptionsContinue();	break;
			case formatSelectorOptionsFinish:		DoOptionsFinish();		break;

			case formatSelectorEstimatePrepare:		DoEstimatePrepare();	break;
			case formatSelectorEstimateStart:		DoEstimateStart();		break;
			case formatSelectorEstimateContinue:	DoEstimateContinue();	break;
			case formatSelectorEstimateFinish:		DoEstimateFinish();		break;

			case formatSelectorWritePrepare:		DoWritePrepare();		break;
			case formatSelectorWriteStart:			DoWriteStart();			break;
			case formatSelectorWriteContinue:		DoWriteContinue();		break;
			case formatSelectorWriteFinish:			DoWriteFinish();		break;

			default: *mResult = formatBadParameters;						break;
		}
		
		
		// unlock the handle containing our globals
		
		if ((Handle)*inData != NULL)
			inFormatRecord->handleProcs->unlockProc ((Handle)*inData);
	}
}

//-------------------------------------------------------------------------------
//	AllocateGlobals
//-------------------------------------------------------------------------------
//	
void PSFormatPlugin::AllocateGlobals
(
	FormatRecord* 	inFormatRecord,
 	long* 			inData,
 	short*			outResult
)
{
	// make sure globals are ready to go
	// allocate them if necessary, set pointer correctly if not needed
	// based heavily on AllocateGlobals() in PIUtilities.c, but modified
	// to allow subclasses to easily extend the Globals struct
	
	mGlobals = NULL;
	
	if (!*inData)
	{ 
		// Data is empty, so initialize our globals
		
		// Create a chunk of memory to put our globals.
		// Have to call HostNewHandle directly, since gStuff (in
		// the PINewHandle macro) hasn't been defined yet
		// use override hook to get size of globals struct
		
		Handle h = inFormatRecord->handleProcs->newProc (GlobalsSize());
		
		if (h != NULL)
		{ 
			// We created a valid handle. Use it.
		
			// lock the handle and move it high
			// (we'll unlock it after we're done):
			
			mGlobals = (PSFormatGlobals*) inFormatRecord->handleProcs->lockProc (h, TRUE);
			
			if (mGlobals != NULL)
			{ 
				// was able to create global pointer.
			
				// if we have revert info, copy it into the globals
				// otherwise, just init them
				
				if (inFormatRecord->revertInfo != NULL)
				{
					char* ptr = inFormatRecord->handleProcs->lockProc (inFormatRecord->revertInfo, false);
					memcpy (mGlobals, ptr, GlobalsSize());
					inFormatRecord->handleProcs->unlockProc (inFormatRecord->revertInfo);
				}
				else				
				{
					InitGlobals ();
				}
				
				
				// store the handle in the passed in long *data:
				
				*inData = (long)h;
				h = NULL; // clear the handle, just in case
			
			}
			else
			{ 
				// There was an error creating the pointer.  Back out
			  	// of all of this.

				inFormatRecord->handleProcs->disposeProc (h);
				h = NULL; // just in case
			}
		}		
	}
	else
	{
		// we've already got a valid structure pointed to by *data
		// lock it, cast the returned pointer to a global pointer
		// and point globals at it:

		mGlobals = (PSFormatGlobals*) inFormatRecord->handleProcs->lockProc ((Handle)*inData, TRUE);
	}	
}

//-------------------------------------------------------------------------------
//	GlobalsSize
//-------------------------------------------------------------------------------

int PSFormatPlugin::GlobalsSize ()
{
	// override if your subclass adds fields to the Globals struct
	// the first two fields must always be:
	//	short*					result;
	//  FormatRecord*			formatParamBlock;

	return sizeof (PSFormatGlobals);
}

//-------------------------------------------------------------------------------
//	InitGlobals
//-------------------------------------------------------------------------------

void PSFormatPlugin::InitGlobals ()
{
	// override hook - PSFormatGlobals are set up in AllocateGlobals()
}

//-------------------------------------------------------------------------------
//	DoAbout
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoAbout (AboutRecord* inAboutRec)
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoReadPrepare
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoReadPrepare ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoReadStart
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoReadStart ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoReadContinue
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoReadContinue ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoReadFinish
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoReadFinish ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoOptionsPrepare
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoOptionsPrepare ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoOptionsStart
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoOptionsStart ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoOptionsContinue
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoOptionsContinue ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoOptionsFinish
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoOptionsFinish ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoEstimatePrepare
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoEstimatePrepare ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoEstimateStart
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoEstimateStart ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoEstimateContinue
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoEstimateContinue ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoEstimateFinish
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoEstimateFinish ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoWritePrepare
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoWritePrepare ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoWriteStart
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoWriteStart ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoWriteContinue
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoWriteContinue ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoWriteFinish
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoWriteFinish ()
{
	// override hook
}

//-------------------------------------------------------------------------------
//	DoFilterFile
//-------------------------------------------------------------------------------

void PSFormatPlugin::DoFilterFile ()
{
	// override hook
}



