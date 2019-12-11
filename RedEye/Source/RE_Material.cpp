#include "RE_Material.h"

#include "Application.h"
#include "RE_FileSystem.h"
#include "OutputLog.h"
#include "RE_ResourceManager.h"
#include "RE_InternalResources.h"
#include "RE_ShaderImporter.h"
#include "RE_Shader.h"
#include "RE_Texture.h"

#include "RE_GLCache.h"

#include "ModuleEditor.h"
#include "EditorWindows.h"

#include "Globals.h"

#include "ImGui/imgui.h"
#include "Glew/include/glew.h"


RE_Material::RE_Material() { }

RE_Material::RE_Material(const char* metapath) : ResourceContainer(metapath) { }
     

RE_Material::~RE_Material() { }

void RE_Material::LoadInMemory()
{
	//TODO ResourcesManager, apply count referending textures
	if (App->fs->Exists(GetLibraryPath())) {
		BinaryDeserialize();
	}
	else if (App->fs->Exists(GetAssetPath())) {
		JsonDeserialize();
		BinarySerialize();
	}
	else if (isInternal())
		ResourceContainer::inMemory = true;
	else {
		LOG_ERROR("Material %s not found on project", GetName());
	}
}

void RE_Material::UnloadMemory()
{
	cDiffuse = math::float3::zero;
	cSpecular = math::float3::zero;
	cAmbient = math::float3::zero;
	cEmissive = math::float3::zero;
	cTransparent = math::float3::zero;

	backFaceCulling = true;
	blendMode = false;

	opacity = 1.0f;
	shininess = 0.f;
	shininessStrenght = 1.0f;
	refraccti = 1.0f;

	ResourceContainer::inMemory = false;
}

void RE_Material::Import(bool keepInMemory)
{
	JsonDeserialize(true);
	BinarySerialize();
	if (!keepInMemory) UnloadMemory();
}

void RE_Material::ProcessMD5()
{
	JsonSerialize(true);
}

void RE_Material::Save()
{
	if(!shaderMD5) {
		//Default Shader
		usingOnMat[CDIFFUSE] = 1;
		usingOnMat[TDIFFUSE] = 1;
	}
	JsonSerialize();
	BinarySerialize();
	SaveMeta();
	applySave = false;
	//UnloadMemory();
}

void RE_Material::Draw()
{
	if (!isInternal() && applySave && ImGui::Button("Save Changes")) {
		Save();
		applySave = false;
	}

	if (App->resources->TotalReferenceCount(GetMD5()) == 0) {
		ImGui::Text("Material is unloaded, load for watch and modify values.");
		if (ImGui::Button("Load")) {
			LoadInMemory();
			applySave = false;
			ResourceContainer::inMemory = false;
		}
		ImGui::Separator();
	}

	DrawMaterialEdit();
}

void RE_Material::SaveResourceMeta(JSONNode* metaNode)
{
	metaNode->PushString("shaderMeta", (shaderMD5) ? App->resources->At(shaderMD5)->GetMetaPath() : "NOMETAPATH");

	JSONNode* diffuseNode = metaNode->PushJObject("DiffuseTextures");
	PushTexturesJson(diffuseNode, &tDiffuse);
	DEL(diffuseNode);
	JSONNode* specularNode = metaNode->PushJObject("SpecularTextures");
	PushTexturesJson(specularNode, &tSpecular);
	DEL(specularNode);
	JSONNode* ambientNode = metaNode->PushJObject("AmbientTextures");
	PushTexturesJson(ambientNode, &tAmbient);
	DEL(ambientNode);
	JSONNode* emissiveNode = metaNode->PushJObject("EmissiveTextures");
	PushTexturesJson(emissiveNode, &tEmissive);
	DEL(emissiveNode);
	JSONNode* opacityNode = metaNode->PushJObject("OpacityTextures");
	PushTexturesJson(opacityNode, &tOpacity);
	DEL(opacityNode);
	JSONNode* shininessNode = metaNode->PushJObject("ShininessTextures");
	PushTexturesJson(shininessNode, &tShininess);
	DEL(shininessNode);
	JSONNode* heightNode = metaNode->PushJObject("HeightTextures");
	PushTexturesJson(heightNode, &tHeight);
	DEL(heightNode);
	JSONNode* normalsNode = metaNode->PushJObject("NormalsTextures");
	PushTexturesJson(normalsNode, &tNormals);
	DEL(normalsNode);
	JSONNode* reflectionNode = metaNode->PushJObject("ReflectionTextures");
	PushTexturesJson(reflectionNode, &tReflection);
	DEL(reflectionNode);
	JSONNode* unknownNode = metaNode->PushJObject("UnknownTextures");
	PushTexturesJson(unknownNode, &tUnknown);
	DEL(unknownNode);
}

void RE_Material::LoadResourceMeta(JSONNode* metaNode)
{
	std::string shaderMeta = metaNode->PullString("shaderMeta", "NOMETAPATH");
	if (shaderMeta.compare("NOMETAPATH") != 0) shaderMD5 = App->resources->FindMD5ByMETAPath(shaderMeta.c_str(), Resource_Type::R_SHADER);

	JSONNode* diffuseNode = metaNode->PullJObject("DiffuseTextures");
	PullTexturesJson(diffuseNode, &tDiffuse);
	DEL(diffuseNode);
	JSONNode* specularNode = metaNode->PullJObject("SpecularTextures");
	PullTexturesJson(specularNode, &tSpecular);
	DEL(specularNode);
	JSONNode* ambientNode = metaNode->PullJObject("AmbientTextures");
	PullTexturesJson(ambientNode, &tAmbient);
	DEL(ambientNode);
	JSONNode* emissiveNode = metaNode->PullJObject("EmissiveTextures");
	PullTexturesJson(emissiveNode, &tEmissive);
	DEL(emissiveNode);
	JSONNode* opacityNode = metaNode->PullJObject("OpacityTextures");
	PullTexturesJson(opacityNode, &tOpacity);
	DEL(opacityNode);
	JSONNode* shininessNode = metaNode->PullJObject("ShininessTextures");
	PullTexturesJson(shininessNode, &tShininess);
	DEL(shininessNode);
	JSONNode* heightNode = metaNode->PullJObject("HeightTextures");
	PullTexturesJson(heightNode, &tHeight);
	DEL(heightNode);
	JSONNode* normalsNode = metaNode->PullJObject("NormalsTextures");
	PullTexturesJson(normalsNode, &tNormals);
	DEL(normalsNode);
	JSONNode* reflectionNode = metaNode->PullJObject("ReflectionTextures");
	PullTexturesJson(reflectionNode, &tReflection);
	DEL(reflectionNode);
	JSONNode* unknownNode = metaNode->PullJObject("UnknownTextures");
	PullTexturesJson(unknownNode, &tUnknown);
	DEL(unknownNode);
}

