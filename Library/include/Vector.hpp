#pragma once

#include "Matrix.hpp"

struct Vector2
{
public:
	float X, Y;

	Vector2() : X(0), Y(0) { }
	Vector2(float x, float y) : X(x), Y(y) { }
	
	static Vector2 Add(const Vector2& v1, const Vector2& v2)
	{
		Vector2 result;
		result.X = v1.X + v2.X;
		result.Y = v1.Y + v2.Y;
		return result;
	}
	static Vector2 Sub(const Vector2& v1, const Vector2& v2)
	{
		Vector2 result;
		result.X = v1.X - v2.X;
		result.Y = v1.Y - v2.Y;
		return result;
	}
	static Vector2 Multiply(const Vector2& v1, const float m)
	{
		Vector2 result;
		result.X = m * v1.X;
		result.Y = m * v1.Y;
		return result;
	}
	Vector2 operator+(const Vector2& v)
	{
		return Add(*this, v);
	}
	Vector2 operator-(const Vector2& v)
	{
		return Sub(*this, v);
	}
	Vector2 operator*(const float& m)
	{
		return Multiply(*this, m);
	}
};

struct Vector3
{
public:
	float X, Y, Z;

	Vector3() : X(0), Y(0), Z(0) { }
	Vector3(float x, float y, float z) : X(x), Y(y), Z(z) { }

	static Vector3 Transform(const Vector3& position, const Matrix& matrix)
	{
		float x = position.X * matrix.M11 + position.Y * matrix.M21 + position.Z * matrix.M31 + matrix.M41;
		float y = position.X * matrix.M12 + position.Y * matrix.M22 + position.Z * matrix.M32 + matrix.M42;
		float z = position.X * matrix.M13 + position.Y * matrix.M23 + position.Z * matrix.M33 + matrix.M43;
		Vector3 result;
		result.X = x;
		result.Y = y;
		result.Z = z;
		return result;
	}
	static Vector3 Add(const Vector3& v1, const Vector3& v2)
	{
		Vector3 result;
		result.X = v1.X + v2.X;
		result.Y = v1.Y + v2.Y;
		result.Z = v1.Z + v2.Z;
		return result;
	}
	static Vector3 Sub(const Vector3& v1, const Vector3& v2)
	{
		Vector3 result;
		result.X = v1.X - v2.X;
		result.Y = v1.Y - v2.Y;
		result.Z = v1.Z - v2.Z;
		return result;
	}
	static Vector3 Multiply(const Vector3& v1, const float m)
	{
		Vector3 result;
		result.X = m * v1.X;
		result.Y = m * v1.Y;
		result.Z = m * v1.Z;
		return result;
	}
	Vector3 operator+(const Vector3& v)
	{
		return Add(*this, v);
	}
	Vector3 operator-(const Vector3& v)
	{
		return Sub(*this, v);
	}
	Vector3 operator*(const float& m)
	{
		return Multiply(*this, m);
	}
};