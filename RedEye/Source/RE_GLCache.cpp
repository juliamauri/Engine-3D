#include "RE_GLCache.h"

#include "RE_ShaderImporter.h"

#include "Glew/include/glew.h"

void RE_GLCache::ChangeShader(unsigned int ID)
{
	static unsigned int currentShaderID = 0;
	if (currentShaderID != ID) RE_ShaderImporter::use(currentShaderID = ID);
}

void RE_GLCache::ChangeVAO(unsigned int VAO)
{
	static unsigned int currenVAO = 0;
	if (currenVAO != VAO) glBindVertexArray(currenVAO = VAO);
}