void RE_Material::DrawMaterialEdit()
{
	RE_Shader* matShader = (shaderMD5) ? (RE_Shader*)App->resources->At(shaderMD5) : (RE_Shader*)App->resources->At(App->internalResources->GetDefaultShader());
	
	ImGui::Text("Shader selected: %s", matShader->GetMD5());
	
	if (!shaderMD5) ImGui::Text("This shader is using the default shader.");

	if (ImGui::Button(matShader->GetName()))
		App->resources->PushSelected(matShader->GetMD5(), (App->resources->GetSelected() != nullptr && (App->resources->At(App->resources->GetSelected())->GetType() == Resource_Type::R_TEXTURE || App->resources->At(App->resources->GetSelected())->GetType() == Resource_Type::R_SHADER)));

	if (shaderMD5) {
		ImGui::SameLine();
		if (ImGui::Button("Back to Default Shader")) {
			shaderMD5 = nullptr;
			applySave = true;
			GetAndProcessUniformsFromShader();
		}
	}

	if (ImGui::BeginMenu("Change shader"))
	{
		std::vector<ResourceContainer*> shaders = App->resources->GetResourcesByType(Resource_Type::R_SHADER);
		bool none = true;
		for (auto  shader :  shaders) {
			if (shader->isInternal())
				continue;
			none = false;
			if (ImGui::MenuItem(shader->GetName())) {
				if (ResourceContainer::inMemory && shaderMD5) App->resources->UnUse(shaderMD5);
				shaderMD5 = shader->GetMD5();
				if (ResourceContainer::inMemory && shaderMD5) App->resources->Use(shaderMD5);
				GetAndProcessUniformsFromShader();

				applySave = true;
			}
		}
		if (none) ImGui::Text("No custom shaders on assets");
		ImGui::EndMenu();
	}

	if (!fromShaderCustomUniforms.empty()) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Custom values from shader")) {
			for (uint i = 0; i < fromShaderCustomUniforms.size(); i++) {
				if (fromShaderCustomUniforms[i].DrawPropieties(ResourceContainer::inMemory))
					applySave = true;
				ImGui::Separator();
			}
			ImGui::EndMenu();
		}
	}

	if (usingOnMat[CDIFFUSE] || usingOnMat[TDIFFUSE]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Diffuse values")) {
			if (usingOnMat[CDIFFUSE]) {
				if (ImGui::ColorEdit3("Diffuse Color", &cDiffuse[0]))
					applySave = true;
			}
			if(usingOnMat[TDIFFUSE]) DrawTextures("Diffuse", &tDiffuse);

			ImGui::EndMenu();
		}
	}

	if (usingOnMat[CSPECULAR] || usingOnMat[TSPECULAR]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Specular values")) {
			if (usingOnMat[CSPECULAR]) {
				if (ImGui::ColorEdit3("Specular Color", &cSpecular[0]))
					applySave = true;
			}
			if(usingOnMat[TSPECULAR]) DrawTextures("Specular", &tSpecular);
			ImGui::EndMenu();
		}
	}

	if (usingOnMat[CAMBIENT] || usingOnMat[TAMBIENT]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Ambient values")) {
			if (usingOnMat[CAMBIENT]) {
				if (ImGui::ColorEdit3("Ambient Color", &cAmbient[0]))
					applySave = true;
			}

			if (usingOnMat[TAMBIENT]) DrawTextures("Ambient", &tAmbient);
			ImGui::EndMenu();
		}
	}

	if (usingOnMat[CEMISSIVE] || usingOnMat[TEMISSIVE]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Emissive values")) {
			if (usingOnMat[CEMISSIVE]) {
				if (ImGui::ColorEdit3("Emissive Color", &cEmissive[0]))
					applySave = true;
			}
			if(usingOnMat[TEMISSIVE]) DrawTextures("Emissive", &tEmissive);
			ImGui::EndMenu();
		}
	}

	if (usingOnMat[OPACITY] || usingOnMat[TOPACITY]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Opacity values")) {
			if (usingOnMat[OPACITY]) {
				if (ImGui::InputFloat("Opacity", &opacity))
					applySave = true;
			}
			if(usingOnMat[TOPACITY]) DrawTextures("Opacity", &tOpacity);
			ImGui::EndMenu();
		}
	}

	if (usingOnMat[SHININESS] || usingOnMat[SHININESSSTRENGHT] || usingOnMat[TSHININESS]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Shininess values")) {
			if (usingOnMat[SHININESS]) {
				if (ImGui::InputFloat("Shininess", &shininess))
					applySave = true;
			}
			if (usingOnMat[SHININESSSTRENGHT]) {
				if (ImGui::InputFloat("Shininess strenght", &shininessStrenght))
					applySave = true;
			}
			if(usingOnMat[TSHININESS]) DrawTextures("Shininess", &tShininess);
			ImGui::EndMenu();
		}
	}

	if (usingOnMat[THEIGHT]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Height values")) {
			DrawTextures("Height", &tHeight);
			ImGui::EndMenu();
		}
	}
	if (usingOnMat[TNORMALS]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Normals values")) {
			DrawTextures("Normals", &tNormals);
			ImGui::EndMenu();
		}
	}
	if (usingOnMat[TREFLECTION]) {
		ImGui::Separator();
		if (ImGui::BeginMenu("Reflection values")) {
			DrawTextures("Reflection", &tReflection);
			ImGui::EndMenu();
		}
	}

	ImGui::Separator();
	if (ImGui::BeginMenu("Others values")) {
		if (usingOnMat[CTRANSPARENT]) {
			if (ImGui::ColorEdit3("Transparent", &cTransparent[0]))
				applySave = true;
			ImGui::Separator();
		}
		if (ImGui::Checkbox("Back face culling", &backFaceCulling))
			applySave = true;
		ImGui::Separator();
		if (ImGui::Checkbox("Blend mode", &blendMode))
			applySave = true;
		ImGui::Separator();
		if (usingOnMat[REFRACCTI]) {
			if (ImGui::InputFloat("Refraccti", &refraccti))
				applySave = true;
			ImGui::Separator();
		}
		DrawTextures("Unknown", &tUnknown);
		ImGui::EndMenu();
	}
}

void RE_Material::SomeResourceChanged(const char* resMD5)
{
	if (shaderMD5 == resMD5) {
		if (!isInMemory()) {
			LoadInMemory();
			ResourceContainer::inMemory = false;
		}
		std::vector<ShaderCvar> beforeCustomUniforms = fromShaderCustomUniforms;
		GetAndProcessUniformsFromShader();
		for (uint b = 0; b < beforeCustomUniforms.size(); b++) {
			for (uint i = 0; i < fromShaderCustomUniforms.size(); i++) {
				if (beforeCustomUniforms[b].name == fromShaderCustomUniforms[i].name && beforeCustomUniforms[b].GetType() == fromShaderCustomUniforms[i].GetType()) {
					fromShaderCustomUniforms[i].SetValue(beforeCustomUniforms[b]);
					break;
				}
			}
		}
		Save();
	}
}

