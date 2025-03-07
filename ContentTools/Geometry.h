// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.

#pragma once
#include "ToolsCommon.h"

namespace zone::tools {

namespace packedvertex {

struct VertexStatic
{
	math::Vec3F		position{};
	uint8			reserved[3];
	uint8			tSign;
	uint16			normal[2];
	uint16			tangent[2];
	math::Vec2F		uv;
};

} // namespace packedvertex

struct Vertex
{
	math::Vec4F tangent{};
	math::Vec3F position{};
	math::Vec3F normal{};
	math::Vec2F uv{};
};

struct Mesh
{
	//initial data
	utl::vector<math::Vec3F>					positions;
	utl::vector<math::Vec3F>					normals;
	utl::vector<math::Vec4F>					tangents;
	utl::vector<utl::vector<math::Vec2F>>		uvSets;
	utl::vector<uint32>							rawIndices;

	//Intermediate data
	utl::vector<Vertex>							vertices;
	utl::vector<uint32>							indices;

	//Output data
	std::string									name;
	utl::vector<packedvertex::VertexStatic>		packedVerticesStatic;
	float										lodThreshold{ -1.0f };
	uint32										lodID{ uint32_invalid_id };
};

struct LodGroup
{
	std::string name;
	std::vector<Mesh> meshes;
};

struct Scene 
{
	std::string name;
	std::vector<LodGroup> lodGroups;
};

struct GeometryImportSettings
{
	float smoothingAngle;
	uint8 calculateNormals;
	uint8 calculateTangents;
	uint8 reverseHandedness;
	uint8 importEmbededTextures;
	uint8 importAnimations;
};

struct SceneData
{
	uint8*					buffer;
	uint32					bufferSize;
	GeometryImportSettings	settings;
};

void processScene(Scene& scene, const GeometryImportSettings& settings);
void packData(const Scene& scene, SceneData& data);

}