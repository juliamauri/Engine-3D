#include "RE_PrimitiveManager.h"

#include "Application.h"
#include "RE_ShaderImporter.h"
#include "RE_ResourceManager.h"
#include "RE_InternalResources.h"
#include "OutputLog.h"
#include "RE_Math.h"

#include "RE_CompPrimitive.h"

#include "RE_GLCache.h"
#include "RE_Shader.h"

#include "SDL2/include/SDL.h"
#include "Glew/include/glew.h"
#include <gl/GL.h>

#ifndef PAR_SHAPES_IMPLEMENTATION
#define PAR_SHAPES_IMPLEMENTATION
#endif

#include "par_shapes.h"


#define CUBE_TRIANGLES 36

RE_PrimitiveManager::RE_PrimitiveManager()
{
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_POINT, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_LINE, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_RAY, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_AXIS, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_TRIANGLE, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_GRID, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_CUBE, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_FUSTRUM, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_SPHERE, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_CYLINDER, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_HEMISHPERE, 0));
	primitives_count.insert(eastl::pair<ComponentType, unsigned int>(C_PLANE, 0));
}

RE_PrimitiveManager::~RE_PrimitiveManager()
{
	glDeleteVertexArrays(1, &(GLuint)vao_cube);
	glDeleteBuffers(1, &(GLuint)vbo_cube);
	glDeleteBuffers(1, &(GLuint)ebo_cube);
}

RE_CompPrimitive * RE_PrimitiveManager::CreateAxis(RE_GameObject* go)
{
	if(primitives_count.find(C_AXIS)->second++ == 0)
	{

	}
	RE_CompPrimitive* ret = new RE_CompAxis(go, vao_axis);
	return ret;
}

