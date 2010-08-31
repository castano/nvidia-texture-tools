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

#include <nvtt/nvtt.h>
#include <nvimage/Image.h>
#include <nvimage/ImageIO.h>
#include <nvimage/BlockDXT.h>
#include <nvimage/ColorBlock.h>
#include <nvcore/Ptr.h>
#include <nvcore/Debug.h>
#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>
#include <nvcore/TextWriter.h>
#include <nvcore/FileSystem.h>
#include <nvcore/Timer.h>

#include <stdlib.h> // free
#include <string.h> // memcpy


using namespace nv;

// Kodak image set
static const char * s_kodakImageSet[] = {
	"kodim01.png",
	"kodim02.png",
	"kodim03.png",
	"kodim04.png",
	"kodim05.png",
	"kodim06.png",
	"kodim07.png",
	"kodim08.png",
	"kodim09.png",
	"kodim10.png",
	"kodim11.png",
	"kodim12.png",
	"kodim13.png",
	"kodim14.png",
	"kodim15.png",
	"kodim16.png",
	"kodim17.png",
	"kodim18.png",
	"kodim19.png",
	"kodim20.png",
	"kodim21.png",
	"kodim22.png",
	"kodim23.png",
	"kodim24.png",
};

// Waterloo image set
static const char * s_waterlooImageSet[] = {
	"clegg.png",
	"frymire.png",
	"lena.png",
	"monarch.png",
	"peppers.png",
	"sail.png",
	"serrano.png",
	"tulips.png",
};

// Epic image set
static const char * s_epicImageSet[] = {
	"Bradley1.png",
	"Gradient.png",
	"MoreRocks.png",
	"Wall.png",
	"Rainbow.png",
	"Text.png",
};

// Farbrausch
static const char * s_farbrauschImageSet[] = {
	"t.2d.pn02.bmp",
	"t.aircondition.01.bmp",
	"t.bricks.02.bmp",
	"t.bricks.05.bmp",
	"t.concrete.cracked.01.bmp",
	"t.envi.colored02.bmp",
	"t.envi.colored03.bmp",
	"t.font.01.bmp",
	"t.sewers.01.bmp",
	"t.train.03.bmp",
	"t.yello.01.bmp",
};

// Lugaru
static const char * s_lugaruImageSet[] = {
	"lugaru-blood.png",
	"lugaru-bush.png",
	"lugaru-cursor.png",
	"lugaru-hawk.png",
};

// Quake3
static const char * s_quake3ImageSet[] = {
	"q3-blocks15cgeomtrn.tga",
	"q3-blocks17bloody.tga",
	"q3-dark_tin2.tga",
	"q3-fan_grate.tga",
	"q3-fan.tga",
	"q3-metal2_2.tga",
	"q3-panel_glo.tga",
	"q3-proto_fence.tga",
	"q3-wires02.tga",
};


struct ImageSet
{
	const char ** fileNames;
	int fileCount;
	nvtt::Format format;
};

static ImageSet s_imageSets[] = {
	{s_kodakImageSet, sizeof(s_kodakImageSet)/sizeof(s_kodakImageSet[0]), nvtt::Format_DXT1},
	{s_waterlooImageSet, sizeof(s_waterlooImageSet)/sizeof(s_waterlooImageSet[0]), nvtt::Format_DXT1},
	{s_epicImageSet, sizeof(s_epicImageSet)/sizeof(s_epicImageSet[0]), nvtt::Format_DXT1},
	{s_farbrauschImageSet, sizeof(s_farbrauschImageSet)/sizeof(s_farbrauschImageSet[0]), nvtt::Format_DXT1},
	{s_lugaruImageSet, sizeof(s_lugaruImageSet)/sizeof(s_lugaruImageSet[0]), nvtt::Format_DXT5},
	{s_quake3ImageSet, sizeof(s_quake3ImageSet)/sizeof(s_quake3ImageSet[0]), nvtt::Format_DXT5},
};
const int s_imageSetCount = sizeof(s_imageSets)/sizeof(s_imageSets[0]);

enum Decoder
{
	Decoder_Reference,
	Decoder_NVIDIA,
};

struct MyOutputHandler : public nvtt::OutputHandler
{
	MyOutputHandler() : m_data(NULL), m_ptr(NULL) {}
	~MyOutputHandler()
	{
		free(m_data);
	}

	virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
	{
		m_size = size;
		m_width = width;
		m_height = height;
		free(m_data);
		m_data = (unsigned char *)malloc(size);
		m_ptr = m_data;
	}
	
	virtual bool writeData(const void * data, int size)
	{
		memcpy(m_ptr, data, size);
		m_ptr += size;
		return true;
	}

