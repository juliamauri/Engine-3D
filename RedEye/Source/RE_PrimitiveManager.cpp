#include "RE_PrimitiveManager.h"

#include "Application.h"
#include "RE_CompPrimitive.h"
#include "RE_GLCache.h"

#include "par_shapes.h"
#include "SDL2/include/SDL.h"
#include "Glew/include/glew.h"
#include <gl/GL.h>

RE_PrimitiveManager::RE_PrimitiveManager()
{
	platonics = new PrimData[5];
}

RE_PrimitiveManager::~RE_PrimitiveManager()
{
	DEL(platonics);
}

void RE_PrimitiveManager::Init()
{
	RE_SOFT_NS("par_shapes.h", "https://github.com/prideout/par");
}

void RE_PrimitiveManager::Clear() { }

void RE_PrimitiveManager::SetUpComponentPrimitive(RE_CompPrimitive* cmpP)
{
	ComponentType type = cmpP->GetType();
	switch (type) {
		// Grid
	case C_GRID: dynamic_cast<RE_CompGrid*>(cmpP)->GridSetUp(50); break;
		// Platonics
	case C_CUBE: dynamic_cast<RE_CompPlatonic*>(cmpP)->PlatonicSetUp(); break;
	case C_DODECAHEDRON: dynamic_cast<RE_CompPlatonic*>(cmpP)->PlatonicSetUp(); break;
	case C_TETRAHEDRON: dynamic_cast<RE_CompPlatonic*>(cmpP)->PlatonicSetUp(); break;
	case C_OCTOHEDRON: dynamic_cast<RE_CompPlatonic*>(cmpP)->PlatonicSetUp(); break;
	case C_ICOSAHEDRON: dynamic_cast<RE_CompPlatonic*>(cmpP)->PlatonicSetUp(); break;
		// Parametrics
	case C_PLANE: dynamic_cast<RE_CompParametric*>(cmpP)->ParametricSetUp(3, 3); break;
	case C_FUSTRUM: break;
	case C_SPHERE: dynamic_cast<RE_CompParametric*>(cmpP)->ParametricSetUp(16, 18); break;
	case C_CYLINDER: dynamic_cast<RE_CompParametric*>(cmpP)->ParametricSetUp(30, 3); break;
	case C_HEMISHPERE: dynamic_cast<RE_CompParametric*>(cmpP)->ParametricSetUp(10, 10); break;
	case C_TORUS: dynamic_cast<RE_CompParametric*>(cmpP)->ParametricSetUp(30, 40, 0.1f); break;
	case C_TREFOILKNOT: dynamic_cast<RE_CompParametric*>(cmpP)->ParametricSetUp(30, 40, 0.5f); break;
		// Rock
	case C_ROCK: dynamic_cast<RE_CompRock*>(cmpP)->RockSetUp(5, 20); break; }
}

