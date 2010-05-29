/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

// NOTE: the compressor will compress RGB tiles where the input alpha is constant at 255
// using modes where the alpha is variable if that mode gives a smaller mean squared error.

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <assert.h>

#include "ImfArray.h"
#include "targa.h"
#include "avpcl.h"

using namespace std;

static void analyze(string in1, string in2)
{
	Array2D<RGBA> pin1, pin2;
	int w1, h1, w2, h2;

	Targa::read(in1, pin1, w1, h1);
	Targa::read(in2, pin2, w2, h2);

	// choose the smaller of the two dimensions (since the old compressor would truncate to multiple-of-4 sizes)
	int w = MIN(w1, w2);
	int h = MIN(h1, h2);

	double nsamples = 0;
	double mabse_rgb = 0, mabse_a = 0, mabse_rgba = 0, mse_rgb = 0, mse_a = 0, mse_rgba = 0;
	int errdist_rgb[9], errdist_a[9], errdist_rgba[9];
	int errs[4*16];

	for (int i=0; i<9; ++i)
		errdist_rgb[i] = errdist_a[i] = errdist_rgba[i] = 0;

	int psnrhist[100];
	for (int i=0; i<100; ++i)
		psnrhist[i] = 0;
	bool first = true;

	int worstx, worsty;
	double worstpsnr = 999.0;

	bool constant_alpha = true;

	for (int y = 0; y < h; y+=4)
	for (int x = 0; x < w; x+=4)
	{
		int xw = MIN(w-x, 4);
		int yw = MIN(h-y, 4);
		int np = 0;

		float a[4], b[4];

		for (int y0=0; y0<yw; ++y0)
		for (int x0=0; x0<xw; ++x0)
		{
			a[0] = (pin1[y+y0][x+x0]).r;
			a[1] = (pin1[y+y0][x+x0]).g;
			a[2] = (pin1[y+y0][x+x0]).b;
			a[3] = (pin1[y+y0][x+x0]).a;

			b[0] = (pin2[y+y0][x+x0]).r;
			b[1] = (pin2[y+y0][x+x0]).g;
			b[2] = (pin2[y+y0][x+x0]).b;
			b[3] = (pin2[y+y0][x+x0]).a;

			if (AVPCL::flag_premult)
			{
				// premultiply
				for (int i=0; i<3; ++i)
				{
					a[i] = Utils::premult(a[i], a[3]);
					b[i] = Utils::premult(b[i], b[3]);
				}
			}

			if (a[3] != RGBA_MAX || b[3] != RGBA_MAX) 
				constant_alpha = false;

			for (int i=0; i<4; ++i)
				errs[np+i] = a[i] - b[i];

			np += 4;
		}

		double msetile = 0.0;

		for (int i = 0; i < np; ++i)
		{
			int err = errs[i];
			int abse = err > 0 ? err : -err;
			int j = i & 3;
			int lsb;

			for (lsb=0; (abse>>lsb)>0; ++lsb)
				;
			assert (lsb <= 8);

			if (j == 3)
			{
				mabse_a += (double)abse;
				mse_a += (double)abse * abse;
				errdist_a[lsb]++;
			}
			else
			{
				mabse_rgb += (double)abse;
				mse_rgb += (double)abse * abse;
				errdist_rgb[lsb]++;
			}
			mabse_rgba += (double)abse;
			mse_rgba += (double)abse * abse;
			errdist_rgba[lsb]++;

			msetile += (double)abse * abse;
		}

		double psnrtile, rmsetile;

		rmsetile = sqrt(msetile / double(np));
		psnrtile = (rmsetile == 0) ? 99.0 : 20.0 * log10(255.0/rmsetile);

		if (psnrtile < worstpsnr)
		{
			worstx = x; worsty = y; worstpsnr = psnrtile;
		}
#ifdef EXTERNAL_RELEASE
		int psnrquant = (int) floor (psnrtile);		// 10 means [10,11) psnrs, e.g.
		// clamp just in case
		psnrquant = (psnrquant < 0) ? 0 : (psnrquant > 99) ? 99 : psnrquant;
		psnrhist[psnrquant]++;
		if (first && psnrquant < 16)
		{
			first = false;
			printf("Tiles with RGBA PSNR's worse than 16dB\n");
		}
		if (psnrquant < 16)
			printf("X %4d Y %4d RGBA PSNR %7.2f\n", x, y, psnrtile);
#endif
	}
	
	nsamples = w * h;

	mabse_a /= nsamples;
	mse_a /= nsamples;
	mabse_rgb /= (nsamples*3);
	mse_rgb /= (nsamples*3);
	mabse_rgba /= (nsamples*4);
	mse_rgba /= (nsamples*4);

	double rmse_a, psnr_a, rmse_rgb, psnr_rgb, rmse_rgba, psnr_rgba;

	rmse_a = sqrt(mse_a);
	psnr_a = (rmse_a == 0) ? 999.0 : 20.0 * log10(255.0/rmse_a);

	rmse_rgb = sqrt(mse_rgb);
	psnr_rgb = (rmse_rgb == 0) ? 999.0 : 20.0 * log10(255.0/rmse_rgb);

	rmse_rgba = sqrt(mse_rgba);
	psnr_rgba = (rmse_rgba == 0) ? 999.0 : 20.0 * log10(255.0/rmse_rgba);

	printf("Image size compared: %dw x %dh\n", w, h);
	printf("Image alpha is %s.\n", constant_alpha ? "CONSTANT" : "VARIABLE");
	if (w != w1 || w != w2 || h != h1 || h != h2)
		printf("--- NOTE: only the overlap between the 2 images (%d,%d) and (%d,%d) was compared\n", w1, h1, w2, h2);
	printf("Total pixels: %12d\n", w * h);

	char *which = !AVPCL::flag_premult ? "RGB" : "aRaGaB";

	printf("\n%s Mean absolute error: %f\n", which, mabse_rgb);
	printf("%s Root mean squared error: %f (MSE %f)\n", which, rmse_rgb, rmse_rgb*rmse_rgb);
	printf("%s Peak signal to noise ratio in dB: %f\n", which, psnr_rgb);
	printf("%s Histogram of number of channels with indicated LSB error\n", which);
	for (int i = 0; i < 9; ++i)
		if (errdist_rgb[i]) printf("%2d LSB error: %10d\n", i, errdist_rgb[i]);

	printf("\nAlpha Mean absolute error: %f\n", mabse_a);
	printf("Alpha Root mean squared error: %f (MSE %f)\n", rmse_a, rmse_a*rmse_a);
	printf("Alpha Peak signal to noise ratio in dB: %f\n", psnr_a);
	printf("Alpha Histogram of number of channels with indicated LSB error\n");
	for (int i = 0; i < 9; ++i)
		if (errdist_a[i]) printf("%2d LSB error: %10d\n", i, errdist_a[i]);

	printf("\nRGBA Mean absolute error: %f\n", mabse_rgba);
	printf("RGBA Root mean squared error: %f (MSE %f)\n", rmse_rgba, rmse_rgba*rmse_rgba);
	printf("RGBA Peak signal to noise ratio in dB: %f\n", psnr_rgba);
	printf("RGBA Histogram of number of channels with indicated LSB error\n");
	for (int i = 0; i < 9; ++i)
		if (errdist_rgba[i]) printf("%2d LSB error: %10d\n", i, errdist_rgba[i]);

	printf("\nWorst tile RGBA PSNR %f at x %d y %d\n", worstpsnr, worstx, worsty);
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
	"avpclc infile.tga outroot       generates outroot-w-h.avpcl and outroot-avpcl.tga" << endl <<
	"avpclc foo-w-h.avpcl outroot    generates outroot-avpcl.tga" << endl <<
	"avpclc infile.tga outfile.tga   compares the two images" << endl << endl <<
	"Flags:" << endl <<
	"-p     use a metric based on AR AG AB A (note: if the image has alpha constant 255 this option is overridden)" << endl <<
	"-n     use a non-uniformly-weighed metric (weights .299 .587 .114)" << endl <<
	"-na	use a non-uniformly-weighed metric (ATI weights .3086 .6094 .0820)" << endl <<
	"-e     dump squared errors for each tile to outroot-errors.bin" << endl;
}

