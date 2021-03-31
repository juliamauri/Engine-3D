#include "RE_CompParticleEmiter.h"

#include "Application.h"
#include "ModulePhysics.h"
#include "ModuleScene.h"
#include "RE_PrimitiveManager.h"
#include "ModuleRenderer3D.h"
#include "RE_CameraManager.h"
#include "RE_ShaderImporter.h"
#include "RE_ResourceManager.h"
#include "RE_InternalResources.h"
#include "RE_Mesh.h"
#include "RE_Material.h"
#include "RE_Shader.h"
#include "RE_GLCache.h"

#include "ModuleScene.h"
#include "RE_GameObject.h"
#include "RE_CompTransform.h"
#include "RE_CompCamera.h"
#include "RE_CompPrimitive.h"

#include "ImGui\imgui.h"

RE_CompParticleEmitter::RE_CompParticleEmitter() : RE_Component(C_PARTICLEEMITER) {
}

RE_CompParticleEmitter::~RE_CompParticleEmitter()
{
	if (simulation != nullptr)
		RE_PHYSICS->RemoveEmitter(simulation);
}

void RE_CompParticleEmitter::CopySetUp(GameObjectsPool* pool, RE_Component* copy, const UID parent)
{}

void RE_CompParticleEmitter::Update()
{
}

void RE_CompParticleEmitter::Draw() const
{
	if (draw) {
		RE_Shader* pS = static_cast<RE_Shader*>(RE_RES->At(RE_RES->internalResources->GetParticleShader()));

		eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
		unsigned int shader = pS->GetID();
		RE_GLCache::ChangeShader(shader);
		if (materialMD5) dynamic_cast<RE_Material*>(RE_RES->At(materialMD5))->UploadAsParticleDataToShader(shader, useTextures, emitlight);
		else {
			RE_ShaderImporter::setFloat(shader, "useColor", 1.0f);
			RE_ShaderImporter::setFloat(shader, "useTexture", 0.0f);
			RE_ShaderImporter::setFloat(shader, "cdiffuse", { 1.0f,1.0f,1.0f });
			RE_ShaderImporter::setFloat(shader, "opacity", 1.0f);
		}

		
		RE_CompTransform* transform = static_cast<RE_CompTransform*>(pool_gos->AtCPtr(go)->GetCompPtr(C_TRANSFORM));
		math::float3 goPosition = transform->GetGlobalPosition();
		math::float3 goUp = transform->GetUp().Normalized();
		math::float3 goRight = transform->GetRight().Normalized();
		RE_CompCamera* c = ModuleRenderer3D::GetCamera();
		
		RE_CompTransform* cT = c->GetTransform();
		math::float3 cUp = cT->GetUp().Normalized();

		math::float3 pFront = direction.Normalized();

		unsigned int triangleCount = 1;
		
		if (primCmp)
		{
			RE_GLCache::ChangeVAO(primCmp->GetVAO());
			triangleCount = primCmp->GetTriangleCount();
		}
		else
			RE_GLCache::ChangeVAO(VAO);

		for (auto p : *particles) {
			math::float3 partcleGlobalpos = goPosition + p->position;

			math::float3 front, right, up;
			math::float4x4 rotm, pMatrix;
			math::float3 roteuler;
			switch (particleDir)
			{
			case RE_CompParticleEmitter::PS_FromPS:
				
				front = (goPosition - partcleGlobalpos).Normalized();

				right = front.Cross(goUp).Normalized();
				if (right.x < 0.0f) right *= -1.0f;

				up = right.Cross(front).Normalized();

				break;
			case RE_CompParticleEmitter::PS_Billboard:

				front = cT->GetGlobalPosition() - partcleGlobalpos;
				front.Normalize();

				right = front.Cross(cUp).Normalized();
				if (right.x < 0.0f) right *= -1.0f;

				up = right.Cross(front).Normalized();

				break;
			case RE_CompParticleEmitter::PS_Custom:

				front = pFront;

				right = front.Cross(cUp).Normalized();
				if (right.x < 0.0f) right *= -1.0f;

				up = right.Cross(front).Normalized();

				break;
			}

			rotm.SetRotatePart(math::float3x3(right, up, front));
			roteuler = rotm.ToEulerXYZ();

			pMatrix = math::float4x4::FromTRS(partcleGlobalpos, math::Quat::identity * math::Quat::FromEulerXYZ(roteuler.x, roteuler.y, roteuler.z), scale).Transposed();

			pS->UploadModel(pMatrix.ptr());


			if (ModuleRenderer3D::GetLightMode() == LightMode::LIGHT_DEFERRED && !materialMD5) {
				bool cNormal = !meshMD5 && !primCmp;
				RE_ShaderImporter::setFloat(shader, "customNormal", static_cast<float>(cNormal));
				if(cNormal) RE_ShaderImporter::setFloat(shader, "normal", front);
				RE_ShaderImporter::setFloat(shader, "specular", 2.5f);
				RE_ShaderImporter::setFloat(shader, "shininess", 16.0f);
			}

			if (meshMD5)
				dynamic_cast<RE_Mesh*>(RE_RES->At(meshMD5))->DrawMesh(shader);
			else
				glDrawElements(GL_TRIANGLES, triangleCount * 3, primCmp ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, nullptr);
		}
	}
}

