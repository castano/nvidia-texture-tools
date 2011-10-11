// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_MATRIX_H
#define NV_MATH_MATRIX_H

#include "Vector.h"

namespace nv
{
    enum identity_t { identity };

    class NVMATH_CLASS Matrix3
    {
    public:
        Matrix3();
        explicit Matrix3(float f);
        explicit Matrix3(identity_t);
        Matrix3(const Matrix3 & m);
        Matrix3(Vector3::Arg v0, Vector3::Arg v1, Vector3::Arg v2);

        scalar get(uint row, uint col) const;
        scalar operator()(uint row, uint col) const;
        scalar & operator()(uint row, uint col);

        Vector3 row(uint i) const;
        Vector3 column(uint i) const;

        void operator*=(float s);
        void operator/=(float s);
        void operator+=(const Matrix3 & m);
        void operator-=(const Matrix3 & m);

        float determinant() const;

    private:
        scalar m_data[9];
    };


    // 4x4 transformation matrix.
    // -# Matrices are stored in memory in column major order.
    // -# Points are to be though of as column vectors.
    // -# Transformation of a point p by a matrix M is: p' = M * p
    class NVMATH_CLASS Matrix
    {
    public:
        typedef Matrix const & Arg;

        Matrix();
        explicit Matrix(float f);
        explicit Matrix(identity_t);
        Matrix(const Matrix & m);
        Matrix(Vector4::Arg v0, Vector4::Arg v1, Vector4::Arg v2, Vector4::Arg v3);
        //explicit Matrix(const scalar m[]);	// m is assumed to contain 16 elements

        scalar data(uint idx) const;
        scalar & data(uint idx);
        scalar get(uint row, uint col) const;
        scalar operator()(uint row, uint col) const;
        scalar & operator()(uint row, uint col);
        const scalar * ptr() const;

        Vector4 row(uint i) const;
        Vector4 column(uint i) const;

        void scale(scalar s);
        void scale(Vector3::Arg s);
        void translate(Vector3::Arg t);
        void rotate(scalar theta, scalar v0, scalar v1, scalar v2);
        scalar determinant() const;

        void apply(Matrix::Arg m);

    private:
        scalar m_data[16];
    };

} // nv namespace

#endif // NV_MATH_MATRIX_H