void RE_Material::DrawTextures(const char* texturesName, std::vector<const char*>* textures)
{
	ImGui::Text(std::string(texturesName + std::string(" textures:")).c_str());
	if (!textures->empty()) {
		ImGui::Separator();
		std::vector<const char*>::iterator toDelete = textures->end();
		uint count = 1;
		for (std::vector<const char*>::iterator md5 = textures->begin(); md5 != textures->end(); ++md5) {
			ResourceContainer* resource = App->resources->At(*md5);

			if (ImGui::Button(std::string("Texture #" + std::to_string(count) + " " + std::string(resource->GetName())).c_str()))
				App->resources->PushSelected(*md5, (App->resources->GetSelected() != nullptr && (App->resources->At(App->resources->GetSelected())->GetType() == Resource_Type::R_TEXTURE || App->resources->At(App->resources->GetSelected())->GetType() == Resource_Type::R_SHADER)));
			
			ImGui::SameLine();
			std::string deletetexture("Delete #");
			deletetexture += std::to_string(count);
			if (ImGui::Button(deletetexture.c_str())) {
				toDelete = md5;
				applySave = true;
			}
			ImGui::SameLine();
			std::string changetexture("Change #");
			changetexture += std::to_string(count++);
			if (ImGui::BeginMenu(changetexture.c_str()))
			{
				std::vector<ResourceContainer*> allTex = App->resources->GetResourcesByType(Resource_Type::R_TEXTURE);
				for (auto textRes : allTex) {
					if (ImGui::MenuItem(textRes->GetName())) {
						if (ResourceContainer::inMemory) App->resources->UnUse(*md5);
						*md5 = textRes->GetMD5();
						if (ResourceContainer::inMemory) App->resources->Use(*md5);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
		}
		if (toDelete != textures->end()) {
			if(ResourceContainer::inMemory) App->resources->UnUse(*toDelete);
			textures->erase(toDelete);
		}
	}
	else {
		ImGui::Text("Empty textures");
	}
	std::string addtexture("Add #");
	addtexture += texturesName;
	addtexture += " texture";
	if (ImGui::BeginMenu(addtexture.c_str()))
	{
		std::vector<ResourceContainer*> allTex = App->resources->GetResourcesByType(Resource_Type::R_TEXTURE);
		for (auto textRes : allTex) {
			if (ImGui::MenuItem(textRes->GetName())) {
				textures->push_back(textRes->GetMD5());
				if (ResourceContainer::inMemory) App->resources->Use(textures->back());
			}
		}
		ImGui::EndMenu();
	}
}

void RE_Material::JsonDeserialize(bool generateLibraryPath)
{
	Config material(GetAssetPath(), App->fs->GetZipPath());
	if (material.Load()) {
		JSONNode* nodeMat = material.GetRootNode("Material");

		shadingType = (RE_ShadingMode)nodeMat->PullInt("ShaderType", RE_ShadingMode::S_FLAT);

		unsigned int* uOM = nodeMat->PullUInt("usingOnMat", 18, 0);
		memcpy(usingOnMat, uOM, sizeof(uint) * 18);
		DEL_A(uOM);

		cDiffuse = nodeMat->PullFloatVector("DiffuseColor", math::vec::zero);
		cSpecular = nodeMat->PullFloatVector("SpecularColor", math::vec::zero);
		cAmbient = nodeMat->PullFloatVector("AmbientColor", math::vec::zero);
		cEmissive = nodeMat->PullFloatVector("EmissiveColor", math::vec::zero);
		cTransparent = nodeMat->PullFloatVector("TransparentColor", math::vec::zero);
		backFaceCulling = nodeMat->PullBool("BackFaceCulling", true);
		blendMode = nodeMat->PullBool("BlendMode", false);
		opacity = nodeMat->PullFloat("Opacity", 1.0f);
		shininess = nodeMat->PullFloat("Shininess", 0.0f);
		shininessStrenght = nodeMat->PullFloat("ShininessStrenght", 1.0f);
		refraccti = nodeMat->PullFloat("Refraccti", 1.0f);

		fromShaderCustomUniforms.clear();
		JSONNode* nuniforms = nodeMat->PullJObject("uniforms");
		uint size = nuniforms->PullUInt("size", 0);
		if (size) {
			bool* b = nullptr;
			int* intPtr = nullptr;
			std::string id;
			for (uint i = 0; i < size; i++)
			{
				ShaderCvar sVar;
				id = "name";
				id += std::to_string(i);
				sVar.name = nuniforms->PullString(id.c_str(), "");
				id = "type";
				id += std::to_string(i);
				Cvar::VAR_TYPE vT = (Cvar::VAR_TYPE)nuniforms->PullUInt(id.c_str(), ShaderCvar::UNDEFINED);
				id = "custom";
				id += std::to_string(i);
				sVar.custom = nuniforms->PullBool(id.c_str(), true);

				id = "value";
				id += std::to_string(i);
				switch (vT)
				{
				case Cvar::BOOL:
					sVar.SetValue(nuniforms->PullBool(id.c_str(), true), true);
					break;
				case Cvar::BOOL2: 
					b = nuniforms->PullBool(id.c_str(), 2, true);
					sVar.SetValue(b, 2, true);
					DEL_A(b);
					break;
				case Cvar::BOOL3:
					b = nuniforms->PullBool(id.c_str(), 3, true);
					sVar.SetValue(b, 3, true);
					DEL_A(b);
					break;
				case Cvar::BOOL4:
					b = nuniforms->PullBool(id.c_str(), 4, true);
					sVar.SetValue(b, 4, true);
					DEL_A(b);
					break;
				case Cvar::INT:
					sVar.SetValue(-1, true);
					break;
				case Cvar::INT2:
					intPtr = nuniforms->PullInt(id.c_str(), 2, true);
					sVar.SetValue(intPtr, 2, true);
					DEL_A(b);
					break;
				case Cvar::INT3:
					intPtr = nuniforms->PullInt(id.c_str(), 3, true);
					sVar.SetValue(intPtr, 3, true);
					DEL_A(b)					
					break;
				case Cvar::INT4:
					intPtr = nuniforms->PullInt(id.c_str(), 4, true);
					sVar.SetValue(intPtr, 4, true);
					DEL_A(b)					
					break;
				case Cvar::FLOAT:
					sVar.SetValue(nuniforms->PullFloat(id.c_str(), 0), true);
					break;
				case Cvar::FLOAT2:
					sVar.SetValue(nuniforms->PullFloat(id.c_str(), math::float2::zero), true);
					break;
				case Cvar::FLOAT3:
					sVar.SetValue(nuniforms->PullFloatVector(id.c_str(), math::float3::zero), true);
					break;
				case Cvar::FLOAT4:
				case Cvar::MAT2:
					sVar.SetValue(nuniforms->PullFloat4(id.c_str(), math::float4::zero), true);
					break;
				case Cvar::MAT3:
					sVar.SetValue(nuniforms->PullMat3(id.c_str(), math::float3x3::zero), true);
					break;
				case Cvar::MAT4:
					sVar.SetValue(nuniforms->PullMat4(id.c_str(), math::float4x4::zero), true);
					break;
				case Cvar::SAMPLER:
					sVar.SetSampler(App->resources->IsReference(nuniforms->PullString(id.c_str(),"")), true);
					break;
				}

				fromShaderCustomUniforms.push_back(sVar);
			}
		}
		DEL(nuniforms);
		DEL(nodeMat);


		if (generateLibraryPath) {
			SetMD5(material.GetMd5().c_str());
			std::string libraryPath("Library/Materials/");
			libraryPath += GetMD5();
			SetLibraryPath(libraryPath.c_str());
		}

		ResourceContainer::inMemory = true;
	}
}

void RE_Material::PullTexturesJson(JSONNode * texturesNode, std::vector<const char*>* textures)
{
	uint texturesSize = texturesNode->PullInt("Size", 0);
	for (uint i = 0; i < texturesSize; i++) {
		std::string idref = "MetaPath";
		idref += std::to_string(i).c_str();
		std::string textureMaT = texturesNode->PullString(idref.c_str(), "");
		const char* textureMD5 = App->resources->FindMD5ByMETAPath(textureMaT.c_str());
		if (textureMD5) textures->push_back(textureMD5);
		else LOG_ERROR("No texture found.\nPath: %s", textureMaT.c_str());
	}
}

void RE_Material::JsonSerialize(bool onlyMD5)
{
	Config materialSerialize(GetAssetPath(), App->fs->GetZipPath());

	JSONNode* materialNode = materialSerialize.GetRootNode("Material");

	materialNode->PushString("name", GetName()); //for get different md5

	materialNode->PushInt("ShaderType", (int)shadingType);

	materialNode->PushUInt("usingOnMat", usingOnMat, 18);

	materialNode->PushFloatVector("DiffuseColor", cDiffuse);
	materialNode->PushFloatVector("SpecularColor", cSpecular);
	materialNode->PushFloatVector("AmbientColor", cAmbient);
	materialNode->PushFloatVector("EmissiveColor", cEmissive);
	materialNode->PushFloatVector("TransparentColor", cTransparent);
	materialNode->PushBool("BackFaceCulling", backFaceCulling);
	materialNode->PushBool("BlendMode", blendMode);
	materialNode->PushFloat("Opacity", opacity);
	materialNode->PushFloat("Shininess", shininess);
	materialNode->PushFloat("ShininessStrenght", shininessStrenght);
	materialNode->PushFloat("Refraccti", refraccti);

	JSONNode* nuniforms = materialNode->PushJObject("uniforms");
	nuniforms->PushUInt("size", fromShaderCustomUniforms.size());
	if (!fromShaderCustomUniforms.empty()) {
		std::string id;
		for (uint i = 0; i < fromShaderCustomUniforms.size(); i++)
		{
			id = "name";
			id += std::to_string(i);
			nuniforms->PushString(id.c_str(), fromShaderCustomUniforms[i].name.c_str());
			id = "type";
			id += std::to_string(i);
			nuniforms->PushUInt(id.c_str(), fromShaderCustomUniforms[i].GetType());
			id = "custom";
			id += std::to_string(i);
			nuniforms->PushBool(id.c_str(), fromShaderCustomUniforms[i].custom);

			id = "value";
			id += std::to_string(i);
			switch (fromShaderCustomUniforms[i].GetType())
			{
			case Cvar::BOOL:
				nuniforms->PushBool(id.c_str(), fromShaderCustomUniforms[i].AsBool());
				break;
			case Cvar::BOOL2:
				nuniforms->PushBool(id.c_str(), fromShaderCustomUniforms[i].AsBool2(), 2);
				break;
			case Cvar::BOOL3:
				nuniforms->PushBool(id.c_str(), fromShaderCustomUniforms[i].AsBool3(), 3);
				break;
			case Cvar::BOOL4:
				nuniforms->PushBool(id.c_str(), fromShaderCustomUniforms[i].AsBool4(), 4);
				break;
			case Cvar::INT:
				nuniforms->PushInt(id.c_str(), fromShaderCustomUniforms[i].AsInt());
				break;
			case Cvar::INT2:
				nuniforms->PushInt(id.c_str(), fromShaderCustomUniforms[i].AsInt2(),2);
				break;
			case Cvar::INT3:
				nuniforms->PushInt(id.c_str(), fromShaderCustomUniforms[i].AsInt3(), 3);
				break;
			case Cvar::INT4:
				nuniforms->PushInt(id.c_str(), fromShaderCustomUniforms[i].AsInt4(), 4);
					break;
			case Cvar::FLOAT:
				nuniforms->PushFloat(id.c_str(), fromShaderCustomUniforms[i].AsFloat());
				break;
			case Cvar::FLOAT2:
				nuniforms->PushFloat(id.c_str(), fromShaderCustomUniforms[i].AsFloat2());
				break;
			case Cvar::FLOAT3:
				nuniforms->PushFloatVector(id.c_str(), fromShaderCustomUniforms[i].AsFloat3());
				break;
			case Cvar::FLOAT4:
			case Cvar::MAT2:
				nuniforms->PushFloat4(id.c_str(), fromShaderCustomUniforms[i].AsFloat4());
				break;
			case Cvar::MAT3:
				nuniforms->PushMat3(id.c_str(), fromShaderCustomUniforms[i].AsMat3());
				break;
			case Cvar::MAT4:
				nuniforms->PushMat4(id.c_str(), fromShaderCustomUniforms[i].AsMat4());
				break;
			case Cvar::SAMPLER:
				nuniforms->PushString(id.c_str(), (fromShaderCustomUniforms[i].AsCharP()) ? fromShaderCustomUniforms[i].AsCharP() : "");
				break;
			}
		}
	}

	if(!onlyMD5) materialSerialize.Save();
	SetMD5(materialSerialize.GetMd5().c_str());

	std::string libraryPath("Library/Materials/");
	libraryPath += GetMD5();
	SetLibraryPath(libraryPath.c_str());

	DEL(nuniforms);
	DEL(materialNode);
}

void RE_Material::PushTexturesJson(JSONNode * texturesNode, std::vector<const char*>* textures)
{
	uint texturesSize = textures->size();
	texturesNode->PushUInt("Size", texturesSize);
	for (uint i = 0; i < texturesSize; i++) {
		std::string idref = "MetaPath";
		idref += std::to_string(i).c_str();
		texturesNode->PushString(idref.c_str(), App->resources->At(textures->operator[](i))->GetMetaPath());
	}
}

void RE_Material::BinaryDeserialize()
{
	RE_FileIO libraryFile(GetLibraryPath());
	if (libraryFile.Load()) {
		char* cursor = libraryFile.GetBuffer();

		size_t size = sizeof(RE_ShadingMode);
		memcpy(&shadingType, cursor, size);
		cursor += size;

		size = sizeof(uint) * 18;
		memcpy(usingOnMat, cursor, size);
		cursor += size;

		size = sizeof(float) * 3;
		cDiffuse = math::vec((float*)cursor);
		cursor += size;

		cSpecular = math::vec((float*)cursor);
		cursor += size;

		cAmbient = math::vec((float*)cursor);
		cursor += size;

		cEmissive = math::vec((float*)cursor);
		cursor += size;

		cTransparent = math::vec((float*)cursor);
		cursor += size;

		size = sizeof(bool);
		memcpy(&backFaceCulling, cursor, size);
		cursor += size;

		memcpy(&blendMode, cursor, size);
		cursor += size;

		size = sizeof(float);
		memcpy(&opacity, cursor, size);
		cursor += size;

		memcpy(&shininess, cursor, size);
		cursor += size;

		memcpy(&shininessStrenght, cursor, size);
		cursor += size;

		memcpy(&refraccti, cursor, size);
		cursor += size;

		fromShaderCustomUniforms.clear();
		uint uSize = 0;
		size = sizeof(uint);
		memcpy( &uSize, cursor, size);
		cursor += size;

		if (uSize > 0) {
			for (uint i = 0; i < uSize; i++) {
				ShaderCvar sVar;
				uint nSize = 0;
				size = sizeof(uint);
				memcpy(&nSize, cursor, size);
				cursor += size;

				size = sizeof(char) * nSize;
				sVar.name = std::string(cursor, nSize);
				cursor += size;

				uint uType = 0;
				size = sizeof(uint);
				memcpy(&uType, cursor, size);
				cursor += size;

				size = sizeof(bool);
				memcpy(&sVar.custom, cursor, size);
				cursor += size;

				float f;
				bool b;
				int int_v;
				int* intCursor;
				float* floatCursor;
				bool* boolCursor;
				switch (Cvar::VAR_TYPE(uType))
				{
				case Cvar::BOOL:
					size = sizeof(bool);
					memcpy( &b, cursor, size);
					cursor += size;
					sVar.SetValue(b, true);
					break;
				case Cvar::BOOL2:
					size = sizeof(bool) * 2;
					boolCursor = (bool*)cursor;
					sVar.SetValue(boolCursor, 2, true);
					cursor += size;
					break;
				case Cvar::BOOL3:
					size = sizeof(bool) * 3;
					boolCursor = (bool*)cursor;
					sVar.SetValue(boolCursor, 3, true);
					cursor += size;
					break;
				case Cvar::BOOL4:
					size = sizeof(bool) * 4;
					boolCursor = (bool*)cursor;
					sVar.SetValue(boolCursor, 4, true);
					cursor += size;
					break;
				case Cvar::INT:
					size = sizeof(int);
					memcpy(&int_v, cursor, size);
					cursor += size;
					sVar.SetValue(int_v, true);
					break;
				case Cvar::INT2:
					size = sizeof(int) * 2;
					intCursor = (int*)cursor;
					sVar.SetValue(intCursor, 2, true);
					cursor += size;
					break;
				case Cvar::INT3:
					size = sizeof(int) * 3;
					intCursor = (int*)cursor;
					sVar.SetValue(intCursor, 3, true);
					cursor += size;
					break;
				case Cvar::INT4:
					size = sizeof(int) * 4;
					intCursor = (int*)cursor;
					sVar.SetValue(intCursor, 4, true);
					cursor += size;
					break;
				case Cvar::FLOAT:
					size = sizeof(float);
					memcpy(&f, cursor, size);
					cursor += size;
					sVar.SetValue(f, true);
					break;
					break;
				case Cvar::FLOAT2:
					size = sizeof(float) * 2;
					floatCursor = (float*)cursor;
					sVar.SetValue(math::float2(floatCursor), true);
					cursor += size;					
					break;
				case Cvar::FLOAT3:
					size = sizeof(float) * 3;
					floatCursor = (float*)cursor;
					sVar.SetValue(math::float3(floatCursor), true);
					cursor += size;								
					break;
				case Cvar::FLOAT4:
				case Cvar::MAT2:
					size = sizeof(float) * 4;
					floatCursor = (float*)cursor;
					sVar.SetValue(math::float4(floatCursor), true);
					cursor += size;
					break;
				case Cvar::MAT3:
					size = sizeof(float) * 9;
					floatCursor = (float*)cursor;
					{
						math::float3x3 mat3;
						mat3.Set(floatCursor);
						sVar.SetValue(mat3, true);
					}
					cursor += size;								
					break;
				case Cvar::MAT4:
					size = sizeof(float) * 16;
					floatCursor = (float*)cursor;
					{
						math::float4x4 mat4;
						mat4.Set(floatCursor);
						sVar.SetValue(mat4, true);
					}
					cursor += size;
					break;
				case Cvar::SAMPLER:
					nSize = 0;
					size = sizeof(uint);
					memcpy( &nSize, cursor, size);
					cursor += size;
					if (nSize > 0) {
						size = sizeof(char) * nSize;
						sVar.SetSampler(App->resources->IsReference(std::string(cursor, nSize).c_str()), true);
						cursor += size;
					}
					else
						sVar.SetSampler(nullptr, true);
					break;
				}

				fromShaderCustomUniforms.push_back(sVar);
			}
		}

		ResourceContainer::inMemory = true;
	}
}

void RE_Material::BinarySerialize()
{
	RE_FileIO libraryFile(GetLibraryPath(), App->fs->GetZipPath());

	uint bufferSize = GetBinarySize();
	char* buffer = new char[bufferSize + 1];
	char* cursor = buffer;

	size_t size = sizeof(RE_ShadingMode);
	memcpy(cursor, &shadingType, size);
	cursor += size;

	size = sizeof(uint) * 18;
	memcpy(cursor, usingOnMat, size);
	cursor += size;

	size = sizeof(float) * 3;
	memcpy(cursor, &cDiffuse.x, size);
	cursor += size;

	memcpy(cursor, &cSpecular.x, size);
	cursor += size;

	memcpy(cursor, &cAmbient.x, size);
	cursor += size;

	memcpy(cursor, &cEmissive.x, size);
	cursor += size;

	memcpy(cursor, &cTransparent.x, size);
	cursor += size;

	size = sizeof(bool);
	memcpy(cursor, &backFaceCulling, size);
	cursor += size;

	memcpy(cursor, &blendMode, size);
	cursor += size;

	size = sizeof(float);
	memcpy(cursor, &opacity, size);
	cursor += size;

	memcpy(cursor, &shininess, size);
	cursor += size;

	memcpy(cursor, &shininessStrenght, size);
	cursor += size;

	memcpy(cursor, &refraccti, size);
	cursor += size;

	uint uSize = fromShaderCustomUniforms.size();
	size = sizeof(uint);
	memcpy(cursor, &uSize, size);
	cursor += size;

	if (uSize > 0) {
		for (uint i = 0; i < uSize; i++) {
			uint nSize = fromShaderCustomUniforms[i].name.size();
			size = sizeof(uint);
			memcpy(cursor, &nSize, size);
			cursor += size;

			size = sizeof(char) * nSize;
			memcpy(cursor, fromShaderCustomUniforms[i].name.c_str(), size);
			cursor += size;

			uint uType = fromShaderCustomUniforms[i].GetType();
			size = sizeof(uint);
			memcpy(cursor, &uType, size);
			cursor += size;

			size = sizeof(bool);
			memcpy(cursor, &fromShaderCustomUniforms[i].custom, size);
			cursor += size;

			bool b;
			int int_v;
			switch (fromShaderCustomUniforms[i].GetType())
			{
			case Cvar::BOOL:
				b = fromShaderCustomUniforms[i].AsBool();
				size = sizeof(bool);
				memcpy(cursor, &b, size);
				cursor += size;
				break;
			case Cvar::BOOL2:
				size = sizeof(bool)* 2;
				memcpy(cursor, fromShaderCustomUniforms[i].AsBool2(), size);
				cursor += size;
				break;
			case Cvar::BOOL3:
				size = sizeof(bool) * 3;
				memcpy(cursor, fromShaderCustomUniforms[i].AsBool3(), size);
				cursor += size;
				break;
			case Cvar::BOOL4:
				size = sizeof(bool) * 4;
				memcpy(cursor, fromShaderCustomUniforms[i].AsBool4(), size);
				cursor += size;
				break;
			case Cvar::INT:
				int_v = fromShaderCustomUniforms[i].AsInt();
				size = sizeof(int);
				memcpy(cursor, &int_v, size);
				cursor += size;				
				break;
			case Cvar::INT2:
				size = sizeof(int) * 2;
				memcpy(cursor, fromShaderCustomUniforms[i].AsInt2(), size);
				cursor += size;				
				break;
			case Cvar::INT3:
				size = sizeof(int) * 3;
				memcpy(cursor, fromShaderCustomUniforms[i].AsInt3(), size);
				cursor += size;							
				break;
			case Cvar::INT4:
				size = sizeof(int) * 4;
				memcpy(cursor, fromShaderCustomUniforms[i].AsInt4(), size);
				cursor += size;			
				break;
			case Cvar::FLOAT:
				size = sizeof(float);
				memcpy(cursor, fromShaderCustomUniforms[i].AsFloatPointer(), size);
				cursor += size;		
				break;
			case Cvar::FLOAT2:
				size = sizeof(float) * 2;
				memcpy(cursor, fromShaderCustomUniforms[i].AsFloatPointer(), size);
				cursor += size;						
				break;
			case Cvar::FLOAT3:
				size = sizeof(float) * 3;
				memcpy(cursor, fromShaderCustomUniforms[i].AsFloatPointer(), size);
				cursor += size;					
				break;
			case Cvar::FLOAT4:
			case Cvar::MAT2:
				size = sizeof(float) * 4;
				memcpy(cursor, fromShaderCustomUniforms[i].AsFloatPointer(), size);
				cursor += size;	
				break;
			case Cvar::MAT3:
				size = sizeof(float) * 9;
				memcpy(cursor, fromShaderCustomUniforms[i].AsFloatPointer(), size);
				cursor += size;	
				break;
			case Cvar::MAT4:
				size = sizeof(float) * 16;
				memcpy(cursor, fromShaderCustomUniforms[i].AsFloatPointer(), size);
				cursor += size;	
				break;
			case Cvar::SAMPLER:
				nSize = (fromShaderCustomUniforms[i].AsCharP()) ? strlen(fromShaderCustomUniforms[i].AsCharP()) : 0;
				size = sizeof(uint);
				memcpy(cursor, &nSize, size);
				cursor += size;	
				if (nSize > 0) {
					size = sizeof(char) * nSize;
					memcpy(cursor, fromShaderCustomUniforms[i].AsCharP(), size);
					cursor += size;
				}
				break;
			}
		}
	}

	char nullchar = '\0';
	memcpy(cursor, &nullchar, sizeof(char));

	libraryFile.Save(buffer, bufferSize + 1);
	DEL_A(buffer);
}

void RE_Material::UseTextureResources()
{
	if(shaderMD5) App->resources->Use(shaderMD5);

	for (auto t : tDiffuse) App->resources->Use(t);
	for (auto t : tSpecular) App->resources->Use(t);
	for (auto t : tAmbient) App->resources->Use(t);
	for (auto t : tEmissive) App->resources->Use(t);
	for (auto t : tOpacity) App->resources->Use(t);
	for (auto t : tShininess) App->resources->Use(t);
	for (auto t : tHeight) App->resources->Use(t);
	for (auto t : tNormals) App->resources->Use(t);
	for (auto t : tReflection) App->resources->Use(t);
	for (auto t : tUnknown) App->resources->Use(t);
}

void RE_Material::UnUseTextureResources()
{
	if (shaderMD5) App->resources->UnUse(shaderMD5);

	for (auto t : tDiffuse) App->resources->UnUse(t);
	for (auto t : tSpecular) App->resources->UnUse(t);
	for (auto t : tAmbient) App->resources->UnUse(t);
	for (auto t : tEmissive) App->resources->UnUse(t);
	for (auto t : tOpacity) App->resources->UnUse(t);
	for (auto t : tShininess) App->resources->UnUse(t);
	for (auto t : tHeight) App->resources->UnUse(t);
	for (auto t : tNormals) App->resources->UnUse(t);
	for (auto t : tReflection) App->resources->UnUse(t);
	for (auto t : tUnknown) App->resources->UnUse(t);

}

void RE_Material::UploadToShader(float* model, bool usingChekers)
{
	const char* usingShader = (shaderMD5) ? shaderMD5 : App->internalResources->GetDefaultShader();
	RE_Shader* shaderRes = (RE_Shader*)App->resources->At(usingShader);
	shaderRes->UploadModel(model);

	unsigned int ShaderID = shaderRes->GetID();

	bool onlyColor = true;
	if (!usingChekers) {
		if (usingOnMat[CDIFFUSE] || usingOnMat[CSPECULAR] || usingOnMat[CAMBIENT] || usingOnMat[CEMISSIVE] || usingOnMat[CTRANSPARENT] || usingOnMat[OPACITY] || usingOnMat[SHININESS] || usingOnMat[SHININESSSTRENGHT] || usingOnMat[REFRACCTI])
			RE_ShaderImporter::setFloat(ShaderID, "useColor", 1.0f);
		else
			RE_ShaderImporter::setFloat(ShaderID, "useColor", 0.0f);

		if ((usingOnMat[TDIFFUSE] && !tDiffuse.empty()) || (usingOnMat[TSPECULAR] && !tSpecular.empty())
			|| (usingOnMat[TAMBIENT] && !tAmbient.empty()) || (usingOnMat[TEMISSIVE] && !tEmissive.empty())
			|| (usingOnMat[TOPACITY] && !tOpacity.empty() || (usingOnMat[TSHININESS] && !tShininess.empty())
				|| (usingOnMat[THEIGHT] && !tHeight.empty()) || !usingOnMat[TNORMALS] && !tNormals.empty())
			|| (usingOnMat[TREFLECTION] && tReflection.empty())) {

			RE_ShaderImporter::setFloat(ShaderID, "useTexture", 1.0f);
			onlyColor = false;
		}
		else
			RE_ShaderImporter::setFloat(ShaderID, "useTexture", 0.0f);
	}
	else
	{
		RE_ShaderImporter::setFloat(ShaderID, "useColor", 0.0f);
		RE_ShaderImporter::setFloat(ShaderID, "useTexture", 1.0f);
		onlyColor = false;
	}

	unsigned int textureCounter = 0;
	// Bind Textures
	if (onlyColor){
		if(usingOnMat[CDIFFUSE]) RE_ShaderImporter::setFloat(ShaderID, "cdiffuse", cDiffuse);
		if(usingOnMat[CSPECULAR]) RE_ShaderImporter::setFloat(ShaderID, "cspecular", cSpecular);
		if(usingOnMat[CAMBIENT]) RE_ShaderImporter::setFloat(ShaderID, "cambient", cAmbient);
		if(usingOnMat[CEMISSIVE]) RE_ShaderImporter::setFloat(ShaderID, "cemissive", cEmissive);
		if(usingOnMat[CTRANSPARENT]) RE_ShaderImporter::setFloat(ShaderID, "ctransparent", cTransparent);
		if(usingOnMat[OPACITY]) RE_ShaderImporter::setFloat(ShaderID, "opacity", opacity);
		if(usingOnMat[SHININESS]) RE_ShaderImporter::setFloat(ShaderID, "shininess", shininess);
		if(usingOnMat[SHININESSSTRENGHT]) RE_ShaderImporter::setFloat(ShaderID, "shininessST", shininessStrenght);
		if(usingOnMat[REFRACCTI]) RE_ShaderImporter::setFloat(ShaderID, "refraccti", refraccti);
	}
	else if (usingChekers)
	{
		glActiveTexture(GL_TEXTURE0);
		std::string name = "tdiffuse0";
		RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), 0);
		RE_GLCache::ChangeTextureBind(App->internalResources->GetTextureChecker());
	}
	else
	{
		if (usingOnMat[CDIFFUSE]) RE_ShaderImporter::setFloat(ShaderID, "cdiffuse", cDiffuse);
		if (usingOnMat[CSPECULAR]) RE_ShaderImporter::setFloat(ShaderID, "cspecular", cSpecular);
		if (usingOnMat[CAMBIENT]) RE_ShaderImporter::setFloat(ShaderID, "cambient", cAmbient);
		if (usingOnMat[CEMISSIVE]) RE_ShaderImporter::setFloat(ShaderID, "cemissive", cEmissive);
		if (usingOnMat[CTRANSPARENT]) RE_ShaderImporter::setFloat(ShaderID, "ctransparent", cTransparent);
		if (usingOnMat[OPACITY]) RE_ShaderImporter::setFloat(ShaderID, "opacity", opacity);
		if (usingOnMat[SHININESS]) RE_ShaderImporter::setFloat(ShaderID, "shininess", shininess);
		if (usingOnMat[SHININESSSTRENGHT]) RE_ShaderImporter::setFloat(ShaderID, "shininessST", shininessStrenght);
		if (usingOnMat[REFRACCTI]) RE_ShaderImporter::setFloat(ShaderID, "refraccti", refraccti);

		for (unsigned int i = 0; i < tDiffuse.size() || i < usingOnMat[TDIFFUSE]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "tdiffuse";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tDiffuse[i]))->use();
		}
		for (unsigned int i = 0; i < tSpecular.size() || i < usingOnMat[TSPECULAR]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "tspecular";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tSpecular[i]))->use();
		}
		for (unsigned int i = 0; i < tAmbient.size() || i < usingOnMat[TAMBIENT]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "tambient";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tAmbient[i]))->use();
		}
		for (unsigned int i = 0; i < tEmissive.size() || i < usingOnMat[TEMISSIVE]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "temissive";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tEmissive[i]))->use();
		}
		for (unsigned int i = 0; i < tOpacity.size() || i < usingOnMat[TOPACITY]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "topacity";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tOpacity[i]))->use();
		}
		for (unsigned int i = 0; i < tShininess.size() || i < usingOnMat[TSHININESS]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "tshininess";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tShininess[i]))->use();
		}
		for (unsigned int i = 0; i < tHeight.size() || i < usingOnMat[THEIGHT]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "theight";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tHeight[i]))->use();
		}
		for (unsigned int i = 0; i < tNormals.size() || i < usingOnMat[TNORMALS]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "tnormals";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tNormals[i]))->use();
		}
		for (unsigned int i = 0; i < tReflection.size() || i < usingOnMat[TREFLECTION]; i++)
		{
			glActiveTexture(GL_TEXTURE0 + textureCounter);
			std::string name = "treflection";
			name += std::to_string(i);
			RE_ShaderImporter::setUnsignedInt(ShaderID, name.c_str(), textureCounter++);
			((RE_Texture*)App->resources->At(tReflection[i]))->use();
		}
	}

	for (uint i = 0; i < fromShaderCustomUniforms.size(); i++)
	{
		bool* b;
		int* int_pv;
		float* f_ptr;
		switch (fromShaderCustomUniforms[i].GetType())
		{
		case Cvar::BOOL:
			RE_ShaderImporter::setBool(ShaderID, fromShaderCustomUniforms[i].name.c_str(), fromShaderCustomUniforms[i].AsBool());
			break;
		case Cvar::BOOL2:
			b = fromShaderCustomUniforms[i].AsBool2();
			RE_ShaderImporter::setBool(ShaderID, fromShaderCustomUniforms[i].name.c_str(), b[0], b[1]);
			break;
		case Cvar::BOOL3:
			b = fromShaderCustomUniforms[i].AsBool3();
			RE_ShaderImporter::setBool(ShaderID, fromShaderCustomUniforms[i].name.c_str(), b[0], b[1], b[2]);
			break;
		case Cvar::BOOL4:
			b = fromShaderCustomUniforms[i].AsBool4();
			RE_ShaderImporter::setBool(ShaderID, fromShaderCustomUniforms[i].name.c_str(), b[0], b[1], b[2], b[3]);
			break;
		case Cvar::INT:
			RE_ShaderImporter::setInt(ShaderID, fromShaderCustomUniforms[i].name.c_str(), fromShaderCustomUniforms[i].AsInt());
			break;
		case Cvar::INT2:
			int_pv = fromShaderCustomUniforms[i].AsInt2();
			RE_ShaderImporter::setInt(ShaderID, fromShaderCustomUniforms[i].name.c_str(), int_pv[0], int_pv[1]);
			break;
		case Cvar::INT3:
			int_pv = fromShaderCustomUniforms[i].AsInt3();
			RE_ShaderImporter::setInt(ShaderID, fromShaderCustomUniforms[i].name.c_str(), int_pv[0], int_pv[1], int_pv[2]);
			break;
		case Cvar::INT4:
			int_pv = fromShaderCustomUniforms[i].AsInt4();
			RE_ShaderImporter::setInt(ShaderID, fromShaderCustomUniforms[i].name.c_str(), int_pv[0], int_pv[1], int_pv[2], int_pv[3]);
			break;
		case Cvar::FLOAT:
			RE_ShaderImporter::setFloat(ShaderID, fromShaderCustomUniforms[i].name.c_str(), fromShaderCustomUniforms[i].AsFloat());
			break;
		case Cvar::FLOAT2:
			f_ptr = fromShaderCustomUniforms[i].AsFloatPointer();
			RE_ShaderImporter::setFloat(ShaderID, fromShaderCustomUniforms[i].name.c_str(), f_ptr[0], f_ptr[1]);
			break;
		case Cvar::FLOAT3:
			f_ptr = fromShaderCustomUniforms[i].AsFloatPointer();
			RE_ShaderImporter::setFloat(ShaderID, fromShaderCustomUniforms[i].name.c_str(), f_ptr[0], f_ptr[1], f_ptr[2]);
			break;
		case Cvar::FLOAT4:
		case Cvar::MAT2:
			f_ptr = fromShaderCustomUniforms[i].AsFloatPointer();
			RE_ShaderImporter::setFloat(ShaderID, fromShaderCustomUniforms[i].name.c_str(), f_ptr[0], f_ptr[1], f_ptr[2], f_ptr[3]);
			break;
		case Cvar::MAT3:
			f_ptr = fromShaderCustomUniforms[i].AsFloatPointer();
			RE_ShaderImporter::setFloat3x3(ShaderID, fromShaderCustomUniforms[i].name.c_str(), f_ptr);
			break;
		case Cvar::MAT4:
			f_ptr = fromShaderCustomUniforms[i].AsFloatPointer();
			RE_ShaderImporter::setFloat4x4(ShaderID, fromShaderCustomUniforms[i].name.c_str(), f_ptr);
			break;
		case Cvar::SAMPLER:
			if (fromShaderCustomUniforms[i].AsCharP()) {
				glActiveTexture(GL_TEXTURE0 + textureCounter);
				RE_ShaderImporter::setUnsignedInt(ShaderID, fromShaderCustomUniforms[i].name.c_str(), textureCounter++);
				((RE_Texture*)App->resources->At(fromShaderCustomUniforms[i].AsCharP()))->use();
			}
			break;
		}
	}
}

