#include "scripted_game.h"
#include "utilities.h"
#include "tmx.h"
#include "tile_map.h"
#include "scripted_entity.h"
#include "texture_manager.h"
#include "entity_manager.h"
#include "camera_entity.h"
#include "vector_ops.h"
#include "rigid_body.h"
#include "tile_map_layer.h"
#include "renderer.h"

#include <SFML/Window.hpp>
#include <Box2D/Box2D.h>

#include <regex>
#include <iterator>

namespace te
{
	// LuaBridge does not support enum
	static int KeyPressed = sf::Event::KeyPressed;
	static int KeyReleased = sf::Event::KeyReleased;

	static int MouseButtonPressed = sf::Event::MouseButtonPressed;
	static int MouseButtonReleased = sf::Event::MouseButtonReleased;

	static int A = 0;
	static int B = 1;
	static int C = 2;
	static int D = 3;
	static int E = 4;
	static int F = 5;
	static int G = 6;
	static int H = 7;
	static int I = 8;
	static int J = 9;
	static int K = 10;
	static int L = 11;
	static int M = 12;
	static int N = 13;
	static int O = 14;
	static int P = 15;
	static int Q = 16;
	static int R = 17;
	static int S = 18;
	static int T = 19;
	static int U = 20;
	static int V = 21;
	static int W = 22;
	static int X = 23;
	static int Y = 24;
	static int Z = 25;
	static int Space = sf::Keyboard::Space;

	static int AxisX = sf::Joystick::X;
	static int AxisY = sf::Joystick::Y;

	static int MouseLeft = sf::Mouse::Button::Left;
	static int MouseRight = sf::Mouse::Button::Right;

	static int StaticBody = b2_staticBody;
	static int KinematicBody = b2_kinematicBody;
	static int DynamicBody = b2_dynamicBody;

	const static std::regex packageExpr{"(?:.*/)*([a-zA-Z_0-9]+)\\.lua"};

	static sf::Vector2f addVec(sf::Vector2f a, sf::Vector2f b) { return a + b; }
	static sf::Vector2f mulVec(float scalar, sf::Vector2f v) { return scalar * v; }

	PolygonShape ScriptedGame::getShape(luabridge::LuaRef obj, EntityID localNodeID) const
	{
		sf::Transform transform = getEntityManager().getEntityFromID(localNodeID).getTransform() * getTransform();
		//sf::Vector2f localPos = localNode->getPosition();
		float x = obj["x"], y = obj["y"], h = obj["h"], w = obj["w"];
		sf::Vector2f v0 = transform * sf::Vector2f{x, y};
		sf::Vector2f v1 = transform * sf::Vector2f{x + w, y};
		sf::Vector2f v2 = transform * sf::Vector2f{x + w, y + h};
		sf::Vector2f v3 = transform * sf::Vector2f{x, y + h};
		b2Vec2 vertices[4];
		vertices[0].Set(v0.x, v0.y);
		vertices[1].Set(v1.x, v1.y);
		vertices[2].Set(v2.x, v2.y);
		vertices[3].Set(v3.x, v3.y);

		b2PolygonShape polygon;
		polygon.Set(vertices, 4);
		return PolygonShape{polygon};
	}

	PolygonShape ScriptedGame::makePolygon(luabridge::LuaRef vertices) const
	{
		b2PolygonShape polygon;

		std::vector<b2Vec2> b2vertices;
		for (auto it = luabridge::Iterator(vertices); !it.isNil(); ++it)
		{
			luabridge::LuaRef vertex = *it;
			b2vertices.push_back({vertex["x"], vertex["y"]});
		}

		polygon.Set(b2vertices.data(), b2vertices.size());
		return PolygonShape{polygon};
	}