RE_CompPrimitive* RE_PrimitiveManager::CreatePoint(RE_GameObject* game_obj, math::vec pos)
{
	if (primitives_count.find(C_POINT)->second++ == 0)
	{
		math::vec p(0.0f, 0.0f, 0.0f);

		glGenVertexArrays(1, &vao_point);
		glGenBuffers(1, &vbo_point);

		RE_GLCache::ChangeVAO(vao_point);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_point);
		glBufferData(GL_ARRAY_BUFFER, sizeof(p), &p, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	RE_CompPoint* ret = new RE_CompPoint(game_obj, vao_point, shaderPrimitive, pos);
	return ret;
}

RE_CompPrimitive * RE_PrimitiveManager::CreateLine(RE_GameObject* game_obj, math::vec origin, math::vec end)
{
	if (primitives_count.find(C_LINE)->second++ == 0)
	{
		math::vec vecline[] = {
			origin,
			end
		};

		glGenVertexArrays(1, &vao_line);
		glGenBuffers(1, &vbo_line);

		RE_GLCache::ChangeVAO(vao_line);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_line);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vecline) * sizeof(math::vec), &vecline[0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(vecline), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	RE_CompLine* ret = new RE_CompLine(game_obj, vao_line, shaderPrimitive,origin, end);
	return ret;
}

RE_CompPrimitive * RE_PrimitiveManager::CreateRay(RE_GameObject* game_obj)
{
	if (primitives_count.find(C_RAY)->second++ == 0)
	{

	}
	RE_CompPrimitive* ret = new RE_CompRay(game_obj, vao_ray);
	return ret;
}

RE_CompPrimitive * RE_PrimitiveManager::CreateTriangle(RE_GameObject* game_obj)
{
	if (primitives_count.find(C_TRIANGLE)->second++ == 0)
	{
		math::vec vPositionTriangle[] = {
			// positions       
			math::vec(1.0f, -1.0f, 0.0f),  // bottom right
			math::vec(-1.0f, -1.0f, 0.0f),   // bottom left
			math::vec(0.0f,  1.0f, 0.0f)      // top 
		};

		unsigned int index[] = { 0,1,2 };

		glGenVertexArrays(1, &vao_triangle);
		glGenBuffers(1, &vbo_triangle);
		glGenBuffers(1, &ebo_triangle);

		RE_GLCache::ChangeVAO(vao_triangle);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vPositionTriangle) * sizeof(math::vec), &vPositionTriangle[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_triangle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(vPositionTriangle), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	}
	RE_CompTriangle* ret = new RE_CompTriangle(game_obj, vao_triangle, shaderPrimitive);
	return ret;
}

RE_CompPrimitive * RE_PrimitiveManager::CreateGrid(RE_GameObject* game_obj)
{
	if (primitives_count.find(C_GRID)->second++ == 0)
	{
		eastl::vector<float> vertices;
		for (float f = 0.f; f < 50.f; f += 0.5f)
		{
			vertices.push_back((f * 5.f) - 125.f);
			vertices.push_back(0.f);
			vertices.push_back(125.f);
			vertices.push_back((f * 5.f) - 125.f);
			vertices.push_back(0.f);
			vertices.push_back(-125.f);
			vertices.push_back(125.f);
			vertices.push_back(0.f);
			vertices.push_back((f * 5.f) - 125.f);
			vertices.push_back(-125.f);
			vertices.push_back(0.f);
			vertices.push_back((f * 5.f) - 125.f);
		}

		glGenVertexArrays(1, &vao_grid);
		glGenBuffers(1, &vbo_grid);

		RE_GLCache::ChangeVAO(vao_grid);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_grid);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		RE_GLCache::ChangeVAO(vao_grid);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	RE_CompPrimitive* ret = new RE_CompGrid(game_obj, vao_grid, shaderPrimitive);
	return ret;
}

RE_CompPrimitive* RE_PrimitiveManager::CreatePlane(RE_GameObject* game_obj, int slices, int stacks)
{
	return new RE_CompPlane(game_obj, shaderPrimitive, slices, stacks);
}

RE_CompPrimitive * RE_PrimitiveManager::CreateCube(RE_GameObject* game_obj)
{
	return new RE_CompCube(game_obj, vao_cube, shaderPrimitive, CUBE_TRIANGLES);
}

RE_CompPrimitive * RE_PrimitiveManager::CreateFustrum(RE_GameObject* game_obj)
{
	if (primitives_count.find(C_FUSTRUM)->second++ == 0)
	{

	}
	RE_CompPrimitive* ret = new RE_CompFustrum(game_obj, vao_fustrum, shaderPrimitive);
	return ret;
}

RE_CompPrimitive * RE_PrimitiveManager::CreateSphere(RE_GameObject* game_obj, int slices, int stacks)
{
	return new RE_CompSphere(game_obj, shaderPrimitive, slices, stacks);
}

RE_CompPrimitive * RE_PrimitiveManager::CreateCylinder(RE_GameObject* game_obj, int slices, int stacks)
{
	return new RE_CompCylinder(game_obj, shaderPrimitive, slices, stacks);
}

RE_CompPrimitive * RE_PrimitiveManager::CreateHemiSphere(RE_GameObject* game_obj, int slices, int stacks)
{
	return new RE_CompHemiSphere(game_obj, shaderPrimitive, slices, stacks);
}

RE_CompPrimitive* RE_PrimitiveManager::CreateTorus(RE_GameObject* game_obj, int slices, int stacks, float radius)
{
	return new RE_CompTorus(game_obj, shaderPrimitive, slices, stacks, radius);
}

unsigned int RE_PrimitiveManager::CheckCubeVAO()
{
	if (vao_cube == 0) {
		par_shapes_mesh* cube = par_shapes_create_cube();
		par_shapes_unweld(cube, true);
		par_shapes_compute_normals(cube);

		float* points = new float[cube->npoints * 3];
		float* normals = new float[cube->npoints * 3];
		//float* texCoords = new float[cube->npoints * 2];

		uint meshSize = 0;
		size_t size = cube->npoints * 3 * sizeof(float);
		uint stride = 0;

		memcpy(points, cube->points, size);
		meshSize += 3 * cube->npoints;
		stride += 3;

		memcpy(normals, cube->normals, size);
		meshSize += 3 * cube->npoints;
		stride += 3;

		//size = cube->npoints * 2 * sizeof(float);
		//memcpy(texCoords, cube->tcoords, size);
		//meshSize += 2 * cube->npoints;
		//stride += 2;

		stride *= sizeof(float);
		float* meshBuffer = new float[meshSize];
		float* cursor = meshBuffer;
		for (uint i = 0; i < cube->npoints; i++) {
			uint cursorSize = 3;
			size_t size = sizeof(float) * 3;

			memcpy(cursor, &points[i * 3], size);
			cursor += cursorSize;

			memcpy(cursor, &normals[i * 3], size);
			cursor += cursorSize;

			//cursorSize = 2;
			//size = sizeof(float) * 2;
			//memcpy(cursor, &texCoords[i * 2], size);
			//cursor += cursorSize;
		}


		glGenVertexArrays(1, &vao_cube);
		RE_GLCache::ChangeVAO(vao_cube);

		glGenBuffers(1, &vbo_cube);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cube);
		glBufferData(GL_ARRAY_BUFFER, meshSize * sizeof(float), meshBuffer, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, NULL);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));

		//glEnableVertexAttribArray(4);
		//glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));

		glGenBuffers(1, &ebo_cube);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_cube);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube->ntriangles * sizeof(unsigned short) * 3, cube->triangles, GL_STATIC_DRAW);


		par_shapes_free_mesh(cube);
		DEL_A(points);
		DEL_A(normals);
		//DEL_A(texCoords);
		DEL_A(meshBuffer);

	}
	return vao_cube;
}

