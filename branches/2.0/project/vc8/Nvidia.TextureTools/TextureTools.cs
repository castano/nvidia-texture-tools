using System;
using System.Security;
using System.Runtime.InteropServices;

namespace Nvidia.TextureTools
{
	#region Enums

	#region public enum Format
	/// <summary>
	/// Compression format.
	/// </summary>
	public enum Format
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

	#region public enum Quality
	/// <summary>
	/// Quality modes.
	/// </summary>
	public enum Quality
	{
		Fastest,
		Normal,
		Production,
		Highest,
	}
	#endregion

	#region public enum WrapMode
	/// <summary>
	/// Wrap modes.
	/// </summary>
	public enum WrapMode
	{
		Clamp,
		Repeat,
		Mirror,
	}
	#endregion

	#region public enum TextureType
	/// <summary>
	/// Texture types.
	/// </summary>
	public enum TextureType
	{
		Texture2D,
		TextureCube,
	}
	#endregion

	#region public enum InputFormat
	/// <summary>
	/// Input formats.
	/// </summary>
	public enum InputFormat
	{
		BGRA_8UB
	}
	#endregion

	#region public enum MipmapFilter
	/// <summary>
	/// Mipmap downsampling filters.
	/// </summary>
	public enum MipmapFilter
	{
		Box,
		Triangle,
		Kaiser
	}
	#endregion

	#region public enum ColorTransform
	/// <summary>
	/// Color transformation.
	/// </summary>
	public enum ColorTransform
	{
		None,
		Linear
	}
	#endregion

	#region public enum RoundMode
	/// <summary>
	/// Extents rounding mode.
	/// </summary>
	public enum RoundMode
	{
		None,
		ToNextPowerOfTwo,
		ToNearestPowerOfTwo,
		ToPreviousPowerOfTwo
	}
	#endregion

	#region public enum AlphaMode
	/// <summary>
	/// Alpha mode.
	/// </summary>
	public enum AlphaMode
	{
		None,
		Transparency,
		Premultiplied
	}
	#endregion

	#region public enum Error
	/// <summary>
	/// Error codes.
	/// </summary>
	public enum Error
	{
        Unknown,
		InvalidInput,
		
        UnsupportedFeature,
		CudaError,
		
        FileOpen,
		FileWrite,
	}
	#endregion


    #endregion
    
    #region Exception Class

    public class TextureToolsException : ApplicationException
    {
        Error errorCode = Error.Unknown;

        public Error ErrorCode
        {
            get { return errorCode; }
        }

        public TextureToolsException(Error errorCode) : base(Compressor.ErrorString(errorCode))
        {
            this.errorCode = errorCode;
        }
    }

    #endregion