eastl::pair<unsigned int, unsigned int> RE_PrimitiveManager::GetPrimitiveMeshData(RE_Component* primComp, int id)
{
	ComponentType pType = primComp->GetType();

	switch (pType)
	{
	case C_CUBE:
	case C_DODECAHEDRON:
	case C_TETRAHEDRON:
	case C_OCTOHEDRON:
	case C_ICOSAHEDRON:
	{
		unsigned short index = pType - C_CUBE;

		if (!platonics[index].refCount)
		{
			par_shapes_mesh* mesh = nullptr;
			switch (pType) {
			case C_CUBE: mesh = par_shapes_create_cube(); break;
			case C_DODECAHEDRON: mesh = par_shapes_create_dodecahedron(); break;
			case C_TETRAHEDRON: mesh = par_shapes_create_tetrahedron(); break;
			case C_OCTOHEDRON: mesh = par_shapes_create_octahedron(); break;
			case C_ICOSAHEDRON: mesh = par_shapes_create_icosahedron(); break;
			}

			UploadPlatonic(platonics[index], mesh);
			par_shapes_free_mesh(mesh);
		}

		platonics[index].refCount += 1;
		return { platonics[index].vao, platonics[index].triangles };
	}
	}

	auto pIter = primReference.find(pType);
	if (pIter != primReference.end()) {
		auto iIter = pIter->second.find(id);
		if (iIter != pIter->second.end()) {
			iIter->second.refCount += 1;
			return { iIter->second.vao, iIter->second.triangles };
		}
	}

	if (pIter == primReference.end())primReference.insert({ pType, {} });
	primReference.at(pType).insert({ id, PrimData() });

	PrimData& newPrim = primReference.at(pType).at(id);
	newPrim.refCount += 1;

	par_shapes_mesh* pMesh = nullptr;
	RE_CompParametric* cP = nullptr;
	switch (pType)
	{
	case C_GRID:
		GenerateGrid(newPrim, dynamic_cast<RE_CompGrid*>(primComp));
		return { newPrim.vao, newPrim.triangles };
	case C_ROCK:
		GenerateRock(newPrim, dynamic_cast<RE_CompRock*>(primComp));
		return { newPrim.vao, newPrim.triangles };
	case C_PLANE:
		cP = dynamic_cast<RE_CompParametric*>(primComp);
		pMesh = par_shapes_create_plane(cP->slices, cP->stacks);
		break;
	case C_SPHERE:
		cP = dynamic_cast<RE_CompParametric*>(primComp);
		pMesh = par_shapes_create_parametric_sphere(cP->slices, cP->stacks);
		break;
	case C_CYLINDER:
		cP = dynamic_cast<RE_CompParametric*>(primComp);
		pMesh = par_shapes_create_cylinder(cP->slices, cP->stacks);
		break;
	case C_HEMISHPERE:
		cP = dynamic_cast<RE_CompParametric*>(primComp);
		pMesh = par_shapes_create_hemisphere(cP->slices, cP->stacks);
		break;
	case C_TORUS:
		cP = dynamic_cast<RE_CompParametric*>(primComp);
		pMesh = par_shapes_create_torus(cP->slices, cP->stacks, cP->radius);
		break;
	case C_TREFOILKNOT:
		cP = dynamic_cast<RE_CompParametric*>(primComp);
		pMesh = par_shapes_create_trefoil_knot(cP->slices, cP->stacks, cP->radius);
		break;
	}

	UploadParametric(newPrim, pMesh);
	par_shapes_free_mesh(pMesh);

	return { newPrim.vao, newPrim.triangles };
}

void RE_PrimitiveManager::UnUsePrimitive(unsigned short pType, int id)
{
	switch (pType)
	{
	case C_CUBE:
	case C_DODECAHEDRON:
	case C_TETRAHEDRON:
	case C_OCTOHEDRON:
	case C_ICOSAHEDRON:
	{
		unsigned short index = pType - C_CUBE;
		platonics[index].refCount -= 1;

		if (platonics[index].refCount == 0) {
			glDeleteVertexArrays(1, &platonics[index].vao);
			glDeleteBuffers(1, &platonics[index].vbo);
			glDeleteBuffers(1, &platonics[index].ebo);
		}
		return;
	}
	}

	PrimData& primD = primReference.at(pType).at(id);
	primD.refCount -= 1;
	if (primD.refCount == 0) {
		if(primD.vao) glDeleteVertexArrays(1, &primD.vao);
		if (primD.vbo) glDeleteBuffers(1, &primD.vbo);
		if (primD.ebo) glDeleteBuffers(1, &primD.ebo);

		primReference.at(pType).erase(id);
	}
}

