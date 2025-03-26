// Copyright (c) CedricZ1, 2024
// Distributed under the MIT license. See the LICENSE file in the project root for more information.

#include "Transform.h"
#include "Entity.h"

namespace zone::transform {

namespace {
utl::vector<math::Vec3F> positions;
utl::vector<math::Vec4F> rotations;
utl::vector<math::Vec3F> scales;
} // anonymous namespace

component create(init_info info, game_entity::entity entity)
{
	assert(entity.is_valid());
	const id::id_type entity_index{ id::index(entity.get_id()) };

	if (positions.size() > entity_index)
	{
		positions[entity_index] = math::Vec3F(info.position);
		rotations[entity_index] = math::Vec4F(info.rotation);
		scales[entity_index] = math::Vec3F(info.scale);
	}
	else
	{
		assert(positions.size() == entity_index);
		rotations.emplace_back(info.rotation);
		positions.emplace_back(info.position);
		scales.emplace_back(info.scale);
	}

	return component(transform_id{ (id::id_type)positions.size() - 1 });
}

void remove(component _component)
{
	assert(_component.is_valid());
}

math::Vec3F component::position() const 
{
	assert(is_valid());
	return positions[id::index(_id)];
}
math::Vec4F component::rotation() const 
{
	assert(is_valid());
	return rotations[id::index(_id)];
}
math::Vec3F component::scale() const 
{
	assert(is_valid());
	return scales[id::index(_id)];
}

}
