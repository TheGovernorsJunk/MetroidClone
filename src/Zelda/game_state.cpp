#include "game_state.h"
#include "state_stack.h"
#include "application.h"

#include <cassert>

namespace te
{
	GameState::GameState(StateStack& ss)
		: mStateStack(ss)
	{}

	void GameState::popState()
	{
		mStateStack.queuePop();
	}

	void GameState::clearStates()
	{
		mStateStack.queueClear();
	}

	TextureManager& GameState::getTextureManager()
	{
		return mStateStack.getApplication().getTextureManager();
	}
}