unsigned int RE_Material::GetShaderID() const
{
	const char* usingShader = (shaderMD5) ? shaderMD5 : App->internalResources->GetDefaultShader();
	return ((RE_Shader*)App->resources->At(usingShader))->GetID();
}

unsigned int RE_Material::GetBinarySize()
{
	uint charCount = 0;

	charCount += sizeof(RE_ShadingMode);
	charCount += sizeof(uint) * 18;
	charCount += sizeof(float) * 15;
	charCount += sizeof(bool) * 2;
	charCount += sizeof(float) * 4;

	//Custom uniforms
	charCount += sizeof(uint);
	if (!fromShaderCustomUniforms.empty()) {
		charCount += (sizeof(uint) * 2 + sizeof(float)) * fromShaderCustomUniforms.size();
		for (uint i = 0; i < fromShaderCustomUniforms.size(); i++) {
			charCount += sizeof(char) * fromShaderCustomUniforms[i].name.size();
			switch (fromShaderCustomUniforms[i].GetType())
			{
			case Cvar::BOOL:
				charCount += sizeof(bool);
				break;
			case Cvar::BOOL2:
				charCount += sizeof(bool) * 2;
				break;
			case Cvar::BOOL3:
				charCount += sizeof(bool) * 3;
				break;
			case Cvar::BOOL4:
				charCount += sizeof(bool) * 4;
				break;
			case Cvar::INT:
				charCount += sizeof(int);
				break;
			case Cvar::INT2:
				charCount += sizeof(int) * 2;
				break;
			case Cvar::INT3:
				charCount += sizeof(int) * 3;
				break;
			case Cvar::INT4:
				charCount += sizeof(int) * 4;
				break;
			case Cvar::FLOAT:
				charCount += sizeof(float);
				break;
			case Cvar::FLOAT2:
				charCount += sizeof(float) * 2;
				break;
			case Cvar::FLOAT3:
				charCount += sizeof(float) * 3;
				break;
			case Cvar::FLOAT4:
			case Cvar::MAT2:
				charCount += sizeof(float) * 4;
				break;
			case Cvar::MAT3:
				charCount += sizeof(float) * 9;
				break;
			case Cvar::MAT4:
				charCount += sizeof(float) * 16;
				break;
			case Cvar::SAMPLER:
				charCount += sizeof(uint);
				if(fromShaderCustomUniforms[i].AsCharP()) charCount += sizeof(char) * strlen(fromShaderCustomUniforms[i].AsCharP());
				break;
			}
		}
	}
	return charCount;
}

