// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "PrimitiveMesh.h"
#include "Geometry.h"
namespace zone::tools {
namespace {
using namespace math;
using namespace DirectX;
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

Mesh createUVSphere(const PrimitiveInitInfo& info)
{
	const uint32 phiCount{ clamp(info.segments[Axis::X], 3u, 64u) };
	const uint32 thetaCount{ clamp(info.segments[Axis::Y], 2u, 64u) };
	const float phiStep{ two_pi / phiCount };
	const float thetaStep{ pi / thetaCount };
	const uint32 numIndices{ 6 * phiCount + 6 * phiCount * (thetaCount - 2) };
	const uint32 numVertices{ 2 + phiCount * (thetaCount - 1) };

	Mesh mesh{};
	mesh.name = "uvSphere";
	mesh.positions.resize(numVertices);

	//Add the top vertex
	uint32 count{ 0 };
	mesh.positions[count++] = { 0.0f, info.size.y, 0.0f };

	for (uint32 j{ 1 }; j <= (thetaCount - 1); ++j)
	{
		const float	theta{ j * thetaStep };
		for (uint32 i{ 0 }; i < phiCount; ++i)
		{
			const float phi{ i * phiStep };
			mesh.positions[count++] = {
				info.size.x * XMScalarSin(theta) * XMScalarCos(phi),
				info.size.y * XMScalarCos(theta),
				-info.size.z * XMScalarSin(theta) * XMScalarSin(phi)
			};
		}
	}

	// Add the bottom vertex
	mesh.positions[count++] = { 0.0f, -info.size.y, 0.0f };
	assert(count == numVertices);

	count = 0;
	mesh.rawIndices.resize(numIndices);
	utl::vector<Vec2F> uvs(numIndices);
	const float invThetaCount{ 1.0f / thetaCount };
	const float invPhiCount{ 1.0f / phiCount };

	// Indices for the top cap, connecting the north pole to the first ring
	for (uint32 i{ 0 }; i < phiCount - 1; ++i)
	{
		uvs[count] = {(2 * i + 1 ) * 0.5f * invPhiCount, 1.0f };
		mesh.rawIndices[count++] = 0;
		uvs[count] = {  i * invPhiCount, 1.0f - invThetaCount };
		mesh.rawIndices[count++] = i + 1;
		uvs[count] = { (i + 1)* invPhiCount, 1.0f - invThetaCount };
		mesh.rawIndices[count++] = i + 2;
	}
	uvs[count] = { 1.0f - 0.5f * invPhiCount, 1.0f };
	mesh.rawIndices[count++] = 0;
	uvs[count] = { 1.0f - invPhiCount, 1.0f - invThetaCount };
	mesh.rawIndices[count++] = phiCount;
	uvs[count] = { 1.0f, 1.0f - invThetaCount };
	mesh.rawIndices[count++] = 1;

	// Indices for the section between the top and bottom rings
	for (uint32 j{ 0 }; j < (thetaCount - 2); ++j)
	{
		for (uint32 i{ 0 }; i < (phiCount - 1); ++i)
		{
			const uint32 index[4]{
				1 + i + j * phiCount,
				1 + i + (j + 1) * phiCount,
				1 + (i + 1) + (j + 1) * phiCount,
				1 + (i + 1) + j * phiCount
			};

			uvs[count] = { i * invPhiCount, 1.0f - (j + 1) * invThetaCount };
			mesh.rawIndices[count++] = index[0];
			uvs[count] = { i * invPhiCount, 1.0f - (j + 2) * invThetaCount };
			mesh.rawIndices[count++] = index[1];
			uvs[count] = { (i + 1.0f) * invPhiCount, 1.0f - (j + 2) * invThetaCount };
			mesh.rawIndices[count++] = index[2];

			uvs[count] = { i * invPhiCount, 1.0f - (j + 1) * invThetaCount };
			mesh.rawIndices[count++] = index[0];
			uvs[count] = { (i + 1.0f) * invPhiCount, 1.0f - (j + 2) * invThetaCount };
			mesh.rawIndices[count++] = index[2];
			uvs[count] = { (i + 1.0f) * invPhiCount, 1.0f - (j + 1) * invThetaCount };
			mesh.rawIndices[count++] = index[3];
		}

		const uint32 index[4]{
			phiCount + j * phiCount,
			phiCount + (j + 1) * phiCount,
			1 + (j + 1) * phiCount,
			1 + j * phiCount
		};
		uvs[count] = { 1.0f - invPhiCount, 1.0f - (j + 1) * invThetaCount };
		mesh.rawIndices[count++] = index[0];
		uvs[count] = { 1.0f - invPhiCount, 1.0f - (j + 2) * invThetaCount };
		mesh.rawIndices[count++] = index[1];
		uvs[count] = { 1.0f , 1.0f - (j + 2) * invThetaCount };
		mesh.rawIndices[count++] = index[2];

		uvs[count] = { 1.0f - invPhiCount, 1.0f - (j + 1) * invThetaCount };
		mesh.rawIndices[count++] = index[0];
		uvs[count] = { 1.0f, 1.0f - (j + 2) * invThetaCount };
		mesh.rawIndices[count++] = index[2];
		uvs[count] = { 1.0f, 1.0f - (j + 1) * invThetaCount };
		mesh.rawIndices[count++] = index[3];
	}

	// Indices for the bottom cap, connecting the south pole to the last ring
	const uint32 southPoleIndex{ static_cast<uint32>(mesh.positions.size()) - 1 };
	for (uint32 i{ 0 }; i < (phiCount - 1); ++i) 
	{
		uvs[count] = { (2 * i + 1) * 0.5f * invPhiCount, 0.0f };
		mesh.rawIndices[count++] = southPoleIndex;
		uvs[count] = { (i + 1) * invPhiCount, invThetaCount };
		mesh.rawIndices[count++] = southPoleIndex - phiCount + i + 1;
		uvs[count] = { i * invPhiCount, invThetaCount };
		mesh.rawIndices[count++] = southPoleIndex - phiCount + i;
	}

	uvs[count] = {   1 - 0.5f * invPhiCount, 0.0f };
	mesh.rawIndices[count++] = southPoleIndex;
	uvs[count] = { 1, invThetaCount };
	mesh.rawIndices[count++] = southPoleIndex - phiCount;
	uvs[count] = { 1 - invPhiCount, invThetaCount };
	mesh.rawIndices[count++] = southPoleIndex - 1;

	assert(count == numIndices);

	mesh.uvSets.emplace_back(uvs);

	return mesh;
}

void createPlane(Scene& scene, const PrimitiveInitInfo& info)
{
	LodGroup lod{};
	lod.name = "plane";
	lod.meshes.emplace_back(createPlane(info));
	scene.lodGroups.emplace_back(lod);
}
void createCube(Scene& scene, const PrimitiveInitInfo& info)
{}
void createUVSphere(Scene& scene, const PrimitiveInitInfo& info)
{
	LodGroup lod{};
	lod.name = "uvSphere";
	lod.meshes.emplace_back(createUVSphere(info));
	scene.lodGroups.emplace_back(lod);
}
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