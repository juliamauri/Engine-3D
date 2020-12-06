#include "RE_FileSystem.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleEditor.h"
#include "RE_LogManager.h"
#include "RE_ResourceManager.h"
#include "RE_TimeManager.h"
#include "RE_PrimitiveManager.h"
#include "RE_ModelImporter.h"
#include "RE_GameObject.h"
#include "RE_CompTransform.h"
#include "RE_CompMesh.h"
#include "RE_CompPrimitive.h"
#include "RE_Mesh.h"
#include "RE_Shader.h"
#include "Globals.h"

#include "ImGui\imgui.h"
#include "SDL2\include\SDL.h"
#include "SDL2\include\SDL_assert.h"
#include "RapidJson\include\pointer.h"
#include "RapidJson\include\stringbuffer.h"
#include "RapidJson\include\writer.h"
#include "libzip/include/zip.h"
#include "PhysFS\include\physfs.h"
#include "md5.h"
#include <EASTL/internal/char_traits.h>
#include <EASTL/algorithm.h>
#include <EASTL/iterator.h>
#include <EAStdC/EASprintf.h>

#pragma comment( lib, "PhysFS/libx86/physfs.lib" )

#ifdef _DEBUG
#pragma comment( lib, "libzip/zip_d.lib" )

#else
#pragma comment( lib, "libzip/zip_r.lib" )
#endif // _DEBUG

RE_FileSystem::RE_FileSystem() : engine_config(nullptr) {}

RE_FileSystem::~RE_FileSystem()
{
	for (auto dir : assetsDirectories)
		for (auto p : dir->tree)
			if (p->pType != PathType::D_FOLDER)
				DEL(p);

	for (auto dir : assetsDirectories) DEL(dir);

	DEL(engine_config);

	PHYSFS_deinit();
}


