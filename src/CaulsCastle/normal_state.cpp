#include "normal_state.h"
#include "game_data.h"
#include <algorithm>

namespace te {

void Normal_state_table::step_input(Entity_id entity_id, Game_data& data)
{
	for (auto& input_pair : data.inputs) {
		const auto player_id = input_pair.first;
		const auto& input = input_pair.second;
		const auto player_entity_id = data.avatars[player_id];

		if (player_entity_id == entity_id) {
			using Vec = std::remove_reference_t<decltype(data.velocities[0])>;

			auto max_speed = data.max_speeds[entity_id];
			data.velocities[entity_id] = max_speed * Vec{
				input.x_movement,
				input.y_movement
			};

			if (input.light_attack.fire) {
				data.light_attack_state_table.insert(entity_id);
				exit_state(entity_id);
			}
		}
	}
}

void Normal_state_table::step_animation(Entity_id entity_id, Game_data& data)
{
	const auto& group = data.entity_animation_groups[entity_id];
	const auto velocity = data.velocities[entity_id];
	auto& animation = data.entity_animations2[entity_id];

	const auto assign_if = [&animation](bool condition,
					    Resource_id<Animation2> animation_id)
	{
		if (condition && animation.id != animation_id) {
			animation.id = animation_id;
			animation.frame_index = 0;
			animation.t = 0;
		}
	};

	auto x_mag = std::abs(velocity.x);
	auto y_mag = std::abs(velocity.y);
	if (x_mag > y_mag) {
		assign_if(velocity.x > 0, group.walk_right);
		assign_if(velocity.x < 0, group.walk_left);
	}
	else if (y_mag > x_mag) {
		assign_if(velocity.y > 0, group.walk_down);
		assign_if(velocity.y < 0, group.walk_up);
	}
	else {
		assign_if(animation.id == group.walk_right, group.idle_right);
		assign_if(animation.id == group.walk_left, group.idle_left);
		assign_if(animation.id == group.walk_down, group.idle_down);
		assign_if(animation.id == group.walk_up, group.idle_up);
	}
}

void Normal_state_table::step_records(Entity_id id, Game_data& data, float dt)
{
	step_input(id, data);
	step_animation(id, data);
}

} // namespace te