    #region public class InputOptions
    /// <summary>
	/// Input options.
	/// </summary>
	public class InputOptions : IDisposable
	{
		#region Bindings
		[DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateInputOptions();

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyInputOptions(IntPtr inputOptions);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsTextureLayout(IntPtr inputOptions, TextureType type, int w, int h, int d);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttResetInputOptionsTextureLayout(IntPtr inputOptions);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static bool nvttSetInputOptionsMipmapData(IntPtr inputOptions, IntPtr data, int w, int h, int d, int face, int mipmap);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsFormat(IntPtr inputOptions, InputFormat format);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsAlphaMode(IntPtr inputOptions, AlphaMode alphaMode);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsGamma(IntPtr inputOptions, float inputGamma, float outputGamma);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsWrapMode(IntPtr inputOptions, WrapMode mode);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsMipmapFilter(IntPtr inputOptions, MipmapFilter filter);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsMipmapGeneration(IntPtr inputOptions, bool generateMipmaps, int maxLevel);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsKaiserParameters(IntPtr inputOptions, float width, float alpha, float stretch);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsNormalMap(IntPtr inputOptions, bool b);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsConvertToNormalMap(IntPtr inputOptions, bool convert);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsHeightEvaluation(IntPtr inputOptions, float redScale, float greenScale, float blueScale, float alphaScale);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsNormalFilter(IntPtr inputOptions, float small, float medium, float big, float large);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsNormalizeMipmaps(IntPtr inputOptions, bool b);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsColorTransform(IntPtr inputOptions, ColorTransform t);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsLinearTransfrom(IntPtr inputOptions, int channel, float w0, float w1, float w2, float w3);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsMaxExtents(IntPtr inputOptions, int d);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsRoundMode(IntPtr inputOptions, RoundMode mode);
		#endregion

		internal IntPtr options;

		public InputOptions()
		{
			options = nvttCreateInputOptions();
		}


		public void SetTextureLayout(TextureType type, int w, int h, int d)
		{
			nvttSetInputOptionsTextureLayout(options, type, w, h, d);
		}
		public void ResetTextureLayout()
		{
			nvttResetInputOptionsTextureLayout(options);
		}

        public void SetMipmapData(byte[] data, int width, int height, int depth, int face, int mipmap)
        {
            //unsafe() would be cleaner, but that would require compiling with the unsafe compiler option....
            GCHandle gcHandle = GCHandle.Alloc(data, GCHandleType.Pinned);

            try
            {
                SetMipmapData(gcHandle.AddrOfPinnedObject(), width, height, depth, face, mipmap);
            }
            finally
            {
                gcHandle.Free();
            }

        }

		public void SetMipmapData(IntPtr data, int width, int height, int depth, int face, int mipmap)
		{
			nvttSetInputOptionsMipmapData(options, data, width, height, depth, face, mipmap);
		}

		public void SetFormat(InputFormat format)
		{
			nvttSetInputOptionsFormat(options, format);
		}

		public void SetAlphaMode(AlphaMode alphaMode)
		{
			nvttSetInputOptionsAlphaMode(options, alphaMode);
		}

		public void SetGamma(float inputGamma, float outputGamma)
		{
			nvttSetInputOptionsGamma(options, inputGamma, outputGamma);
		}

		public void SetWrapMode(WrapMode wrapMode)
		{
			nvttSetInputOptionsWrapMode(options, wrapMode);
		}

		public void SetMipmapFilter(MipmapFilter filter)
		{
			nvttSetInputOptionsMipmapFilter(options, filter);
		}

		public void SetMipmapGeneration(bool enabled)
		{
			nvttSetInputOptionsMipmapGeneration(options, enabled, -1);
		}

		public void SetMipmapGeneration(bool enabled, int maxLevel)
		{
			nvttSetInputOptionsMipmapGeneration(options, enabled, maxLevel);
		}

		public void SetKaiserParameters(float width, float alpha, float stretch)
		{
			nvttSetInputOptionsKaiserParameters(options, width, alpha, stretch);
		}

		public void SetNormalMap(bool b)
		{
			nvttSetInputOptionsNormalMap(options, b);
		}

		public void SetConvertToNormalMap(bool convert)
		{
			nvttSetInputOptionsConvertToNormalMap(options, convert);
		}

		public void SetHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale)
		{
			nvttSetInputOptionsHeightEvaluation(options, redScale, greenScale, blueScale, alphaScale);
		}

		public void SetNormalFilter(float small, float medium, float big, float large)
		{
			nvttSetInputOptionsNormalFilter(options, small, medium, big, large);
		}

		public void SetNormalizeMipmaps(bool b)
		{
			nvttSetInputOptionsNormalizeMipmaps(options, b);
		}

		public void SetColorTransform(ColorTransform t)
		{
			nvttSetInputOptionsColorTransform(options, t);
		}

		public void SetLinearTransfrom(int channel, float w0, float w1, float w2, float w3)
		{
			nvttSetInputOptionsLinearTransfrom(options, channel, w0, w1, w2, w3);
		}

		public void SetMaxExtents(int dim)
		{
			nvttSetInputOptionsMaxExtents(options, dim);
		}

		public void SetRoundMode(RoundMode mode)
		{
			nvttSetInputOptionsRoundMode(options, mode);
		}

        #region IDisposable Members

        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (options != IntPtr.Zero)
            {
                nvttDestroyInputOptions(options);
                options = IntPtr.Zero;
            }
        }

        ~InputOptions()
        {
            Dispose(false);
        }