void RE_CompParticleEmitter::DrawProperties()
{
	if (simulation == nullptr)
		simulation = RE_PHYSICS->AddEmitter();

	if (ImGui::CollapsingHeader("Particle Emitter"))
	{
		if (simulation != nullptr)
		{
			if (!draw && ImGui::Button("Start Draw")) {
				glGenVertexArrays(1, &VAO);
				glGenBuffers(1, &VBO);
				glGenBuffers(1, &EBO);

				RE_GLCache::ChangeVAO(VAO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);

				float triangle[9]{
					-0.5f, -0.5f, 0.0f,
					0.5f, -0.5f, 0.0f,
					0.0f,  0.5f, 0.0f
				};

				unsigned int index[3] = { 0, 1, 2 };

				glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), &triangle[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(unsigned int), &index[0], GL_STATIC_DRAW);

				// vertex positions
				int accumulativeOffset = 0;
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
				accumulativeOffset += sizeof(float) * 3;

				RE_GLCache::ChangeVAO(0);

				draw = true;
			}


			switch (simulation->state)
			{
			case RE_ParticleEmitter::STOP:
			{
				if (ImGui::Button("Play")) simulation->state = RE_ParticleEmitter::PLAY;
				break;
			}
			case RE_ParticleEmitter::PLAY:
			{
				if (ImGui::Button("Pause")) simulation->state = RE_ParticleEmitter::PAUSE;
				if (ImGui::Button("Stop")) simulation->state = RE_ParticleEmitter::STOP;
				break;
			}
			case RE_ParticleEmitter::PAUSE:
			{
				if (ImGui::Button("Resume")) simulation->state = RE_ParticleEmitter::PLAY;
				if (ImGui::Button("Stop")) simulation->state = RE_ParticleEmitter::STOP;
				break;
			}
			}

			ImGui::Text("Current particles: %i", RE_PHYSICS->GetParticleCount(simulation->id));

			if (ImGui::DragFloat3("Scale", scale.ptr(), 0.1f, -10000.f, 10000.f, "%.2f")) {
				if (!scale.IsFinite())scale.Set(0.5f, 0.5f, 0.5f);
			}

			int pDir = particleDir;
			if (ImGui::Combo("Particle Direction", &pDir, "Normal\0Billboard\0Custom\0")) 
				particleDir = static_cast<Particle_Dir>(pDir);

			if(particleDir == RE_CompParticleEmitter::PS_Custom)
				ImGui::DragFloat3("Custom Direction", direction.ptr(), 0.1f, -1.f, 1.f, "%.2f");

			ImGui::Checkbox(emitlight ? "Disable lighting" : "Enable lighting", &emitlight);
			ImGui::ColorEdit3("Light Color", &lightColor[0]);
			if (particleLColor) {
				if (ImGui::Button("Set particles light color")) {
					eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
					for (auto p : *particles) p->lightColor = lightColor;
				}
			}
			ImGui::DragFloat("Specular", &specular, 0.01f, 0.f, 1.f, "%.2f");
			if (particleLColor) {
				if (ImGui::Button("Set particles specular")) {
					eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
					for (auto p : *particles) p->specular = specular;
				}
			}
			ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 50.0f, "%.2f");
			if (particleLColor) {
				if (ImGui::Button("Set particles intensity")) {
					eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
					for (auto p : *particles) p->intensity = intensity;
				}
			}
			ImGui::DragFloat("Constant", &constant, 0.01f, 0.001f, 5.0f, "%.2f");
			ImGui::DragFloat("Linear", &linear, 0.001f, 0.001f, 5.0f, "%.3f");
			ImGui::DragFloat("Quadratic", &quadratic, 0.001f, 0.001f, 5.0f, "%.3f");
			ImGui::Separator();
			ImGui::Checkbox(particleLColor ? "Disable single particle lighting" : "Enable single particle lighting", &particleLColor);
			if (particleLColor) {
				if (ImGui::Button("Randomize light color")) {
					eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
					for (auto p : *particles) p->lightColor.Set(RE_MATH->RandomF(), RE_MATH->RandomF(), RE_MATH->RandomF());
				}
				static float sClamp[2] = { 0.f, 1.f };
				ImGui::DragFloat2("Specular min-max", sClamp, 0.005f, 0.0f, 1.0f);
				if (ImGui::Button("Randomize Specular")) {
					eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
					for (auto p : *particles) p->specular = RE_MATH->RandomF(sClamp[0], sClamp[1]);
				}
				static float iClamp[2] = { 0.0f, 50.0f };
				ImGui::DragFloat2("Intensity min-max", iClamp, 0.1f, 0.0f, 50.0f);
				if (ImGui::Button("Randomize intensity")) {
					eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
					for (auto p : *particles) p->intensity = RE_MATH->RandomF(iClamp[0], iClamp[1]);
				}
			}

			ImGui::Separator();

			if (meshMD5)
			{
				if (ImGui::Button(eastl::string("Resource Mesh").c_str()))
					RE_RES->PushSelected(meshMD5, true);
			}
			else if (primCmp) 
			{
				primCmp->DrawPrimPropierties();
			}
			else ImGui::TextWrapped("Select mesh resource or select primitive");

			ImGui::Separator();

			static bool clearMesh = false, setUpPrimitive = false;
			if (ImGui::BeginMenu("Primitive"))
			{
				if (ImGui::MenuItem("Point")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					clearMesh = true;
					useTextures = false;
				}
				if (ImGui::MenuItem("Cube")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompCube();
					setUpPrimitive = clearMesh = true;
					useTextures = false;
				}
				if (ImGui::MenuItem("Dodecahedron")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompDodecahedron();
					setUpPrimitive = clearMesh = true;
					useTextures = false;
				}
				if (ImGui::MenuItem("Tetrahedron")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompTetrahedron();
					setUpPrimitive = clearMesh = true;
					useTextures = false;
				}
				if (ImGui::MenuItem("Octohedron")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompOctohedron();
					setUpPrimitive = clearMesh = true;
					useTextures = false;
				}
				if (ImGui::MenuItem("Icosahedron")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompIcosahedron();
					setUpPrimitive = clearMesh = true;
					useTextures = false;
				}
				if (ImGui::MenuItem("Plane")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompPlane();
					useTextures = setUpPrimitive = clearMesh = true;
				}
				if (ImGui::MenuItem("Sphere")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompSphere();
					useTextures = setUpPrimitive = clearMesh = true;
				}
				if (ImGui::MenuItem("Cylinder")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompCylinder();
					useTextures = setUpPrimitive = clearMesh = true;
				}
				if (ImGui::MenuItem("HemiSphere")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompHemiSphere();
					useTextures = setUpPrimitive = clearMesh = true;
				}
				if (ImGui::MenuItem("Torus")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompTorus();
					useTextures = setUpPrimitive = clearMesh = true;
				}
				if (ImGui::MenuItem("Trefoil Knot")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompTrefoiKnot();
					useTextures = setUpPrimitive = clearMesh = true;
				}
				if (ImGui::MenuItem("Rock")) {
					if (primCmp) {
						primCmp->UnUseResources();
						DEL(primCmp);
					}
					primCmp = new RE_CompRock();
					setUpPrimitive = clearMesh = true;
					useTextures = false;
				}

				ImGui::EndMenu();
			}

			if (clearMesh)
			{
				if (meshMD5) {
					RE_RES->UnUse(meshMD5);
					meshMD5 = nullptr;
				}

				if (setUpPrimitive) {
					RE_SCENE->primitives->SetUpComponentPrimitive(primCmp);
					setUpPrimitive = false;
				}

				clearMesh = false;
			}



			if (ImGui::BeginMenu("Change mesh"))
			{
				eastl::vector<ResourceContainer*> meshes = RE_RES->GetResourcesByType(Resource_Type::R_MESH);
				bool none = true;
				unsigned int count = 0;
				for (auto m : meshes)
				{
					if (m->isInternal()) continue;

					none = false;
					eastl::string name = eastl::to_string(count++) + m->GetName();
					if (ImGui::MenuItem(name.c_str()))
					{
						if (meshMD5) RE_RES->UnUse(meshMD5);
						meshMD5 = m->GetMD5();
						if (meshMD5) RE_RES->Use(meshMD5);\

						useTextures = true;
					}
				}
				if (none) ImGui::Text("No custom materials on assets");

				ImGui::EndMenu();
			}

			//if (ImGui::BeginDragDropTarget())
			//{
			//	if (const ImGuiPayload* dropped = ImGui::AcceptDragDropPayload("#ModelReference"))
			//	{
			//		if (meshMD5) RE_RES->UnUse(meshMD5);
			//		meshMD5 = *static_cast<const char**>(dropped->Data);
			//		if (meshMD5) RE_RES->Use(meshMD5);
			//	}
			//	ImGui::EndDragDropTarget();
			//}



			if (!materialMD5) ImGui::Text("NMaterial not selected.");
			else
			{
				ImGui::Separator();
				RE_Material* matRes = dynamic_cast<RE_Material*>(RE_RES->At(materialMD5));
				if (ImGui::Button(matRes->GetName())) RE_RES->PushSelected(matRes->GetMD5(), true);
				
				matRes->DrawMaterialParticleEdit(useTextures);
			}


			if (ImGui::BeginMenu("Change material"))
			{
				eastl::vector<ResourceContainer*> materials = RE_RES->GetResourcesByType(Resource_Type::R_MATERIAL);
				bool none = true;
				for (auto material : materials)
				{
					if (material->isInternal()) continue;

					none = false;
					if (ImGui::MenuItem(material->GetName()))
					{
						if (materialMD5)
						{
							(dynamic_cast<RE_Material*>(RE_RES->At(materialMD5)))->UnUseResources();
							RE_RES->UnUse(materialMD5);
						}
						materialMD5 = material->GetMD5();
						if (materialMD5)
						{
							RE_RES->Use(materialMD5);
							(dynamic_cast<RE_Material*>(RE_RES->At(materialMD5)))->UseResources();
						}
					}
				}
				if (none) ImGui::Text("No custom materials on assets");
				ImGui::EndMenu();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* dropped = ImGui::AcceptDragDropPayload("#MaterialReference"))
				{
					if (materialMD5) RE_RES->UnUse(materialMD5);
					materialMD5 = *static_cast<const char**>(dropped->Data);
					if (materialMD5) RE_RES->Use(materialMD5);
				}
				ImGui::EndDragDropTarget();
			}
		}
		else
		{
			ImGui::Text("Unregistered simulation");
		}

		ImGui::Separator();

		//ImGui::SliderFloat("Emissor Life", &emissor_life, -1.0f, 10.0f, "%.2f");
		//ImGui::Separator();
		//
		//ImGui::SliderFloat("Emission Rate", &emissionRate, 0.0f, 20.f, "%.2f");
		//float ps[3] = { spawn_position_offset.x, spawn_position_offset.y, spawn_position_offset.z };
		//if (ImGui::DragFloat3("Spawn Position Offset", ps, 0.1f, -10000.f, 10000.f, "%.2f"))
		//	spawn_position_offset.Set(ps[0], ps[1], ps[2]);
		//float gp[3] = { gravity.x, gravity.y, gravity.z };
		//if (ImGui::DragFloat3("Gravity", gp, 0.1f, -10000.f, 10000.f, "%.2f"))
		//	gravity.Set(gp[0], gp[1], gp[2]);
		//ImGui::Checkbox("Local Emission", &local_emission);
		//ImGui::Separator();
		//
		//ImGui::SliderFloat("Lifetime", &lifetime, 0.0f, 10.0f, "%.2f");
		//ImGui::SliderFloat("Initial Speed", &initial_speed, 0.0f, 10.0f, "%.2f");
		//ImGui::Separator();
		//
		//float am[3] = { math::RadToDeg(direction_margin.x), math::RadToDeg(direction_margin.y), math::RadToDeg(direction_margin.z) };
		//if (ImGui::DragFloat3("Direction Margin", am, 0.1f, 0.f, 180.f, "%.2f"))
		//	direction_margin.Set(math::DegToRad(am[0]), math::DegToRad(am[1]), math::DegToRad(am[2]));
		//ImGui::SliderFloat("Speed Margin", &speed_margin, 0.0f, 10.0f, "%.2f");
		//ImGui::SliderFloat("Lifetime Margin", &lifetime_margin, 0.0f, 10.0f, "%.2f");
		//ImGui::Separator();
		//
		//float rgba[3] = { rgb_alpha.x, rgb_alpha.y, rgb_alpha.z };
		//if (ImGui::DragFloat3("RGB Alpha", rgba, 0.1f, 0.0f, 255.0f, "%.2f"))
		//	rgb_alpha.Set(rgba[0], rgba[1], rgba[2]);

		//if (mParticle)
		//{
		//	ImGui::Text("Particle Texture");
		//	//((RE_Texture*)App->resources->At(mParticle->ma))->DrawTextureImGui();
		//}
	}
}

