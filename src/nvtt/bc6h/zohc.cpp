/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

// WORK: the widecolorgamut.exr image is the poster child for not doing independent compression of each 4x4 tile. (The image is available from www.openexr.org.)
// At the lower-left vertex, the companded image shows a visible artifact due to the vertex being compressed with 6 bit endpoint accuracy
// but the constant tile right next to it being compressed with 16 bit endpoint accuracy. It's an open problem to figure out how to deal with that in the best possible way.
//
// WORK: we removed 4 codes since we couldn't come up with anything to use them for that showed a worthwhile improvement in PSNR. Clearly the compression format can be improved since
// we're only using 7/8 of the available code space. But how?
// 
// NOTE: HDR compression formats that compress luminance and chrominance separatey and multiply them together to get the final decompressed channels tend to do poorly at extreme values.

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#include <ImfArray.h>
#include "exr.h"
#include "zoh.h"
#include "utils.h"

using namespace std;

static int mpsnr_low = -10, mpsnr_high = 10;

static void dump(char *tag, Array2D<Rgba> &in1, int x, int y)
{
	printf("\n%s\n", tag);
	for (int y0=0; y0<4; ++y0)
	{
		for (int x0=0; x0<4; ++x0)
			printf("%6d%6d%6d   ", Utils::ushort_to_format((in1[y+y0][x+x0].r).bits()), Utils::ushort_to_format((in1[y+y0][x+x0].g).bits()), Utils::ushort_to_format((in1[y+y0][x+x0].b).bits()));
		printf("\n");
	}
}

static void analyze(string in1, string in2)
{
	Array2D<Rgba> pin1, pin2;
	int w1, h1, w2, h2;

	Exr::readRgba(in1, pin1, w1, h1);
	Exr::readRgba(in2, pin2, w2, h2);

	// choose the smaller of the two dimensions (since the old compressor would truncate to multiple-of-4 sizes)
	int w = MIN(w1, w2);
	int h = MIN(h1, h2);

	double nsamples = 0;
	double mabse = 0, mse = 0, mpsnre = 0;
	int errdist[17];
	int errs[3*16];

	for (int i=0; i<17; ++i)
		errdist[i] = 0;

	int psnrhist[100];
	for (int i=0; i<100; ++i)
		psnrhist[i] = 0;
	bool first = true;

	for (int y = 0; y < h; y+=4)
	for (int x = 0; x < w; x+=4)
	{
		int xw = MIN(w-x, 4);
		int yw = MIN(h-y, 4);
		int np = 0;

		Vec3 a, b;

		for (int y0=0; y0<yw; ++y0)
		for (int x0=0; x0<xw; ++x0)
		{
			a.X() = Utils::ushort_to_format(((pin1[y+y0][x+x0]).r).bits());
			a.Y() = Utils::ushort_to_format(((pin1[y+y0][x+x0]).g).bits());
			a.Z() = Utils::ushort_to_format(((pin1[y+y0][x+x0]).b).bits());

			b.X() = Utils::ushort_to_format(((pin2[y+y0][x+x0]).r).bits());
			b.Y() = Utils::ushort_to_format(((pin2[y+y0][x+x0]).g).bits());
			b.Z() = Utils::ushort_to_format(((pin2[y+y0][x+x0]).b).bits());

			for (int exposure = mpsnr_low; exposure <= mpsnr_high; ++exposure)
				mpsnre += Utils::mpsnr_norm(a, exposure, b);

			errs[np+0] = a.X() - b.X();
			errs[np+1] = a.Y() - b.Y();
			errs[np+2] = a.Z() - b.Z();
			np += 3;
		}

		double msetile = 0.0;

		for (int i = 0; i < np; ++i)
		{
			int err = errs[i];
			int abse = err > 0 ? err : -err;
			mabse += (double)abse;
			mse += (double)abse * abse;
			msetile += (double)abse * abse;

			int lsb;

			for (lsb=0; abse>0; ++lsb, abse >>= 1)
				;

			errdist[lsb]++;
		}

		double psnrtile, rmsetile;

		rmsetile = sqrt(msetile / double(np));
		psnrtile = (rmsetile == 0) ? 99.0 : 20.0 * log10(32767.0/rmsetile);

		int psnrquant = (int) floor (psnrtile);		// 10 means [10,11) psnrs, e.g.
		// clamp just in case
		psnrquant = (psnrquant < 0) ? 0 : (psnrquant > 99) ? 99 : psnrquant;
		psnrhist[psnrquant]++;
		if (first && psnrquant < 20)
		{
			first = false;
			printf("Tiles with PSNR's worse than 20dB\n");
		}
		if (psnrquant < 20)
			printf("X %4d Y %4d PSNR %7.2f\n", x, y, psnrtile);
	}
	
	nsamples = w * h * 3;

	mabse /= nsamples;
	mse /= nsamples;

	double rmse, psnr;

	rmse = sqrt(mse);
	psnr = (rmse == 0) ? 999.0 : 20.0 * log10(32767.0/rmse);

	mpsnre /= (mpsnr_high-mpsnr_low+1) * w * h;

	double mpsnr = (mpsnre == 0) ? 999.0 : 10.0 * log10(3.0 * 255.0 * 255.0 / mpsnre);

	printf("Image size compared: %dw x %dh\n", w, h);
	if (w != w1 || w != w2 || h != h1 || h != h2)
		printf("--- NOTE: only the overlap between the 2 images (%d,%d) and (%d,%d) was compared\n", w1, h1, w2, h2);
	printf("Total pixels: %12.0f\n", nsamples/3);
	printf("Mean absolute error: %f\n", mabse);
	printf("Root mean squared error: %f\n", rmse);
	printf("Peak signal to noise ratio in dB: %f\n", psnr);
	printf("mPSNR for exposure range %d..%d: %8.3f\n", mpsnr_low, mpsnr_high, mpsnr);
	printf("Histogram of number of channels with indicated LSB error\n");
	for (int i = 0; i < 17; ++i)
		if (errdist[i])
			printf("%2d LSB error: %10d\n", i, errdist[i]);
#if 0
	printf("Histogram of per-tile PSNR\n");
	for (int i = 0; i < 100; ++i)
		if (psnrhist[i])
			printf("[%2d,%2d) %6d\n", i, i+1, psnrhist[i]);
#endif
}