	Image * decompress(nvtt::Format format, Decoder decoder)
	{
		int bw = (m_width + 3) / 4;
		int bh = (m_height + 3) / 4;

		AutoPtr<Image> img( new Image() );
		img->allocate(m_width, m_height);

		if (format == nvtt::Format_BC1)
		{
			BlockDXT1 * block = (BlockDXT1 *)m_data;

			for (int y = 0; y < bh; y++)
			{
				for (int x = 0; x < bw; x++)
				{
					ColorBlock colors;
					if (decoder == Decoder_Reference) {
						block->decodeBlock(&colors);
					}
					else if (decoder == Decoder_NVIDIA) {
						block->decodeBlockNV5x(&colors);
					}

					for (int yy = 0; yy < 4; yy++)
					{
						for (int xx = 0; xx < 4; xx++)
						{
							Color32 c = colors.color(xx, yy);

							if (x * 4 + xx < m_width && y * 4 + yy < m_height)
							{
								img->pixel(x * 4 + xx, y * 4 + yy) = c;
							}
						}
					}

					block++;
				}
			}
		}
		else if (format == nvtt::Format_BC3)
		{
			BlockDXT5 * block = (BlockDXT5 *)m_data;

			for (int y = 0; y < bh; y++)
			{
				for (int x = 0; x < bw; x++)
				{
					ColorBlock colors;
					if (decoder == Decoder_Reference) {
						block->decodeBlock(&colors);
					}
					else if (decoder == Decoder_NVIDIA) {
						block->decodeBlockNV5x(&colors);
					}

					for (int yy = 0; yy < 4; yy++)
					{
						for (int xx = 0; xx < 4; xx++)
						{
							Color32 c = colors.color(xx, yy);

							if (x * 4 + xx < m_width && y * 4 + yy < m_height)
							{
								img->pixel(x * 4 + xx, y * 4 + yy) = c;
							}
						}
					}

					block++;
				}
			}
		}


		return img.release();
	}

	int m_size;
	int m_width;
	int m_height;
	unsigned char * m_data;
	unsigned char * m_ptr;
};


float rmsError(const Image * a, const Image * b)
{
	nvCheck(a != NULL);
	nvCheck(b != NULL);
	nvCheck(a->width() == b->width());
	nvCheck(a->height() == b->height());

	double mse = 0;

	const uint count = a->width() * a->height();

	for (uint i = 0; i < count; i++)
	{
		Color32 c0 = a->pixel(i);
		Color32 c1 = b->pixel(i);

		int r = c0.r - c1.r;
		int g = c0.g - c1.g;
		int b = c0.b - c1.b;
		int a = c0.a - c1.a;

		mse += double(r * r * c0.a) / 255;
		mse += double(g * g * c0.a) / 255;
		mse += double(b * b * c0.a) / 255;
	}

	return float(sqrt(mse / count));
}


