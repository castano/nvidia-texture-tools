
using System;
using System.Runtime.InteropServices;

namespace Nvidia.TextureTools
{
	#region Enums
	
	#region enum Format
	/// <summary>
	/// Compression format.
	/// </summary>
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
	/// <summary>
	/// Quality modes.
	/// </summary>
	enum Quality
	{
		Fastest,
		Normal,
		Production,
		Highest,
	}
	#endregion

	#region enum TextureType
	/// <summary>
	/// Texture types.
	/// </summary>
	enum TextureType
	{
		Texture2D,
		TextureCube,
	}
	#endregion
	
	#endregion


	#region public class InputOptions
	/// <summary>
	/// Input options.
	/// </summary>
	public class InputOptions
	{
		#region Bindings
		[DllImport("nvtt")]
		private extern static IntPtr nvttCreateInputOptions();

		[DllImport("nvtt")]
		private extern static void nvttDestroyInputOptions(IntPtr inputOptions);

		[DllImport("nvtt")]
		private extern static void nvttSetInputOptionsTextureLayout(IntPtr inputOptions, TextureType type, int w, int h, int d);
		
		[DllImport("nvtt")]
		private extern static void nvttResetInputOptionsTextureLayout(IntPtr inputOptions);
		
		[DllImport("nvtt")]
		private extern static bool nvttSetInputOptionsMipmapData(IntPtr inputOptions, IntPtr data, int w, int h, int d, int face, int mipmap);
		#endregion

		internal IntPtr options;
			
		public InputOptions()
		{
			options = nvttCreateInputOptions();
		}
		public ~InputOptions()
		{
			nvttDestroyInputOptions(options);
		}
		
		public void SetTextureLayout(TextureType type, int w, int h, int d)
		{
			nvttSetInputOptionsTextureLayout(options, type, w, h, d);
		}
		public void ResetTextureLayout()
		{
			nvttResetInputOptionsTextureLayout(options);
		}
		
		public void SetMipmapData(IntPtr data, int width, int height, int depth, int face, int mipmap)
		{
			nvttSetInputOptionsMipmapData(options, data, width, height, depth, face, mipmap);
		}
	}
	#endregion

	#region public class CompressionOptions
	/// <summary>
	/// Compression options.
	/// </summary>
	public class CompressionOptions
	{
		#region Bindings
		[DllImport("nvtt")]
		private extern static IntPtr nvttCreateCompressionOptions();

		[DllImport("nvtt")]
		private extern static void nvttDestroyCompressionOptions(IntPtr compressionOptions);

		[DllImport("nvtt")]
		private extern static void nvttSetCompressionOptionsFormat(IntPtr compressionOptions, Format format);
		
		[DllImport("nvtt")]
		private extern static void nvttSetCompressionOptionsQuality(IntPtr compressionOptions, Quality quality);
		
		[DllImport("nvtt")]
		private extern static void nvttSetCompressionOptionsPixelFormat(IntPtr compressionOptions, uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);
		#endregion
		
		internal IntPtr options;
		
		public CompressionOptions()
		{
			options = nvttCreateCompressionOptions();
		}
		public ~CompressionOptions()
		{
			nvttDestroyCompressionOptions(options);
		}
		
		public void SetFormat(Format format)
		{
			nvttSetCompressionOptionsFormat(options, format);
		}
		public void SetQuality(Quality quality)
		{
			nvttSetCompressionOptionsQuality(options, quality);
		}
		public void SetPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
		{
			nvttSetCompressionOptionsPixelFormat(options, bitcount, rmask, gmask, bmask, amask);
		}
	}
	#endregion

	#region public class OutputOptions
	/// <summary>
	/// Output options.
	/// </summary>
	public class OutputOptions
	{
		#region Bindings
		[DllImport("nvtt")]
		private extern static IntPtr nvttCreateOutputOptions();

		[DllImport("nvtt")]
		private extern static void nvttDestroyOutputOptions(IntPtr outputOptions);

		[DllImport("nvtt", CharSet = CharSet.Ansi)]
		private extern static void nvttSetOutputOptionsFileName(IntPtr outputOptions, string fileName);
		
		#endregion

		internal IntPtr options;
		
		public OutputOptions()
		{
			options = nvttCreateOutputOptions();
		}
		public ~OutputOptions()
		{
			nvttDestroyOutputOptions(options);
		}
		
		public void SetFileName(string fileName)
		{
			nvttSetOutputOptionsFileName(options, fileName);
		}
	}
	#endregion

	#region public static class Compressor
	public static class Compressor
	{
		#region Bindings
		[DllImport("nvtt")]
		private extern static bool nvttCompress(IntPtr inputOptions, IntPtr compressionOptions, IntPtr outputOptions);

		[DllImport("nvtt")]
		private extern static void nvttEstimateSize(IntPtr inputOptions, IntPtr compressionOptions);

		#endregion
		
		public bool Compress(InputOptions inputOptions, CompressionOptions compressionOptions, OutputOptions outputOptions)
		{
			nvttCompress(inputOptions.options, compressionOptions.options, outputOptions.options);
		}
		
		public bool EstimateSize(InputOptions inputOptions, CompressionOptions compressionOptions)
		{
			nvttEstimateSize(inputOptions.options, compressionOptions.options);
		}
	}
	#endregion
		
} // Nvidia.TextureTools namespace