static bool ext(string inf, char *extension)
{
	size_t n = inf.rfind('.', inf.length()-1);
	if (n != string::npos)
		return inf.substr(n, inf.length()) == extension;
	else if (*extension != '\0')
		return false;
	else
		return true;	// extension is null and we didn't find a .
}

template <typename T>
std::string toString(const T &thing) 
{
	std::stringstream os;
	os << thing;
	return os.str();
}

static int str2int(std::string s) 
{
	int thing;
	std::stringstream str (stringstream::in | stringstream::out);
	str << s;
	str >> thing;
	return thing;
}

static void usage()
{
	cout << endl <<
	"Usage:" << endl <<
	"zohc infile.exr outroot             generates outroot-w-h.bc6, outroot-bc6.exr" << endl <<
	"zohc foo-w-h.bc6 outroot            generates outroot-bc6.exr" << endl <<
	"zohc infile.exr outfile.exr [e1 e2] compares the two images; optionally specify the mPSNR exposure range" << endl << endl <<
	"Flags:" << endl <<
	"-u     treat the input as unsigned. negative values are clamped to zero. (default)" << endl <<
	"-s     treat the input as signed." << endl;
}

Format Utils::FORMAT = UNSIGNED_F16;

int main(int argc, char* argv[])
{
#ifdef EXTERNAL_RELEASE
	cout << "BC6H OpenEXR RGB Compressor/Decompressor version 1.61 (May 27 2010)." << endl <<
			"Bug reports, questions, and suggestions to wdonovan a t nvidia d o t com." << endl << endl;
#endif
	try
	{
		char * args[4];
		int nargs = 0;
		bool is_unsigned = true;
		bool is_float = true;

		// process flags, copy any non flag arg to args[]
		for (int i = 1; i < argc; ++i)
		{
			if ((argv[i])[0] == '-')
				switch ((argv[i])[1]) {
					case 'u': is_unsigned = true; break;
					case 's': is_unsigned = false; break;
					default:  throw "bad flag arg";
				}
			else
			{
				if (nargs >= 6) throw "Incorrect number of args";
				args[nargs++] = argv[i];
			}
		}

		Utils::FORMAT = (!is_unsigned) ? SIGNED_F16 : UNSIGNED_F16;

		if (nargs < 2) throw "Incorrect number of args";

		string inf(args[0]), outroot(args[1]);

		cout << "Input format is: " << (is_unsigned ? "UNSIGNED FLOAT_16" : "SIGNED FLOAT_16") << endl;

		if (ext(outroot, ""))
		{
			if (ext(inf, ".exr"))
			{
				int width, height;
				Exr::fileinfo(inf, width, height);
				string outf, zohf;
				outf = outroot + "-bc6.exr";
				zohf = outroot + "-" + toString(width) + "-" + toString(height) + ".bc6";
				cout << "Compressing " << inf << " to " << zohf << endl;
				ZOH::compress(inf, zohf);
				cout << "Decompressing " << zohf << " to " << outf << endl;
				ZOH::decompress(zohf, outf);
				analyze(inf, outf);
			}
			else if (ext(inf, ".bc6"))
			{
				string outf;
				outf = outroot + "-bc6.exr";
				cout << "Decompressing " << inf << " to " << outf << endl;
				ZOH::decompress(inf, outf);
			}
			else throw "Invalid file args";
		}
		else if (ext(inf, ".exr") && ext(outroot, ".exr"))
		{
			if (nargs == 4)
			{
				string low(args[2]), high(args[3]);
				mpsnr_low = str2int(low);
				mpsnr_high = str2int(high);
				if (mpsnr_low > mpsnr_high) throw "Invalid exposure range";
			}
			analyze(inf, outroot);
		}
		else throw "Invalid file args";
	}
	catch(const exception& e)
	{
		// Print error message and usage instructions
		cerr << e.what() << endl;
		usage();
		return 1;
	}
	catch(char * msg)
	{
		cerr << msg << endl;
		usage();
		return 1;
	}
	return 0;
}
