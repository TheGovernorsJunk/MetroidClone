#ifndef TE_DRAW_MANAGER_H
#define TE_DRAW_MANAGER_H

#include "game_data.h"

#include <SFML/Graphics.hpp>

namespace te
{
	class DrawManager
	{
	public:
		DrawManager(const decltype(GameData::pixelToWorldScale)& pixelToWorldScale, const decltype(GameData::mainView)& mainView, decltype(GameData::pendingDraws)& pendingDraws, sf::RenderTarget& target);

		void update();
	private:
		const decltype(GameData::pixelToWorldScale)& m_rPixelToWorldScale;
		const decltype(GameData::mainView)& m_rMainView;
		decltype(GameData::pendingDraws)& m_PendingDraws;
		sf::RenderTarget& m_Target;
	};
}

#endif
