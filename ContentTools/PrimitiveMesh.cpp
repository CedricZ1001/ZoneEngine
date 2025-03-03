// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "PrimitiveMesh.h"
#include "Geometry.h"
namespace zone::tools {
namespace {
using namespace math;
using PrimitiveMeshCreator = void(*)(Scene&, const PrimitiveInitInfo& info);

void createPlane(Scene& scene, const PrimitiveInitInfo& info);
void createCube(Scene& scene, const PrimitiveInitInfo& info);
void createUVSphere(Scene& scene, const PrimitiveInitInfo& info);
void createICOSphere(Scene& scene, const PrimitiveInitInfo& info);
void createCylinder(Scene& scene, const PrimitiveInitInfo& info);
void createCapsule(Scene& scene, const PrimitiveInitInfo& info);

PrimitiveMeshCreator creators[]
{
	createPlane,
	createCube,
	createUVSphere,
	createICOSphere,
	createCylinder,
	createCapsule
};

static_assert(_countof(creators) == PrimitiveMeshType::Count);

struct Axis {
	enum :uint32 {
		X = 0,
		Y = 1,
		Z = 2
	};
};

Mesh createPlane(const PrimitiveInitInfo& info,
	uint32 horizontalIndex = Axis::X, uint32 verticalIndex = Axis::Z, bool flipWinding = false,
	Vec3F offset = { -0.5f, 0.0f, -0.5f }, Vec2F uRange = { 0.0f, 1.0f }, Vec2F vRange = { 0.0f, 1.0f })
{
	assert(horizontalIndex < 3 && verticalIndex < 3);
	assert(horizontalIndex != verticalIndex);

	const uint32 horizontalCount{ clamp(info.segments[horizontalIndex], 1u, 10u) };
	const uint32 verticalCount{ clamp(info.segments[verticalIndex], 1u, 10u) };
	const float horizontalStep{ 1.0f / horizontalCount };
	const float verticalStep{ 1.0f / verticalCount };
	const float uStep{ (uRange.y - uRange.x) / horizontalCount };
	const float vStep{ (vRange.y - vRange.x) / verticalCount };

	Mesh mesh{};
	utl::vector<Vec2F> uvs;

	for (uint32 j{ 0 }; j <= verticalCount; ++j)
	{
		for (uint32 i{ 0 }; i <= horizontalCount; ++i)
		{
			Vec3F position{ offset };
			float* const asArray{ &position.x };
			asArray[horizontalIndex] += i * horizontalStep;
			asArray[verticalIndex] += j * verticalStep;
			mesh.positions.emplace_back(position.x * info.size.x, position.y * info.size.y, position.z * info.size.z);

			Vec2F uv{ uRange.x , 1.0f - vRange.x };
			uv.x += i * uStep;
			uv.y -= j * vStep;
			uvs.emplace_back(uv);
		}
	}

	assert(mesh.positions.size() == (((uint64)horizontalCount + 1) * ((uint64)verticalCount + 1)));

	const uint32 rowLength{ horizontalCount + 1 }; // number of vertices in a row
	for (uint32 j{ 0 }; j < verticalCount; ++j)
	{
		for (uint32 i{ 0 }; i < horizontalCount; ++i)
		{
			const uint32 index[4]
			{
				i + j * rowLength,
				i + (j + 1) * rowLength,
				i + 1 + j * rowLength,
				i + 1 + (j + 1) * rowLength
			};

			mesh.rawIndices.emplace_back(index[0]);
			mesh.rawIndices.emplace_back(index[flipWinding ? 2 : 1]);
			mesh.rawIndices.emplace_back(index[flipWinding ? 1 : 2]);

			mesh.rawIndices.emplace_back(index[2]);
			mesh.rawIndices.emplace_back(index[flipWinding ? 3 : 1]);
			mesh.rawIndices.emplace_back(index[flipWinding ? 1 : 3]);
		}
	}

	const uint32 numIndices{ 3 * 2 * horizontalCount * verticalCount };
	assert(mesh.rawIndices.size() == numIndices);

	mesh.uvSets.resize(1);

	for (uint32 i{ 0 }; i < numIndices; ++i)
	{
		mesh.uvSets[0].emplace_back(uvs[mesh.rawIndices[i]]);
	}

	return mesh;
}

void createPlane(Scene& scene, const PrimitiveInitInfo& info)
{
	LodGroup lod{};
	lod.name = "plane";
	lod.meshes.emplace_back(createPlane(info));
	scene.LodGroups.emplace_back(lod);
}
void createCube(Scene& scene, const PrimitiveInitInfo& info)
{}
void createUVSphere(Scene& scene, const PrimitiveInitInfo& info)
{}
void createICOSphere(Scene& scene, const PrimitiveInitInfo& info)
{}
void createCylinder(Scene& scene, const PrimitiveInitInfo& info)
{}
void createCapsule(Scene& scene, const PrimitiveInitInfo& info)
{}

} // anonymous namespace

EDITOR_INTERFACE void CreatePrimitiveMesh(SceneData* data, PrimitiveInitInfo* info)
{
	assert(data && info);
	assert(info->type < PrimitiveMeshType::Count);
	Scene scene{};
	creators[info->type](scene, *info);
	
	data->settings.calculateNormals = 1;
	processScene(scene, data->settings);
	packData(scene, *data);
}


}