bool RE_FileSystem::Init(int argc, char* argv[])
{
	bool ret = false;

	if (PHYSFS_init(argv[0]) != 0)
	{
		PHYSFS_Version physfs_version;
		PHYSFS_VERSION(&physfs_version);
		char tmp[8];
		EA::StdC::Snprintf(tmp, 8, "%u.%u.%u", static_cast<int>(physfs_version.major), static_cast<int>(physfs_version.minor), static_cast<int>(physfs_version.patch));
		RE_SOFT_NVS("PhysFS", tmp, "https://icculus.org/physfs/");
		RE_SOFT_NVS("Rapidjson", RAPIDJSON_VERSION_STRING, "http://rapidjson.org/");
		RE_SOFT_NVS("LibZip", "1.5.0", "https://libzip.org/");
		
		engine_path = "engine";
		library_path = "Library";
		assets_path = "Assets";

		zip_path = (GetExecutableDirectory());
		zip_path += "data.zip";
		PHYSFS_mount(zip_path.c_str(), NULL, 1);

		char **i;
		for (i = PHYSFS_getSearchPath(); *i != NULL; i++) RE_LOG("[%s] is in the search path.\n", *i);
		PHYSFS_freeList(*i);

		const char* config_file = "Settings/config.json";
		engine_config = new Config(config_file, zip_path.c_str());
		if (ret = engine_config->Load())
		{
			rootAssetDirectory = new RE_Directory();
			rootAssetDirectory->SetPath("Assets/");
			assetsDirectories = rootAssetDirectory->MountTreeFolders();
			assetsDirectories.push_front(rootAssetDirectory);
			dirIter = assetsDirectories.begin();
		}
		else
			RE_LOG_ERROR("Error while loading Engine Configuration file: %s\nRed Eye Engine will initialize with default configuration parameters.", config_file);
	}
	else
	{
		RE_LOG_ERROR("PhysFS could not initialize! Error: %s\n", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}

	return ret;
}

Config* RE_FileSystem::GetConfig() const
{
	return engine_config;
}

unsigned int RE_FileSystem::ReadAssetChanges(unsigned int extra_ms, bool doAll)
{
	OPTICK_CATEGORY("Read Assets Changes", Optick::Category::IO);

	Timer time;
	bool run = true;

	if ((dirIter == assetsDirectories.begin()) && (!assetsToProcess.empty() || !filesToFindMeta.empty() || !toImport.empty() || !toReImport.empty()))
		dirIter = assetsDirectories.end();

	while ((doAll || run) && dirIter != assetsDirectories.end())
	{
		eastl::stack<RE_ProcessPath*> toProcess = (*dirIter)->CheckAndApply(&metaRecentlyAdded);
		while (!toProcess.empty())
		{
			RE_ProcessPath* process = toProcess.top();
			toProcess.pop();
			assetsToProcess.push(process);
		}

		dirIter++;

		//timer
		if (!doAll && extra_ms < time.Read()) run = false;
	}

	if (dirIter == assetsDirectories.end())
	{
		dirIter = assetsDirectories.begin();

		while ((doAll || run) && !assetsToProcess.empty())
		{
			RE_ProcessPath* process = assetsToProcess.top();
			assetsToProcess.pop();

			switch (process->whatToDo)
			{
			case P_ADDFILE:
			{
				RE_File* file = process->toProcess->AsFile();
				RE_Meta* metaFile = file->AsMeta();

				FileType type = file->fType;

				if (type == F_META)
				{
					const char* isReferenced = App::resources->FindMD5ByMETAPath(file->path.c_str());

					if (!isReferenced)
					{
						Config metaLoad(file->path.c_str(), App::fs->GetZipPath());
						if (metaLoad.Load())
						{
							JSONNode* metaNode = metaLoad.GetRootNode("meta");
							Resource_Type type = (Resource_Type)metaNode->PullInt("Type", Resource_Type::R_UNDEFINED);
							DEL(metaNode);

							if (type != Resource_Type::R_UNDEFINED)
								metaFile->resource = App::resources->ReferenceByMeta(file->path.c_str(), type);
						}
					}
					else
						metaFile->resource = isReferenced;

					metaToFindFile.push_back(metaFile);
				}
				else if (type > F_NONE) filesToFindMeta.push_back(file);
				else RE_LOG_ERROR("Unsupported file type");
				break;
			}
			case P_ADDFOLDER:
			{
				RE_Directory* dir = (RE_Directory*)process->toProcess;
				eastl::stack<RE_ProcessPath*> toProcess = dir->CheckAndApply(&metaRecentlyAdded);

				while (!toProcess.empty())
				{
					RE_ProcessPath* process = toProcess.top();
					toProcess.pop();
					assetsToProcess.push(process);
				}

				assetsDirectories.push_back(dir);
				break;
			}
			case P_REIMPORT:
			{
				toReImport.push_back(process->toProcess->AsMeta());
				break;
			}
			}
			DEL(process);

			//timer
			if (!doAll && extra_ms < time.Read()) run = false;
		}

		if ((doAll || run) && !metaToFindFile.empty())
		{
			eastl::vector<RE_File*> toRemoveF;
			eastl::vector<RE_Meta*> toRemoveM;

			for (RE_Meta* meta : metaToFindFile)
			{
				ResourceContainer* res = App::resources->At(meta->resource);
				const char* assetPath = App::resources->At(meta->resource)->GetAssetPath();

				if (!res->isInternal())
				{
					int sizefile = 0;
					if (!filesToFindMeta.empty())
					{
						for (RE_File* file : filesToFindMeta)
						{
							sizefile = eastl::CharStrlen(assetPath);
							if (sizefile > 0 && eastl::Compare(assetPath, file->path.c_str(), sizefile) == 0)
							{
								meta->fromFile = file;
								file->metaResource = meta;
								toRemoveF.push_back(file);
								toRemoveM.push_back(meta);
							}
						}
					}
					if (res->GetType() == R_SHADER)  toRemoveM.push_back(meta);
				}
				else toRemoveM.push_back(meta);

				//timer
				if (!doAll && extra_ms < time.Read()) run = false;
			}
			if (!toRemoveF.empty())
			{
				//https://stackoverflow.com/questions/21195217/elegant-way-to-remove-all-elements-of-a-vector-that-are-contained-in-another-vec
				filesToFindMeta.erase(eastl::remove_if(eastl::begin(filesToFindMeta), eastl::end(filesToFindMeta),
					[&](auto x) {return eastl::find(begin(toRemoveF), end(toRemoveF), x) != end(toRemoveF); }), eastl::end(filesToFindMeta));
			}
			if (!toRemoveM.empty())
			{
				metaToFindFile.erase(eastl::remove_if(eastl::begin(metaToFindFile), eastl::end(metaToFindFile),
					[&](auto x) {return eastl::find(begin(toRemoveM), end(toRemoveM), x) != end(toRemoveM); }), eastl::end(metaToFindFile));
			}
		}

		if ((doAll || run) && !filesToFindMeta.empty())
		{
			for (RE_File* file : filesToFindMeta)
			{
				switch (file->fType)
				{
				case F_MODEL:
				case F_PREFAB:
				case F_SCENE:
					toImport.push_back(file);
					break;
				case F_SKYBOX:
				{
					eastl::list<RE_File*>::const_iterator iter = toImport.end();
					if (!toImport.empty())
					{
						iter--;
						FileType type = (*iter)->fType;
						int count = toImport.size();
						while (count > 1)
						{
							if (type == F_PREFAB || type == F_SCENE || type == F_MODEL)
								break;

							count--;
							iter--;
							type = (*iter)->fType;
						}
						iter++;
					}
					toImport.insert(iter, file);
					break;
				}
				case F_TEXTURE:
				case F_MATERIAL:
					toImport.push_front(file);
					break;
				}
			}
			filesToFindMeta.clear();
		}

		if ((doAll || run) && !toImport.empty())
		{
			RE_LogManager::ScopeProcedureLogging();
			eastl::vector<RE_File*> toRemoveF;

			//Importing
			for (RE_File* file : toImport) {
				RE_LOG("Importing %s", file->path.c_str());

				const char* newRes = nullptr;
				switch (file->fType) {
				case F_MODEL:	 newRes = App::resources->ImportModel(file->path.c_str()); break;
				case F_TEXTURE:	 newRes = App::resources->ImportTexture(file->path.c_str()); break;
				case F_MATERIAL: newRes = App::resources->ImportMaterial(file->path.c_str()); break;
				case F_SKYBOX:	 newRes = App::resources->ImportSkyBox(file->path.c_str()); break;
				case F_PREFAB:	 newRes = App::resources->ImportPrefab(file->path.c_str()); break;
				case F_SCENE:	 newRes = App::resources->ImportScene(file->path.c_str()); break; }

				if (newRes != nullptr)
				{
					RE_Meta* newMetaFile = new RE_Meta();
					newMetaFile->resource = newRes;
					newMetaFile->fromFile = file;
					file->metaResource = newMetaFile;
					RE_File* fromMetaF = newMetaFile->AsFile();
					fromMetaF->fType = FileType::F_META;
					fromMetaF->path = App::resources->At(newRes)->GetMetaPath();
					newMetaFile->AsPath()->pType = PathType::D_FILE;
					metaRecentlyAdded.push_back(newMetaFile);
				}
				else
					file->fType = F_NOTSUPPORTED;

				toRemoveF.push_back(file);

				if (!doAll && extra_ms < time.Read())
				{
					run = false;
					break;
				}
			}

			if (!toRemoveF.empty())
			{
				//https://stackoverflow.com/questions/21195217/elegant-way-to-remove-all-elements-of-a-vector-that-are-contained-in-another-vec
				toImport.erase(eastl::remove_if(eastl::begin(toImport), eastl::end(toImport),
					[&](auto x) {return eastl::find(begin(toRemoveF), end(toRemoveF), x) != end(toRemoveF); }), eastl::end(toImport));
			}

			RE_LogManager::EndScope();
		}

		if ((doAll || run) && !toReImport.empty())
		{
			eastl::vector<RE_Meta*> toRemoveM;
			for (RE_Meta* meta : toReImport)
			{
				RE_LOG("ReImporting %s", meta->path.c_str());
				App::resources->At(meta->resource)->ReImport();
				toRemoveM.push_back(meta);

				if (!doAll && extra_ms < time.Read())
				{
					run = false;
					break;
				}
			}

			if (!toRemoveM.empty())
			{
				//https://stackoverflow.com/questions/21195217/elegant-way-to-remove-all-elements-of-a-vector-that-are-contained-in-another-vec
				toReImport.erase(eastl::remove_if(eastl::begin(toReImport), eastl::end(toReImport),
					[&](auto x) {return eastl::find(begin(toRemoveM), end(toRemoveM), x) != end(toRemoveM); }), eastl::end(toReImport));
			}
		}
	}

	unsigned int realTime = time.Read();
	return (extra_ms < realTime) ? 0 : extra_ms - realTime;
}

void RE_FileSystem::DrawEditor()
{
	if (ImGui::CollapsingHeader("File System"))
	{
		ImGui::Text("Executable Directory:");
		ImGui::TextWrappedV(GetExecutableDirectory(), "");

		ImGui::Separator();
		ImGui::Text("All assets directories:");
		for (auto dir : assetsDirectories)ImGui::Text(dir->path.c_str());
		ImGui::Separator();

		ImGui::Text("Write Directory");
		ImGui::TextWrappedV(write_path.c_str(), "");
	}
}

bool RE_FileSystem::AddPath(const char * path_or_zip, const char * mount_point)
{
	bool ret = true;

	if (PHYSFS_mount(path_or_zip, mount_point, 1) == 0)
	{
		RE_LOG_ERROR("File System error while adding a path or zip(%s): %s\n", path_or_zip, PHYSFS_getLastError());
		ret = false;
	}
	return ret;
}

bool RE_FileSystem::RemovePath(const char * path_or_zip)
{
	bool ret = true;

	if (PHYSFS_removeFromSearchPath(path_or_zip) == 0)
	{
		RE_LOG_ERROR("Error removing PhysFS Directory (%s): %s", path_or_zip, PHYSFS_getLastError());
		ret = false;
	}
	return ret;
}

bool RE_FileSystem::SetWritePath(const char * dir)
{
	bool ret = true;

	if (!PHYSFS_setWriteDir(dir))
	{
		RE_LOG_ERROR("Error setting PhysFS Directory: %s", PHYSFS_getLastError());
		ret = false;
	}
	else write_path = dir;

	return ret;
}

const char * RE_FileSystem::GetWritePath() const { return write_path.c_str(); }

void RE_FileSystem::LogFolderItems(const char * folder)
{
	char **rc = PHYSFS_enumerateFiles(folder), **i;
	for (i = rc; *i != NULL; i++) RE_LOG(" * We've got [%s].\n", *i);
	PHYSFS_freeList(rc);
}

// Quick Buffer From Platform-Dependent Path
RE_FileIO* RE_FileSystem::QuickBufferFromPDPath(const char * full_path)// , char** buffer, unsigned int size)
{
	RE_FileIO* ret = nullptr;
	if (full_path)
	{
		eastl::string file_path = full_path;
		eastl::string file_name = file_path.substr(file_path.find_last_of("\\") + 1);
		eastl::string ext = file_name.substr(file_name.find_last_of(".") + 1);
		file_path.erase(file_path.length() - file_name.length(), file_path.length());

		eastl::string tmp = "/Export/";
		tmp += file_name;
		ret = new RE_FileIO(tmp.c_str());

		if (App::fs->AddPath(file_path.c_str(), "/Export/"))
		{
			if (!ret->Load()) DEL(ret);
			App::fs->RemovePath(file_path.c_str());
		}
		else DEL(ret);
	}

	return ret;
}

bool RE_FileSystem::ExistsOnOSFileSystem(const char* path, bool isFolder) const
{
	eastl::string full_path(path);
	eastl::string directory = full_path.substr(0, full_path.find_last_of('\\') + 1);
	eastl::string fileNameExtension = full_path.substr(full_path.find_last_of("\\") + 1);

	eastl::string tempPath("/Check/");
	if (PHYSFS_mount(directory.c_str(), "/Check/", 1) != 0)
	{
		if (!isFolder)
		{
			if (Exists((tempPath += fileNameExtension).c_str()))
			{
				PHYSFS_removeFromSearchPath(directory.c_str());
				return true;
			}
		}
		PHYSFS_removeFromSearchPath(directory.c_str());
		return true;
	}
	return false;
}

bool RE_FileSystem::Exists(const char* file) const { return PHYSFS_exists(file) != 0; }
bool RE_FileSystem::IsDirectory(const char* file) const { return PHYSFS_isDirectory(file) != 0; }
const char* RE_FileSystem::GetExecutableDirectory() const { return PHYSFS_getBaseDir(); }
const char * RE_FileSystem::GetZipPath() { return zip_path.c_str(); }

void RE_FileSystem::HandleDropedFile(const char * file)
{
	OPTICK_CATEGORY("Dropped File", Optick::Category::IO);
	eastl::string full_path(file);
	eastl::string directory = full_path.substr(0, full_path.find_last_of('\\') + 1);
	eastl::string fileNameExtension = full_path.substr(full_path.find_last_of("\\") + 1);
	eastl::string fileName = fileNameExtension.substr(0, fileNameExtension.find_last_of("."));
	eastl::string ext = full_path.substr(full_path.find_last_of(".") + 1);
	eastl::string exportPath("Exporting/");
	eastl::string finalPath = exportPath;

	bool newDir = (ext.compare("zip") == 0 || ext.compare("7z") == 0 || ext.compare("iso") == 0);
	if (!newDir)
	{
		RE_FileIO* fileLoaded = QuickBufferFromPDPath(file);
		if (fileLoaded)
		{
			eastl::string destPath = App::editor->GetAssetsPanelPath();
			destPath += fileNameExtension;
			RE_FileIO toSave(destPath.c_str(), GetZipPath());
			toSave.Save(fileLoaded->GetBuffer(), fileLoaded->GetSize());
		}
		else newDir = true;
	}

	if (newDir)
	{
		finalPath += fileName + "/";
		App::fs->AddPath(file, finalPath.c_str());
		CopyDirectory(exportPath.c_str(), App::editor->GetAssetsPanelPath());
		App::fs->RemovePath(file);
	}
}

RE_FileSystem::RE_Directory* RE_FileSystem::GetRootDirectory() const { return rootAssetDirectory; }

void RE_FileSystem::DeleteUndefinedFile(const char* filePath)
{
	RE_FileSystem::RE_Directory* dir = FindDirectory(filePath);

	if (dir != nullptr)
	{
		RE_FileSystem::RE_Path* file = FindPath(filePath, dir);

		if (file != nullptr)
		{
			auto iterPath = dir->tree.begin();
			while (iterPath != dir->tree.end())
			{
				if (*iterPath == file)
				{
					dir->tree.erase(iterPath);
					break;
				}
				iterPath++;
			}

			RE_FileIO fileToDelete(filePath, GetZipPath());
			fileToDelete.Delete();
			DEL(file);
		}
		else
			RE_LOG_ERROR("Error deleting file. The file culdn't be located: %s", filePath);
	}
	else
		RE_LOG_ERROR("Error deleting file. The dir culdn't be located: %s", filePath);
}

void RE_FileSystem::DeleteResourceFiles(ResourceContainer* resContainer)
{
	if (resContainer->GetType() != R_SHADER) DeleteUndefinedFile(resContainer->GetAssetPath());
	DeleteUndefinedFile(resContainer->GetMetaPath());

	if (Exists(resContainer->GetLibraryPath()))
	{
		RE_FileIO fileToDelete(resContainer->GetLibraryPath(), GetZipPath());
		fileToDelete.Delete();
	}
}

RE_FileSystem::RE_Directory* RE_FileSystem::FindDirectory(const char* pathToFind)
{
	RE_FileSystem::RE_Directory* ret = nullptr;

	eastl::string fullpath(pathToFind);
	eastl::string directoryStr = fullpath.substr(0, fullpath.find_last_of('/') + 1);

	for (auto dir : assetsDirectories)
	{
		if (dir->path == directoryStr)
		{
			ret = dir;
			break;
		}
	}

	return ret;
}

RE_FileSystem::RE_Path* RE_FileSystem::FindPath(const char* pathToFind, RE_Directory* dir)
{
	RE_FileSystem::RE_Path* ret = nullptr;

	eastl::string fullpath(pathToFind);
	eastl::string directoryStr = fullpath.substr(0, fullpath.find_last_of('/') + 1);

	if (!dir) dir = FindDirectory(pathToFind);
	if (dir)
	{
		for (auto file : dir->tree)
		{
			if (file->path == fullpath)
			{
				ret = file;
				break;
			}
		}
	}

	return ret;
}

signed long long RE_FileSystem::GetLastTimeModified(const char* path)
{
	PHYSFS_Stat stat;
	eastl::string full_path(path);
	eastl::string directory = full_path.substr(0, full_path.find_last_of('\\') + 1);
	eastl::string fileNameExtension = full_path.substr(full_path.find_last_of("\\") + 1);

	eastl::string tempPath("/Check/");
	if (PHYSFS_mount(directory.c_str(), "/Check/", 1) != 0)
	{
		tempPath += fileNameExtension;
		if (PHYSFS_stat(tempPath.c_str(), &stat) != 0)
		{
			PHYSFS_removeFromSearchPath(directory.c_str());
			return stat.modtime;
		}
		PHYSFS_removeFromSearchPath(directory.c_str());
	}

	return 0;
}

void RE_FileSystem::CopyDirectory(const char * origin, const char * dest)
{
	eastl::stack<eastl::pair< eastl::string, eastl::string>> copyDir;
	copyDir.push({ origin , dest });

	while (!copyDir.empty()) {

		eastl::string origin(copyDir.top().first);
		eastl::string dest(copyDir.top().second);
		copyDir.pop();

		char** rc = PHYSFS_enumerateFiles(origin.c_str());
		for (char** i = rc; *i != NULL; i++)
		{
			eastl::string inOrigin = origin + *i;
			eastl::string inDest(dest);

			inDest += *i;

			PHYSFS_Stat fileStat;
			if (PHYSFS_stat(inOrigin.c_str(), &fileStat))
			{
				if (fileStat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY)
				{
					inOrigin += "/";
					inDest += "/";
					copyDir.push({ inOrigin, inDest });
				}
				else if (fileStat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_REGULAR)
				{
					RE_FileIO fileOrigin(inOrigin.c_str());
					if (fileOrigin.Load())
					{
						RE_FileIO fileDest(inDest.c_str(), GetZipPath());
						fileDest.Save(fileOrigin.GetBuffer(), fileOrigin.GetSize());
					}
				}
			}
		}
		PHYSFS_freeList(rc);
	}
}


////////////////////////////////////////////////////////////////////
///////////////////RE_FileIO
////////////////////////////////////////////////////////////////////

RE_FileIO::RE_FileIO(const char* file_name, const char* from_zip) : buffer(nullptr), file_name(file_name), from_zip(from_zip) {}
RE_FileIO::~RE_FileIO() { DEL_A(buffer); }

bool RE_FileIO::Load()
{
	size = HardLoad();
	return size > 0;
}

void RE_FileIO::Save() { HardSave(buffer); }
void RE_FileIO::Save(char * buffer, unsigned int size) { WriteFile(from_zip, file_name, buffer, this->size = ((size == 0) ? strnlen_s(buffer, 0xffff) : size)); }

void RE_FileIO::Delete()
{
	if (PHYSFS_removeFromSearchPath(from_zip) == 0)
		RE_LOG_ERROR("Ettot when unmount: %s", PHYSFS_getLastError());

	struct zip* f_zip = NULL;
	int error = 0;
	f_zip = zip_open(from_zip, ZIP_CHECKCONS, &error); /* on ouvre l'archive zip */
	if (error)	RE_LOG_ERROR("could not open or create archive: %s", from_zip);

	zip_int64_t index = zip_name_locate(f_zip, file_name, NULL);
	if(index == -1) RE_LOG_ERROR("file culdn't locate: %s", file_name);
	else if (zip_delete(f_zip, index) == -1) RE_LOG_ERROR("file culdn't delete: %s", file_name);

	zip_close(f_zip);
	f_zip = NULL;

	PHYSFS_mount(from_zip, NULL, 1);
}

void RE_FileIO::ClearBuffer()
{
	delete[] buffer;
	buffer = nullptr;
}

char * RE_FileIO::GetBuffer() { return (buffer); }
const char* RE_FileIO::GetBuffer() const { return (buffer); }
inline bool RE_FileIO::operator!() const { return buffer == nullptr; }
unsigned int RE_FileIO::GetSize() { return size; }
eastl::string RE_FileIO::GetMd5() { return (!buffer && !Load()) ? "" : md5(eastl::string(buffer, size)); }

unsigned int RE_FileIO::HardLoad()
{
	unsigned int ret = 0u;

	if (PHYSFS_exists(file_name))
	{
		PHYSFS_File* fs_file = PHYSFS_openRead(file_name);

		if (fs_file != NULL)
		{
			signed long long sll_size = PHYSFS_fileLength(fs_file);
			if (sll_size > 0)
			{
				if (buffer) DEL_A(buffer);
				buffer = new char[(unsigned int)sll_size + 1];
				signed long long amountRead = PHYSFS_read(fs_file, buffer, 1, (signed int)sll_size);
				
				if (amountRead != sll_size)
				{
					RE_LOG_ERROR("File System error while reading from file %s: %s", file_name, PHYSFS_getLastError());
					delete (buffer);
				}
				else
				{
					ret = (unsigned int)amountRead;
					buffer[ret] = '\0';
				}
			}

			if (PHYSFS_close(fs_file) == 0) RE_LOG_ERROR("File System error while closing file %s: %s", file_name, PHYSFS_getLastError());
		}
		else RE_LOG_ERROR("File System error while opening file %s: %s", file_name, PHYSFS_getLastError());
	}
	else RE_LOG_ERROR("File System error while checking file %s: %s", file_name, PHYSFS_getLastError());

	return ret;
}

void RE_FileIO::HardSave(const char* buffer)
{
	PHYSFS_file* file = PHYSFS_openWrite(file_name);

	if (file != NULL)
	{
		long long written = PHYSFS_write(file, (const void*)buffer, 1, size = (strnlen_s(buffer, 0xffff)));
		if (written != size) RE_LOG_ERROR("Error while writing to file %s: %s", file, PHYSFS_getLastError());
		if (PHYSFS_close(file) == 0) RE_LOG_ERROR("Error while closing save file %s: %s", file, PHYSFS_getLastError());
	}
	else RE_LOG_ERROR("Error while opening save file %s: %s", file, PHYSFS_getLastError());
}

void RE_FileIO::WriteFile(const char * zip_path, const char * filename, const char * buffer, unsigned int size)
{
	if (PHYSFS_removeFromSearchPath(from_zip) == 0)
		RE_LOG_ERROR("Ettot when unmount: %s", PHYSFS_getLastError());

	struct zip *f_zip = NULL;
	int error = 0;
	f_zip = zip_open(zip_path, ZIP_CHECKCONS, &error); /* on ouvre l'archive zip */
	if (error)	RE_LOG_ERROR("could not open or create archive: %s", zip_path);

	zip_source_t* s = zip_source_buffer(f_zip, buffer, size, 0);
	if (s == NULL || zip_file_add(f_zip, filename, s, ZIP_FL_OVERWRITE + ZIP_FL_ENC_UTF_8) < 0)
	{
		zip_source_free(s);
		RE_LOG_ERROR("error adding file: %s\n", zip_strerror(f_zip));
	}

	zip_close(f_zip);
	f_zip = NULL;
	PHYSFS_mount(from_zip, NULL, 1);
}


////////////////////////////////////////////////////////////////////
///////////////////Config
////////////////////////////////////////////////////////////////////

//Tutorial http://rapidjson.org/md_doc_tutorial.html

Config::Config(const char* file_name, const char* _from_zip) : RE_FileIO(file_name)
{
	zip_path = _from_zip;
	from_zip = zip_path.c_str();
}

bool Config::Load()
{
	// Open file
	bool ret = false;
	size = HardLoad();
	if (ret = (size > 0))
	{
		rapidjson::StringStream s(buffer);
		document.ParseStream(s);
	}

	return ret;
}

bool Config::LoadFromWindowsPath()
{
	// Open file
	bool ret = false;
	RE_FileIO* fileToLoad =  App::fs->QuickBufferFromPDPath(file_name);
	if (fileToLoad != nullptr)
	{
		rapidjson::StringStream s(fileToLoad->GetBuffer());
		document.ParseStream(s);
		ret = true;
		DEL(fileToLoad);
	}
	return ret;
}

void Config::Save()
{
	rapidjson::StringBuffer s_buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s_buffer);
	document.Accept(writer);
	s_buffer.Put('\0');
	size = s_buffer.GetSize();
	eastl::string file(file_name);
	WriteFile(from_zip, file.c_str(), s_buffer.GetString(), size);
}

