#ifndef TE_STATE_STACK_H
#define TE_STATE_STACK_H

#include "runnable.h"
#include "game_state.h"

#include <vector>
#include <memory>

namespace te
{
	class Application;

	class StateStack : public Runnable
	{
	public:
		template<typename T, typename... Args>
		static std::unique_ptr<StateStack> make(Application& app, Args&&... args)
		{
			std::unique_ptr<StateStack> pStack{new StateStack{app}};
			pStack->mStack.push_back(T::make(std::forward<Args>(args)...));
			pStack->mStack[0]->mStateStack = pStack.get();
			return pStack;
		}

		void processInput(const sf::Event&);
		void update(const sf::Time&);

	private:
		friend class GameState;

		StateStack(Application& app);

		void queuePush(std::unique_ptr<GameState>&&);
		void queuePop();
		void queueClear();

		void processPendingActions();

		enum class ActionType { Push, Pop, Clear };
		struct Action
		{
			ActionType type;
			std::unique_ptr<GameState> pState;
		};

		void draw(sf::RenderTarget&, sf::RenderStates) const;
		Application& getApplication();

		Application& mApp;
		std::vector<std::unique_ptr<GameState>> mStack;
		std::vector<Action> mPendingActions;
	};
}

#endif
