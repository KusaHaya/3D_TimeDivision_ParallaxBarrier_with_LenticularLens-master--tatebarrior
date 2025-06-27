#pragma once

namespace vmlab
{
struct Viewport
{
	int X;
	int Y;
	int Width;
	int Height;

	Viewport() : X(0), Y(0), Width(0), Height(0) { }
	Viewport(int x, int y, int width, int height) : X(x), Y(y), Width(width), Height(height) { }
};
}