JSONNode* Config::GetRootNode(const char* member)
{
	JSONNode* ret = nullptr;

	if (member != nullptr)
	{
		eastl::string path("/");
		path += member;
		rapidjson::Value* value = rapidjson::Pointer(path.c_str()).Get(document);

		if (value == nullptr)
		{
			value = &rapidjson::Pointer(path.c_str()).Create(document);
			RE_LOG("Configuration node not found for %s, created new pointer", path.c_str());
		}

		ret = new JSONNode(path.c_str(), this);
	}
	else RE_LOG("Error Loading Configuration node: Empty Member Name");

	return ret;
}

inline bool Config::operator!() const { return document.IsNull(); }

eastl::string Config::GetMd5()
{
	rapidjson::StringBuffer s_buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s_buffer);
	document.Accept(writer);
	s_buffer.Put('\0');
	size = s_buffer.GetSize();
	return md5(s_buffer.GetString());
}


////////////////////////////////////////////////////////////////////
///////////////////JSONNode
////////////////////////////////////////////////////////////////////

JSONNode::JSONNode(const char* path, Config* config, bool isArray) : pointerPath(path), config(config)
{
	if (isArray) rapidjson::Pointer(path).Get(config->document)->SetArray();
}

JSONNode::JSONNode(JSONNode& node) : pointerPath(node.pointerPath), config(node.config) {}
JSONNode::~JSONNode() { config = nullptr; }


