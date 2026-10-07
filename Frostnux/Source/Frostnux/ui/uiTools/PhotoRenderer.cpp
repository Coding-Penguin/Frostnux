#include <fxpch.h>
#include "PhotoRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/glad.h>
#include "Frostnux/Core/Log.h"

namespace Frostnux {

	PhotoRenderer::PhotoRenderer()
	{
	}

	PhotoRenderer::~PhotoRenderer() 
	{
		Unload();
	}

	bool PhotoRenderer::LoadFromFile(const std::string& filepath)
	{
		Unload();

		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(filepath.c_str(), &m_Width, &m_Height, &m_Channels, 4);
		if (!data)
		{
			FX_CORE_ERROR("Failed to load image: {}", filepath);
			return false;
		}

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);

		FX_CORE_INFO("Loaded image: {} ({} x {})", filepath, m_Width, m_Height);
		return true;
	}

	void PhotoRenderer::Unload()
	{
		if (m_TextureID) 
		{
			glDeleteTextures(1, &m_TextureID);
			m_TextureID = 0;
		}
		m_Width = m_Height = m_Channels = 0;
	}

	void PhotoRenderer::Draw(float x, float y, float width, float height) const
	{
		if (!IsLoaded()) return;

		GLboolean texWasEnabled = glIsEnabled(GL_TEXTURE_2D);
		GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
		GLint blendSrc, blendDst;
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		float verts[] =
		{
			// x,		y,			s,		t
			x,			y,			0.0f,	1.0f,
			x + width,	y,			1.0f,	1.0f,
			x + width,	y + height,	1.0f,	0.0f,
			x,			y + height,	0.0f,	0.0f
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), verts);
		glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), verts + 2);

		glDrawArrays(GL_QUADS, 0, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		if (!texWasEnabled) glDisable(GL_TEXTURE_2D);
		if (!blendWasEnabled) glDisable(GL_BLEND);
		glBlendFunc(blendSrc, blendDst);
	}

}