	ScriptedGame::ScriptedGame(Application& app, const std::string& initFilename)
		: Game{app}
		, mpL{luaL_newstate(), [](lua_State* L) { lua_close(L); }}
		, mKeyInputFn{mpL.get()}
		, mMouseButtonInputFn{mpL.get()}
		, mAxisInputFn{mpL.get()}
		, mUpdateFn{mpL.get()}
		, mpCamera{nullptr}
	{
		auto pCamera = CameraEntity::make(*this);
		mpCamera = pCamera.get();
		addEntity(std::move(pCamera));

		lua_State* L = mpL.get();

		luaL_openlibs(L);

		luabridge::getGlobalNamespace(L)
			.beginNamespace("Event")
				.addVariable("KeyPressed", &KeyPressed, false)
				.addVariable("KeyReleased", &KeyReleased, false)
				.addVariable("MouseButtonPressed", &MouseButtonPressed, false)
				.addVariable("MouseButtonReleased", &MouseButtonReleased, false)
			.endNamespace()
			.beginNamespace("Key")
				.addVariable("W", &W, false)
				.addVariable("A", &A, false)
				.addVariable("S", &S, false)
				.addVariable("D", &D, false)
				.addVariable("Space", &Space, false)
			.endNamespace()
			.beginNamespace("Axis")
				.addVariable("X", &AxisX, false)
				.addVariable("Y", &AxisY, false)
			.endNamespace()
			.beginNamespace("Mouse")
				.addVariable("Left", &MouseLeft, false)
				.addVariable("Right", &MouseRight, false)
			.endNamespace()
			.beginNamespace("BodyType")
				.addVariable("Static", &StaticBody, false)
				.addVariable("Kinematic", &KinematicBody, false)
				.addVariable("Dynamic", &DynamicBody, false)
			.endNamespace()
			.beginClass<sf::Vector2f>("Vec")
				.addConstructor<void(*)(float,float)>()
				.addData("x", &sf::Vector2f::x)
				.addData("y", &sf::Vector2f::y)
			.endClass()
			.beginClass<Shape>("Shape")
			.endClass()
			.deriveClass<PolygonShape, Shape>("PolygonShape")
			.endClass()
			.deriveClass<CircleShape, Shape>("CircleShape")
				.addConstructor<void(*)(sf::Vector2f, float)>()
			.endClass()
			.beginClass<AABB>("AABB")
				.addConstructor<void(*)(sf::Vector2f, sf::Vector2f)>()
			.endClass()
			.addFunction("addVec", &addVec)
			.addFunction("mulVec", &mulVec)
			.addFunction<sf::Vector2f(*)(const sf::Vector2f&)>("normalizeVec", &normalize)
			.beginClass<RayCastHit>("RayCastHit")
				.addConstructor<void(*)(void)>()
				.addData("entityID", &RayCastHit::entityID, false)
				.addData("point", &RayCastHit::point, false)
			.endClass()
			.beginClass<ScriptedTelegram>("Telegram")
				.addData("dispatchTime", &ScriptedTelegram::dispatchTime)
				.addData("sender", &ScriptedTelegram::sender)
				.addData("receiver", &ScriptedTelegram::receiver)
				.addData("info", &ScriptedTelegram::info)
			.endClass()
			.beginClass<ResourceID<TMX>>("TMXID").endClass()
			.beginClass<ResourceID<TextureAtlas>>("AtlasID").endClass()
			.beginClass<ResourceID<sf::Texture>>("TextureID").endClass()
			.beginClass<ResourceID<sf::Sprite>>("SpriteID").endClass()
			.beginClass<ResourceID<Animation>>("AnimationID").endClass()
			.beginClass<ResourceID<TileMapLayer>>("MapLayerID").endClass()
			.beginClass<TileMapLayer>("TileMapLayer")
				.addProperty("index", &TileMapLayer::getIndex)
			.endClass()
			.beginClass<Game>("Game")
				.addProperty("unitToPixelScale", &Game::getUnitToPixelScale, &Game::setUnitToPixelScale)
				.addFunction("loadTMX", &Game::load<TMX>)
				.addFunction("getMapLayer", &Game::get<TileMapLayer>)
				.addFunction("loadAtlas", &Game::load<TextureAtlas>)
				.addFunction("loadTexture", &Game::load<sf::Texture>)
			.endClass()
			.deriveClass<ScriptedGame, Game>("ScriptedGame")
				.addFunction("makeMapLayers", &ScriptedGame::makeMapLayers)
				.addFunction("getSpriteData", &ScriptedGame::getAtlasSprites)
				.addFunction("makeSprite", &ScriptedGame::makeSprite)
				.addFunction("makeAnimation", &ScriptedGame::makeAnimation)
				.addFunction("makeEntity", &ScriptedGame::makeEntity)
				.addFunction("getEntity", &ScriptedGame::getScriptedEntity)
				.addProperty("camera", &ScriptedGame::getCamera)
				.addFunction("dispatchMessage", &ScriptedGame::dispatchMessage)
				.addFunction("getMap", &ScriptedGame::getMap)
				.addFunction("getObjects", &ScriptedGame::getObjects)
				.addFunction("getLayerNames", &ScriptedGame::getLayerNames)
				.addFunction("getAnimationDuration", &ScriptedGame::getAnimationDuration)
				.addFunction("rayCast", &ScriptedGame::rayCast)
				.addFunction("getEntitiesInRegion", &ScriptedGame::getEntitiesInRegion)
				.addFunction("getShape", &ScriptedGame::getShape)
				.addFunction("makePolygon", &ScriptedGame::makePolygon)
			.endClass()
			.beginClass<BaseGameEntity>("BaseGameEntity")
				.addProperty("position", &BaseGameEntity::getPosition, &BaseGameEntity::setPosition)
				.addFunction<void(BaseGameEntity::*)(float,float)>("move", &BaseGameEntity::move)
				.addFunction("die", &BaseGameEntity::die)
				.addProperty("rigidBody", &BaseGameEntity::getComponent<RigidBody>)
				.addProperty("spriteRenderer", &BaseGameEntity::getComponent<Renderer<sf::Sprite>>)
				.addFunction("addRigidBody", &BaseGameEntity::addComponent<RigidBody, const int&>)
				.addFunction("addSpriteRenderer", &BaseGameEntity::addComponent<Renderer<sf::Sprite>>)
				.addFunction("addAnimator", &BaseGameEntity::addComponent<Animator>)
				.addFunction("addLayerRenderer", &BaseGameEntity::addComponent<Renderer<TileMapLayer>>)
			.endClass()
			.deriveClass<CameraEntity, BaseGameEntity>("Camera")
				.addFunction("setViewSize", &CameraEntity::setViewSize)
			.endClass()
			.deriveClass<TileMap, BaseGameEntity>("TileMap")
			.endClass()
			.deriveClass<ScriptedEntity, BaseGameEntity>("Entity")
				.addProperty("world", &ScriptedEntity::getWorld)
				.addProperty("data", &ScriptedEntity::getUserData)
				.addFunction("initMachine", &ScriptedEntity::initMachine)
			.endClass()
			.beginClass<RigidBody>("RigidBody")
				.addFunction("attachFixture", &RigidBody::attachFixture)
				.addFunction("setVelocity", &RigidBody::setVelocity)
			.endClass()
			.beginClass<Animator>("Animator")
				.addProperty("animation", &Animator::getAnimation, &Animator::setAnimation)
			.endClass()
			.beginClass<DrawComponent>("DrawComponent")
				.addProperty("drawOrder", &DrawComponent::getDrawOrder, &DrawComponent::setDrawOrder)
			.endClass()
			.deriveClass<Renderer<TileMapLayer>, DrawComponent>("LayerRenderer")
				.addProperty("layer", &Renderer<TileMapLayer>::getDrawable, &Renderer<TileMapLayer>::setDrawable)
			.endClass()
			.deriveClass<Renderer<sf::Sprite>, DrawComponent>("SpriteRenderer")
				.addProperty("sprite", &Renderer<sf::Sprite>::getDrawable, &Renderer<sf::Sprite>::setDrawable)
			.endClass();

		doLuaFile(*L, initFilename);

		std::string packageName{};
		std::smatch match;
		if (std::regex_match(initFilename, match, packageExpr))
		{
			packageName = match[1];
		}
		else
		{
			throw std::runtime_error{"Could not match Lua script name."};
		}

		luabridge::LuaRef pkg = luabridge::getGlobal(L, packageName.c_str());
		if (!pkg.isTable()) throw std::runtime_error{"Package `" + packageName + "' not found. Package name must match filename."};

		mKeyInputFn = pkg["processKeyInput"];
		if (!mKeyInputFn.isNil() && !mKeyInputFn.isFunction()) throw std::runtime_error{"processKeyInput must be a function."};

		mMouseButtonInputFn = pkg["processMouseButtonInput"];
		if (!mMouseButtonInputFn.isNil() && !mMouseButtonInputFn.isFunction()) throw std::runtime_error{"processMouseButtonInput must be a function."};

		mAxisInputFn = pkg["processAxisInput"];
		if (!mAxisInputFn.isNil() && !mAxisInputFn.isFunction()) throw std::runtime_error{"processAxisInput must be a function."};

		if (pkg["update"].isFunction()) mUpdateFn = pkg["update"];
		else if (!pkg["update"].isNil()) throw std::runtime_error{"update must be a function."};

		luabridge::LuaRef init = pkg["init"];
		if (!init.isFunction()) throw std::runtime_error{"Must supply init function."};
		try
		{
			init(this);
		}
		catch (luabridge::LuaException& ex)
		{
			storeLuaState(std::move(mpL));
			throw ex;
		}
		catch (std::exception& ex)
		{
			storeLuaState(std::move(mpL));
			throw ex;
		}
	}

