#ifndef __RE_COMPMESH_H__
#define __RE_COMPMESH_H__

#include "RE_Component.h"

class RE_CompMesh : public RE_Component
{
public:
	RE_CompMesh(RE_GameObject* go = nullptr, const char *path = nullptr, const bool file_dropped = false, const bool start_active = true);
	~RE_CompMesh();

	unsigned int LoadMesh(const char* path, const bool dropped = false);

	void Draw() override;

protected:

	unsigned int reference = 0u;
};


#include <vector>
#include "MathGeoLib/include/MathGeoLib.h"

struct _Vertex
{
	math::vec Position;
	math::vec Normal;
	math::float2 TexCoords;
};

struct _Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;

class RE_UnregisteredMesh
{
public:
	RE_UnregisteredMesh(std::vector<_Vertex> vertices, std::vector<unsigned int> indices, std::vector<_Texture> textures, unsigned int triangles);
	//~RE_UnregisteredMesh();

	void Draw(unsigned int shader_ID, bool f_normals = false, bool v_normals = false);

private:

	void _setupMesh();

public:

	std::vector<_Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<_Texture> textures;
	std::string name;
	unsigned int triangle_count = 0;
	unsigned int VAO;
	unsigned int VAO_Vertex, VAO_FaceNormals, VAO_VertexNormals;

	void loadVertexNormals();
	void loadFaceNormals();
	void clearVertexNormals();
	void clearFaceNormals();
	bool lVertexNormals = false, lFaceNormals = false;

private:
	unsigned int VBO, EBO;
	unsigned int VBO_Vertex, VBO_FaceNormals, VBO_VertexNormals;

	void loadVertex();
	void clearVertex();
};

class RE_CompUnregisteredMesh
{
public:
	RE_CompUnregisteredMesh(char *path);
	RE_CompUnregisteredMesh(char *path, const char* buffer, unsigned int size);

	void Draw(unsigned int shader);
	void DrawProperties();

private:

	void loadModel(std::string path);
	void processNode(aiNode *node, const aiScene *scene);
	RE_UnregisteredMesh processMesh(aiMesh *mesh, const aiScene *scene, const unsigned int pos = 1);
	std::vector<_Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);

public:

	const char* buffer_file = nullptr;
	std::string name;
	std::string directory;
	unsigned int buffer_size = 0;

	bool dropped = false;
	unsigned int total_triangle_count = 0;

	std::vector<RE_UnregisteredMesh> meshes;
	std::vector<_Texture> textures_loaded;

	math::AABB bounding_box;

	bool error_loading = false;

	bool show_f_normals = false;
	bool show_v_normals = false;
};

#endif // !__RE_COMPMESH_H__