void RE_Material::GetAndProcessUniformsFromShader()
{
	static const char* materialNames[18] = {  "cdiffuse", "tdiffuse", "cspecular", "tspecular", "cambient",  "tambient", "cemissive", "temissive", "ctransparent", "opacity", "topacity",  "shininess", "shininessST", "tshininess", "refraccti", "theight",  "tnormals", "treflection" };
	static const char* materialTextures[9] = {  "tdiffuse", "tspecular", "tambient", "temissive", "topacity", "tshininess", "theight", "tnormals", "treflection" };
	for (uint i = 0; i < 18; i++) usingOnMat[i] = 0;
	fromShaderCustomUniforms.clear();

	if (shaderMD5) {
		std::vector<ShaderCvar> fromShader = ((RE_Shader*)App->resources->At(shaderMD5))->GetUniformValues();
		for (auto sVar : fromShader) {
			int pos = sVar.name.find_first_of("0123456789");

			std::string name = (pos != std::string::npos) ? sVar.name.substr(0,pos) : sVar.name;
			if (sVar.custom) fromShaderCustomUniforms.push_back(sVar);
			else {
				MaterialUINT index = UNDEFINED;
				for (uint i = 0; i < 18; i++) {
					if (name.compare(materialNames[i]) == 0) {
						index = (MaterialUINT)i;
						break;
					}
				}
				if (index != UNDEFINED) {
					bool texture = false;
					for (uint i = 0; i < 9; i++) {
						if (name.compare(materialTextures[i]) == 0) {
							texture = true;
							break;
						}
					}	
					if (texture) {
						int count = std::stoi(&sVar.name.back()) + 1;
						if (usingOnMat[index] < count) usingOnMat[index] = count;
					}
					else
						usingOnMat[index] = 1;
				}
			}
		}
	}
	else {
		//Default Shader
		usingOnMat[CDIFFUSE] = 1;
		usingOnMat[TDIFFUSE] = 1;
	}
}