	ScriptedGame::~ScriptedGame()
	{
		storeLuaState(std::move(mpL));
	}

	luabridge::LuaRef ScriptedGame::makeMapLayers(ResourceID<TMX> id)
	{
		luabridge::LuaRef table = luabridge::newTable(mpL.get());

		TMX& tmx = getTMXManager().get(id);
		std::vector<std::string> tilesetFilenames{};
		getTilesetFilenames(tmx, std::back_inserter(tilesetFilenames));
		std::vector<const sf::Texture*> textures{};
		//ResourceManager<sf::Texture>& textureManager = getTextureManager();
		std::transform(tilesetFilenames.begin(), tilesetFilenames.end(), std::back_inserter(textures), [this](const std::string& filename) {
			return get<sf::Texture>(load<sf::Texture>(filename));
		});
		std::vector<TileMapLayer> layers{};
		TileMapLayer::make(tmx, textures.begin(), textures.end(), std::back_inserter(layers));
		for (auto& layer : layers) table[layer.getName()] = store(std::make_unique<TileMapLayer>(layer));

		return table;
	}

	luabridge::LuaRef ScriptedGame::getAtlasSprites(ResourceID<TextureAtlas> id) const
	{
		luabridge::LuaRef table{luabridge::newTable(mpL.get())};
		const TextureAtlas& atlas = getAtlasManager().get(id);

		table["imagePath"] = atlas.getImagePath();

		std::vector<TextureAtlas::Sprite> atlasSprites{};
		atlas.insertSprites(std::back_inserter(atlasSprites));
		int index = 1;
		for (auto& sprite : atlasSprites)
		{
			luabridge::LuaRef data{luabridge::newTable(mpL.get())};
			data["pX"] = sprite.pX;
			data["pY"] = sprite.pY;
			data["w"] = sprite.w;
			data["h"] = sprite.h;
			data["x"] = sprite.x;
			data["y"] = sprite.y;
			data["n"] = sprite.n;
			data["r"] = sprite.r;
			table[index++] = data;
		}

		return table;
	}

