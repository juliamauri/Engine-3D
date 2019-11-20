#include "RE_InternalResources.h"

#include "Application.h"

#include "FileSystem.h"
#include "ResourceManager.h"

#include "ShaderManager.h"
#include "RE_TextureImporter.h"

#include "RE_SkyBox.h"

#include "OutputLog.h"

#include "Glew/include/glew.h"
#include <gl/GL.h>

#include <vector>
#include <string>

#define SKYBOXDEFSIZE 2000.0f

RE_InternalResources::RE_InternalResources()
{
}

RE_InternalResources::~RE_InternalResources()
{
	if(checkerTexture != 0) glDeleteTextures(1, &checkerTexture);
}

bool RE_InternalResources::Init()
{
	bool ret = true;

	ret = InitShaders();

	ret = InitChecker();

	ret = InitSkyBox();

	return false;
}

bool RE_InternalResources::InitShaders()
{
	bool ret = true;

	//Loading Shaders
	if (App->shaders)
	{
		if (!App->shaders->Load("default", &defaultShader, true)) {
			LOG("%s\n", App->shaders->GetShaderError());
			ret = false;
		}

		if (!App->shaders->Load("skybox", &skyboxShader, true)) {
			LOG("%s\n", App->shaders->GetShaderError());
			ret = false;
		}
	}

	return ret;
}

bool RE_InternalResources::InitChecker()
{
	// Checkers
	int value;
	int IMAGE_ROWS = 264;
	int IMAGE_COLS = 264;
	GLubyte imageData[264][264][3];
	for (int row = 0; row < IMAGE_ROWS; row++) {
		for (int col = 0; col < IMAGE_COLS; col++) {
			// Each cell is 8x8, value is 0 or 255 (black or white)
			value = (((row & 0x8) == 0) ^ ((col & 0x8) == 0)) * 255;
			imageData[row][col][0] = (GLubyte)value;
			imageData[row][col][1] = (GLubyte)value;
			imageData[row][col][2] = (GLubyte)value;
		}
	}
	
	glGenTextures(1, &checkerTexture);
	glBindTexture(GL_TEXTURE_2D, checkerTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, IMAGE_COLS, IMAGE_ROWS, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

bool RE_InternalResources::InitSkyBox()
{
	defaultSkybox = new RE_SkyBox();
	defaultSkybox->SetName("defaultSkyBox");
	defaultSkybox->SetType(Resource_Type::R_SKYBOX);
	defaultSkybox->AddTexture(RE_TextureFace::RE_RIGHT, "Assets/Skyboxes/default/1right.jpg");
	defaultSkybox->AddTexture(RE_TextureFace::RE_LEFT, "Assets/Skyboxes/default/2left.jpg");
	defaultSkybox->AddTexture(RE_TextureFace::RE_TOP, "Assets/Skyboxes/default/3top.jpg");
	defaultSkybox->AddTexture(RE_TextureFace::RE_BOTTOM, "Assets/Skyboxes/default/4bottom.jpg");
	defaultSkybox->AddTexture(RE_TextureFace::RE_FRONT, "Assets/Skyboxes/default/5front.jpg");
	defaultSkybox->AddTexture(RE_TextureFace::RE_BACK, "Assets/Skyboxes/default/6back.jpg");
	defaultSkybox->AssetSave();
	defaultSkybox->SaveMeta();
	return true;
}

unsigned int RE_InternalResources::GetDefaultShader() const
{
	return defaultShader;
}

unsigned int RE_InternalResources::GetSkyBoxShader() const
{
	return skyboxShader;
}

unsigned int RE_InternalResources::GetTextureChecker() const
{
	return checkerTexture;
}

unsigned int RE_InternalResources::GetSkyBoxVAO() const
{
	return defaultSkybox->GetVAO();
}

unsigned int RE_InternalResources::GetSkyBoxTexturesID() const
{
	return defaultSkybox->GetID();
}
