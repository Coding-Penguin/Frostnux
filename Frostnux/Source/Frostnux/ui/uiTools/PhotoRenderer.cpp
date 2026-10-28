#include <fxpch.h>
#include "PhotoRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Frostnux/Core/Log.h"
#include "Frostnux/Renderer/Renderer2D.h"

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
		Renderer2D::DrawImageQuad(x, y, width, height, 0.0f, 1.0f, 1.0f, 0.0f, m_TextureID, 1.0f, 1.0f, 1.0f, 1.0f);
	}

}