void RE_PrimitiveManager::CreateSphere(int slices, int stacks, unsigned int& vao, unsigned int& vbo, unsigned int& ebo, unsigned int& triangles)
{
	if (slices < 3) slices = 3;
	if (stacks < 3) stacks = 3;

	par_shapes_mesh* sphere = par_shapes_create_parametric_sphere(slices, stacks);

	float* points = new float[sphere->npoints * 3];
	float* normals = new float[sphere->npoints * 3];
	float* texCoords = new float[sphere->npoints * 2];

	uint meshSize = 0;
	size_t size = sphere->npoints * 3 * sizeof(float);
	uint stride = 0;

	memcpy(points, sphere->points, size);
	meshSize += 3 * sphere->npoints;
	stride += 3;

	memcpy(normals, sphere->normals, size);
	meshSize += 3 * sphere->npoints;
	stride += 3;

	size = sphere->npoints * 2 * sizeof(float);
	memcpy(texCoords, sphere->tcoords, size);
	meshSize += 2 * sphere->npoints;
	stride += 2;

	stride *= sizeof(float);
	float* meshBuffer = new float[meshSize];
	float* cursor = meshBuffer;
	for (int i = 0; i < sphere->npoints; i++)
	{
		uint cursorSize = 3;
		size_t size = sizeof(float) * 3;

		memcpy(cursor, &points[i * 3], size);
		cursor += cursorSize;

		memcpy(cursor, &normals[i * 3], size);
		cursor += cursorSize;

		cursorSize = 2;
		size = sizeof(float) * 2;
		memcpy(cursor, &texCoords[i * 2], size);
		cursor += cursorSize;
	}

	glGenVertexArrays(1, &vao);
	RE_GLCache::ChangeVAO(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, meshSize * sizeof(float), meshBuffer, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 3u));

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 6u));

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere->ntriangles * sizeof(unsigned short) * 3, sphere->triangles, GL_STATIC_DRAW);

	RE_GLCache::ChangeVAO(0);

	triangles = sphere->ntriangles;

	par_shapes_free_mesh(sphere);
	DEL_A(points);
	DEL_A(normals);
	DEL_A(texCoords);
	DEL_A(meshBuffer);
}