void RE_CompParticleEmitter::SerializeJson(RE_Json* node, eastl::map<const char*, int>* resources) const
{
}

void RE_CompParticleEmitter::DeserializeJson(RE_Json* node, eastl::map<int, const char*>* resources)
{
}

unsigned int RE_CompParticleEmitter::GetBinarySize() const
{
	return 0;
}

void RE_CompParticleEmitter::SerializeBinary(char*& cursor, eastl::map<const char*, int>* resources) const
{
}

void RE_CompParticleEmitter::DeserializeBinary(char*& cursor, eastl::map<int, const char*>* resources)
{
}

void RE_CompParticleEmitter::UseResources()
{
	if (meshMD5)RE_RES->Use(meshMD5);
	if (materialMD5) {
		RE_RES->Use(materialMD5);
		dynamic_cast<RE_Material*>(RE_RES->At(materialMD5))->UseResources();
	}
}

void RE_CompParticleEmitter::UnUseResources()
{
	if (meshMD5)RE_RES->UnUse(meshMD5);
	if (materialMD5) {
		dynamic_cast<RE_Material*>(RE_RES->At(materialMD5))->UnUseResources();
		RE_RES->UnUse(materialMD5);
	}
}

bool RE_CompParticleEmitter::isLighting() const { return emitlight; }

