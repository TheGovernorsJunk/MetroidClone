#include "game_data.h"

#include <Box2D/Box2D.h>

namespace te {

Entity_manager::Entity_manager()
	: m_next_id{ 1 }
{}

Entity_id Entity_manager::get_free_id()
{
	return m_next_id++;
}

Game_data::Game_data()
	: physics_world{ std::make_unique<b2World>(b2Vec2{ 0, 0 }) }
{}
Game_data::~Game_data() = default;

Game_data::Body_deleter::Body_deleter(b2World& world)
	: p_world{ &world }
{}

void Game_data::Body_deleter::operator()(b2Body* p_body) const
{
	p_world->DestroyBody(p_body);
}

} // namespace te