// Push ============================================================

void JSONNode::PushBool(const char* name, const bool value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushBool(const char* name, const bool* value, unsigned int quantity)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		for (uint i = 0; i < quantity; i++) float_array.PushBack(value[i], config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushInt(const char* name, const int value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushInt(const char* name, const int* value, unsigned int quantity)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		for (uint i = 0; i < quantity; i++) float_array.PushBack(value[i], config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushUInt(const char* name, const unsigned int value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushUInt(const char* name, const unsigned int* value, unsigned int quantity)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		for (uint i = 0; i < quantity; i++) float_array.PushBack(value[i], config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushFloat(const char* name, const float value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushFloat(const char* name, math::float2 value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		float_array.PushBack(value.x, config->document.GetAllocator()).PushBack(value.y, config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushFloatVector(const char * name, math::vec vector)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		float_array.PushBack(vector.x, config->document.GetAllocator()).PushBack(vector.y, config->document.GetAllocator()).PushBack(vector.z, config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushFloat4(const char * name, math::float4 vector)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		float_array.PushBack(vector.x, config->document.GetAllocator()).PushBack(vector.y, config->document.GetAllocator()).PushBack(vector.z, config->document.GetAllocator()).PushBack(vector.w, config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushMat3(const char* name, math::float3x3 mat3)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		for (uint i = 0; i < 9; i++) float_array.PushBack(mat3.ptr()[i], config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushMat4(const char* name, math::float4x4 mat4)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value float_array(rapidjson::kArrayType);
		for (uint i = 0; i < 16; i++) float_array.PushBack(mat4.ptr()[i], config->document.GetAllocator());
		rapidjson::Pointer(path.c_str()).Set(config->document, float_array);
	}
}

void JSONNode::PushDouble(const char* name, const double value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushSignedLongLong(const char* name, const signed long long value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushUnsignedLongLong(const char* name, const unsigned long long value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushString(const char* name, const char* value)
{
	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Pointer(path.c_str()).Set(config->document, value);
	}
}

void JSONNode::PushValue(rapidjson::Value * val)
{
	rapidjson::Value* val_push = rapidjson::Pointer(pointerPath.c_str()).Get(config->document);
	if (val_push->IsArray()) val_push->PushBack(*val, config->document.GetAllocator());
}

JSONNode* JSONNode::PushJObject(const char* name)
{
	JSONNode* ret = nullptr;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		ret = new JSONNode(path.c_str(), config);
	}

	return ret;
}

// Pull ============================================================

bool JSONNode::PullBool(const char* name, bool deflt)
{
	bool ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetBool();
	}

	return ret;
}

bool* JSONNode::PullBool(const char* name, unsigned int quantity, bool deflt)
{
	bool* ret = new bool[quantity];
	bool* cursor = ret;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val)
		{
			for (uint i = 0; i < quantity; i++, cursor++)
			{
				bool f = val->GetArray()[i].GetBool();
				memcpy(cursor, &f, sizeof(bool));
			}
		}
		else
		{
			bool f = deflt;
			for (uint i = 0; i < quantity; i++, cursor++) memcpy(cursor, &f, sizeof(bool));
		}
	}

	return ret;
}

int JSONNode::PullInt(const char* name, int deflt)
{
	int ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetInt();
	}

	return ret;
}

int* JSONNode::PullInt(const char* name, unsigned int quantity, int deflt)
{
	int* ret = new int[quantity];
	int* cursor = ret;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);

		if (val)
		{
			for (uint i = 0; i < quantity; i++, cursor++)
			{
				int f = val->GetArray()[i].GetInt();
				memcpy(cursor, &f, sizeof(int));
			}
		}
		else
		{
			int f = deflt;
			for (uint i = 0; i < quantity; i++, cursor++) memcpy(cursor, &f, sizeof(int));
		}
	}

	return ret;
}

unsigned int JSONNode::PullUInt(const char* name, const unsigned int deflt)
{
	unsigned int ret = 0u;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetUint();
	}

	return ret;
}

unsigned int* JSONNode::PullUInt(const char* name, unsigned int quantity, unsigned int deflt)
{
	unsigned int* ret = new unsigned int[quantity];
	unsigned int* cursor = ret;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);

		if (val)
		{
			for (uint i = 0; i < quantity; i++, cursor++)
			{
				int f = val->GetArray()[i].GetUint();
				memcpy(cursor, &f, sizeof(unsigned int));
			}
		}
		else
		{
			int f = deflt;
			for (uint i = 0; i < quantity; i++, cursor++) memcpy(cursor, &f, sizeof(unsigned int));
		}
	}

	return ret;
}


float JSONNode::PullFloat(const char* name, float deflt)
{
	float ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetFloat();
	}

	return ret;
}

math::float2 JSONNode::PullFloat(const char* name, math::float2 deflt)
{
	math::float2 ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret.Set(val->GetArray()[0].GetFloat(), val->GetArray()[1].GetFloat());
	}

	return ret;
}

