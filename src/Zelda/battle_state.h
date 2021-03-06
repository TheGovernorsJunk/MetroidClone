#ifndef TE_BATTLE_STATE_H
#define TE_BATTLE_STATE_H

#include "game_state.h"
#include "base_game_entity.h"
#include "game.h"
#include "world_state.h"
#include "typedefs.h"
#include "state_machine.h"
#include "state.h"
#include "animator.h"
#include "sprite_renderer.h"

#include <memory>

namespace te
{
	class BattleGame;
	class SpriteRenderer;
	class Animator;

	class Fighter : public BaseGameEntity
	{
	public:
		virtual ~Fighter() {}
		enum Message { Attack, Dodge, ImminentAttack, IncomingAttack };
		static std::unique_ptr<Fighter> make(BattleGame& world, sf::Vector2f pos = {0, 0});
		bool handleMessage(const Telegram&);
		void setFoe(EntityID id) { mFoeID = id; }
		EntityID getFoe() const { return mFoeID; }
		Animator& getAnimator() { return *mpAnimator; }
	protected:
		Fighter(BattleGame& world, sf::Vector2f pos);
		void onUpdate(const sf::Time&);
	private:
		void onDraw(sf::RenderTarget& target, sf::RenderStates states) const;
		EntityID mFoeID;
		std::unique_ptr<SpriteRenderer> mpRenderer;
		std::unique_ptr<Animator> mpAnimator;
		StateMachine<Fighter> mStateMachine;
	};

	class BattleGame : public Game
	{
	public:
		static std::unique_ptr<BattleGame> make(Application& app);
		void processInput(const sf::Event&);
		void update(const sf::Time&);
		void draw(sf::RenderTarget&, sf::RenderStates) const;
	private:
		BattleGame(Application& app);
		EntityID mPlayerID;
		EntityID mOpponentID;

		sf::Texture mIsometricTexture;
		sf::Sprite mIsometricSprite;
	};

	using BattleState = WorldState<true, true, BattleGame>;
}

#endif