void RE_CompParticleEmitter::CallLightShaderUniforms(unsigned int shader, const char* u_name, unsigned int& count, unsigned int maxLights, bool sharedLight) const
{
	//float cutOff[2]; // cos(radians(12.5f))
	//float outerCutOff[2]; // cos(radians(17.5f))
	//cutOff[0] = 12.5f;
	//outerCutOff[0] = 17.5f;
	//cutOff[1] = math::Cos(math::DegToRad(cutOff[0]));
	//outerCutOff[1] = math::Cos(math::DegToRad(outerCutOff[0]));

	eastl::list<RE_Particle*>* particles = RE_PHYSICS->GetParticles(simulation->id);
	RE_CompTransform* transform = GetGOPtr()->GetTransformPtr();
	math::vec objectPos = transform->GetGlobalPosition();

	eastl::string array_name(u_name);
	array_name += "[";
	eastl::string unif_name;

	if (!sharedLight) {
		eastl::string uniform_name("pInfo.tclq");
		RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (uniform_name).c_str()), float(L_POINT), constant, linear, quadratic);


		for (auto p : *particles) {
			if (count == maxLights) return;

			unif_name = array_name + eastl::to_string(count++) + "].";
			
			if (particleLColor)
				RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "diffuseSpecular").c_str()), p->lightColor.x, p->lightColor.y, p->lightColor.z, p->specular);
			else
				RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "diffuseSpecular").c_str()), lightColor.x, lightColor.y, lightColor.z, specular);

			math::float3 partcleGlobalpos = objectPos + p->position;
			RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "positionIntensity").c_str()), partcleGlobalpos.x, partcleGlobalpos.y, partcleGlobalpos.z, particleLColor ? p->intensity : intensity);
		}
	}
	else
	{
		for (auto p : *particles) {
			if (count == maxLights) return;

			unif_name = array_name + eastl::to_string(count++) + "].";

			//math::vec f = transform->GetFront();
			RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "directionIntensity").c_str()),0.0,0.0,0.0, particleLColor ? p->intensity : intensity);


			if (particleLColor)
				RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "diffuseSpecular").c_str()), p->lightColor.x, p->lightColor.y, p->lightColor.z, p->specular);
			else
				RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "diffuseSpecular").c_str()), lightColor.x, lightColor.y, lightColor.z, specular);


			//if (L_POINT != L_DIRECTIONAL)
			//{
				math::float3 partcleGlobalpos = objectPos + p->position;

				RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "positionType").c_str()), partcleGlobalpos.x, partcleGlobalpos.y, partcleGlobalpos.z, float(L_POINT));
				RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "clq").c_str()), constant, linear, quadratic, 0.0f);

				//if (type == L_SPOTLIGHT)
				//	RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "co").c_str()), cutOff[1], outerCutOff[1], 0.0f, 0.0f);
			//}
			//else
			//	RE_ShaderImporter::setFloat(RE_ShaderImporter::getLocation(shader, (unif_name + "positionType").c_str()), 0.0f, 0.0f, 0.0f, float(type));
		}

	}
}