math::vec JSONNode::PullFloatVector(const char * name, math::vec deflt)
{
	math::vec ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret.Set(val->GetArray()[0].GetFloat(), val->GetArray()[1].GetFloat(), val->GetArray()[2].GetFloat());
	}

	return ret;
}

math::float4 JSONNode::PullFloat4(const char * name, math::float4 deflt)
{
	math::float4 ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret.Set(val->GetArray()[0].GetFloat(), val->GetArray()[1].GetFloat(), val->GetArray()[2].GetFloat(), val->GetArray()[3].GetFloat());
	}

	return ret;
}

math::float3x3 JSONNode::PullMat3(const char* name, math::float3x3 deflt)
{
	math::float3x3 ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);

		if (val)
		{
			float* fBuffer = new float[9];
			float* cursor = fBuffer;

			for (uint i = 0; i < 9; i++, cursor++)
			{
				int f = val->GetArray()[i].GetInt();
				memcpy(cursor, &f, sizeof(int));
			}

			ret.Set(fBuffer);
			DEL_A(fBuffer);
		}
	}

	return ret;
}

math::float4x4 JSONNode::PullMat4(const char* name, math::float4x4 deflt)
{
	math::float4x4 ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);

		if (val != nullptr)
		{
			float* fBuffer = new float[16];
			float* cursor = fBuffer;

			for (uint i = 0; i < 16; i++, cursor++)
			{
				int f = val->GetArray()[i].GetInt();
				memcpy(cursor, &f, sizeof(int));
			}

			ret.Set(fBuffer);
			DEL_A(fBuffer);
		}
	}

	return ret;
}

