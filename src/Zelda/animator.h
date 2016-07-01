#ifndef TE_ANIMATOR_H
#define TE_ANIMATOR_H

#include "typedefs.h"
#include "renderer.h"
#include "component.h"

#include <SFML/System.hpp>

#include <memory>

namespace te
{
	class BaseGameEntity;
	class TextureManager;
	class Animation;

	class Animator : public Component
	{
	public:
		static std::unique_ptr<Animator> make(BaseGameEntity& owner);

		void setAnimation(const std::string& animationStr);
		const std::string& getAnimation() const { return mAnimationStr; }
		void update(const sf::Time& dt);

	private:
		Animator(BaseGameEntity& owner);

		TextureManager& mTextureManager;
		Renderer<sf::Sprite>* mpSpriteRenderer;

		std::string mAnimationStr;
		const Animation* mpAnimation;
		sf::Time mCurrPlayTime;
	};
}

#endif
