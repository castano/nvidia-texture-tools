using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Content.Pipeline;
using Microsoft.Xna.Framework.Content.Pipeline.Graphics;
using Microsoft.Xna.Framework.Content.Pipeline.Processors;
using Nvidia.TextureTools;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.IO;


namespace NvidiaTextureToolsProcessor
{

    [ContentProcessor(DisplayName = "NvidiaTextureProcessor")]
    public class NvidiaTextureProcessor : ContentProcessor<TextureContent, TextureContent>
    {
        // Note: We dont expose all of Texture Tools formats since XNA does not support the custom formats(eg ATI1 / ATI2 etc).
        // Plus we can use some friendlier/more consistant names
        public enum TextureOutputFormat
        {
            Colour,
            Normals,

            DXT1,
            DXT1a, // With 1 bit alpha
            DXT3,
            DXT5,
            DXT5n, // Compressed Normals HILO: R=1, G=y, B=0, A=x 
        }

        bool convertToNormalMap = false;

        bool generateMipmaps = true;
        int maxMipLevel = -1;

        float inputGamma = 2.2f;
        float outputGamma = 2.2f;

        TextureOutputFormat textureFormat = TextureOutputFormat.DXT1;
        Quality quality = Quality.Normal;
        WrapMode wrapMode = WrapMode.Mirror;
        MipmapFilter mipMapFilter = MipmapFilter.Box;
        RoundMode roundMode = RoundMode.None;
        AlphaMode alphaMode = AlphaMode.None;

        bool enableCuda = true;

        [DisplayName("Convert To Normal Map")]
        [DefaultValue(false)]
        [Description("When true the input data is converted from colour/height data into a normal map if the output is a normal map.")]
        [Category("Input")]
        public bool ConvertToNormalMap
        {
            get { return convertToNormalMap; }
            set { convertToNormalMap = value; }
        }

        [DisplayName("Generate Mip Maps")]
        [DefaultValue(true)]
        [Description("When set to true the processor will generate mip maps")]
        [Category("Mip Mapping")]
        public bool GenerateMipmaps
        {
            get { return generateMipmaps; }
            set { generateMipmaps = value; }
        }

        [DisplayName("Max Mip Map Level")]
        [DefaultValue(-1)]
        [Description("Setting the max mip map  level allows partial mip map chains to be generated. -1 generates all levels if mip map generation is enabled.")]
        [Category("Mip Mapping")]
        public int MaxMipMapLevel
        {
            get { return maxMipLevel; }
            set { maxMipLevel = value; }
        }

        [DisplayName("Input Gamma")]
        [DefaultValue(2.2f)]
        [Description("The gamma conversion performed before mip map generation, for best results mip maps should be generated in linear colour space.")]
        [Category("Gamma")]
        public float InputGamma
        {
            get { return inputGamma; }
            set { inputGamma = value; }
        }

        [DisplayName("Output Gamma")]
        [DefaultValue(2.2f)]
        [Description("The gamma conversion applied during output. In general this should be left at 2.2 for LDR images.")]
        [Category("Gamma")]
        public float OutputGamma
        {
            get { return outputGamma; }
            set { outputGamma = value; }
        }

        [DisplayName("Texture Output Format")]
        [DefaultValue(TextureOutputFormat.DXT1)]
        [Description("The format which the processor generates, Color means no compression, DXT1 if useful for textures with no alpha, DXT5 for textures with alpha and DXT5n for normal maps.")]
        [Category("Output")]
        public TextureOutputFormat TextureFormat
        {
            get { return textureFormat; }
            set { textureFormat = value; }
        }

        [DisplayName("Compression Quality")]
        [DefaultValue(Quality.Normal)]
        [Description("Specifies the amount of time the processor will spend trying to find the best quality compression. Highest should only be used for testing as it uses a brute force approach.")]
        [Category("Output")]
        public Quality Quality
        {
            get { return quality; }
            set { quality = value; }
        }

        [DisplayName("Wrap Mode")]
        [DefaultValue(WrapMode.Mirror)]
        [Description("Specifying the wrap mode used for the texture can sometimes improve the quality of filtering. In general Mirror should give good results.")]
        [Category("Input")]
        public WrapMode WrapMode
        {
            get { return wrapMode; }
            set { wrapMode = value; }
        }

        [DisplayName("Mip Map Filter")]
        [DefaultValue(MipmapFilter.Box)]
        [Description("Specifies which filter to use for down sampling mip maps. Box generally gives good results, Triangle will often appear blurry and Kaiser is the slowest but best quality.")]
        [Category("Mip Mapping")]
        public MipmapFilter MipMapFilter
        {
            get { return mipMapFilter; }
            set { mipMapFilter = value; }
        }

        [DisplayName("Texture Size Rounding Mode")]
        [DefaultValue(RoundMode.None)]
        [Description("Setting the rounding mode allows the texture to be resized to a power of 2, often needed for less capable hardware.")]
        [Category("Input")]
        public RoundMode TextureRoundingMode
        {
            get { return roundMode; }
            set { roundMode = value; }
        }