double JSONNode::PullDouble(const char* name, double deflt)
{
	double ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetDouble();
	}

	return ret;
}

signed long long JSONNode::PullSignedLongLong(const char* name, signed long long deflt)
{
	signed long long ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetInt64();
	}

	return ret;
}

unsigned long long JSONNode::PullUnsignedLongLong(const char* name, unsigned long long deflt)
{
	unsigned long long ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetUint64();
	}

	return ret;
}

const char*	JSONNode::PullString(const char* name, const char* deflt)
{
	const char* ret = deflt;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		rapidjson::Value* val = rapidjson::Pointer(path.c_str()).Get(config->document);
		if (val) ret = val->GetString();
	}

	return ret;
}

JSONNode* JSONNode::PullJObject(const char * name)
{
	JSONNode* ret = nullptr;

	if (name)
	{
		eastl::string path = pointerPath + "/" + name;
		ret = new JSONNode(path.c_str(), config);
	}

	return ret;
}

rapidjson::Value::Array JSONNode::PullValueArray() { return config->document.FindMember(pointerPath.c_str())->value.GetArray(); }
inline bool JSONNode::operator!() const { return config || pointerPath.empty(); }
const char * JSONNode::GetDocumentPath() const { return pointerPath.c_str(); }
rapidjson::Document * JSONNode::GetDocument() { return &config->document; }

