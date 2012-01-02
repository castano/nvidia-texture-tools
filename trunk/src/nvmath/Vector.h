// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_VECTOR_H
#define NV_MATH_VECTOR_H

#include "nvmath.h"

namespace nv
{
    class NVMATH_CLASS Vector2
    {
    public:
        typedef Vector2 const & Arg;

        Vector2();
        explicit Vector2(float f);
        Vector2(float x, float y);
        Vector2(Vector2::Arg v);

        const Vector2 & operator=(Vector2::Arg v);

        const float * ptr() const;

        void set(float x, float y);

        Vector2 operator-() const;
        void operator+=(Vector2::Arg v);
        void operator-=(Vector2::Arg v);
        void operator*=(float s);
        void operator*=(Vector2::Arg v);

        friend bool operator==(Vector2::Arg a, Vector2::Arg b);
        friend bool operator!=(Vector2::Arg a, Vector2::Arg b);

        union {
            struct {
                float x, y;
            };
            float component[2];
        };
    };

    // Helpers to convert vector types. Assume T has x,y members and 2 argument constructor.
    template <typename T> T to(Vector2::Arg v) { return T(v.x, v.y); }


    class NVMATH_CLASS Vector3
    {
    public:
        typedef Vector3 const & Arg;

        Vector3();
        explicit Vector3(float x);
        Vector3(float x, float y, float z);
        Vector3(Vector2::Arg v, float z);
        Vector3(Vector3::Arg v);

        const Vector3 & operator=(Vector3::Arg v);

        Vector2 xy() const;

        const float * ptr() const;

        void set(float x, float y, float z);

        Vector3 operator-() const;
        void operator+=(Vector3::Arg v);
        void operator-=(Vector3::Arg v);
        void operator*=(float s);
        void operator/=(float s);
        void operator*=(Vector3::Arg v);

        friend bool operator==(Vector3::Arg a, Vector3::Arg b);
        friend bool operator!=(Vector3::Arg a, Vector3::Arg b);

        union {
            struct {
                float x, y, z;
            };
            float component[3];
        };
    };

    // Helpers to convert vector types. Assume T has x,y,z members and 3 argument constructor.
    template <typename T> T to(Vector3::Arg v) { return T(v.x, v.y, v.z); }


    class NVMATH_CLASS Vector4
    {
    public:
        typedef Vector4 const & Arg;

        Vector4();
        explicit Vector4(float x);
        Vector4(float x, float y, float z, float w);
        Vector4(Vector2::Arg v, float z, float w);
        Vector4(Vector2::Arg v, Vector2::Arg u);
        Vector4(Vector3::Arg v, float w);
        Vector4(Vector4::Arg v);
        //	Vector4(const Quaternion & v);

        const Vector4 & operator=(Vector4::Arg v);

        Vector2 xy() const;
        Vector2 zw() const;
        Vector3 xyz() const;

        const float * ptr() const;

        void set(float x, float y, float z, float w);

        Vector4 operator-() const;
        void operator+=(Vector4::Arg v);
        void operator-=(Vector4::Arg v);
        void operator*=(float s);
        void operator*=(Vector4::Arg v);

        friend bool operator==(Vector4::Arg a, Vector4::Arg b);
        friend bool operator!=(Vector4::Arg a, Vector4::Arg b);

        union {
            struct {
                float x, y, z, w;
            };
            float component[4];
        };
    };

    // Helpers to convert vector types. Assume T has x,y,z members and 3 argument constructor.
    template <typename T> T to(Vector4::Arg v) { return T(v.x, v.y, v.z, v.w); }


} // nv namespace

#endif // NV_MATH_VECTOR_H