        [DisplayName("Alpha Mode")]
        [DefaultValue(AlphaMode.None)]
        [Description("Setting the alpha mode allows improved quality when generating mip maps.")]
        [Category("Input")]
        public AlphaMode AlphaMode
        {
            get { return alphaMode; }
            set { alphaMode = value; }
        }

        [DisplayName("Enable Cuda")]
        [DefaultValue(true)]
        [Description("When true Cuda will be utilised if available.")]
        [Category("Compressor")]
        public bool EnableCuda
        {
            get { return enableCuda; }
            set { enableCuda = value; }
        }

        public override TextureContent Process(TextureContent input, ContentProcessorContext context)
        {
            //System.Diagnostics.Debugger.Launch();

            input.Validate();

            try
            {

                using (CompressorOptionsBundle compressor = new CompressorOptionsBundle())
                {
                    compressor.InputOptions.ResetTextureLayout();

                    TextureType textureType = FindTextureType(input);

                    /*
                     * Set options
                     */

                    compressor.InputOptions.SetTextureLayout(textureType, input.Faces[0][0].Width, input.Faces[0][0].Height, 1);
                    compressor.InputOptions.SetFormat(InputFormat.BGRA_8UB);
                    compressor.InputOptions.SetAlphaMode(AlphaMode);
                    compressor.InputOptions.SetMipmapFilter(MipMapFilter);
                    compressor.InputOptions.SetMipmapGeneration(GenerateMipmaps, MaxMipMapLevel);
                    compressor.InputOptions.SetRoundMode(TextureRoundingMode);
                    compressor.InputOptions.SetWrapMode(WrapMode);

                    compressor.InputOptions.SetGamma(InputGamma, OutputGamma);
                    compressor.InputOptions.SetNormalizeMipmaps(false);
                    compressor.InputOptions.SetNormalMap(false);
                    compressor.InputOptions.SetConvertToNormalMap(false);

                    compressor.CompressionOptions.SetQuality(Quality);

                    GeneralOutputHandler outputHandler;

                    switch (TextureFormat)
                    {
                        case TextureOutputFormat.Colour:
                            compressor.CompressionOptions.SetFormat(Format.RGBA);
                            outputHandler = new PixelOutputHandler<Color>(textureType);
                            break;

                        case TextureOutputFormat.Normals:
                            compressor.CompressionOptions.SetFormat(Format.RGBA);
                            outputHandler = new PixelOutputHandler<Color>(textureType);

                            compressor.InputOptions.SetNormalizeMipmaps(true);
                            compressor.InputOptions.SetNormalMap(true);
                            compressor.InputOptions.SetConvertToNormalMap(ConvertToNormalMap);
                            compressor.InputOptions.SetGamma(1.0f, 1.0f);
                            break;

                        case TextureOutputFormat.DXT1:
                            compressor.CompressionOptions.SetFormat(Format.DXT1);
                            outputHandler = new Dxt1OutputHandler(textureType);
                            break;

                        case TextureOutputFormat.DXT1a:
                            compressor.CompressionOptions.SetFormat(Format.DXT1a);
                            outputHandler = new Dxt1OutputHandler(textureType);
                            break;

                        case TextureOutputFormat.DXT3:
                            compressor.CompressionOptions.SetFormat(Format.DXT3);
                            outputHandler = new Dxt3OutputHandler(textureType);
                            break;

                        case TextureOutputFormat.DXT5:
                            compressor.CompressionOptions.SetFormat(Format.DXT5);
                            outputHandler = new Dxt5OutputHandler(textureType);
                            break;

                        case TextureOutputFormat.DXT5n:
                            //FIXME: We force fastest quality since the normal compression mode is _very_ slow.
                            compressor.CompressionOptions.SetQuality(Quality.Fastest);

                            compressor.CompressionOptions.SetFormat(Format.DXT5n);

                            compressor.InputOptions.SetNormalizeMipmaps(true);
                            compressor.InputOptions.SetNormalMap(true);
                            compressor.InputOptions.SetConvertToNormalMap(ConvertToNormalMap);
                            compressor.InputOptions.SetGamma(1.0f, 1.0f);

                            outputHandler = new Dxt5OutputHandler(textureType);
                            break;

                        default:
                            throw new NotSupportedException("Unknown texture output format: " + TextureFormat);
                    }

                    /*
                     * Set input data
                     */

                    //TODO: Use a float format when texture tools support it.
                    input.ConvertBitmapType(typeof(PixelBitmapContent<Color>));

                    for (int i = 0; i < input.Faces.Count; i++)
                    {
                        MipmapChain mipChain = input.Faces[i];

                        for (int j = 0; j < mipChain.Count; j++)
                        {
                            BitmapContent bitmap = mipChain[j];

                            byte[] bitmapData = bitmap.GetPixelData();

                            //FIXME: When we move to XNA 4 the layout of Color will change, hence we need to swizzle the input.
                            compressor.InputOptions.SetMipmapData(bitmapData, bitmap.Width, bitmap.Height, 1, i, j);
                        }
                    }

                    /*
                     * Setup output
                     */
                    

                    compressor.OutputOptions.SetOutputHandler(outputHandler);
                    compressor.OutputOptions.SetOutputHeader(false);

                    /*
                     * Go!
                     */
                    compressor.Compressor.SetEnableCuda(EnableCuda);

                    compressor.Compress();
                    /*
                     * Check the output makes sense.
                     */

                    outputHandler.OutputTextureContent.Validate();

                    return outputHandler.OutputTextureContent;
                }
            }
            catch (TextureToolsException ttexcept)
            {
                throw ConvertException(ttexcept);
            }
        }