void JSONNode::SetArray()  { rapidjson::Pointer(pointerPath.c_str()).Get(config->document)->SetArray(); }
void JSONNode::SetObject() { rapidjson::Pointer(pointerPath.c_str()).Get(config->document)->SetObject(); }

RE_FileSystem::FileType RE_FileSystem::RE_File::DetectExtensionAndType(const char* _path, const char*& _extension)
{
	static const char* extensionsSuported[12] = { "meta", "re","refab", "pupil", "sk",  "fbx", "jpg", "dds", "png", "tga", "tiff", "bmp" };
	
	RE_FileSystem::FileType ret = F_NOTSUPPORTED;
	eastl::string modPath(_path);
	eastl::string filename = modPath.substr(modPath.find_last_of("/") + 1);
	eastl::string extensionStr = filename.substr(filename.find_last_of(".") + 1);
	eastl::transform(extensionStr.begin(), extensionStr.end(), extensionStr.begin(), [](unsigned char c) { return eastl::CharToLower(c); });

	for (uint i = 0; i < 12; i++)
	{
		if (extensionStr.compare(extensionsSuported[i]) == 0)
		{
			_extension = extensionsSuported[i];
			switch (i) {
			case 0: ret = F_META; break;
			case 1: ret = F_SCENE; break;
			case 2: ret = F_PREFAB; break;
			case 3: ret = F_MATERIAL; break;
			case 4: ret = F_SKYBOX; break;
			case 5: ret = F_MODEL; break;
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11: ret = F_TEXTURE; break; }
		}
	}
	return ret;
}

bool RE_FileSystem::RE_Meta::IsModified() const
{
	return (App::resources->At(resource)->GetType() != Resource_Type::R_SHADER
		&& fromFile->lastModified != App::resources->At(resource)->GetLastTimeModified());
}

void RE_FileSystem::RE_Directory::SetPath(const char* _path)
{
	path = _path;
	name = eastl::string(path.c_str(), path.size() - 1);
	name = name.substr(name.find_last_of("/") + 1);
}

eastl::list< RE_FileSystem::RE_Directory*> RE_FileSystem::RE_Directory::MountTreeFolders()
{
	eastl::list< RE_Directory*> ret;
	if (App::fs->Exists(path.c_str()))
	{
		eastl::string iterPath(path);
		char** rc = PHYSFS_enumerateFiles(iterPath.c_str());
		for (char** i = rc; *i != NULL; i++)
		{
			eastl::string inPath(iterPath);
			inPath += *i;

			PHYSFS_Stat fileStat;
			if (PHYSFS_stat(inPath.c_str(), &fileStat) && fileStat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY)
			{
				RE_Directory* newDirectory = new RE_Directory();
				newDirectory->SetPath((inPath += "/").c_str());
				newDirectory->parent = this;
				newDirectory->pType = D_FOLDER;
				tree.push_back(newDirectory->AsPath());
				ret.push_back(newDirectory);
			}
		}
		PHYSFS_freeList(rc);

		if (!tree.empty())
		{
			for (RE_Path* path : tree)
			{
				eastl::list< RE_Directory*> fromChild = path->AsDirectory()->MountTreeFolders();
				if (!fromChild.empty()) ret.insert(ret.end(), fromChild.begin(), fromChild.end());
			}
		}
	}
	return ret;
}

