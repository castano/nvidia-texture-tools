using NUnit.Framework;
using System;
using Nvidia.TextureTools;
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

			compressor.Compress (inputOptions, compressionOptions, outputOptions);
		}
	}
}
