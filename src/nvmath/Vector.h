// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_VECTOR_H
#define NV_MATH_VECTOR_H

#include "nvmath.h"
#include "nvcore/Utils.h" // min, max

namespace nv
{

    // I should probably use templates.
    typedef float scalar;

    class NVMATH_CLASS Vector2
    {
    public:
        typedef Vector2 const & Arg;

        Vector2();
        explicit Vector2(scalar f);
        Vector2(scalar x, scalar y);
        Vector2(Vector2::Arg v);

        const Vector2 & operator=(Vector2::Arg v);

        const scalar * ptr() const;

        void set(scalar x, scalar y);

        Vector2 operator-() const;
        void operator+=(Vector2::Arg v);
        void operator-=(Vector2::Arg v);
        void operator*=(scalar s);
        void operator*=(Vector2::Arg v);

        friend bool operator==(Vector2::Arg a, Vector2::Arg b);
        friend bool operator!=(Vector2::Arg a, Vector2::Arg b);

        union {
            struct {
                scalar x, y;
            };
            scalar component[2];
        };
    };


    class NVMATH_CLASS Vector3
    {
    public:
        typedef Vector3 const & Arg;

        Vector3();
        explicit Vector3(scalar x);
        Vector3(scalar x, scalar y, scalar z);
        Vector3(Vector2::Arg v, scalar z);
        Vector3(Vector3::Arg v);

        const Vector3 & operator=(Vector3::Arg v);

        Vector2 xy() const;

        const scalar * ptr() const;

        void set(scalar x, scalar y, scalar z);

        Vector3 operator-() const;
        void operator+=(Vector3::Arg v);
        void operator-=(Vector3::Arg v);
        void operator*=(scalar s);
        void operator/=(scalar s);
        void operator*=(Vector3::Arg v);

        friend bool operator==(Vector3::Arg a, Vector3::Arg b);
        friend bool operator!=(Vector3::Arg a, Vector3::Arg b);

        union {
            struct {
                scalar x, y, z;
            };
            scalar component[3];
        };
    };

    // Helpers to convert vector types. Assume T has x,y,z members and 3 argument constructor.
    template <typename T> Vector3 from(const T & v) { return Vector3(v.x, v.y, v.z); }
    template <typename T> T to(Vector3::Arg v) { return T(v.x, v.y, v.z); }


    class NVMATH_CLASS Vector4
    {
    public:
        typedef Vector4 const & Arg;

        Vector4();
        explicit Vector4(scalar x);
        Vector4(scalar x, scalar y, scalar z, scalar w);
        Vector4(Vector2::Arg v, scalar z, scalar w);
        Vector4(Vector3::Arg v, scalar w);
        Vector4(Vector4::Arg v);
        //	Vector4(const Quaternion & v);

        const Vector4 & operator=(Vector4::Arg v);

        Vector2 xy() const;
        Vector3 xyz() const;

        const scalar * ptr() const;

        void set(scalar x, scalar y, scalar z, scalar w);

        Vector4 operator-() const;
        void operator+=(Vector4::Arg v);
        void operator-=(Vector4::Arg v);
        void operator*=(scalar s);
        void operator*=(Vector4::Arg v);

        friend bool operator==(Vector4::Arg a, Vector4::Arg b);
        friend bool operator!=(Vector4::Arg a, Vector4::Arg b);

        union {
            struct {
                scalar x, y, z, w;
            };
            scalar component[4];
        };
    };


    // Vector2

    inline Vector2::Vector2() {}
    inline Vector2::Vector2(scalar f) : x(f), y(f) {}
    inline Vector2::Vector2(scalar x, scalar y) : x(x), y(y) {}
    inline Vector2::Vector2(Vector2::Arg v) : x(v.x), y(v.y) {}

    inline const Vector2 & Vector2::operator=(Vector2::Arg v)
    {
        x = v.x;
        y = v.y;
        return *this;
    }