eastl::stack<RE_FileSystem::RE_ProcessPath*> RE_FileSystem::RE_Directory::CheckAndApply(eastl::vector<RE_Meta*>* metaRecentlyAdded)
{
	eastl::stack<RE_ProcessPath*> ret;
	if (App::fs->Exists(path.c_str()))
	{
		eastl::string iterPath(path);
		eastl::vector<RE_Meta*> toRemoveM;
		char** rc = PHYSFS_enumerateFiles(iterPath.c_str());
		pathIterator iter = tree.begin();
		for (char** i = rc; *i != NULL; i++)
		{
			PathType iterTreeType = (iter != tree.end()) ? (*iter)->pType : PathType::D_NULL;
			eastl::string inPath(iterPath);
			inPath += *i;
			PHYSFS_Stat fileStat;
			if (PHYSFS_stat(inPath.c_str(), &fileStat))
			{
				if (fileStat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY)
				{
					bool newFolder = (iterTreeType == PathType::D_NULL || iterTreeType != PathType::D_FOLDER || (*iter)->path != (inPath += "/"));
					if (newFolder && iter != tree.end())
					{
						pathIterator postPath = iter;
						bool found = false;
						while (postPath != tree.end())
						{
							if (inPath == (*postPath)->path)
							{
								found = true;
								break;
							}
							postPath++;
						}
						if (found) newFolder = false;
					}

					if (newFolder)
					{
						RE_Directory* newDirectory = new RE_Directory();
						
						if (inPath.back() != '/')inPath += "/";
						newDirectory->SetPath(inPath.c_str());
						newDirectory->parent = this;
						newDirectory->pType = D_FOLDER;

						AddBeforeOf(newDirectory->AsPath(), iter);
						iter--;

						RE_ProcessPath* newProcess = new RE_ProcessPath();
						newProcess->whatToDo = P_ADDFOLDER;
						newProcess->toProcess = newDirectory->AsPath();
						ret.push(newProcess);
					}
				}
				else if (fileStat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_REGULAR)
				{
					const char* extension = nullptr;
					FileType fileType = RE_File::DetectExtensionAndType(inPath.c_str(), extension);
					bool newFile = (iterTreeType == PathType::D_NULL || iterTreeType != PathType::D_FILE || (*iter)->path != inPath);
					if (newFile && fileType == FileType::F_META && !metaRecentlyAdded->empty())
					{
						int sizemetap = 0;
						for (RE_Meta* metaAdded : *metaRecentlyAdded)
						{
							sizemetap = eastl::CharStrlen(metaAdded->path.c_str());
							if (sizemetap > 0 && eastl::Compare(metaAdded->path.c_str(), inPath.c_str(), sizemetap) == 0)
							{
								AddBeforeOf(metaAdded->AsPath(), iter);
								iter--;
								toRemoveM.push_back(metaAdded);
								metaRecentlyAdded->erase(eastl::remove_if(eastl::begin(*metaRecentlyAdded), eastl::end(*metaRecentlyAdded),
									[&](auto x) {return eastl::find(begin(toRemoveM), end(toRemoveM), x) != end(toRemoveM); }), eastl::end(*metaRecentlyAdded));
								toRemoveM.clear();
								newFile = false;
								break;
							}
						}
					}

					if (newFile && iter != tree.end())
					{
						pathIterator postPath = iter;
						bool found = false;
						while (postPath != tree.end())
						{
							if (inPath == (*postPath)->path)
							{
								found = true;
								break;
							}
							postPath++;
						}
						if (found) newFile = false;
					}

					 if(newFile)
					 {
						RE_File* newFile = (fileType != FileType::F_META) ? new RE_File() : (RE_File*)new RE_Meta();
						newFile->path = inPath;
						newFile->filename = inPath.substr(inPath.find_last_of("/") + 1);
						newFile->lastSize = fileStat.filesize;
						newFile->pType = PathType::D_FILE;
						newFile->fType = fileType;
						newFile->extension = extension;

						RE_ProcessPath* newProcess = new RE_ProcessPath();
						newProcess->whatToDo = P_ADDFILE;
						newProcess->toProcess = newFile->AsPath();
						ret.push(newProcess);
						AddBeforeOf(newFile->AsPath(),iter);
						iter--;
					}
					else if ((*iter)->AsFile()->fType == FileType::F_META)
					{
						 bool reimport = false;
						 ResourceContainer* res = App::resources->At((*iter)->AsFile()->AsMeta()->resource);
						 if (res->GetType() == Resource_Type::R_SHADER)
						 {
							 RE_Shader* shadeRes = dynamic_cast<RE_Shader*>(res);
							 if (shadeRes->isShaderFilesChanged())
							 {
								 RE_ProcessPath* newProcess = new RE_ProcessPath();
								 newProcess->whatToDo = P_REIMPORT;
								 newProcess->toProcess = (*iter);
								 ret.push(newProcess);
							 }
						 }
					}
				}
			}
			if (iter != tree.end()) iter++;
		}
		PHYSFS_freeList(rc);
	}
	return ret;
}

eastl::stack<RE_FileSystem::RE_Path*> RE_FileSystem::RE_Directory::GetDisplayingFiles() const
{
	eastl::stack<RE_FileSystem::RE_Path*> ret;

	for (auto path : tree)
	{
		switch (path->pType)
		{
		case RE_FileSystem::PathType::D_FILE:
		{
			switch (path->AsFile()->fType)
			{
			case RE_FileSystem::FileType::F_META:
			{
				if (path->AsMeta()->resource)
				{
					ResourceContainer* res = App::resources->At(path->AsMeta()->resource);
					if (res->GetType() == R_SHADER) ret.push(path);
				}
				break;
			}
			case RE_FileSystem::FileType::F_NONE: break;
			default: ret.push(path); break;
			}
			break;
		}
		case RE_FileSystem::PathType::D_FOLDER: ret.push(path); break;
		}
	}

	return ret;
}

eastl::list<RE_FileSystem::RE_Directory*> RE_FileSystem::RE_Directory::FromParentToThis()
{
	eastl::list<RE_Directory*> ret;
	RE_Directory* iter = this;
	while (iter != nullptr)
	{
		ret.push_front(iter);
		iter = iter->parent;
	}
	return ret;
}
