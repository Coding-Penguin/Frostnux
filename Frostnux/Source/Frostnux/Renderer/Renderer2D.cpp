#include <fxpch.h>
#include "Renderer2D.h"

namespace Frostnux
{

	static GLuint s_VAO = 0, s_VBO = 0, s_EBO = 0;
	static GLuint s_ShaderProgram = 0;
	static std::vector<Vertex2D> s_Vertices;
	static std::vector<GLuint> s_Indices;
	static GLuint s_CurrentTexture = 0;
	static float s_WindowW = 0, s_WindowH = 0;

	const char *vertSrc = R"(
		#version 330 core
		layout (location = 0) in vec2 a_Pos;
		layout (location = 1) in vec2 a_TexCoord;
		layout (location = 2) in vec4 a_Color;
		layout (location = 3) in float a_TexId;
		
		out vec2 v_TexCoord;
		out vec4 v_Color;
		flat out float v_TexId;

		uniform vec2 u_Resolution;

		void main() {
			vec2 ndc;
			ndc.x = (a_Pos.x / u_Resolution.x) * 2.0 - 1.0;
			ndc.y = 1.0 - (a_Pos.y / u_Resolution.y) * 2.0;
			gl_Position = vec4(ndc, 0.0, 1.0);
			v_TexCoord = a_TexCoord;
			v_Color = a_Color;
			v_TexId = a_TexId;
		}
	)";

	const char* fragSrc = R"(
		#version 330 core
		in vec2 v_TexCoord;
		in vec4 v_Color;
		flat in float v_TexId;
		
		out vec4 FragColor;
		uniform sampler2D u_Texture;
		
		void main()
 {
			vec4 texColor = vec4(1.0);
			if (v_TexId > 1.5)
			{
				texColor = texture(u_Texture, v_TexCoord);
			} else if (v_TexId > 0.5)
			{
				float alpha = texture(u_Texture, v_TexCoord).r;
				texColor = vec4(1.0, 1.0, 1.0, alpha);
			}
			FragColor = v_Color * texColor;
		}
	)";

	static GLuint CompileShader(const char *source, GLenum type)
	{
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);
		int success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			FX_CORE_ERROR("Shader error: {}", infoLog);
		}
		return shader;
	}

	void Renderer2D::Init()
	{
		FX_CORE_INFO("Renderer2D: Compiling shaders...");
		GLuint vs = CompileShader(vertSrc, GL_VERTEX_SHADER);
		GLuint fs = CompileShader(fragSrc, GL_FRAGMENT_SHADER);

		s_ShaderProgram = glCreateProgram();
		glAttachShader(s_ShaderProgram, vs);
		glAttachShader(s_ShaderProgram, fs);
		glLinkProgram(s_ShaderProgram);

		GLint success;
		glGetProgramiv(s_ShaderProgram, GL_LINK_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetProgramInfoLog(s_ShaderProgram, 512, nullptr, infoLog);
			FX_CORE_ERROR("Shader Link Failed: {}", infoLog);
			return;
		}
		glDeleteShader(vs);
		glDeleteShader(fs);

		FX_CORE_INFO("Renderer2D: Generating VAO/VBO...");
		glGenVertexArrays(1, &s_VAO);
		glGenBuffers(1, &s_VBO);
		glGenBuffers(1, &s_EBO);

		if (s_VAO == 0 || s_VBO == 0)
		{
			FX_CORE_ERROR("Failed to generate VAO/VBO");
			return;
		}

		glBindVertexArray(s_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, x));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, u));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, r));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, texId));

		glBindVertexArray(0);
		FX_CORE_INFO("Renderer2D: Init success!");
	}

	void Renderer2D::Begin(float width, float height)
	{
		s_WindowW = width;
		s_WindowH = height;
		s_Vertices.clear();
		s_Indices.clear();
		s_CurrentTexture = 0;
	}

	void Renderer2D::End()
	{
		Flush();
	}

	void Renderer2D::DrawQuad(float x, float y, float w, float h, float r, float g, float b, float a)
	{
		if (s_CurrentTexture != 0)
			Flush();

		Vertex2D v0 = {x, y, 0, 0, r, g, b, a, 0.0f};
		Vertex2D v1 = {x + w, y, 0, 0, r, g, b, a, 0.0f};
		Vertex2D v2 = {x + w, y + h, 0, 0, r, g, b, a, 0.0f};
		Vertex2D v3 = {x, y + h, 0, 0, r, g, b, a, 0.0f};

		uint32_t offset = static_cast<uint32_t>(s_Vertices.size());
		s_Vertices.push_back(v0);
		s_Vertices.push_back(v1);
		s_Vertices.push_back(v2);
		s_Vertices.push_back(v3);

		s_Indices.push_back(offset);
		s_Indices.push_back(offset + 1);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 3);
		s_Indices.push_back(offset);
	}

	void Renderer2D::DrawTextQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, GLuint textureId, float r, float g, float b, float a)
	{
		if (s_CurrentTexture != textureId)
		{
			Flush();
			s_CurrentTexture = textureId;
		}

		Vertex2D vertex0 = { x, y, u0, v0, r, g, b, a, 1.0f };
		Vertex2D vertex1 = { x + w, y, u1, v0, r, g, b, a, 1.0f };
		Vertex2D vertex2 = { x + w, y + h, u1, v1, r, g, b, a, 1.0f };
		Vertex2D vertex3 = { x, y + h, u0, v1, r, g, b, a, 1.0f };

		uint32_t offset = static_cast<uint32_t>(s_Vertices.size());
		s_Vertices.push_back(vertex0);
		s_Vertices.push_back(vertex1);
		s_Vertices.push_back(vertex2);
		s_Vertices.push_back(vertex3);

		s_Indices.push_back(offset);
		s_Indices.push_back(offset + 1);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 3);
		s_Indices.push_back(offset);
	}

	void Renderer2D::DrawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a)
	{
		float dx = x2 - x1;
		float dy = y2 - y1;
		float len = std::sqrt(dx * dx + dy * dy);
		if (len == 0)
			return;
		dx /= len;
		dy /= len;

		float nx = -dy * (thickness * 0.5f);
		float ny = dx * (thickness * 0.5f);

		if (s_CurrentTexture != 0)
			Flush();

		Vertex2D v0 = {x1 + nx, y1 + ny, 0, 0, r, g, b, a, 0.0f};
		Vertex2D v1 = {x1 - nx, y1 - ny, 0, 0, r, g, b, a, 0.0f};
		Vertex2D v2 = {x2 - nx, y2 - ny, 0, 0, r, g, b, a, 0.0f};
		Vertex2D v3 = {x2 + nx, y2 + ny, 0, 0, r, g, b, a, 0.0f};

		uint32_t offset = static_cast<uint32_t>(s_Vertices.size());
		s_Vertices.push_back(v0);
		s_Vertices.push_back(v1);
		s_Vertices.push_back(v2);
		s_Vertices.push_back(v3);

		s_Indices.push_back(offset);
		s_Indices.push_back(offset + 1);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 3);
		s_Indices.push_back(offset);
	}

	void Renderer2D::Flush()
	{
		if (s_Vertices.empty())
			return;

		glUseProgram(s_ShaderProgram);
		glUniform2f(glGetUniformLocation(s_ShaderProgram, "u_Resolution"), s_WindowW, s_WindowH);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (s_CurrentTexture != 0)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, s_CurrentTexture);
			glUniform1i(glGetUniformLocation(s_ShaderProgram, "u_Texture"), 0);
		}

		glBindVertexArray(s_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
		glBufferData(GL_ARRAY_BUFFER, s_Vertices.size() * sizeof(Vertex2D), s_Vertices.data(), GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, s_Indices.size() * sizeof(GLuint), s_Indices.data(), GL_STREAM_DRAW);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(s_Indices.size()), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		s_Vertices.clear();
		s_Indices.clear();
		s_CurrentTexture = 0;
	}

	void Renderer2D::DrawCircle(float cx, float cy, float radius, float r, float g, float b, float a)
	{
		if (s_CurrentTexture != 0) Flush();

		const int segments = 64;
		uint32_t offset = static_cast<uint32_t>(s_Vertices.size());

		s_Vertices.push_back({ cx, cy, 0, 0, r, g, b, a, 0.0f });

		for (int i = 0; i <= segments; ++i)
		{
			float angle = (float)i / segments * 3.1415926535f * 2.0f;
			float x = cx + std::cos(angle) * radius;
			float y = cy + std::sin(angle) * radius;
			s_Vertices.push_back({ x, y, 0, 0, r, g, b, a, 0.0f });
		}

		for (int i = 1; i <= segments; ++i)
		{
			s_Indices.push_back(offset);
			s_Indices.push_back(offset + i);
			s_Indices.push_back(offset + i + 1);
		}
	}
	void Renderer2D::DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a)
	{
		if (s_CurrentTexture != 0) Flush();

		Vertex2D vertex0 = { x1, y1, 0, 0, r, g, b, a, 0.0f };
		Vertex2D vertex1 = { x2, y2, 0, 0, r, g, b, a, 0.0f };
		Vertex2D vertex2 = { x3, y3, 0, 0, r, g, b, a, 0.0f };

		uint32_t offset = static_cast<uint32_t>(s_Vertices.size());
		s_Vertices.push_back(vertex0); s_Vertices.push_back(vertex1); s_Vertices.push_back(vertex2);

		s_Indices.push_back(offset); s_Indices.push_back(offset + 1); s_Indices.push_back(offset + 2);
	}

	void Renderer2D::DrawImageQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, GLuint textureId, float r, float g, float b, float a)
	{
		if (s_CurrentTexture != textureId)
		{
			Flush();
			s_CurrentTexture = textureId;
		}

		Vertex2D vertex0 = { x, y, u0, v0, r, g, b, a, 2.0f };
		Vertex2D vertex1 = { x + w, y, u1, v0, r, g, b, a, 2.0f };
		Vertex2D vertex2 = { x + w, y + h, u1, v1, r, g, b, a, 2.0f };
		Vertex2D vertex3 = { x, y + h, u0, v1, r, g, b, a, 2.0f };

		uint32_t offset = static_cast<uint32_t>(s_Vertices.size());
		s_Vertices.push_back(vertex0);
		s_Vertices.push_back(vertex1);
		s_Vertices.push_back(vertex2);
		s_Vertices.push_back(vertex3);

		s_Indices.push_back(offset);
		s_Indices.push_back(offset + 1);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 2);
		s_Indices.push_back(offset + 3);
		s_Indices.push_back(offset);
	}

}
