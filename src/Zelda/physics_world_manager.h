#ifndef TE_PHYSICS_WORLD_MANAGER_H
#define TE_PHYSICS_WORLD_MANAGER_H

#include "game_data.h"

class b2World;

namespace sf
{
	class Time;
}

namespace te
{
	class PhysicsWorldManager
	{
	public:
		PhysicsWorldManager(b2World&, const decltype(GameData::rigidBodies)& rigidBodies, decltype(GameData::positions)& positions);
		void update(const sf::Time& dt);
	private:
		b2World& m_rPhysicsWorld;
		const decltype(GameData::rigidBodies)& m_rRigidBodies;
		decltype(GameData::positions)& m_rPositions;
	};
}

#endif
