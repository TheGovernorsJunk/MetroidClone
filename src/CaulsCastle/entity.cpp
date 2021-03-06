#include "entity.h"
#include "game_data.h"
#include "entity_states.h"
#include "utilities.h"

#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include <Box2D/Box2D.h>

#include <memory>

namespace te {

Entity_xml::Entity_xml(const std::string& filename)
{
	rapidxml::file<> file{ filename.c_str() };
	rapidxml::xml_document<> xml;
	xml.parse<0>(file.data());
	auto* p_root = xml.first_node("entity");

	name = p_root->first_attribute("name")->value();
	animation_group = p_root->first_node("animationgroup")->first_attribute("value")->value();
	max_speed = std::stof(p_root->first_node("speed")->first_attribute("value")->value());
	initial_state = p_root->first_node("state")->first_attribute("value")->value();

	if (auto* p_rigid_body = p_root->first_node("rigidbody")) {
		rigid_body_type = p_rigid_body->first_attribute("type")->value();
		for (auto* p_fixture = p_rigid_body->first_node("fixture"); p_fixture != NULL; p_fixture = p_fixture->next_sibling("fixture")) {
			if (p_fixture->first_attribute("type")->value() == std::string{ "rect" }) {
				auto* hitbox_node = p_fixture->first_attribute("is-hitbox");
				rect_fixtures.push_back({
					std::stof(p_fixture->first_attribute("halfwidth")->value()),
					std::stof(p_fixture->first_attribute("halfheight")->value()),
					hitbox_node && hitbox_node->value() == std::string{"true"} ? true : false
				});
			}
		}
	}

	if (auto* p_stats = p_root->first_node("stats")) {
		for (auto* p_stat = p_root->first_node("stat");
		     p_stat != NULL;
		     p_stat = p_stat->next_sibling("stat")) {
			std::string type = p_stat->first_attribute("type")->value();
			int value = std::stoi(p_stat->first_attribute("value")->value());
			if (type == "vitality") {
				stats.vitality = value;
			}
			else if (type == "endurance") {
				stats.endurance = value;
			}
			else if (type == "strength") {
				stats.strength = value;
			}
			else if (type == "dexterity") {
				stats.dexterity = value;
			}
			else {
				assert(false && "Unsupported stat type");
			}
		}
	}
}

void load_entity_xml(const std::string& filename, Game_data& data)
{
	Entity_xml entity_xml{ filename };
	std::string entity_name{ entity_xml.name };
	data.entity_table.insert(decltype(data.entity_table)::value_type({
		entity_name,
		std::move(entity_xml)
	}));
}

Entity_id make_entity(const Entity_xml& entity_xml, Game_data& data, vec2 position)
{
	auto entity_id = data.entity_manager.get_free_id();

	data.entity_animation_groups.insert(decltype(data.entity_animation_groups)::value_type{
		entity_id,
		Entity_animation{ entity_xml.animation_group, data }
	});
	auto group_found = data.entity_animation_groups.find(entity_id);
	assert(group_found != data.entity_animation_groups.end());
	set_animation(data,
		      entity_id,
		      group_found->second.idle_down);
	data.max_speeds.insert(decltype(data.max_speeds)::value_type{ entity_id, entity_xml.max_speed });

	if (entity_xml.initial_state == "normal") {
		data.normal_state_table.insert(entity_id);
	}

	data.positions[entity_id] = position;

	b2Body* p_rigid_body = nullptr;
	if (entity_xml.rigid_body_type == "dynamic") {
		b2BodyDef body_def;
		body_def.type = b2_dynamicBody;
		body_def.position = {
			position.x,
			position.y
		};
		p_rigid_body = &data.physics_manager.add_rigid_body(entity_id, body_def);
	}
	if (p_rigid_body) {
		for (const auto& rect_fixture : entity_xml.rect_fixtures) {
			b2PolygonShape rect_shape{};
			rect_shape.SetAsBox(rect_fixture.half_width, rect_fixture.half_height);
			auto* p_fixture = p_rigid_body->CreateFixture(&rect_shape, 1);
			p_fixture->SetSensor(rect_fixture.is_hitbox);
		}
	}

	auto& stats = data.stats[entity_id];
	stats.vitality = entity_xml.stats.vitality;
	stats.endurance = entity_xml.stats.endurance;
	stats.strength = entity_xml.stats.strength;
	stats.dexterity = entity_xml.stats.dexterity;

	return entity_id;
}

void load_entities_xml(const std::string& filename, Game_data& data)
{
	const auto dir = get_directory(filename);

	rapidxml::file<> file{ filename.c_str() };
	rapidxml::xml_document<> xml;
	xml.parse<0>(file.data());
	auto* p_root = xml.first_node("entities");

	for (auto* p_file = p_root->first_node("file"); p_file != NULL; p_file = p_file->next_sibling("file")) {
		std::string filename{ p_file->first_attribute("name")->value() };
		load_entity_xml(dir + filename, data);
	}
}

} // namespace te