void RE_PrimitiveManager::GenerateGrid(PrimData& prim, RE_CompGrid* gridC)
{
	eastl::vector<float> vertices;
	float d = static_cast<float>(gridC->divisions);
	float distance = gridC->distance = d * 2.5f;
	float f = 0.f;
	for (; f < d; f += 0.5f)
	{
		vertices.push_back((f * 5.f) - distance);
		vertices.push_back(0.f);
		vertices.push_back(distance);
		vertices.push_back((f * 5.f) - distance);
		vertices.push_back(0.f);
		vertices.push_back(-distance);
		vertices.push_back(distance);
		vertices.push_back(0.f);
		vertices.push_back((f * 5.f) - distance);
		vertices.push_back(-distance);
		vertices.push_back(0.f);
		vertices.push_back((f * 5.f) - distance);
	}

	vertices.push_back((f * 5.f) - distance);
	vertices.push_back(0.f);
	vertices.push_back(distance);
	vertices.push_back((f * 5.f) - distance);
	vertices.push_back(0.f);
	vertices.push_back(-distance);
	vertices.push_back(distance);
	vertices.push_back(0.f);
	vertices.push_back((f * 5.f) - distance);
	vertices.push_back(-distance);
	vertices.push_back(0.f);
	vertices.push_back((f * 5.f) - distance);


	glGenVertexArrays(1, &prim.vao);
	glGenBuffers(1, &prim.vbo);

	RE_GLCache::ChangeVAO(prim.vao);

	glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	RE_GLCache::ChangeVAO(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RE_PrimitiveManager::GenerateRock(PrimData& prim, RE_CompRock* rockC)
{
	par_shapes_mesh* rock = par_shapes_create_rock(rockC->seed, rockC->nsubdivisions);

	float* points = new float[rock->npoints * 3];
	float* normals = new float[rock->npoints * 3];

	uint meshSize = 0;
	size_t size = rock->npoints * 3 * sizeof(float);
	uint stride = 0;

	memcpy(points, rock->points, size);
	meshSize += 3 * rock->npoints;
	stride += 3;

	memcpy(normals, rock->normals, size);
	meshSize += 3 * rock->npoints;
	stride += 3;

	stride *= sizeof(float);
	float* meshBuffer = new float[meshSize];
	float* cursor = meshBuffer;
	for (int i = 0; i < rock->npoints; i++)
	{
		uint cursorSize = 3;
		size_t size = sizeof(float) * 3;

		memcpy(cursor, &points[i * 3], size);
		cursor += cursorSize;

		memcpy(cursor, &normals[i * 3], size);
		cursor += cursorSize;
	}

	glGenVertexArrays(1, &prim.vao);
	RE_GLCache::ChangeVAO(prim.vao);

	glGenBuffers(1, &prim.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
	glBufferData(GL_ARRAY_BUFFER, meshSize * sizeof(float), meshBuffer, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));

	glGenBuffers(1, &prim.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rock->ntriangles * sizeof(unsigned short) * 3, rock->triangles, GL_STATIC_DRAW);

	RE_GLCache::ChangeVAO(0);

	prim.triangles = rock->ntriangles;

	par_shapes_free_mesh(rock);
	DEL_A(points);
	DEL_A(normals);
	DEL_A(meshBuffer);
}

void RE_PrimitiveManager::UploadPlatonic(PrimData& prim, par_shapes_mesh_s* plato)
{
	prim.triangles = static_cast<unsigned int>(plato->ntriangles);

	par_shapes_unweld(plato, true);
	par_shapes_compute_normals(plato);

	float* points = new float[plato->npoints * 3];
	float* normals = new float[plato->npoints * 3];

	uint meshSize = 0;
	size_t size = plato->npoints * 3 * sizeof(float);
	uint stride = 0;

	memcpy(points, plato->points, size);
	meshSize += 3 * plato->npoints;
	stride += 3;

	memcpy(normals, plato->normals, size);
	meshSize += 3 * plato->npoints;
	stride += 3;

	stride *= sizeof(float);
	float* meshBuffer = new float[meshSize];
	float* cursor = meshBuffer;
	for (int i = 0; i < plato->npoints; i++)
	{
		uint cursorSize = 3;
		size_t size = sizeof(float) * 3;

		memcpy(cursor, &points[i * 3], size);
		cursor += cursorSize;

		memcpy(cursor, &normals[i * 3], size);
		cursor += cursorSize;
	}

	glGenVertexArrays(1, &prim.vao);
	RE_GLCache::ChangeVAO(prim.vao);

	glGenBuffers(1, &prim.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
	glBufferData(GL_ARRAY_BUFFER, meshSize * sizeof(float), meshBuffer, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 3));

	glGenBuffers(1, &prim.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.triangles * sizeof(unsigned short) * 3u, plato->triangles, GL_STATIC_DRAW);

	RE_GLCache::ChangeVAO(0);
	
	DEL_A(points);
	DEL_A(normals);
	DEL_A(meshBuffer);
}

void RE_PrimitiveManager::UploadParametric(PrimData& prim, par_shapes_mesh_s* param)
{
	prim.triangles = param->ntriangles;

	float* points = new float[param->npoints * 3];
	float* normals = new float[param->npoints * 3];
	float* texCoords = new float[param->npoints * 2];

	uint meshSize = 0;
	size_t size = param->npoints * 3 * sizeof(float);
	uint stride = 0;

	memcpy(points, param->points, size);
	meshSize += 3 * param->npoints;
	stride += 3;

	memcpy(normals, param->normals, size);
	meshSize += 3 * param->npoints;
	stride += 3;

	size = param->npoints * 2 * sizeof(float);
	memcpy(texCoords, param->tcoords, size);
	meshSize += 2 * param->npoints;
	stride += 2;

	stride *= sizeof(float);
	float* meshBuffer = new float[meshSize];
	float* cursor = meshBuffer;

	for (int i = 0; i < param->npoints; i++)
	{
		uint cursorSize = 3;
		size_t size = sizeof(float) * 3;

		memcpy(cursor, &points[i * 3], size);
		cursor += cursorSize;

		memcpy(cursor, &normals[i * 3], size);
		cursor += cursorSize;

		cursorSize = 2;
		size = sizeof(float) * 2;
		memcpy(cursor, &texCoords[i * 2], size);
		cursor += cursorSize;
	}

	glGenVertexArrays(1, &prim.vao);
	RE_GLCache::ChangeVAO(prim.vao);

	glGenBuffers(1, &prim.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
	glBufferData(GL_ARRAY_BUFFER, meshSize * sizeof(float), meshBuffer, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 3u));

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 6u));

	glGenBuffers(1, &prim.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.triangles * sizeof(unsigned short) * 3u, param->triangles, GL_STATIC_DRAW);

	RE_GLCache::ChangeVAO(0);

	DEL_A(points);
	DEL_A(normals);
	DEL_A(texCoords);
	DEL_A(meshBuffer);
}