	ResourceID<sf::Sprite> ScriptedGame::makeSprite(ResourceID<sf::Texture> textureID, luabridge::LuaRef rect)
	{
		auto pSprite = std::make_unique<sf::Sprite>(*get(textureID), sf::IntRect{rect["x"], rect["y"], rect["w"], rect["h"]});
		pSprite->setOrigin(rect["w"].cast<float>() * rect["pX"].cast<float>(), rect["h"].cast<float>() * rect["pY"].cast<float>());
		return store(std::move(pSprite));
	}

	ResourceID<Animation> ScriptedGame::makeAnimation(luabridge::LuaRef spriteArr, int millisecondsPerFrame)
	{
		std::vector<Animation::Frame> frames{};
		int index = 1;
		luabridge::LuaRef frame = spriteArr[index];
		while (!frame.isNil())
		{
			frames.push_back({frame.cast<ResourceID<sf::Sprite>>()});
			frame = spriteArr[++index];
		}
		return store(std::make_unique<Animation>(frames.begin(), frames.end(), sf::milliseconds(millisecondsPerFrame)));
	}

	EntityID ScriptedGame::makeEntity(luabridge::LuaRef entityTable, luabridge::LuaRef argsTable)
	{
		auto upEntity = ScriptedEntity::make(*this, entityTable, argsTable, {0, 0});
		ScriptedEntity* pEntity = upEntity.get();
		addEntity(std::move(upEntity));
		return pEntity->getID();
	}

	ScriptedEntity* ScriptedGame::getScriptedEntity(EntityID id) const
	{
		EntityManager& em = getEntityManager();
		if (em.hasEntity(id))
		{
			auto& entity = getEntityManager().getEntityFromID(id);
			assert(dynamic_cast<ScriptedEntity*>(&entity) != nullptr);
			return static_cast<ScriptedEntity*>(&entity);
		}
		return nullptr;
	}

	TileMap* ScriptedGame::getMap(EntityID id) const
	{
		EntityManager& em = getEntityManager();
		if (em.hasEntity(id))
		{
			auto& entity = getEntityManager().getEntityFromID(id);
			assert(dynamic_cast<TileMap*>(&entity) != nullptr);
			return static_cast<TileMap*>(&entity);
		}
		return nullptr;
	}