        private TextureType FindTextureType(TextureContent input)
        {
            if (input is Texture2DContent)
                return TextureType.Texture2D;
            else if (input is TextureCubeContent)
                return TextureType.TextureCube;
            else
                throw new InvalidContentException("Invalid texture type, cube maps are not supported", input.Identity);
        }


        private Exception ConvertException(TextureToolsException ttexcept)
        {
            switch (ttexcept.ErrorCode)
            {
                case Error.UnsupportedFeature:
                    return new NotSupportedException("Attempt to use a unsupported feature of NVIDIA Texture Tools",ttexcept);

                case Error.InvalidInput:
                    return new InvalidContentException("Invalid input to NVIDIA texture tools", ttexcept);

                case Error.CudaError:
                case Error.Unknown:
                    return new InvalidOperationException("NVIDIA Texture Tools returned the following error: " + ttexcept.Message, ttexcept);

                case Error.FileOpen:
                case Error.FileWrite:
                    return new IOException("NVIDIA Texture Tools returned the following error: " + ttexcept.Message, ttexcept);

                default:
                    return new InvalidOperationException("NVIDIA Texture Tools returned an unknown error: " + ttexcept.Message, ttexcept);
            }
        }

        private class Dxt1OutputHandler : GeneralOutputHandler
        {
            public Dxt1OutputHandler(TextureType textureType) : base(textureType)
            {
            }

            protected override BitmapContent CreateBitmapContent(int width, int height)
            {
                return new Dxt1BitmapContent(width, height);
            }
        }

        private class Dxt3OutputHandler : GeneralOutputHandler
        {
            public Dxt3OutputHandler(TextureType textureType)
                : base(textureType)
            {
            }

            protected override BitmapContent CreateBitmapContent(int width, int height)
            {
                return new Dxt3BitmapContent(width, height);
            }
        }

        private class Dxt5OutputHandler : GeneralOutputHandler
        {
            public Dxt5OutputHandler(TextureType textureType)
                : base(textureType)
            {
            }

            protected override BitmapContent CreateBitmapContent(int width, int height)
            {
                return new Dxt5BitmapContent(width, height);
            }
        }

        private class PixelOutputHandler<T> : GeneralOutputHandler 
            where T : struct, System.IEquatable<T>
        {
            public PixelOutputHandler(TextureType textureType)
                : base(textureType)
            {
            }

            protected override BitmapContent CreateBitmapContent(int width, int height)
            {
                return new PixelBitmapContent<T>(width, height);
            }
        }

        private abstract class GeneralOutputHandler : OutputHandlerBase
        {
            TextureContent outputTextureContent;

            byte[] tempBitmapData;

            int dataWidth = -1;
            int dataHeight = -1;
            int dataSize = -1;

            int faceIndex = -1;
            int mipIndex = -1;
            int dataIndex = 0;

            public TextureContent OutputTextureContent
            {
                get
                {
                    CommitLevel();
                    return outputTextureContent;
                }
            }

            protected GeneralOutputHandler(TextureType textureType)
            {
                switch (textureType)
                {
                    case TextureType.Texture2D:
                        outputTextureContent = new Texture2DContent();    
                        break;

                    case TextureType.TextureCube:
                        outputTextureContent = new TextureCubeContent();
                        break;

                    default:
                        throw new NotSupportedException("Unknown texture type: " + textureType);
                }
            }

            protected override void BeginImage(int size, int width, int height, int depth, int face, int miplevel)
            {
                CommitLevel();

                dataIndex = 0;
                mipIndex = miplevel;
                faceIndex = face;

                dataWidth = width;
                dataHeight = height;
                dataSize = size;

                tempBitmapData = new byte[size];
            }

            protected override void WriteData(byte[] dataBuffer, int startIndex, int count)
            {
                Array.Copy(dataBuffer, startIndex, tempBitmapData, dataIndex, count);
                dataIndex += count;
            }

            protected abstract BitmapContent CreateBitmapContent(int width, int height);

            private void CommitLevel()
            {
                if (faceIndex >= 0)
                {
                    BitmapContent newBitmap = CreateBitmapContent(dataWidth, dataHeight);

                    newBitmap.SetPixelData(tempBitmapData);

                    outputTextureContent.Faces[faceIndex].Add(newBitmap);

                    dataSize = -1;
                    dataWidth = dataHeight = -1;
                    faceIndex = -1;
                    mipIndex = -1;
                    dataIndex = 0;
                }
            }
        }
    }
}