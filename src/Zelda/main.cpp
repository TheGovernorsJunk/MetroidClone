#define NOMINMAX

#include "game_data.h"
#include "manager_runner.h"
#include "scripting.h"

#include <SFML/System.hpp>
#include <lua.hpp>
#include <LuaBridge.h>

#include <Windows.h>
#include <iostream>

int main(int argc, char* argv[])
{
	try
	{
		using namespace te;

		GameData gameData;
		ManagerRunner runner{ gameData };
		ScriptInit scriptInit{ gameData };

		sf::Clock clock;
		sf::Time timeSinceLastUpdate = sf::Time::Zero;
		const sf::Time timePerFrame = sf::seconds(1.f / gameData.config.fps);

		while (gameData.pWindow->isOpen())
		{
			gameData.pWindow->clear();

			sf::Time dt = clock.restart();
			timeSinceLastUpdate += dt;
			while (timeSinceLastUpdate > timePerFrame)
			{
				timeSinceLastUpdate -= timePerFrame;

				sf::Event evt;
				while (gameData.pWindow->pollEvent(evt))
				{
					if (evt.type == sf::Event::Closed)
					{
						gameData.pWindow->close();
					}
				}

				runner.fixedUpdate(timePerFrame);
			}

			runner.renderUpdate();
			gameData.pWindow->display();
		}

		return 0;
	}
	catch (const luabridge::LuaException& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
	catch (...) {}
	return -1;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return main(0, nullptr);
}