    inline const scalar * Vector2::ptr() const
    {
        return &x;
    }

    inline void Vector2::set(scalar x, scalar y)
    {
        this->x = x;
        this->y = y;
    }

    inline Vector2 Vector2::operator-() const
    {
        return Vector2(-x, -y);
    }

    inline void Vector2::operator+=(Vector2::Arg v)
    {
        x += v.x;
        y += v.y;
    }

    inline void Vector2::operator-=(Vector2::Arg v)
    {
        x -= v.x;
        y -= v.y;
    }

    inline void Vector2::operator*=(scalar s)
    {
        x *= s;
        y *= s;
    }

    inline void Vector2::operator*=(Vector2::Arg v)
    {
        x *= v.x;
        y *= v.y;
    }

    inline bool operator==(Vector2::Arg a, Vector2::Arg b)
    {
        return a.x == b.x && a.y == b.y; 
    }
    inline bool operator!=(Vector2::Arg a, Vector2::Arg b)
    {
        return a.x != b.x || a.y != b.y; 
    }


    // Vector3

    inline Vector3::Vector3() {}
    inline Vector3::Vector3(scalar f) : x(f), y(f), z(f) {}
    inline Vector3::Vector3(scalar x, scalar y, scalar z) : x(x), y(y), z(z) {}
    inline Vector3::Vector3(Vector2::Arg v, scalar z) : x(v.x), y(v.y), z(z) {}
    inline Vector3::Vector3(Vector3::Arg v) : x(v.x), y(v.y), z(v.z) {}

    inline const Vector3 & Vector3::operator=(Vector3::Arg v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }


    inline Vector2 Vector3::xy() const
    {
        return Vector2(x, y);
    }

    inline const scalar * Vector3::ptr() const
    {
        return &x;
    }

