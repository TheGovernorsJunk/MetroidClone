#include "state_stack.h"

namespace te
{
	StateFactory::StateFactory(StateStack& ss)
		: mStack(ss)
	{}

	StateStack::StateStack(Application& app)
		: mApp(app)
		, mStateFactory(*this)
		, mStack()
		, mPendingActions()
	{}

	void StateStack::queuePush(std::unique_ptr<GameState>&& pState)
	{
		mPendingActions.push_back(Action{
			ActionType::Push,
			std::move(pState)
		});
	}
	void StateStack::queuePop()
	{
		mPendingActions.push_back(Action{
			ActionType::Pop,
			nullptr
		});
	}

	void StateStack::queueClear()
	{
		mPendingActions.push_back(Action{
			ActionType::Clear,
			nullptr
		});
	}

	void StateStack::processInput(const sf::Event& evt)
	{
		for (auto iter = mStack.rbegin(); iter != mStack.rend(); ++iter)
			if ((*iter)->processInput(evt)) break;
	}

	void StateStack::update(const sf::Time& dt)
	{
		for (auto iter = mStack.rbegin(); iter != mStack.rend(); ++iter)
			if ((*iter)->update(dt)) break;
		processPendingActions();
	}

	void StateStack::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (auto& state : mStack) target.draw(*state, states);
	}

	Application& StateStack::getApplication()
	{
		return mApp;
	}

	const StateFactory& StateStack::getStateFactory() const
	{
		return mStateFactory;
	}

	void StateStack::processPendingActions()
	{
		for (auto& action : mPendingActions)
		{
			switch (action.type)
			{
			case ActionType::Push:
				mStack.push_back(std::move(action.pState));
				break;
			case ActionType::Pop:
				mStack.pop_back();
				break;
			case ActionType::Clear:
				mStack.clear();
				break;
			}
		}

		mPendingActions.clear();
	}
}
