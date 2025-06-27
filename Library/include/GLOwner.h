#pragma once

#include <memory>
#include <functional>
#include <GL/freeglut.h>
#include "Viewport.hpp"

namespace vmlab
{
class GLOwner
{
private:
	static Viewport m_lightViewport;
	static Viewport m_displayViewport;
	static Viewport m_wholeViewport;

	static int m_updateMilliSeconds;

	static std::unique_ptr<IAutostereoscopy> m_pautostereocopy;

	static void Display();
	static void Reshape(int x, int y);
	static void Keyboard(unsigned char key, int x, int y);
	static void Special(int key, int x, int y);
	static void Timer(int value);

public:
	static Viewport GetLightViewport(){ return m_lightViewport; }
	static Viewport GetDisplayViewport(){ return m_displayViewport; }
	static Viewport GetWholeViewport(){ return m_wholeViewport; }

	static void Init(int argc, char** argv);
	static void SetViewport(const Viewport& viewport);
	static void Run();
};
}