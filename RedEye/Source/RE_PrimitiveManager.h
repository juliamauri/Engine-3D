#ifndef __RE_PRIMITVEMANAGER_H__
#define __RE_PRIMITVEMANAGER_H__

#include <map>

#include "MathGeoLib/include/Math/float3.h"

class RE_GameObject;
class RE_CompPrimitive;


class RE_PrimitiveManager
{
public:
	RE_PrimitiveManager();
	~RE_PrimitiveManager();

	bool Init(const char* def_shader);

	 //Create
	RE_CompPrimitive* CreateAxis(RE_GameObject* game_obj);
	RE_CompPrimitive* CreatePoint(RE_GameObject* game_obj, math::vec pos);
	RE_CompPrimitive* CreateLine(RE_GameObject* game_obj, math::vec origin, math::vec end);
	RE_CompPrimitive* CreateRay(RE_GameObject* game_obj);
	RE_CompPrimitive* CreateTriangle(RE_GameObject* game_obj);
	RE_CompPrimitive* CreateGrid(RE_GameObject* game_obj);
	RE_CompPrimitive* CreatePlane(RE_GameObject* game_obj, int slices = 3, int stacks = 3);
	RE_CompPrimitive* CreateCube(RE_GameObject* game_obj);
	RE_CompPrimitive* CreateFustrum(RE_GameObject* game_obj);
	RE_CompPrimitive* CreateSphere(RE_GameObject* game_obj, int slices  = 16, int stacks = 18);
	RE_CompPrimitive* CreateCylinder(RE_GameObject* game_obj);
	RE_CompPrimitive* CreateCapsule(RE_GameObject* game_obj);

	//Check
	unsigned int CheckCubeVAO();

	//count
	void Rest(unsigned short int count);

private:
	//Vertex Array Object
	unsigned int vao_point = 0, vao_line = 0, vao_ray = 0, vao_axis = 0, vao_triangle = 0, vao_grid = 0,
		vao_cube = 0, vao_fustrum = 0, vao_cylinder = 0, vao_capsule = 0;
	
	//Vertex Buffer Object
	unsigned int vbo_point = 0, vbo_line = 0, vbo_ray = 0, vbo_axis = 0, vbo_triangle = 0, vbo_grid = 0,
		vbo_cube = 0, vbo_fustrum = 0, vbo_cylinder = 0, vbo_capsule = 0;

	//Element Buffer Objects
	unsigned int ebo_triangle = 0, ebo_cube = 0, ebo_fustrum = 0, ebo_cylinder = 0, ebo_capsule = 0;

	//Primitives Count
	std::map<unsigned short int, unsigned int> primitives_count;

	//Delete VAO of primitive when its count is 0
	void DeleteVAOPrimitive(unsigned short int primitive);

	//Shader for primitives
	unsigned int shaderPrimitive = 0;
};

#endif // !__RE_PRIMITVEMANAGER_H__#