    inline void Vector3::set(scalar x, scalar y, scalar z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    inline Vector3 Vector3::operator-() const
    {
        return Vector3(-x, -y, -z);
    }

    inline void Vector3::operator+=(Vector3::Arg v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
    }

    inline void Vector3::operator-=(Vector3::Arg v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }

    inline void Vector3::operator*=(scalar s)
    {
        x *= s;
        y *= s;
        z *= s;
    }

    inline void Vector3::operator/=(scalar s)
    {
        float is = 1.0f / s;
        x *= is;
        y *= is;
        z *= is;
    }

    inline void Vector3::operator*=(Vector3::Arg v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
    }

    inline bool operator==(Vector3::Arg a, Vector3::Arg b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z; 
    }
    inline bool operator!=(Vector3::Arg a, Vector3::Arg b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z; 
    }


    // Vector4

    inline Vector4::Vector4() {}
    inline Vector4::Vector4(scalar f) : x(f), y(f), z(f), w(f) {}
    inline Vector4::Vector4(scalar x, scalar y, scalar z, scalar w) : x(x), y(y), z(z), w(w) {}
    inline Vector4::Vector4(Vector2::Arg v, scalar z, scalar w) : x(v.x), y(v.y), z(z), w(w) {}
    inline Vector4::Vector4(Vector3::Arg v, scalar w) : x(v.x), y(v.y), z(v.z), w(w) {}
    inline Vector4::Vector4(Vector4::Arg v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

    inline const Vector4 & Vector4::operator=(const Vector4 & v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        w = v.w;
        return *this;
    }

    inline Vector2 Vector4::xy() const
    {
        return Vector2(x, y);
    }

    inline Vector3 Vector4::xyz() const
    {
        return Vector3(x, y, z);
    }

    inline const scalar * Vector4::ptr() const
    {
        return &x;
    }

    inline void Vector4::set(scalar x, scalar y, scalar z, scalar w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    inline Vector4 Vector4::operator-() const
    {
        return Vector4(-x, -y, -z, -w);
    }

    inline void Vector4::operator+=(Vector4::Arg v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
    }

    inline void Vector4::operator-=(Vector4::Arg v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
    }

    inline void Vector4::operator*=(scalar s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
    }

    inline void Vector4::operator*=(Vector4::Arg v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
    }

    inline bool operator==(Vector4::Arg a, Vector4::Arg b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; 
    }
    inline bool operator!=(Vector4::Arg a, Vector4::Arg b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w; 
    }



    // Functions


    // Vector2

    inline Vector2 add(Vector2::Arg a, Vector2::Arg b)
    {
        return Vector2(a.x + b.x, a.y + b.y);
    }
    inline Vector2 operator+(Vector2::Arg a, Vector2::Arg b)
    {
        return add(a, b);
    }

    inline Vector2 sub(Vector2::Arg a, Vector2::Arg b)
    {
        return Vector2(a.x - b.x, a.y - b.y);
    }
    inline Vector2 operator-(Vector2::Arg a, Vector2::Arg b)
    {
        return sub(a, b);
    }

    inline Vector2 scale(Vector2::Arg v, scalar s)
    {
        return Vector2(v.x * s, v.y * s);
    }

    inline Vector2 scale(Vector2::Arg v, Vector2::Arg s)
    {
        return Vector2(v.x * s.x, v.y * s.y);
    }

    inline Vector2 operator*(Vector2::Arg v, scalar s)
    {
        return scale(v, s);
    }

    inline Vector2 operator*(Vector2::Arg v1, Vector2::Arg v2)
    {
        return Vector2(v1.x*v2.x, v1.y*v2.y);
    }

    inline Vector2 operator*(scalar s, Vector2::Arg v)
    {
        return scale(v, s);
    }

    inline Vector2 operator/(Vector2::Arg v, scalar s)
    {
        return scale(v, 1.0f/s);
    }

    inline scalar dot(Vector2::Arg a, Vector2::Arg b)
    {
        return a.x * b.x + a.y * b.y;
    }

    inline scalar lengthSquared(Vector2::Arg v)
    {
        return v.x * v.x + v.y * v.y;
    }

    inline scalar length(Vector2::Arg v)
    {
        return sqrtf(lengthSquared(v));
    }

    inline scalar inverseLength(Vector2::Arg v)
    {
        return 1.0f / sqrtf(lengthSquared(v));
    }

    inline bool isNormalized(Vector2::Arg v, float epsilon = NV_NORMAL_EPSILON)
    {
        return equal(length(v), 1, epsilon);
    }

    inline Vector2 normalize(Vector2::Arg v, float epsilon = NV_EPSILON)
    {
        float l = length(v);
        nvDebugCheck(!isZero(l, epsilon));
        Vector2 n = scale(v, 1.0f / l);
        nvDebugCheck(isNormalized(n));
        return n;
    }

    inline Vector2 normalizeSafe(Vector2::Arg v, Vector2::Arg fallback, float epsilon = NV_EPSILON)
    {
        float l = length(v);
        if (isZero(l, epsilon)) {
            return fallback;
        }
        return scale(v, 1.0f / l);
    }


    inline bool equal(Vector2::Arg v1, Vector2::Arg v2, float epsilon = NV_EPSILON)
    {
        return equal(v1.x, v2.x, epsilon) && equal(v1.y, v2.y, epsilon);
    }

    inline Vector2 min(Vector2::Arg a, Vector2::Arg b)
    {
        return Vector2(min(a.x, b.x), min(a.y, b.y));
    }

    inline Vector2 max(Vector2::Arg a, Vector2::Arg b)
    {
        return Vector2(max(a.x, b.x), max(a.y, b.y));
    }

    inline bool isValid(Vector2::Arg v)
    {
        return isFinite(v.x) && isFinite(v.y);
    }


    // Vector3

    inline Vector3 add(Vector3::Arg a, Vector3::Arg b)
    {
        return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
    }
    inline Vector3 add(Vector3::Arg a, float b)
    {
        return Vector3(a.x + b, a.y + b, a.z + b);
    }
    inline Vector3 operator+(Vector3::Arg a, Vector3::Arg b)
    {
        return add(a, b);
    }
    inline Vector3 operator+(Vector3::Arg a, float b)
    {
        return add(a, b);
    }

    inline Vector3 sub(Vector3::Arg a, Vector3::Arg b)
    {
        return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
    }
    inline Vector3 sub(Vector3::Arg a, float b)
    {
        return Vector3(a.x - b, a.y - b, a.z - b);
    }
    inline Vector3 operator-(Vector3::Arg a, Vector3::Arg b)
    {
        return sub(a, b);
    }
    inline Vector3 operator-(Vector3::Arg a, float b)
    {
        return sub(a, b);
    }

    inline Vector3 cross(Vector3::Arg a, Vector3::Arg b)
    {
        return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    inline Vector3 scale(Vector3::Arg v, scalar s)
    {
        return Vector3(v.x * s, v.y * s, v.z * s);
    }

    inline Vector3 scale(Vector3::Arg v, Vector3::Arg s)
    {
        return Vector3(v.x * s.x, v.y * s.y, v.z * s.z);
    }

    inline Vector3 operator*(Vector3::Arg v, scalar s)
    {
        return scale(v, s);
    }

    inline Vector3 operator*(scalar s, Vector3::Arg v)
    {
        return scale(v, s);
    }

    inline Vector3 operator*(Vector3::Arg v, Vector3::Arg s)
    {
        return scale(v, s);
    }

    inline Vector3 operator/(Vector3::Arg v, scalar s)
    {
        return scale(v, 1.0f/s);
    }

    inline Vector3 add_scaled(Vector3::Arg a, Vector3::Arg b, scalar s)
    {
        return Vector3(a.x + b.x * s, a.y + b.y * s, a.z + b.z * s);
    }

    inline Vector3 lerp(Vector3::Arg v1, Vector3::Arg v2, scalar t)
    {
        const scalar s = 1.0f - t;
        return Vector3(v1.x * s + t * v2.x, v1.y * s + t * v2.y, v1.z * s + t * v2.z);
    }

    inline scalar dot(Vector3::Arg a, Vector3::Arg b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline scalar lengthSquared(Vector3::Arg v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    inline scalar length(Vector3::Arg v)
    {
        return sqrtf(lengthSquared(v));
    }

    inline scalar inverseLength(Vector3::Arg v)
    {
        return 1.0f / sqrtf(lengthSquared(v));
    }

    inline bool isNormalized(Vector3::Arg v, float epsilon = NV_NORMAL_EPSILON)
    {
        return equal(length(v), 1, epsilon);
    }

    inline Vector3 normalize(Vector3::Arg v, float epsilon = NV_EPSILON)
    {
        float l = length(v);
        nvDebugCheck(!isZero(l, epsilon));
        Vector3 n = scale(v, 1.0f / l);
        nvDebugCheck(isNormalized(n));
        return n;
    }

    inline Vector3 normalizeSafe(Vector3::Arg v, Vector3::Arg fallback, float epsilon = NV_EPSILON)
    {
        float l = length(v);
        if (isZero(l, epsilon)) {
            return fallback;
        }
        return scale(v, 1.0f / l);
    }

    inline bool equal(Vector3::Arg v1, Vector3::Arg v2, float epsilon = NV_EPSILON)
    {
        return equal(v1.x, v2.x, epsilon) && equal(v1.y, v2.y, epsilon) && equal(v1.z, v2.z, epsilon);
    }

    inline Vector3 min(Vector3::Arg a, Vector3::Arg b)
    {
        return Vector3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
    }

    inline Vector3 max(Vector3::Arg a, Vector3::Arg b)
    {
        return Vector3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
    }

    inline Vector3 clamp(Vector3::Arg v, float min, float max)
    {
        return Vector3(clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max));
    }

    inline bool isValid(Vector3::Arg v)
    {
        return isFinite(v.x) && isFinite(v.y) && isFinite(v.z);
    }

    inline Vector3 floor(Vector3::Arg v)
    {
        return Vector3(floorf(v.x), floorf(v.y), floorf(v.z));
    }

    inline Vector3 ceil(Vector3::Arg v)
    {
        return Vector3(ceilf(v.x), ceilf(v.y), ceilf(v.z));
    }

    // Vector4

    inline Vector4 add(Vector4::Arg a, Vector4::Arg b)
    {
        return Vector4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
    }
    inline Vector4 operator+(Vector4::Arg a, Vector4::Arg b)
    {
        return add(a, b);
    }

    inline Vector4 sub(Vector4::Arg a, Vector4::Arg b)
    {
        return Vector4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
    }
    inline Vector4 operator-(Vector4::Arg a, Vector4::Arg b)
    {
        return sub(a, b);
    }

    inline Vector4 scale(Vector4::Arg v, scalar s)
    {
        return Vector4(v.x * s, v.y * s, v.z * s, v.w * s);
    }

    inline Vector4 scale(Vector4::Arg v, Vector4::Arg s)
    {
        return Vector4(v.x * s.x, v.y * s.y, v.z * s.z, v.w * s.w);
    }

    inline Vector4 operator*(Vector4::Arg v, scalar s)
    {
        return scale(v, s);
    }

    inline Vector4 operator*(scalar s, Vector4::Arg v)
    {
        return scale(v, s);
    }

    inline Vector4 operator/(Vector4::Arg v, scalar s)
    {
        return scale(v, 1.0f/s);
    }

    inline Vector4 add_scaled(Vector4::Arg a, Vector4::Arg b, scalar s)
    {
        return Vector4(a.x + b.x * s, a.y + b.y * s, a.z + b.z * s, a.w + b.w * s);
    }

    inline scalar dot(Vector4::Arg a, Vector4::Arg b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    inline scalar lengthSquared(Vector4::Arg v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    }

    inline scalar length(Vector4::Arg v)
    {
        return sqrtf(lengthSquared(v));
    }

    inline scalar inverseLength(Vector4::Arg v)
    {
        return 1.0f / sqrtf(lengthSquared(v));
    }

    inline bool isNormalized(Vector4::Arg v, float epsilon = NV_NORMAL_EPSILON)
    {
        return equal(length(v), 1, epsilon);
    }

    inline Vector4 normalize(Vector4::Arg v, float epsilon = NV_EPSILON)
    {
        float l = length(v);
        nvDebugCheck(!isZero(l, epsilon));
        Vector4 n = scale(v, 1.0f / l);
        nvDebugCheck(isNormalized(n));
        return n;
    }

    inline Vector4 normalizeSafe(Vector4::Arg v, Vector4::Arg fallback, float epsilon = NV_EPSILON)
    {
        float l = length(v);
        if (isZero(l, epsilon)) {
            return fallback;
        }
        return scale(v, 1.0f / l);
    }

    inline bool equal(Vector4::Arg v1, Vector4::Arg v2, float epsilon = NV_EPSILON)
    {
        return equal(v1.x, v2.x, epsilon) && equal(v1.y, v2.y, epsilon) && equal(v1.z, v2.z, epsilon) && equal(v1.w, v2.w, epsilon);
    }

    inline Vector4 min(Vector4::Arg a, Vector4::Arg b)
    {
        return Vector4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w));
    }

    inline Vector4 max(Vector4::Arg a, Vector4::Arg b)
    {
        return Vector4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
    }

    inline bool isValid(Vector4::Arg v)
    {
        return isFinite(v.x) && isFinite(v.y) && isFinite(v.z) && isFinite(v.w);
    }

} // nv namespace

#endif // NV_MATH_VECTOR_H