        #endregion
    }
	#endregion

	#region public class CompressionOptions
	/// <summary>
	/// Compression options.
	/// </summary>
	public class CompressionOptions : IDisposable
	{
		#region Bindings
        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateCompressionOptions();

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyCompressionOptions(IntPtr compressionOptions);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsFormat(IntPtr compressionOptions, Format format);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsQuality(IntPtr compressionOptions, Quality quality);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsColorWeights(IntPtr compressionOptions, float red, float green, float blue, float alpha);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsPixelFormat(IntPtr compressionOptions, uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsQuantization(IntPtr compressionOptions, bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold);

		#endregion

		internal IntPtr options;

		public CompressionOptions()
		{
			options = nvttCreateCompressionOptions();
		}

		public void SetFormat(Format format)
		{
			nvttSetCompressionOptionsFormat(options, format);
		}
		
		public void SetQuality(Quality quality)
		{
			nvttSetCompressionOptionsQuality(options, quality);
		}

		public void SetColorWeights(float red, float green, float blue)
		{
			nvttSetCompressionOptionsColorWeights(options, red, green, blue, 1.0f);
		}

		public void SetColorWeights(float red, float green, float blue, float alpha)
		{
			nvttSetCompressionOptionsColorWeights(options, red, green, blue, alpha);
		}

		public void SetPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
		{
			nvttSetCompressionOptionsPixelFormat(options, bitcount, rmask, gmask, bmask, amask);
		}

		public void SetQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha)
		{
			nvttSetCompressionOptionsQuantization(options, colorDithering, alphaDithering, binaryAlpha, 127);
		}

		public void SetQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold)
		{
			nvttSetCompressionOptionsQuantization(options, colorDithering, alphaDithering, binaryAlpha, alphaThreshold);
		}


        #region IDisposable Members

        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (options != IntPtr.Zero)
            {
                nvttDestroyCompressionOptions(options);
                options = IntPtr.Zero;
            }
        }

        ~CompressionOptions()
        {
            Dispose(false);
        }

        #endregion
    }
	#endregion

	#region public class OutputOptions

    public interface IOutputHandler    
    {
        void BeginImage(int size, int width, int height, int depth, int face, int miplevel);
        bool WriteDataUnsafe(IntPtr data, int size);
    }

    /* 
     * This class provides a nicer interface for the output handler by taking care of the marshalling of the image data.
     * However the IOutputHandler interface is still provided to allow the user to do this themselves to avoid the
     * additional copying and memory allocations.
     */
    public abstract class OutputHandlerBase : IOutputHandler
    {
        private byte[] tempData;

        protected OutputHandlerBase()
        {
        }

        protected abstract void BeginImage(int size, int width, int height, int depth, int face, int miplevel);
        protected abstract void WriteData(byte[] dataBuffer, int startIndex, int count);

        #region IOutputHandler Members

        void IOutputHandler.BeginImage(int size, int width, int height, int depth, int face, int miplevel)
        {
            BeginImage(size, width, height, depth, face, miplevel);
        }

        bool IOutputHandler.WriteDataUnsafe(IntPtr data, int size)
        {
            //TODO: Exception handling and return an error result?

            if ((tempData == null) || (size > tempData.Length))
                tempData = new byte[size];

            Marshal.Copy(data, tempData, 0, size);

            // Zero additional buffer elements to to aid reproducability of bugs.
            Array.Clear(tempData, size, tempData.Length - size);

            WriteData(tempData, 0, size);

            return true;
        }

        #endregion
    }

	/// <summary>
	/// Output options.
	/// </summary>
	public class OutputOptions : IDisposable
	{
        #region Delegates

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void InternalErrorHandlerDelegate(Error error);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate bool WriteDataDelegate(IntPtr data, int size);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void ImageDelegate(int size, int width, int height, int depth, int face, int miplevel);

		#endregion

        private Error lastErrorCode = Error.Unknown;

        internal Error LastErrorCode
        {
            get { return lastErrorCode; }
        }

        #region Bindings
        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateOutputOptions();

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyOutputOptions(IntPtr outputOptions);

        [DllImport("nvtt", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetOutputOptionsFileName(IntPtr outputOptions, string fileName);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetOutputOptionsErrorHandler(IntPtr outputOptions, InternalErrorHandlerDelegate errorHandler);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetOutputOptionsOutputHeader(IntPtr outputOptions, bool b);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetOutputOptionsOutputHandler(IntPtr outputOptions, IntPtr writeData, IntPtr image);

		#endregion

		internal IntPtr options;
        
        //Note: these references are used to prevent garbage collection of the delegates(and hence class) if they are
        //only referenced from unmanaged land.
        private WriteDataDelegate writeDataDelegate;
        private ImageDelegate beginImageDelegate;

        private IOutputHandler currentOutputHandler;

		public OutputOptions()
		{
			options = nvttCreateOutputOptions();
			nvttSetOutputOptionsErrorHandler(options, ErrorCallback);
		}


		public void SetFileName(string fileName)
		{
			nvttSetOutputOptionsFileName(options, fileName);
		}
        
		public void SetOutputHeader(bool b)
		{
			nvttSetOutputOptionsOutputHeader(options, b);
		}

        public void SetOutputHandler(IOutputHandler outputHandler)
        {
            if (outputHandler != null)
            {
                //We need to store a ref in order to prevent garbage collection.
                WriteDataDelegate tmpWriteDataDelegate = new WriteDataDelegate(WriteDataCallback);
                ImageDelegate tmpBeginImageDelegate = new ImageDelegate(ImageCallback);

                IntPtr ptrWriteData = Marshal.GetFunctionPointerForDelegate(tmpWriteDataDelegate);
                IntPtr ptrBeginImage = Marshal.GetFunctionPointerForDelegate(tmpBeginImageDelegate);


                nvttSetOutputOptionsOutputHandler(options, ptrWriteData, ptrBeginImage);

                writeDataDelegate = tmpWriteDataDelegate;
                beginImageDelegate = tmpBeginImageDelegate;

                currentOutputHandler = outputHandler;
            }
            else
            {
                nvttSetOutputOptionsOutputHandler(options, IntPtr.Zero, IntPtr.Zero);

                writeDataDelegate = null;
                beginImageDelegate = null;

                currentOutputHandler = null;
            }
        }

        private void ErrorCallback(Error error)
        {
            lastErrorCode = error;
        }

        private bool WriteDataCallback(IntPtr data, int size)
        {
            if (currentOutputHandler != null)
                return currentOutputHandler.WriteDataUnsafe(data, size);
            else
                return true;
        }

        private void ImageCallback(int size, int width, int height, int depth, int face, int miplevel)
        {
            if (currentOutputHandler != null) currentOutputHandler.BeginImage(size, width, height, depth, face, miplevel);
        }

        #region IDisposable Members

        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (options != IntPtr.Zero)
            {
                nvttDestroyOutputOptions(options);

                options = IntPtr.Zero;
            }

            writeDataDelegate = null;
            beginImageDelegate = null;
            currentOutputHandler = null;
        }

        ~OutputOptions()
        {
            Dispose(false);
        }

        #endregion
    }
	#endregion

	#region public class Compressor
	public class Compressor : IDisposable
	{
		#region Bindings
        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateCompressor();

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyCompressor(IntPtr compressor);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static bool nvttCompress(IntPtr compressor, IntPtr inputOptions, IntPtr compressionOptions, IntPtr outputOptions);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private extern static int nvttEstimateSize(IntPtr compressor, IntPtr inputOptions, IntPtr compressionOptions);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
		private static extern IntPtr nvttErrorString(Error error);

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
        private extern static uint nvttVersion();

        [DllImport("nvtt", CallingConvention = CallingConvention.Cdecl), SuppressUnmanagedCodeSecurity]
        private extern static void nvttEnableCudaCompression(IntPtr compressor, bool enable);

		#endregion

		internal IntPtr compressor;

		public Compressor()
		{
			compressor = nvttCreateCompressor();
		}
        
		public void Compress(InputOptions input, CompressionOptions compression, OutputOptions output)
		{
            if (!nvttCompress(compressor, input.options, compression.options, output.options))
            {
                //An error occured, use the last error registered.
                throw new TextureToolsException(output.LastErrorCode);
            }
		}

		public int EstimateSize(InputOptions input, CompressionOptions compression)
		{
			return nvttEstimateSize(compressor, input.options, compression.options);
		}

		public static string ErrorString(Error error)
		{
			return Marshal.PtrToStringAnsi(nvttErrorString(error));
		}

        public static uint Version()
        {
            return nvttVersion();
        }

        public void SetEnableCuda(bool enableCuda)
        {
            nvttEnableCudaCompression(compressor, enableCuda);
        }

        #region IDisposable Members

        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (compressor != IntPtr.Zero)
            {
                nvttDestroyCompressor(compressor);
                compressor = IntPtr.Zero;
            }
        }

        ~Compressor()
        {
            Dispose(false);
        }

        #endregion
    }
	#endregion

    #region public class CompressorOptionsBundle

    /*
     * We provide a class which combines all the objects, this simplifies usage such as:
     * 
     * using(CompressorOptionsBundle compressor = new CompressorOptionsBundle())
     * {
     *     compressor.InputOptions.SetMipmapData(...);
     *     ...
     * }
     * 
     * Making it easy to write exception safe code etc.
     */
    public class CompressorOptionsBundle : IDisposable
    {
        InputOptions inputOptions;
        CompressionOptions compressionOptions;
        OutputOptions outputOptions;
        
        Compressor compressor;

        public InputOptions InputOptions
        {
            get { return inputOptions; }
        }


        public CompressionOptions CompressionOptions
        {
            get { return compressionOptions; }
        }

        public OutputOptions OutputOptions
        {
            get { return outputOptions; }
        }

        public Compressor Compressor
        {
            get { return compressor; }
        }

        public CompressorOptionsBundle()
        {
            inputOptions = new InputOptions();
            compressionOptions = new CompressionOptions();
            outputOptions = new OutputOptions();
            compressor = new Compressor();
        }

        public void Compress()
        {
            compressor.Compress(inputOptions, compressionOptions, outputOptions);
        }
        
        #region IDisposable Members

        public void Dispose()
        {
            Dispose(true);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (inputOptions != null)
                {
                    inputOptions.Dispose();
                    inputOptions = null;
                }

                if (compressionOptions != null)
                {
                    compressionOptions.Dispose();
                    compressionOptions = null;
                }

                if (outputOptions != null)
                {
                    outputOptions.Dispose();
                    outputOptions = null;
                }

                if (compressor != null)
                {
                    compressor.Dispose();
                    compressor = null;
                }
            }
        }
        #endregion

    }

    #endregion

} // Nvidia.TextureTools namespace