void RE_PrimitiveManager::Rest(unsigned short int count)
{
	primitives_count.find(count)->second--;
	if (primitives_count.find(count)->second == 0)
		DeleteVAOPrimitive(count);
}

bool RE_PrimitiveManager::Init(const char* def_shader)
{
	
	shaderPrimitive = ((RE_Shader*)App->resources->At(App->internalResources->GetDefaultShader()))->GetID();

	CheckCubeVAO();

	App->ReportSoftware("par_shapes.h", nullptr, "https://github.com/prideout/par");

	return true;
}

void RE_PrimitiveManager::DeleteVAOPrimitive(unsigned short int primitive)
{
	switch (primitive)
	{
	case C_AXIS:
		glDeleteVertexArrays(1, &(GLuint)vao_axis);
		glDeleteBuffers(1, &(GLuint)vbo_axis);
		break;

	case C_POINT:
		glDeleteVertexArrays(1, &(GLuint)vao_point);
		glDeleteBuffers(1, &(GLuint)vbo_point);
		break;

	case C_LINE:
		glDeleteVertexArrays(1, &(GLuint)vao_line);
		glDeleteBuffers(1, &(GLuint)vbo_line);
		break;

	case C_RAY:
		glDeleteVertexArrays(1, &(GLuint)vao_ray);
		glDeleteBuffers(1, &(GLuint)vbo_ray);
		break;

	case C_TRIANGLE:
		glDeleteVertexArrays(1, &(GLuint)vao_triangle);
		glDeleteBuffers(1, &(GLuint)vbo_triangle);
		glGenBuffers(1, &(GLuint)ebo_triangle);
		break;

	case C_GRID:
		glDeleteVertexArrays(1, &(GLuint)vao_grid);
		glDeleteBuffers(1, &(GLuint)vbo_grid);
		break;
	case C_CUBE:

		break;

	case C_FUSTRUM:
		glDeleteVertexArrays(1, &(GLuint)vao_fustrum);
		glDeleteBuffers(1, &(GLuint)vbo_fustrum);
		glDeleteBuffers(1, &(GLuint)ebo_fustrum);
		break;

	case C_CYLINDER:
		glDeleteVertexArrays(1, &(GLuint)vao_cylinder);
		glDeleteBuffers(1, &(GLuint)vbo_cylinder);
		glDeleteBuffers(1, &(GLuint)ebo_cylinder);
		break;

	case C_HEMISHPERE:
		glDeleteVertexArrays(1, &(GLuint)vao_capsule);
		glDeleteBuffers(1, &(GLuint)vbo_capsule);
		glDeleteBuffers(1, &(GLuint)ebo_capsule);
		break;
	}
}
