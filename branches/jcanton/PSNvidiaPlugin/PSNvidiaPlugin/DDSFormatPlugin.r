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
//	ExrFormat.r				Part of OpenEXR
// ===========================================================================


#include "PIGeneral.r"



//-------------------------------------------------------------------------------
//	PiPL resource
//-------------------------------------------------------------------------------

resource 'PiPL' (16000, "NvidiaPlugin PiPL", purgeable)
{
    {
		Kind 				{ ImageFormat },
		Name 				{ "NvidiaPlugin" },
		Version 			{ (latestFormatVersion << 16) | latestFormatSubVersion },		

#if MSWindows
		CodeWin32X86       { "PluginMain" },
#elif TARGET_API_MAC_CARBON
		CodeCarbonPowerPC	{ 0, 0, "" },
#else
        CodePowerPC 		{ 0, 0, "" },
#endif
		
		FmtFileType 		{ 'EXR ', '8BIM' },
		ReadTypes 			{ { 'EXR ', '    ' } },
		FilteredTypes 		{ { 'EXR ', '    ' } },
		ReadExtensions 		{ { 'exr ' } },
		WriteExtensions 	{ { 'exr ' } },
		FilteredExtensions 	{ { 'exr ' } },
		FormatMaxSize		{ { 32767, 32767 } },

		
		// this is to make us available when saving a 16-bit image  

		EnableInfo          
		{ 
			"in (PSHOP_ImageMode, RGBMode, RGB48Mode)"		
		},      


		// this is apparently just for backwards compatability

		SupportedModes
		{
			noBitmap,
			noGrayScale,
			noIndexedColor,
			doesSupportRGBColor,	// yes
			noCMYKColor,
			noHSLColor,
			noHSBColor,
			noMultichannel,
			noDuotone,
			noLABColor
#if !MSWindows			
			,
			noGray16,
			doesSupportRGB48,		// yes
			noLab48,
			noCMYK64,
			noDeepMultichannel,
			noDuotone16
#endif
		},

		FormatFlags 
		{
			fmtDoesNotSaveImageResources, 
			fmtCanRead, 
			fmtCanWrite, 
			fmtCanWriteIfRead, 
			fmtCannotWriteTransparency
#if MSWindows			
			,
			fmtCannotCreateThumbnail
#endif
		},	
		
		FormatMaxChannels 
		{ 
			{
				1, 
				4, 
				4, 
				4, 
				4, 
				4, 
				4, 
				4, 
				4, 
				4, 
				4, 
				4
			}
		}
	}
};



