#include "Shader.h"

#include "ModuleEditor.h"
#include "SDL2/include/SDL.h"
#include "Glew/include/glew.h"
#include "FileSystem.h"

Shader::Shader(const char * vertexPath, const char * fragmentPath)
{
	const char* buffer = nullptr;

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	//load source
	RE_FileIO file_vertexShader(vertexPath);
	if (file_vertexShader.Load())
		buffer = file_vertexShader.GetBuffer();
	glShaderSource(vertexShader, 1, &buffer, NULL);
	glCompileShader(vertexShader);

	//check
	int  success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		LOG("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n", infoLog);
	}

	//compiling fragment shader
	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	//load source
	RE_FileIO file_fragmentShader(fragmentPath);
	if (file_fragmentShader.Load())
		buffer = file_fragmentShader.GetBuffer();
	glShaderSource(fragmentShader, 1, &buffer, NULL);
	glCompileShader(fragmentShader);

	//check
	int  success2;
	char infoLog2[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success2);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog2);
		LOG("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s\n", infoLog2);
	}

	//creating Shader program, link the once shaders types
	ID = glCreateProgram();

	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	glLinkProgram(ID);

	//check
	int success3;
	char infoLog3[512];
	glGetProgramiv(ID, GL_LINK_STATUS, &success3);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog3);
		LOG("ERROR::SHADER::PROGRAM::COMPILATION_FAILED: %s\n", infoLog3);
	}

	//deleting shaders, no needed after link
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::use()
{
	glUseProgram(ID);
}

void Shader::setBool(const char* name, bool value) const
{
	glUniform1i(glGetUniformLocation(ID, name), (int)value);
}

void Shader::setBool(const char* name, bool value, bool value2) const
{
	glUniform2i(glGetUniformLocation(ID, name), (int)value, (int)value2);
}

void Shader::setBool(const char* name, bool value, bool value2, bool value3) const
{
	glUniform3i(glGetUniformLocation(ID, name), (int)value, (int)value2, (int)value3);
}

void Shader::setBool(const char* name, bool value, bool value2, bool value3, bool value4) const
{
	glUniform4i(glGetUniformLocation(ID, name), (int)value, (int)value2, (int)value3, (int)value4);
}

void Shader::setInt(const char* name, int value) const
{
	glUniform1i(glGetUniformLocation(ID, name), value);
}

void Shader::setInt(const char * name, int value, int value2) const
{
	glUniform2i(glGetUniformLocation(ID, name), value, value2);
}

void Shader::setInt(const char * name, int value, int value2, int value3) const
{
	glUniform3i(glGetUniformLocation(ID, name), value, value2, value3);
}

void Shader::setInt(const char * name, int value, int value2, int value3, int value4) const
{
	glUniform4i(glGetUniformLocation(ID, name), value, value2, value3, value4);
}

void Shader::setFloat(const char*name, float value) const
{
	glUniform1f(glGetUniformLocation(ID, name), value);
}

void Shader::setFloat(const char * name, float value, float value2) const
{
	glUniform2f(glGetUniformLocation(ID, name), value, value2);
}

void Shader::setFloat(const char * name, float value, float value2, float value3) const
{
	glUniform3f(glGetUniformLocation(ID, name), value, value2, value3);
}

void Shader::setFloat(const char * name, float value, float value2, float value3, float value4) const
{
	glUniform4f(glGetUniformLocation(ID, name), value, value2, value3, value4);
}

void Shader::setUnsignedInt(const char * name, unsigned int value)
{
	glUniform1ui(glGetUniformLocation(ID, name), value);
}

void Shader::setUnsignedInt(const char * name, unsigned int value, unsigned int value2)
{
	glUniform2ui(glGetUniformLocation(ID, name), value, value2);
}

void Shader::setUnsignedInt(const char * name, unsigned int value, unsigned int value2, unsigned int value3)
{
	glUniform3ui(glGetUniformLocation(ID, name), value, value2, value3);
}

void Shader::setUnsignedInt(const char * name, unsigned int value, unsigned int value2, unsigned int value3, unsigned int value4)
{
	glUniform4ui(glGetUniformLocation(ID, name), value, value2, value3, value4);
}

void Shader::setFloat4x4(const char * name, math::float4x4* trans)
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, trans->ptr());
}