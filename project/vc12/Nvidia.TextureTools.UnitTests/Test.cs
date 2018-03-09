using NUnit.Framework;
using System;
using Nvidia.TextureTools;
using System.Runtime.InteropServices;

namespace Nvidia.TextureTools.UnitTests {
	[TestFixture ()]
	public class Test {
		[Test ()]
		public void TestCase ()
		{
			var inputOptions = new InputOptions ();
			var outputOptions = new OutputOptions ();
			var compressionOptions = new CompressionOptions ();
			var compressor = new Compressor ();
			inputOptions.SetAlphaMode (AlphaMode.Premultiplied);
			inputOptions.SetTextureLayout (TextureType.Texture2D, 128, 128, 1);
			byte [] sourceData = new byte [128*128*4];
			var dataHandle = GCHandle.Alloc (sourceData, GCHandleType.Pinned);
			var BeginImage = new OutputOptions.BeginImageHandler (BeginImageInternal);
			var WriteData = new OutputOptions.OutputHandler (WriteDataInternal);
			var EndImage = new OutputOptions.EndImageHandler (EndImageInternal);
			var a = GCHandle.Alloc (BeginImage);
			var b = GCHandle.Alloc (WriteData);
			var c = GCHandle.Alloc (EndImage);
			try {
				var dataPtr = dataHandle.AddrOfPinnedObject ();
				inputOptions.SetMipmapData (dataPtr, 128, 128, 1, 0, 0);
				inputOptions.SetMipmapGeneration (false);
				inputOptions.SetGamma (1.0f, 1.0f);
				outputOptions.SetOutputHeader (false);
				outputOptions.SetOutputOptionsOutputHandler (BeginImage, WriteData, EndImage);
				var estsize = compressor.EstimateSize (inputOptions, compressionOptions);
				Assert.True (compressor.Compress (inputOptions, compressionOptions, outputOptions));
			}finally {
				a.Free ();
				b.Free ();
				c.Free ();
				dataHandle.Free ();
			}
		}

		byte [] buffer;
		int offset;

		void BeginImageInternal (int size, int width, int height, int depth, int face, int miplevel)
		{
			buffer = new byte [size];
			offset = 0;
		}

		bool WriteDataInternal (IntPtr data, int length)
		{
			Marshal.Copy (data, buffer, offset, length);
			offset += length;
			if (offset == buffer.Length)
			{
				
			}
			return true;
		}

		void EndImageInternal ()
		{
			Console.WriteLine ("EndImageInternal");
		}
	}
}