int main(int argc, char *argv[])
{
	const uint version = nvtt::version();
	const uint major = version / 100;
	const uint minor = version % 100;
	
	printf("NVIDIA Texture Tools %u.%u - Copyright NVIDIA Corporation 2007 - 2008\n\n", major, minor);
	
	int set = 0;
	bool fast = false;
	bool nocuda = false;
	bool showHelp = false;
	Decoder decoder = Decoder_Reference;
	const char * basePath = "";
	const char * outPath = "output";
	const char * regressPath = NULL;
	
	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		if (strcmp("-set", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				set = atoi(argv[i+1]);
				i++;
			}
		}
		else if (strcmp("-dec", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				decoder = (Decoder)atoi(argv[i+1]);
				i++;
			}
		}
		else if (strcmp("-fast", argv[i]) == 0)
		{
			fast = true;
		}
		else if (strcmp("-nocuda", argv[i]) == 0)
		{
			nocuda = true;
		}
		else if (strcmp("-help", argv[i]) == 0)
		{
			showHelp = true;
		}
		else if (strcmp("-path", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				basePath = argv[i+1];
				i++;
			}
		}
		else if (strcmp("-out", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				outPath = argv[i+1];
				i++;
			}
		}
		else if (strcmp("-regress", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				regressPath = argv[i+1];
				i++;
			}
		}
	}

	if (showHelp)
	{
		printf("usage: nvtestsuite [options]\n\n");
		
		printf("Input options:\n");
		printf("  -path <path>   \tInput image path.\n");
		printf("  -regress <path>\tRegression directory.\n");
		printf("  -set [0:2]     \tImage set.\n");
		printf("    0:           \tKodak.\n");
		printf("    1:           \tWaterloo.\n");
		printf("    2:           \tEpic.\n");
		printf("    3:           \tFarbrausch.\n");
		printf("  -dec x         \tDecompressor.\n");
		printf("    0:           \tReference.\n");
		printf("    1:           \tNVIDIA.\n");

		printf("Compression options:\n");
		printf("  -fast          \tFast compression.\n");
		printf("  -nocuda        \tDo not use cuda compressor.\n");
		
		printf("Output options:\n");
		printf("  -out <path>    \tOutput directory.\n");

		return 1;
	}
	
	nvtt::InputOptions inputOptions;
	inputOptions.setMipmapGeneration(false);
	inputOptions.setAlphaMode(nvtt::AlphaMode_Transparency);

	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setFormat(nvtt::Format_BC1);
	if (fast)
	{
		compressionOptions.setQuality(nvtt::Quality_Fastest);
	}
	else
	{
		compressionOptions.setQuality(nvtt::Quality_Production);
	}
	//compressionOptions.setExternalCompressor("ati");
	//compressionOptions.setExternalCompressor("squish");
	//compressionOptions.setExternalCompressor("d3dx");
	//compressionOptions.setExternalCompressor("stb");

	compressionOptions.setFormat(s_imageSets[set].format);


	nvtt::OutputOptions outputOptions;
	outputOptions.setOutputHeader(false);

	MyOutputHandler outputHandler;
	outputOptions.setOutputHandler(&outputHandler);

	nvtt::Context context;
	context.enableCudaAcceleration(!nocuda);

	FileSystem::changeDirectory(basePath);
	FileSystem::createDirectory(outPath);

	Path csvFileName;
	csvFileName.format("%s/result.csv", outPath);
	StdOutputStream csvStream(csvFileName.str());
	TextWriter csvWriter(&csvStream);

	float totalTime = 0;
	float totalRMSE = 0;
	int failedTests = 0;
	float totalDiff = 0;

	const char ** fileNames = s_imageSets[set].fileNames;
	int fileCount = s_imageSets[set].fileCount;

	Timer timer;

	for (int i = 0; i < fileCount; i++)
	{
		AutoPtr<Image> img( new Image() );
		
		if (!img->load(fileNames[i]))
		{
			printf("Input image '%s' not found.\n", fileNames[i]);
			return EXIT_FAILURE;
		}

		inputOptions.setTextureLayout(nvtt::TextureType_2D, img->width(), img->height());
		inputOptions.setMipmapData(img->pixels(), img->width(), img->height());

		printf("Compressing: \t'%s'\n", fileNames[i]);

		timer.start();

		context.process(inputOptions, compressionOptions, outputOptions);

		timer.stop();
		printf("  Time: \t%.3f sec\n", float(timer.elapsed()) / 1000);
		totalTime += float(timer.elapsed()) / 1000;

		AutoPtr<Image> img_out( outputHandler.decompress(s_imageSets[set].format, decoder) );

		Path outputFileName;
		outputFileName.format("%s/%s", outPath, fileNames[i]);
		outputFileName.stripExtension();
		outputFileName.append(".png");
		if (!ImageIO::save(outputFileName.str(), img_out.ptr()))
		{
			printf("Error saving file '%s'.\n", outputFileName.str());
		}

		float rmse = rmsError(img.ptr(), img_out.ptr());
		totalRMSE += rmse;

		printf("  RMSE:  \t%.4f\n", rmse);

		// Output csv file
		csvWriter << "\"" << fileNames[i] << "\"," << rmse << "\n";

		if (regressPath != NULL)
		{
			Path regressFileName;
			regressFileName.format("%s/%s", regressPath, fileNames[i]);
			regressFileName.stripExtension();
			regressFileName.append(".png");

			AutoPtr<Image> img_reg( new Image() );
			if (!img_reg->load(regressFileName.str()))
			{
				printf("Regression image '%s' not found.\n", regressFileName.str());
				return EXIT_FAILURE;
			}

			float rmse_reg = rmsError(img.ptr(), img_reg.ptr());

			float diff = rmse_reg - rmse;
			totalDiff += diff;

			const char * text = "PASSED";
			if (equal(diff, 0)) text = "PASSED";
			else if (diff < 0) {
				text = "FAILED";
				failedTests++;
			}

			printf("  Diff: \t%.4f (%s)\n", diff, text);
		}

		fflush(stdout);
	}

	totalRMSE /= fileCount;
	totalDiff /= fileCount;

	printf("Total Results:\n");
	printf("  Total Time: \t%.3f sec\n", totalTime);
	printf("  Average RMSE:\t%.4f\n", totalRMSE);

	if (regressPath != NULL)
	{
		printf("Regression Results:\n");
		printf("  Diff: %.4f\n", totalDiff);
		printf("  %d/%d tests failed.\n", failedTests, fileCount);
	}

	return EXIT_SUCCESS;
}

