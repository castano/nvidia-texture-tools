// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_COLOR_INL
#define NV_MATH_COLOR_INL

#include "Color.h"
#include "Vector.inl"


namespace nv
{

    // Clamp color components.
    inline Vector3 colorClamp(Vector3::Arg c)
    {
        return Vector3(clamp(c.x, 0.0f, 1.0f), clamp(c.y, 0.0f, 1.0f), clamp(c.z, 0.0f, 1.0f));
    }

    // Clamp without allowing the hue to change.
    inline Vector3 colorNormalize(Vector3::Arg c)
    {
        float scale = 1.0f;
        if (c.x > scale) scale = c.x;
        if (c.y > scale) scale = c.y;
        if (c.z > scale) scale = c.z;
        return c / scale;
    }

    // Convert Color32 to Color16.
    inline Color16 toColor16(Color32 c)
    {
        Color16 color;
        //         rrrrrggggggbbbbb
        // rrrrr000gggggg00bbbbb000
        // color.u = (c.u >> 3) & 0x1F;
        // color.u |= (c.u >> 5) & 0x7E0;
        // color.u |= (c.u >> 8) & 0xF800;

        color.r = c.r >> 3;
        color.g = c.g >> 2;
        color.b = c.b >> 3;
        return color; 
    }


    // Promote 16 bit color to 32 bit using regular bit expansion.
    inline Color32 toColor32(Color16 c)
    {
        Color32 color;
        // c.u = ((col0.u << 3) & 0xf8) | ((col0.u << 5) & 0xfc00) | ((col0.u << 8) & 0xf80000);
        // c.u |= (c.u >> 5) & 0x070007;
        // c.u |= (c.u >> 6) & 0x000300;

        color.b = (c.b << 3) | (c.b >> 2);
        color.g = (c.g << 2) | (c.g >> 4);
        color.r = (c.r << 3) | (c.r >> 2);
        color.a = 0xFF;

        return color;
    }

    inline Color32 toColor32(Vector4::Arg v)
    {
        Color32 color;
        color.r = uint8(clamp(v.x, 0.0f, 1.0f) * 255);
        color.g = uint8(clamp(v.y, 0.0f, 1.0f) * 255);
        color.b = uint8(clamp(v.z, 0.0f, 1.0f) * 255);
        color.a = uint8(clamp(v.w, 0.0f, 1.0f) * 255);

        return color;
    }

    inline Vector4 toVector4(Color32 c)
    {
        const float scale = 1.0f / 255.0f;
        return Vector4(c.r * scale, c.g * scale, c.b * scale, c.a * scale);
    }


    inline float perceptualColorDistance(Vector3::Arg c0, Vector3::Arg c1)
    {
        float rmean = (c0.x + c1.x) * 0.5f;
        float r = c1.x - c0.x;
        float g = c1.y - c0.y;
        float b = c1.z - c0.z;
        return sqrtf((2 + rmean)*r*r + 4*g*g + (3 - rmean)*b*b);
    }

} // nv namespace

#endif // NV_MATH_COLOR_INL
