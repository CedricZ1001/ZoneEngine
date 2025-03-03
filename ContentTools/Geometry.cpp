// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.

#include "Geometry.h"

namespace zone::tools {
namespace {

using namespace math;
using namespace DirectX;

void recalculateNormals(Mesh& mesh)
{
	const uint32 numIndices{ static_cast<uint32>(mesh.rawIndices.size()) };
	mesh.normals.resize(numIndices);

	for (uint32 i{ 0 }; i < numIndices; ++i)
	{
		const uint32 i0{ mesh.rawIndices[i] };
		const uint32 i1{ mesh.rawIndices[++i] };
		const uint32 i2{ mesh.rawIndices[++i] };

		XMVECTOR v0{ XMLoadFloat3(&mesh.positions[i0]) };
		XMVECTOR v1{ XMLoadFloat3(&mesh.positions[i1]) };
		XMVECTOR v2{ XMLoadFloat3(&mesh.positions[i2]) };

		XMVECTOR e0{ v1 - v0 };
		XMVECTOR e1{ v2 - v0 };

		XMVECTOR normal{ XMVector3Normalize(XMVector3Cross(e0, e1)) };

		XMStoreFloat3(&mesh.normals[i], normal);
		mesh.normals[i - 1] = mesh.normals[i];
		mesh.normals[i - 2] = mesh.normals[i];
	}
}

void processNormals(Mesh& mesh, float smoothingAngle)
{
	const float cosAlpha{ XMScalarCos(pi - smoothingAngle * pi / 180.0f) };
	const bool isHardEdge{ XMScalarNearEqual(smoothingAngle, 180.0f, epsilon) };
	const bool isSoftEdge{ XMScalarNearEqual(smoothingAngle, 0.0f, epsilon) };
	const uint32 numIndices{ static_cast<uint32>(mesh.rawIndices.size()) };
	const uint32 numVertices{ static_cast<uint32>(mesh.positions.size()) };
	assert(numIndices && numVertices);

	mesh.indices.resize(numIndices);

	utl::vector<utl::vector<uint32>> idxRef(numVertices);
	for (uint32 i{ 0 }; i < numIndices; ++i)
	{
		idxRef[mesh.rawIndices[i]].emplace_back(i);
	}

	for (uint32 i{ 0 }; i < numVertices; ++i)
	{
		auto& refs{ idxRef[i] };
		uint32 numRefs{ static_cast<uint32>(refs.size()) };
		for (uint32 j{ 0 }; j < numRefs; ++j)
		{
			mesh.indices[refs[j]] = static_cast<uint32>(mesh.vertices.size());
			Vertex& vertex{ mesh.vertices.emplace_back() };
			vertex.position = mesh.positions[mesh.rawIndices[refs[j]]];

			XMVECTOR n1{ XMLoadFloat3(&mesh.normals[refs[j]]) };
			if (!isHardEdge)
			{
				for (uint32 k{ j + 1 }; k < numRefs; ++k)
				{
					float cosTheta{ 0.0f };
					XMVECTOR n2{ XMLoadFloat3(&mesh.normals[refs[k]]) };
					if (!isSoftEdge)
					{
						// NOTE: cos(angle) = dot(n1, n2) / (||n1|| * ||n2||)
						XMStoreFloat(&cosTheta, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
					}

					if (isSoftEdge || cosTheta >= cosAlpha)
					{
						n1 += n2;
						mesh.indices[refs[k]] = mesh.indices[refs[j]];
						refs.erase(refs.begin() + k);
						--numRefs;
						--k;
					}

				}
			}
			XMStoreFloat3(&vertex.normal, XMVector3Normalize(n1));
		}
	}
}

void processUVs(Mesh& mesh)
{
	utl::vector<Vertex> oldVertices;
	oldVertices.swap(mesh.vertices);
	utl::vector<uint32> oldIndices(mesh.indices.size());
	oldIndices.swap(mesh.indices);

	const uint32 numVertices{ static_cast<uint32>(oldVertices.size()) };
	const uint32 numIndices{ static_cast<uint32>(oldIndices.size()) };
	assert(numVertices && numIndices);

	utl::vector<utl::vector<uint32>> idxRef(numVertices);
	for (uint32 i{ 0 }; i < numIndices; ++i)
	{
		idxRef[oldIndices[i]].emplace_back(i);
	}

	for (uint32 i{ 0 }; i < numVertices; ++i)
	{
		auto& refs{ idxRef[i] };
		uint32 numRefs{ static_cast<uint32>(refs.size()) };
		for (uint32 j{ 0 }; j < numRefs; ++j)
		{
			mesh.indices[refs[j]] = static_cast<uint32>(mesh.vertices.size());
			Vertex& v{ oldVertices[oldIndices[refs[j]]] };
			v.uv = mesh.uvSets[0][refs[j]];
			mesh.vertices.emplace_back(v);

			for (uint32 k{ j + 1 }; k < numRefs; ++k)
			{
				Vec2F& uv1{ mesh.uvSets[0][refs[k]] };
				if (XMScalarNearEqual(v.uv.x, uv1.x, epsilon) && XMScalarNearEqual(v.uv.y, uv1.y, epsilon))
				{
					mesh.indices[refs[k]] = mesh.indices[refs[j]];
					refs.erase(refs.begin() + k);
					--numRefs;
					--k;
				}
			}
		}
	}

}

void packVerticesStatic(Mesh& mesh)
{
	const uint32 numVertices{ static_cast<uint32>(mesh.vertices.size()) };
	assert(numVertices);
	mesh.packedVerticesStatic.reserve(numVertices);

	for (uint32 i{ 0 }; i < numVertices; ++i)
	{
		Vertex& v{ mesh.vertices[i] };
		const uint8 signs{ static_cast<uint8>((v.normal.z > 0.0f) << 1) };
		const uint16 normalX{ static_cast<uint16>(packFloat<16>(v.tangent.x , -1.0f, 1.0f)) };
		const uint16 normalY{ static_cast<uint16>(packFloat<16>(v.tangent.y , -1.0f, 1.0f)) };

		mesh.packedVerticesStatic.emplace_back(packedvertex::VertexStatic{ v.position,{0, 0, 0},signs,{normalX,normalY},{},v.uv });
	}
}

void processVertices(Mesh& mesh, const GeometryImportSettings& settings) 
{
	assert((mesh.rawIndices.size() % 3) == 0);
	if (settings.calculateNormals||mesh.normals.empty())
	{
		recalculateNormals(mesh);
	}

	processNormals(mesh, settings.smoothingAngle);

	if (!mesh.uvSets.empty())
	{
		processUVs(mesh);
	}
	packVerticesStatic(mesh);
}

uint64 getMeshSize(const Mesh& mesh)
{
	const uint64 numVertices{ mesh.vertices.size() };
	const uint64 vertexBufferSize{ sizeof(packedvertex::VertexStatic) * numVertices };
	const uint64 indexSize{ (numVertices < (1 << 16)) ? sizeof(uint16) : sizeof(uint32) };
	const uint64 indexBufferSize{ sizeof(uint32) * mesh.indices.size() };
	constexpr uint64 su32{ sizeof(uint32) };
	const uint64 size{
		su32 + mesh.name.size() +		// Mesh name length
		su32 +							// lod id
		su32 +							// vertex buffer size
		su32 +							// number of vertices
		su32 +							// index buffer size (16 bit or 32 bit)
		su32 +							// number of indices
		sizeof(float) +					// LOD threshold
		vertexBufferSize +				// vertex buffer
		indexBufferSize					// index buffer
	};

	return size;
}

uint64 getSceneSize(const Scene& scene)
{
	constexpr uint64 su32{ sizeof(uint32) };
	uint64 size
	{
		su32 +				 // Scene name length
		scene.name.size() +  // room for scene name string
		su32				 // number of LOD groups
	};

	for (auto& lod : scene.LodGroups)
	{
		uint64 lodSize
		{
			su32 + lod.name.size() + // LOD name length
			su32					 // number of meshes in LOD
		};

		for (auto& mesh : lod.meshes)
		{
			lodSize += getMeshSize(mesh);
		}

		size += lodSize;
	}

	return size;

}

void packMeshData(const Mesh& mesh, uint8* const buffer, uint64& at)
{
	constexpr uint64 su32{ sizeof(uint32) };
	uint32 s{ 0 };

	// mesh name
	s = static_cast<uint32>(mesh.name.size());
	memcpy(&buffer[at], &s, su32);
	at += su32;
	memcpy(&buffer[at], mesh.name.c_str(), s);
	at += s;

	// lod id
	s = mesh.lodID;
	memcpy(&buffer[at], &s, su32);
	at += su32;

	// vertex size
	constexpr uint32 vertexSize{ sizeof(packedvertex::VertexStatic) };
	s = vertexSize;
	memcpy(&buffer[at], &s, su32);
	at += su32;

	// number of vertices
	const uint32 numVertices{ static_cast<uint32>(mesh.vertices.size()) };
	s = numVertices;
	memcpy(&buffer[at], &s, su32);
	at += su32;

	// index size (16 bit or 32 bit)
	const uint32 indexSize{ (numVertices < (1 << 16)) ? sizeof(uint16) : sizeof(uint32) };
	s = indexSize;
	memcpy(&buffer[at], &s, su32);
	at += su32;

	// number of indices
	const uint32 numIndices{ static_cast<uint32>(mesh.indices.size()) };
	s = numIndices;
	memcpy(&buffer[at], &s, su32);
	at += su32;

	// LOD threshold
	memcpy(&buffer[at], &mesh.lodThreshold, sizeof(float));
	at += sizeof(float);

	// vertex data
	s = vertexSize * numVertices;
	memcpy(&buffer[at], mesh.packedVerticesStatic.data(), s);
	at += s;

	//index data 
	s = indexSize * numIndices;
	void* data{ (void*)mesh.indices.data() };
	utl::vector<uint16> indices;

	if (indexSize == sizeof(uint16))
	{
		indices.resize(numIndices);
		for (uint32 i{ 0 }; i < numIndices; ++i)
		{
			indices[i] = static_cast<uint16>(mesh.indices[i]);
		}
		data = (void*)indices.data();
	}
	memcpy(&buffer[at], data, s);
	at += s;
}


} // anonymous namespace

void processScene(Scene& scene, const GeometryImportSettings& settings)
{
	for (auto& lod : scene.LodGroups)
	{
		for (auto& mesh : lod.meshes)
		{
			processVertices(mesh, settings);
		}
	}
}


void packData(const Scene& scene, SceneData& data)
{
	constexpr uint64 su32{ sizeof(uint64) };
	const uint64 sceneSize{ getSceneSize(scene) };
	data.bufferSize = static_cast<uint8>(sceneSize);
	data.buffer = static_cast<uint8*>(CoTaskMemAlloc(sceneSize));
	assert(data.buffer);

	uint8* const buffer{ data.buffer };
	uint64 at{ 0 };
	uint32 s{ 0 };

	// scene name
	s = static_cast<uint32>(scene.name.size());
	memcpy(&buffer[at], &s, su32);
	at += su32;
	memcpy(&buffer[at], scene.name.c_str(), s); 
	at += s;

	// number of LODs
	s = static_cast<uint32>(scene.LodGroups.size());
	memcpy(&buffer[at], &s, su32);
	at += su32;

	for (auto& lod : scene.LodGroups)
	{
		// LOD name
		s = static_cast<uint32>(lod.name.size());
		memcpy(&buffer[at], &s, su32);
		at += su32;
		memcpy(&buffer[at], lod.name.c_str(), s);
		at += s;

		//number of meshes in this LOD
		s = static_cast<uint32>(lod.meshes.size());
		memcpy(&buffer[at], &s, su32);
		at += su32;

		for (auto& mesh : lod.meshes)
		{
			packMeshData(mesh, buffer, at);
		}
	}

	assert(at == sceneSize);

}

}

