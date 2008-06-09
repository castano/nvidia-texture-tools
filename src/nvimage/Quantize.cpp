// This code is in the public domain -- castanyo@yahoo.es

/*
http://www.visgraf.impa.br/Courses/ip00/proj/Dithering1/floyd_steinberg_dithering.html
http://www.gamedev.net/reference/articles/article341.asp

@@ Look at LPS: http://www.cs.rit.edu/~pga/pics2000/i.html
 
This is a really nice guide to dithering algorithms:
http://www.efg2.com/Lab/Library/ImageProcessing/DHALF.TXT

@@ This code needs to be reviewed, I'm not sure it's correct.
*/

#include <nvmath/Color.h>

#include <nvimage/Image.h>
#include <nvimage/Quantize.h>

using namespace nv;


// Simple quantization.
void nv::Quantize::BinaryAlpha( Image * image, int alpha_threshold /*= 127*/ )
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);
			
			// Convert color.
			if( pixel.a > alpha_threshold ) pixel.a = 255;
			else pixel.a = 0;
			
			// Store color.
			image->pixel(x, y) = pixel;
		}
	}
}


// Simple quantization.
void nv::Quantize::RGB16( Image * image )
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel32 = image->pixel(x, y);
			
			// Convert to 16 bit and back to 32 using regular bit expansion.
			Color32 pixel16 = toColor32( toColor16(pixel32) );
			
			// Store color.
			image->pixel(x, y) = pixel16;
		}
	}
}

// Alpha quantization.
void nv::Quantize::Alpha4( Image * image )
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);
			
			// Convert to 4 bit using regular bit expansion.
			pixel.a = (pixel.a & 0xF0) | ((pixel.a & 0xF0) >> 4);
			
			// Store color.
			image->pixel(x, y) = pixel;
		}
	}
}


// Error diffusion. Floyd Steinberg.
void nv::Quantize::FloydSteinberg_RGB16( Image * image )
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	// @@ Use fixed point?
	Vector3 * row0 = new Vector3[w+2];
	Vector3 * row1 = new Vector3[w+2];
	memset(row0, 0, sizeof(Vector3)*(w+2));
	memset(row1, 0, sizeof(Vector3)*(w+2));
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel32 = image->pixel(x, y);
			
			// Add error.	// @@ We shouldn't clamp here!
			pixel32.r = clamp(int(pixel32.r) + int(row0[1+x].x()), 0, 255);
			pixel32.g = clamp(int(pixel32.g) + int(row0[1+x].y()), 0, 255);
			pixel32.b = clamp(int(pixel32.b) + int(row0[1+x].z()), 0, 255);
			
			// Convert to 16 bit. @@ Use regular clamp?
			Color32 pixel16 = toColor32( toColor16(pixel32) );
			
			// Store color.
			image->pixel(x, y) = pixel16;
			
			// Compute new error.
			Vector3 diff(float(pixel32.r - pixel16.r), float(pixel32.g - pixel16.g), float(pixel32.b - pixel16.b));
			
			// Propagate new error.
			row0[1+x+1] += 7.0f / 16.0f * diff;
			row1[1+x-1] += 3.0f / 16.0f * diff;
			row1[1+x+0] += 5.0f / 16.0f * diff;
			row1[1+x+1] += 1.0f / 16.0f * diff;
		}
		
		swap(row0, row1);
		memset(row1, 0, sizeof(Vector3)*(w+2));
	}
	
	delete [] row0;
	delete [] row1;
}


// Error diffusion. Floyd Steinberg.
void nv::Quantize::FloydSteinberg_BinaryAlpha( Image * image, int alpha_threshold /*= 127*/ ) 
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	// @@ Use fixed point?
	float * row0 = new float[(w+2)];
	float * row1 = new float[(w+2)];
	memset(row0, 0, sizeof(float)*(w+2));
	memset(row1, 0, sizeof(float)*(w+2));
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);
			
			// Add error.
			int alpha = int(pixel.a) + int(row0[1+x]);
			
			// Convert color.
			if( alpha > alpha_threshold ) pixel.a = 255;
			else pixel.a = 0;
			
			// Store color.
			image->pixel(x, y) = pixel;
			
			// Compute new error.
			float diff = float(alpha - pixel.a);
			
			// Propagate new error.
			row0[1+x+1] += 7.0f / 16.0f * diff;
			row1[1+x-1] += 3.0f / 16.0f * diff;
			row1[1+x+0] += 5.0f / 16.0f * diff;
			row1[1+x+1] += 1.0f / 16.0f * diff;
		}
		
		swap(row0, row1);
		memset(row1, 0, sizeof(float)*(w+2));
	}
	
	delete [] row0;
	delete [] row1;
}


// Error diffusion. Floyd Steinberg.
void nv::Quantize::FloydSteinberg_Alpha4( Image * image )
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	// @@ Use fixed point?
	float * row0 = new float[(w+2)];
	float * row1 = new float[(w+2)];
	memset(row0, 0, sizeof(float)*(w+2));
	memset(row1, 0, sizeof(float)*(w+2));
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);
			
			// Add error.
			int alpha = int(pixel.a) + int(row0[1+x]);
			
			// Convert to 4 bit using regular bit expansion.
			pixel.a = (pixel.a & 0xF0) | ((pixel.a & 0xF0) >> 4);
			
			// Store color.
			image->pixel(x, y) = pixel;
			
			// Compute new error.
			float diff = float(alpha - pixel.a);
			
			// Propagate new error.
			row0[1+x+1] += 7.0f / 16.0f * diff;
			row1[1+x-1] += 3.0f / 16.0f * diff;
			row1[1+x+0] += 5.0f / 16.0f * diff;
			row1[1+x+1] += 1.0f / 16.0f * diff;
		}
		
		swap(row0, row1);
		memset(row1, 0, sizeof(float)*(w+2));
	}
	
	delete [] row0;
	delete [] row1;
}