	luabridge::LuaRef ScriptedGame::getObjects(ResourceID<TMX> tmxID, const std::string& groupName) const
	{
		luabridge::LuaRef table = luabridge::newTable(mpL.get());

		const TMX& tmx = getTMXManager().get(tmxID);
		std::vector<TMX::Object> objects;
		getObjectsInGroup(tmx, groupName, std::back_inserter(objects));

		int index = 1;
		std::for_each(objects.begin(), objects.end(), [this, &table, &index](TMX::Object& area) {
			luabridge::LuaRef obj = luabridge::newTable(mpL.get());
			obj["id"] = area.id;
			if (area.name != "") obj["name"] = area.name;
			if (area.type != "") obj["type"] = area.type;
			obj["x"] = area.x;
			obj["y"] = area.y;
			obj["w"] = area.width;
			obj["h"] = area.height;
			table[index++] = obj;
		});

		return table;
	}

	luabridge::LuaRef ScriptedGame::getLayerNames(ResourceID<TMX> tmxID) const
	{
		luabridge::LuaRef table = luabridge::newTable(mpL.get());

		const TMX& tmx = getTMXManager().get(tmxID);
		std::vector<std::string> layerNames;
		tmx.getLayerNames(std::back_inserter(layerNames));
		size_t index = 1;
		std::for_each(layerNames.begin(), layerNames.end(), [&index, &table](const std::string& name) {
			table[index++] = name;
		});

		return table;
	}

	float ScriptedGame::getAnimationDuration(const std::string& animationStr) const
	{
		//return getTextureManager().getAnimation(TextureManager::getID(animationStr)).getDuration().asSeconds();
		return 1.f;
	}

	bool ScriptedGame::rayCast(sf::Vector2f origin, sf::Vector2f direction, RayCastHit* pHitInfo, float maxDistance)
	{
		return te::rayCast(this, origin, direction, pHitInfo, maxDistance);
	}

	luabridge::LuaRef ScriptedGame::getEntitiesInRegion(const AABB* pAABB) const
	{
		class QueryCallback : public b2QueryCallback
		{
		public:
			bool ReportFixture(b2Fixture* fixture)
			{
				entities.push_back(static_cast<BaseGameEntity*>(fixture->GetBody()->GetUserData()));
				return true;
			}

			std::vector<BaseGameEntity*> entities;
		};

		assert(pAABB);

		QueryCallback callback;
		getPhysicsWorld().QueryAABB(&callback, pAABB->getAABB());

		std::sort(callback.entities.begin(), callback.entities.end());
		callback.entities.erase(std::unique(callback.entities.begin(), callback.entities.end()), callback.entities.end());

		luabridge::LuaRef table = luabridge::newTable(mpL.get());
		int index = 1;
		std::for_each(callback.entities.begin(), callback.entities.end(), [&table, &index](BaseGameEntity* pEntity) {
			table[index++] = pEntity;
		});

		return table;
	}

	CameraEntity& ScriptedGame::getCamera() const
	{
		return *mpCamera;
	}

	void ScriptedGame::dispatchMessage(double delay, EntityID sender, EntityID receiver, luabridge::LuaRef info)
	{
		getMessageDispatcher().dispatchMessage(delay, sender, receiver, 0, std::make_unique<ScriptedInfo>(info));
	}

	std::unique_ptr<ScriptedGame> ScriptedGame::make(Application& app, const std::string& initFilename)
	{
		return std::unique_ptr<ScriptedGame>{new ScriptedGame{app, initFilename}};
	}

	void ScriptedGame::processInput(const sf::Event& evt)
	{
		switch (evt.type)
		{
		case sf::Event::KeyPressed:
		case sf::Event::KeyReleased:
			if (mKeyInputFn.isFunction())
				mKeyInputFn(this, (int)evt.key.code, (int)evt.type);
			break;
		case sf::Event::JoystickMoved:
			if (sf::Joystick::isConnected(0) && sf::Joystick::hasAxis(0, sf::Joystick::X) && mAxisInputFn.isFunction())
			{
				mAxisInputFn(this, 0, (int)sf::Joystick::X, sf::Joystick::getAxisPosition(0, sf::Joystick::X) / 100.f);
				mAxisInputFn(this, 0, (int)sf::Joystick::Y, sf::Joystick::getAxisPosition(0, sf::Joystick::Y) / 100.f);
			}
			break;
		case sf::Event::MouseButtonPressed:
			if (mMouseButtonInputFn.isFunction())
				mMouseButtonInputFn(this, (int)evt.mouseButton.button, (int)evt.type);
		}
	}

	void ScriptedGame::update(const sf::Time& dt)
	{
		Game::update(dt);
		if (mUpdateFn.isFunction())
			mUpdateFn(this, dt.asSeconds());
	}

	void ScriptedGame::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		target.clear();
		sf::View cameraView = mpCamera->getView();
		sf::View originalView = target.getView();
		target.setView(cameraView);
		Game::draw(target, states);
		target.setView(originalView);
	}
}
