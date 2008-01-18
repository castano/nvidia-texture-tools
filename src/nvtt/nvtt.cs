

namespace nvtt
{
	#region Enums
	
	#region enum Format
	enum Format
	{
		// No compression.
		RGB,
		RGBA = RGB,

		// DX9 formats.
		DXT1,
		DXT1a,
		DXT3,
		DXT5,
		DXT5n,
		
		// DX10 formats.
		BC1 = DXT1,
		BC1a = DXT1a,
		BC2 = DXT3,
		BC3 = DXT5,
		BC3n = DXT5n,
		BC4,
		BC5,
	}
	#endregion

	#region enum Quality
	/// Quality modes.
	enum Quality
	{
		Fastest,
		Normal,
		Production,
		Highest,
	}
	#endregion

	#region enum TextureType
	/// Texture types.
	enum TextureType
	{
		Texture2D,
		TextureCube,
	}
	#endregion
	
	#endregion

	#region private static class Bindings
	private static class Bindings
	{
		// Input Options
		//NVTT_API NvttInputOptions nvttCreateInputOptions();
		//NVTT_API void nvttDestroyInputOptions(NvttInputOptions inputOptions);

		//NVTT_API void nvttSetInputOptionsTextureLayout(NvttInputOptions inputOptions, NvttTextureType type, int w, int h, int d);
		//NVTT_API void nvttResetInputOptionsTextureLayout(NvttInputOptions inputOptions);
		//NVTT_API NvttBoolean nvttSetInputOptionsMipmapData(NvttInputOptions inputOptions, const void * data, int w, int h, int d, int face, int mipmap);


		// Compression Options
		//NVTT_API NvttCompressionOptions nvttCreateCompressionOptions();
		//NVTT_API void nvttDestroyCompressionOptions(NvttCompressionOptions compressionOptions);

		//NVTT_API void nvttSetCompressionOptionsFormat(NvttCompressionOptions compressionOptions, NvttFormat format);
		//NVTT_API void nvttSetCompressionOptionsQuality(NvttCompressionOptions compressionOptions, NvttQuality quality);
		//NVTT_API void nvttSetCompressionOptionsPixelFormat(NvttCompressionOptions compressionOptions, unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask);


		// Output Options
		//NVTT_API NvttOutputOptions nvttCreateOutputOptions();
		//NVTT_API void nvttDestroyOutputOptions(NvttOutputOptions outputOptions);

		//NVTT_API void nvttSetOutputOptionsFileName(NvttOutputOptions outputOptions, const char * fileName);


		// Main entrypoint of the compression library.
		//NVTT_API NvttBoolean nvttCompress(NvttInputOptions inputOptions, NvttOutputOptions outputOptions, NvttCompressionOptions compressionOptions);

		// Estimate the size of compressing the input with the given options.
		//NVTT_API int nvttEstimateSize(NvttInputOptions inputOptions, NvttCompressionOptions compressionOptions);
	}
	#endregion
	
	#region public class InputOptions
	public class InputOptions
	{
		public InputOptions()
		{
			options = Bindings.CreateInputOptions();
		}
		public ~InputOptions()
		{
			Bindings.DestroyInputOptions(options);
		}
		
		public void SetTextureLayout(TextureType type, int w, int h, int d)
		{
			Bindings.InputOptions_SetTextureLayout(options, type, w, h, d);
		}
		public void ResetTextureLayout()
		{
			Bindings.InputOptions_ResetTextureLayout(options);
		}
		
		public void SetMipmapData(Image img, int face, int mipmap)
		{
			// TODO
			//Bindings.InputOptions_SetMipmapData(options, img.Data, img.Width, img.Height, 1, face, mipmap);
		}
	
		private IntPtr options;
	}
	#endregion

	#region public class CompressionOptions
	public class CompressionOptions
	{
		public CompressionOptions()
		{
			options = Bindings.CreateCompressionOptions();
		}
		public ~CompressionOptions()
		{
			Bindings.DestroyCompressionOptions(options);
		}
		
		public void SetFormat(Format format)
		{
			Bindings.CompressionOptions_SetFormat(format);
		}
		public void SetQuality(Quality quality)
		{
			Bindings.CompressionOptions_SetQuality(quality);
		}
		public void SetPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
		{
			Bindings.CompressionOptions_SetPixelFormat(bitcount, rmask, gmask, bmask, amask);
		}
		
		private IntPtr options;
	}
	#endregion

	#region public class OutputOptions
	public class OutputOptions
	{
		public OutputOptions()
		{
			options = Bindings.CreateOutputOptions();
		}
		public ~OutputOptions()
		{
			Bindings.DestroyOutputOptions(options);
		}
		
		public void SetFileName(string fileName)
		{
			// TODO
			//Bindings.OutputOptions_SetFileName(fileName)
		}
		
		private IntPtr options;
	}
	#endregion

	#region public static class Compressor
	public static class Compressor
	{
		public bool Compress(InputOptions inputOptions, CompressionOptions compressionOptions, OutputOptions outputOptions)
		{
			Bindings.Compress(inputOptions.options, compressionOptions.options, outputOptions.options);
		}
		
		public bool EstimateSize(InputOptions inputOptions, CompressionOptions compressionOptions)
		{
			Bindings.EstimateSize(inputOptions.options, compressionOptions.options);
		}
	}
	#endregion
}