/*
void RE_CompParticleEmitter::Update()
{
	int spawns_needed = 0;
	float dt = 0.f;// RE_TIME->GetDeltaTime();

	if (emissor_life > 0.f)
	{
		time_counter += dt;
		if (time_counter <= emissor_life)
		{
			spawn_counter += dt;
			while (spawn_counter > 1000.f / emissionRate)
			{
				spawn_counter -= (1000.f / emissionRate);
				spawns_needed++;
			}
		}
	}
	else
	{
		spawn_counter += dt;
		while (spawn_counter > 1.f / emissionRate)
		{
			spawn_counter -= (1.f / emissionRate);
			spawns_needed++;
		}
	}

	UpdateParticles(spawns_needed);
}

void RE_CompParticleEmitter::PostUpdate() {}

void RE_CompParticleEmitter::OnPlay()
{
	//RE_LOG_SECONDARY("particle emitter play");

	if (particles)
		DEL_A(particles);

	particles = new Particle[max_particles = (unsigned int)(emissionRate * (lifetime + lifetime_margin))];

	time_counter = 0.0f;
	spawn_counter = 0.0f;

	for (int i = 0; i < max_particles; i++)
		particles[i].SetUp(this);
}

void RE_CompParticleEmitter::OnPause() {}

void RE_CompParticleEmitter::OnStop()
{
	//RE_LOG_SECONDARY("particle emitter stop");
}
*/


/*
bool RE_CompParticleEmitter::LocalEmission() const { return local_emission; }
bool RE_CompParticleEmitter::EmmissionFinished() const { return emissor_life < 0.f ? false : time_counter <= emissor_life; }
RE_Mesh * RE_CompParticleEmitter::GetMesh() const { return mParticle; }


void RE_CompParticleEmitter::ResetParticle(Particle * p) { p->Emit(); }

void RE_CompParticleEmitter::UpdateParticles(int spawns_needed)
{
	Particle* p = nullptr;
	for (int i = 0; i < max_particles; i++)
	{
		if (!((p = &particles[i])->Alive()))
		{
			if (spawns_needed > 0)
			{
				ResetParticle(p);
				spawns_needed--;
			}
		}
		else p->Update();
	}
}
*/