bool AVPCL::flag_premult = false;
bool AVPCL::flag_nonuniform = false;
bool AVPCL::flag_nonuniform_ati = false;

bool AVPCL::mode_rgb = false;

int main(int argc, char* argv[])
{
	bool noerrfile = true;
#ifdef EXTERNAL_RELEASE
	cout << "avpcl/BC7L Targa RGBA Compressor/Decompressor version 1.41 (May 27, 2010)." << endl <<
			"Bug reports, questions, and suggestions to wdonovan a t nvidia d o t com." << endl;
#endif
	try
	{
		char * args[2];
		int nargs = 0;

		// process flags, copy any non flag arg to args[]
		for (int i = 1; i < argc; ++i)
			if ((argv[i])[0] == '-')
				switch ((argv[i])[1]) {
					case 'p': AVPCL::flag_premult = true; break;
					case 'n': if ((argv[i])[2] == 'a') { AVPCL::flag_nonuniform_ati = true; AVPCL::flag_nonuniform = false; }
							  else { AVPCL::flag_nonuniform = true; AVPCL::flag_nonuniform_ati = false; }
							  break;
					case 'e': noerrfile = false; break;
					default:  throw "bad flag arg";
				}
			else
			{
				if (nargs > 1) throw "Incorrect number of args";
				args[nargs++] = argv[i];
			}

		if (nargs != 2) throw "Incorrect number of args";

		string inf(args[0]), outroot(args[1]);

		if (ext(outroot, ""))
		{
			if (ext(inf, ".tga"))
			{
				int width, height;

				Targa::fileinfo(inf, width, height, AVPCL::mode_rgb);

				string outf, avpclf, errf;
				outf = outroot + "-avpcl.tga";
				avpclf = outroot + "-" + toString(width) + "-" + toString(height) + "-" + (AVPCL::mode_rgb ? "RGB" : "RGBA") + ".avpcl";
				cout << "Compressing " << (AVPCL::mode_rgb ? "RGB file " : "RGBA file ") << inf << " to " << avpclf << endl;
				if (!noerrfile)
				{
					errf = outroot + "-errors" + ".bin";
					cout << "Errors output file is " << errf << endl;
				}
				else
					errf = "";
				AVPCL::compress(inf, avpclf, errf);
				cout << "Decompressing " << avpclf << " to " << outf << endl;
				AVPCL::decompress(avpclf, outf);
				analyze(inf, outf);
			}
			else if (ext(inf, ".avpcl"))
			{
				string outf;
				outf = outroot + "-avpcl.tga";
				cout << "Decompressing " << inf << " to " << outf << endl;
				AVPCL::decompress(inf, outf);
			}
			else throw "Invalid file args";
		}
		else if (ext(inf, ".tga") && ext(outroot, ".tga"))
		{
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
