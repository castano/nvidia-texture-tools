// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_MATRIX_H
#define NV_MATH_MATRIX_H

#include "Vector.h"

// - Matrices are stored in memory in *column major* order.
// - Points are to be though of as column vectors.
// - Transformation of a point p by a matrix M is: p' = M * p

namespace nv
{
    enum identity_t { identity };

    // 2x2 matrix.
    class NVMATH_CLASS Matrix2
    {
    public:
        Matrix2();
        explicit Matrix2(float f);
        explicit Matrix2(identity_t);
        Matrix2(const Matrix2 & m);
        Matrix2(Vector2::Arg v0, Vector2::Arg v1);
        Matrix2(float a, float b, float c, float d);
        
        float data(uint idx) const;
        float & data(uint idx);
        float get(uint row, uint col) const;
        float operator()(uint row, uint col) const;
        float & operator()(uint row, uint col);
        
        Vector2 row(uint i) const;
        Vector2 column(uint i) const;
        
        void operator*=(float s);
        void operator/=(float s);
        void operator+=(const Matrix2 & m);
        void operator-=(const Matrix2 & m);
        
        void scale(float s);
        void scale(Vector2::Arg s);
        float determinant() const;
        
    private:
        float m_data[4];
    };
    
    // Solve equation system using LU decomposition and back-substitution.
    NVMATH_API bool solveLU(const Matrix2 & m, const Vector2 & b, Vector2 * x);
    
    // Solve equation system using Cramer's inverse.
    NVMATH_API bool solveCramer(const Matrix2 & A, const Vector2 & b, Vector2 * x);
    
    
    // 3x3 matrix.
    class NVMATH_CLASS Matrix3
    {
    public:
        Matrix3();
        explicit Matrix3(float f);
        explicit Matrix3(identity_t);
        Matrix3(const Matrix3 & m);
        Matrix3(Vector3::Arg v0, Vector3::Arg v1, Vector3::Arg v2);

        float data(uint idx) const;
        float & data(uint idx);
        float get(uint row, uint col) const;
        float operator()(uint row, uint col) const;
        float & operator()(uint row, uint col);

        Vector3 row(uint i) const;
        Vector3 column(uint i) const;

        void operator*=(float s);
        void operator/=(float s);
        void operator+=(const Matrix3 & m);
        void operator-=(const Matrix3 & m);

        void scale(float s);
        void scale(Vector3::Arg s);
        float determinant() const;

    private:
        float m_data[9];
    };

    // Solve equation system using LU decomposition and back-substitution.
    NVMATH_API bool solveLU(const Matrix3 & m, const Vector3 & b, Vector3 * x);

    // Solve equation system using Cramer's inverse.
    NVMATH_API bool solveCramer(const Matrix3 & A, const Vector3 & b, Vector3 * x);

    NVMATH_API Matrix3 inverse(const Matrix3 & m);
    

    // 4x4 matrix.
    class NVMATH_CLASS Matrix
    {
    public:
        typedef Matrix const & Arg;

        Matrix();
        explicit Matrix(float f);
        explicit Matrix(identity_t);
        Matrix(const Matrix3 & m);
        Matrix(const Matrix & m);
        Matrix(Vector4::Arg v0, Vector4::Arg v1, Vector4::Arg v2, Vector4::Arg v3);
        //explicit Matrix(const float m[]);	// m is assumed to contain 16 elements

        float data(uint idx) const;
        float & data(uint idx);
        float get(uint row, uint col) const;
        float operator()(uint row, uint col) const;
        float & operator()(uint row, uint col);
        const float * ptr() const;

        Vector4 row(uint i) const;
        Vector4 column(uint i) const;

        void zero();
        void identity();

        void scale(float s);
        void scale(Vector3::Arg s);
        void translate(Vector3::Arg t);
        void rotate(float theta, float v0, float v1, float v2);
        float determinant() const;

        void operator+=(const Matrix & m);
        void operator-=(const Matrix & m);

        void apply(Matrix::Arg m);

    private:
        float m_data[16];
    };

    // Solve equation system using LU decomposition and back-substitution.
    NVMATH_API bool solveLU(const Matrix & A, const Vector4 & b, Vector4 * x);

    // Solve equation system using Cramer's inverse.
    NVMATH_API bool solveCramer(const Matrix & A, const Vector4 & b, Vector4 * x);

    // Compute inverse using LU decomposition.
    NVMATH_API Matrix inverseLU(const Matrix & m);

    // Compute inverse using Gaussian elimination and partial pivoting.
    NVMATH_API Matrix inverse(const Matrix & m);

} // nv namespace

#endif // NV_MATH_MATRIX_H
