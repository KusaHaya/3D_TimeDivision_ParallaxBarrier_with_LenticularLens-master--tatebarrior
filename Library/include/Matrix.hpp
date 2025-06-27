#pragma once

#include "Quaternion.hpp"

class Matrix
{
public:
	float	M11, M12, M13, M14,
			M21, M22, M23, M24,
			M31, M32, M33, M34,
			M41, M42, M43, M44;

	Matrix() :	M11(0), M12(0), M13(0), M14(0),
				M21(0), M22(0), M23(0), M24(0),
				M31(0), M32(0), M33(0), M34(0),
				M41(0), M42(0), M43(0), M44(0) { }

	static void CreateFromQuaternion(const Quaternion& quaternion, Matrix& result)
	{
		float num = quaternion.X * quaternion.X;
		float num2 = quaternion.Y * quaternion.Y;
		float num3 = quaternion.Z * quaternion.Z;
		float num4 = quaternion.X * quaternion.Y;
		float num5 = quaternion.Z * quaternion.W;
		float num6 = quaternion.Z * quaternion.X;
		float num7 = quaternion.Y * quaternion.W;
		float num8 = quaternion.Y * quaternion.Z;
		float num9 = quaternion.X * quaternion.W;
		result.M11 = 1 - 2 * (num2 + num3);
		result.M12 = 2 * (num4 + num5);
		result.M13 = 2 * (num6 - num7);
		result.M14 = 0;
		result.M21 = 2 * (num4 - num5);
		result.M22 = 1 - 2 * (num3 + num);
		result.M23 = 2 * (num8 + num9);
		result.M24 = 0;
		result.M31 = 2 * (num6 + num7);
		result.M32 = 2 * (num8 - num9);
		result.M33 = 1 - 2 * (num2 + num);
		result.M34 = 0;
		result.M41 = 0;
		result.M42 = 0;
		result.M43 = 0;
		result.M44 = 1;
	}

	// 指定したヨー、ピッチ、ロールで行列を作成します。
	// yaw : y 軸を回転軸とする回転の角度 (ラジアン単位)。
	// pitch : x 軸を回転軸とする回転の角度 (ラジアン単位)。
	// roll : z 軸を回転軸とする回転の角度 (ラジアン単位)。
	static Matrix CreateFromYawPitchRoll(float yaw, float pitch, float roll)
	{
		Quaternion quaternion =	Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);
		Matrix result;
		Matrix::CreateFromQuaternion(quaternion, result);
		return result;
	}
};