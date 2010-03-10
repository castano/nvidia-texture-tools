// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_COLORBLOCK_H
#define NV_IMAGE_COLORBLOCK_H

#include <nvmath/Color.h>

namespace nv
{
	class Image;

	/// Uncompressed 4x4 color block.
	struct ColorBlock
	{
		ColorBlock();
		ColorBlock(const uint * linearImage);
		ColorBlock(const ColorBlock & block);
		ColorBlock(const Image * img, uint x, uint y);
		
		void init(const Image * img, uint x, uint y);
		void init(uint w, uint h, uint * data, uint x, uint y);
		void init(uint w, uint h, float * data, uint x, uint y);
		
		void swizzle(uint x, uint y, uint z, uint w); // 0=r, 1=g, 2=b, 3=a, 4=0xFF, 5=0
		
		bool isSingleColor() const;
		//bool isSingleColorNoAlpha() const;
		uint countUniqueColors() const;
		Color32 averageColor() const;
		bool hasAlpha() const;
		
		void diameterRange(Color32 * start, Color32 * end) const;
		void luminanceRange(Color32 * start, Color32 * end) const;
		void boundsRange(Color32 * start, Color32 * end) const;
		void boundsRangeAlpha(Color32 * start, Color32 * end) const;
		
		void sortColorsByAbsoluteValue();
		
		void computeRange(const Vector3 & axis, Color32 * start, Color32 * end) const;
		void sortColors(const Vector3 & axis);
		
		float volume() const;
		
		// Accessors
		const Color32 * colors() const;

		Color32 color(uint i) const;
		Color32 & color(uint i);
		
		Color32 color(uint x, uint y) const;
		Color32 & color(uint x, uint y);
		
	private:
		
		Color32 m_color[4*4];
		
	};
	

	/// Get pointer to block colors.
	inline const Color32 * ColorBlock::colors() const
	{
		return m_color;
	}
	
	/// Get block color.
	inline Color32 ColorBlock::color(uint i) const
	{
		nvDebugCheck(i < 16);
		return m_color[i];
	}
	
	/// Get block color.
	inline Color32 & ColorBlock::color(uint i)
	{
		nvDebugCheck(i < 16);
		return m_color[i];
	}
	
	/// Get block color.
	inline Color32 ColorBlock::color(uint x, uint y) const
	{
		nvDebugCheck(x < 4 && y < 4);
		return m_color[y * 4 + x];
	}
	
	/// Get block color.
	inline Color32 & ColorBlock::color(uint x, uint y)
	{
		nvDebugCheck(x < 4 && y < 4);
		return m_color[y * 4 + x];
	}
	
} // nv namespace

#endif // NV_IMAGE_COLORBLOCK_H
