#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace Frostnux
{

	struct Vertex2D
	{
		float x, y;
		float u, v;
		float r, g, b, a;
		float texId;
	};

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();
		static void Begin(float screenWidth, float screenHeight);
		static void End();

		static void DrawQuad(float x, float y, float w, float h, float r, float g, float b, float a);
		static void DrawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a);
		static void DrawTextQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, GLuint textureId, float r, float g, float b, float a);
		static void DrawCircle(float cx, float cy, float radius, float r, float g, float b, float a);
		static void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a);

		static void DrawImageQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, GLuint textureId, float r, float g, float b, float a);
	private:
		static void Flush();
	};
}