#pragma once

#include <cmath>

class Quaternion
{
public:
	float X, Y, Z, W;

	Quaternion() : X(0), Y(0), Z(0), W(0) { }

	static Quaternion CreateFromYawPitchRoll(float yaw, float pitch, float roll)
	{
		float num = roll * 0.5f;
		float num2 = (float)sin((double)num);
		float num3 = (float)cos((double)num);
		float num4 = pitch * 0.5f;
		float num5 = (float)sin((double)num4);
		float num6 = (float)cos((double)num4);
		float num7 = yaw * 0.5f;
		float num8 = (float)sin((double)num7);
		float num9 = (float)cos((double)num7);
		Quaternion result;
		result.X = num9 * num5 * num3 + num8 * num6 * num2;
		result.Y = num8 * num6 * num3 - num9 * num5 * num2;
		result.Z = num9 * num6 * num2 - num8 * num5 * num3;
		result.W = num9 * num6 * num3 + num8 * num5 * num2;
		return result;
	}
};