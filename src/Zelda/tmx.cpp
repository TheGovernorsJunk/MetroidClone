#include "tmx.h"

#include "texture_manager.h"
#include "tile_map.h"
#include "composite_collider.h"
#include "nav_graph_node.h"
#include "nav_graph_edge.h"
#include "vector_ops.h"
#include "utilities.h"

#include <SFML/Graphics.hpp>
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>

#include <algorithm>
#include <array>
#include <set>
#include <cmath>
#include <sstream>

constexpr bool std::less<sf::Vector2f>::operator()(const sf::Vector2f& a, const sf::Vector2f& b) const
{
	return a.x < b.x || (a.x == b.x && a.y < b.y);
}

constexpr bool std::less<te::NavGraphEdge>::operator()(const te::NavGraphEdge& a, const te::NavGraphEdge& b) const
{
	return a.getFrom() < b.getFrom() ||
		(a.getFrom() == b.getFrom() && a.getTo() < b.getTo()) ||
		(a.getFrom() == b.getFrom() && a.getTo() == b.getTo() && a.getCost() < b.getCost());
}

namespace te
{
	const int TMX::NULL_TILE = -1;
	const TMX::TileData TMX::NULL_DATA = TMX::TileData{ NULL_TILE, TMX::ObjectGroup() };

	TMX::TMX()
		: mFilename{""}, mOrientation{Orientation::Orthogonal}, mWidth{0}, mHeight{0}, mTilewidth{0}, mTileheight{0}, mTilesets{}, mLayers{}, mObjectGroups{}, mLayerNames{}
	{}

	bool TMX::loadFromFile(const std::string& filename)
	{
		rapidxml::file<> tmxFile(filename.c_str());
		rapidxml::xml_document<> tmx;
		tmx.parse<0>(tmxFile.data());

		mFilename = filename;

		rapidxml::xml_node<char>* pMapNode = tmx.first_node("map");

		std::string orientationStr = pMapNode->first_attribute("orientation")->value();
		if (orientationStr == "orthogonal") mOrientation = Orientation::Orthogonal;
		else if (orientationStr == "isometric") mOrientation = Orientation::Isometric;
		else throw std::runtime_error{"Unsupported TMX orientation."};

		mWidth = std::stoi(pMapNode->first_attribute("width")->value());
		mHeight = std::stoi(pMapNode->first_attribute("height")->value());
		mTilewidth = std::stoi(pMapNode->first_attribute("tilewidth")->value());
		mTileheight = std::stoi(pMapNode->first_attribute("tileheight")->value());

		for (rapidxml::xml_node<char>* pTileset = tmx.first_node("map")->first_node("tileset"); pTileset != 0; pTileset = pTileset->next_sibling("tileset"))
		{
			std::vector<TileData> tiles;
			for (rapidxml::xml_node<char>* pTile = pTileset->first_node("tile"); pTile != 0; pTile = pTile->next_sibling("tile"))
			{
				rapidxml::xml_node<char>* pObjectGroup = pTile->first_node("objectgroup");
				if (pObjectGroup != 0)
				{
					std::vector<Object> objects;
					for (rapidxml::xml_node<char>* pObject = pObjectGroup->first_node("object"); pObject != 0; pObject = pObject->next_sibling("object"))
					{
						std::vector<Polygon> polygons;
						for (rapidxml::xml_node<char>* pPolygon = pObject->first_node("polygon"); pPolygon != 0; pPolygon = pPolygon->next_sibling("polygon"))
						{
							std::vector<sf::Vector2i> pointsVec;

							std::string pointsStr(pPolygon->first_attribute("points")->value());
							std::istringstream iss(pointsStr);
							std::vector<std::string> points{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };
							for (auto& point : points)
							{
								std::stringstream ss(point);
								std::string xCoord;
								std::getline(ss, xCoord, ',');
								std::string yCoord;
								std::getline(ss, yCoord);
								pointsVec.push_back({ std::stoi(xCoord), std::stoi(yCoord) });
							}

							polygons.push_back({ std::move(pointsVec) });
						}

						objects.push_back(Object{
							std::stoi(pObject->first_attribute("id")->value()),
							"",
							"",
							std::stoi(pObject->first_attribute("x")->value()),
							std::stoi(pObject->first_attribute("y")->value()),
							pObject->first_attribute("width") != 0 ? std::stoi(pObject->first_attribute("width")->value()) : 0,
							pObject->first_attribute("height") != 0 ? std::stoi(pObject->first_attribute("height")->value()) : 0,
							std::move(polygons)
						});
					}

					tiles.push_back({
						std::stoi(pTile->first_attribute("id")->value()),
						ObjectGroup {
							"",
							pObjectGroup->first_attribute("draworder")->value(),
							std::move(objects),
							0
						}
					});
				}
				else
				{
					tiles.push_back({
						std::stoi(pTile->first_attribute("id")->value()),
						ObjectGroup {}
					});
				}
			}

			rapidxml::xml_node<char>* pImage = pTileset->first_node("image");

			mTilesets.push_back({
				std::stoi(pTileset->first_attribute("firstgid")->value()),
				pTileset->first_attribute("name")->value(),
				std::stoi(pTileset->first_attribute("tilewidth")->value()),
				std::stoi(pTileset->first_attribute("tileheight")->value()),
				std::stoi(pTileset->first_attribute("tilecount")->value()), {
					pImage->first_attribute("source")->value(),
					std::stoi(pImage->first_attribute("width")->value()),
				    std::stoi(pImage->first_attribute("height")->value())
				},
				std::move(tiles)
			});
		}

		rapidxml::xml_node<char>* pLastTileset = nullptr;
		for (auto* pTileset = tmx.first_node("map")->first_node("tileset"); pTileset != 0; pTileset = pTileset->next_sibling("tileset"))
			pLastTileset = pTileset;
		Index currIndex = 0;
		std::vector<Index> tileLayerIndices;
		std::vector<Index> objectLayerIndices;
		for (auto* pLayer = pLastTileset->next_sibling(); pLayer != 0; pLayer = pLayer->next_sibling())
		{
			if (std::string(pLayer->name()) == "layer")
				tileLayerIndices.push_back(currIndex);
			if (std::string(pLayer->name()) == "objectgroup")
				objectLayerIndices.push_back(currIndex);
			++currIndex;
		}

		auto layerIndexIter = tileLayerIndices.begin();
		for (rapidxml::xml_node<char>* pLayer = tmx.first_node("map")->first_node("layer"); pLayer != 0; pLayer = pLayer->next_sibling("layer"))
		{
			std::vector<Tile> tiles;
			for (rapidxml::xml_node<char>* pTile = pLayer->first_node("data")->first_node("tile"); pTile != 0; pTile = pTile->next_sibling("tile"))
			{
				tiles.push_back({
					std::stoi(pTile->first_attribute("gid")->value())
				});
			}
			mLayers.push_back({
				pLayer->first_attribute("name")->value(),
				std::stoi(pLayer->first_attribute("width")->value()),
				std::stoi(pLayer->first_attribute("height")->value()),
				{std::move(tiles)},
				*layerIndexIter++
			});
		}

		auto objectIndexIter = objectLayerIndices.begin();
		for (rapidxml::xml_node<char>* pObjectgroup = tmx.first_node("map")->first_node("objectgroup"); pObjectgroup != 0; pObjectgroup = pObjectgroup->next_sibling("objectgroup"))
		{
			std::vector<Object> objects;
			for (rapidxml::xml_node<char>* pObject = pObjectgroup->first_node("object"); pObject != 0; pObject = pObject->next_sibling("object"))
			{
				objects.push_back({
					std::stoi(pObject->first_attribute("id")->value()),
					pObject->first_attribute("name") != 0 ? pObject->first_attribute("name")->value() : "",
					pObject->first_attribute("type") != 0 ? pObject->first_attribute("type")->value() : "",
					std::stoi(pObject->first_attribute("x")->value()),
					std::stoi(pObject->first_attribute("y")->value()),
					pObject->first_attribute("width") != 0 ? std::stoi(pObject->first_attribute("width")->value()) : 0,
					pObject->first_attribute("height") != 0 ? std::stoi(pObject->first_attribute("height")->value()) : 0
				});
			}
			mObjectGroups.push_back({
				pObjectgroup->first_attribute("name")->value(),
				"",
				std::move(objects),
				*objectIndexIter++
			});
		}

		// This is some accidental duplication
		rapidxml::xml_node<char>* pLayerOrObjectNode = tmx.first_node("map")->first_node("tileset")->next_sibling();
		while (pLayerOrObjectNode != 0)
		{
			std::string nodeName{pLayerOrObjectNode->name()};
			if (nodeName == "layer" || nodeName == "objectgroup")
			{
				mLayerNames.push_back(pLayerOrObjectNode->first_attribute("name")->value());
			}
			pLayerOrObjectNode = pLayerOrObjectNode->next_sibling();
		}

		return true;
	}

	size_t TMX::getTilesetIndex(int gid) const
	{
		auto retIt = mTilesets.end();
		for (auto it = mTilesets.begin(); it != mTilesets.end(); ++it)
		{
			if (gid >= it->firstgid && gid < it->firstgid + it->tilecount)
			{
				retIt = it;
			}
		}
		if (retIt == mTilesets.end())
		{
			throw std::out_of_range("GID does not exist for TMX.");
		}
		return retIt - mTilesets.begin();
	}

	//void TMX::makeVertices(TextureManager& textureManager, std::vector<const sf::Texture*>& textures, std::vector<std::vector<sf::VertexArray>>& layers, std::vector<int>& drawOrders) const
	//{
	//	sf::Transform transform = getTileToPixelTransform();

	//	std::function<std::vector<sf::VertexArray>(const Layer& layer)> makeLayers;
	//	switch (mOrientation)
	//	{
	//	case Orientation::Orthogonal:
	//		makeLayers = [this, transform](const Layer& layer) {
	//			std::vector<sf::VertexArray> vertexArrays(mTilesets.size());
	//			for (auto& va : vertexArrays) va.setPrimitiveType(sf::Quads);

	//			int tileIndex = 0;
	//			for (auto tile : layer.data)
	//			{
	//				if (tile.gid != 0)
	//				{
	//					int x = tileIndex % mWidth;
	//					int y = tileIndex / mWidth;

	//					std::array<sf::Vertex, 4> quad;
	//					quad[0].position = sf::Vector2f((float)x, (float)y);
	//					quad[1].position = sf::Vector2f(x + 1.f, (float)y);
	//					quad[2].position = sf::Vector2f(x + 1.f, y + 1.f);
	//					quad[3].position = sf::Vector2f((float)x, y + 1.f);

	//					std::for_each(quad.begin(), quad.end(), [&transform](sf::Vertex& v) {
	//						v.position = transform.transformPoint(v.position);
	//					});

	//					size_t tilesetIndex = getTilesetIndex(tile.gid);
	//					const Tileset& tileset = mTilesets[tilesetIndex];
	//					int localId = tile.gid - tileset.firstgid;
	//					int tu = localId % (tileset.image.width / tileset.tilewidth);
	//					int tv = localId / (tileset.image.width / tileset.tilewidth);

	//					quad[0].texCoords = sf::Vector2f((float)tu * mTilewidth, (float)tv * mTileheight);
	//					quad[1].texCoords = sf::Vector2f((tu + 1.f) * mTilewidth, (float)tv * mTileheight);
	//					quad[2].texCoords = sf::Vector2f((tu + 1.f) * mTilewidth, (tv + 1.f) * mTileheight);
	//					quad[3].texCoords = sf::Vector2f((float)tu * mTilewidth, (tv + 1.f) * mTileheight);

	//					std::for_each(quad.begin(), quad.end(), [&vertexArrays, tilesetIndex](sf::Vertex& v) {
	//						vertexArrays[tilesetIndex].append(v);
	//					});
	//				}
	//				++tileIndex;
	//			}

	//			return vertexArrays;
	//		};
	//		break;
	//	case Orientation::Isometric:
	//		makeLayers = [this, transform](const Layer& layer) {
	//			std::vector<sf::VertexArray> vertexArrays(mTilesets.size());
	//			for (auto& va : vertexArrays) va.setPrimitiveType(sf::Quads);

	//			int tileIndex = 0;
	//			for (auto tile : layer.data)
	//			{
	//				if (tile.gid != 0)
	//				{
	//					int x = tileIndex % mWidth;
	//					int y = tileIndex / mWidth;

	//					std::array<sf::Vertex, 4> quad;
	//					quad[0].position = sf::Vector2f((float)x, (float)y);
	//					quad[1].position = sf::Vector2f(x + 1.f, (float)y);
	//					quad[2].position = sf::Vector2f(x + 1.f, y + 1.f);
	//					quad[3].position = sf::Vector2f((float)x, y + 1.f);

	//					std::for_each(quad.begin(), quad.end(), [&transform](sf::Vertex& v) {
	//						v.position = transform.transformPoint(v.position);
	//					});

	//					size_t tilesetIndex = getTilesetIndex(tile.gid);
	//					const Tileset& tileset = mTilesets[tilesetIndex];
	//					int localId = tile.gid - tileset.firstgid;
	//					int tu = localId % (tileset.image.width / tileset.tilewidth);
	//					int tv = localId / (tileset.image.width / tileset.tilewidth);

	//					quad[0].texCoords = sf::Vector2f((tu + 0.5f) * mTilewidth, (float)tv * mTileheight);
	//					quad[1].texCoords = sf::Vector2f((tu + 1.f) * mTilewidth, (tv + 0.5f) * mTileheight);
	//					quad[2].texCoords = sf::Vector2f((tu + 0.5f) * mTilewidth, (tv + 1.f) * mTileheight);
	//					quad[3].texCoords = sf::Vector2f((float)tu * mTilewidth, (tv + 0.5f) * mTileheight);

	//					std::for_each(quad.begin(), quad.end(), [&vertexArrays, tilesetIndex](sf::Vertex& v) {
	//						vertexArrays[tilesetIndex].append(v);
	//					});
	//				}
	//				++tileIndex;
	//			}

	//			return vertexArrays;
	//		};
	//		break;
	//	default:
	//		throw std::runtime_error{"Unsupported orientation for vertices."};
	//		break;
	//	}

	//	std::string dir = getDir(mFilename);
	//	textures.clear();
	//	std::transform(mTilesets.begin(), mTilesets.end(), std::back_inserter(textures), [&textureManager, &dir](const Tileset& tileset) {
	//		return &textureManager.getTexture(textureManager.load(dir + tileset.image.source));
	//	});

	//	layers.clear();
	//	std::transform(mLayers.begin(), mLayers.end(), std::back_inserter(layers), makeLayers);

	//	drawOrders.clear();
	//	std::transform(mLayers.begin(), mLayers.end(), std::back_inserter(drawOrders), [](auto& layer) { return static_cast<int>(layer.index); });
	//}

	const TMX::TileData& TMX::getTileData(int x, int y, const TMX::Layer& layer) const
	{
		int i = y * mWidth + x;
		if (i >= mWidth * mHeight)
		{
			throw std::out_of_range("Tile coordinates are out of bounds.");
		}
		const Tile& tile = layer.data.tiles[i];
		if (tile.gid == 0)
		{
			return TMX::NULL_DATA;
		}
		const Tileset& tileset = mTilesets[getTilesetIndex(tile.gid)];
		const int localId = tile.gid - tileset.firstgid;
		const std::vector<TileData>& tiles = tileset.tiles;
		auto result = std::find_if(tiles.begin(), tiles.end(), [localId](const TileData& data) {
			return localId == data.id;
		});
		if (result == tiles.end())
		{
			return TMX::NULL_DATA;
		}
		return *result;
	}

	CompositeCollider* TMX::makeCollider(const sf::Transform& sourceTransform) const
	{
		CompositeCollider* pCollider = new CompositeCollider();
		std::for_each(mLayers.begin(), mLayers.end(), [pCollider, &sourceTransform, this](const Layer& layer) {
			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					const TMX::TileData& tileData = getTileData(x, y, layer);
					if (tileData.id != NULL_TILE)
					{
						sf::Transform transform = sourceTransform;
						transform.translate((float)x * mTilewidth, (float)y * mTileheight);
						std::for_each(tileData.objectgroup.objects.begin(), tileData.objectgroup.objects.end(), [pCollider, &transform](const Object& obj) {
							if (obj.polygons.size() == 0) {
								pCollider->addCollider({ transform.transformRect({ (float)obj.x, (float)obj.y, (float)obj.width, (float)obj.height }) });
							}
						});
					}
				}
			}
		});
		return pCollider;
	}

	int TMX::index(int x, int y) const
	{
		return y * mWidth + x;
	}

	SparseGraph<NavGraphNode, NavGraphEdge>* TMX::makeNavGraph(const sf::Transform& transform) const
	{
		std::unique_ptr<CompositeCollider> pCollider(makeCollider(transform));
		NavGraphNode seedNode;
		seedNode.setIndex(-1);
		int x = 0, y = 0;
		while (seedNode.getIndex() == -1 && y < mHeight)
		{
			sf::Vector2f coords = transform.transformPoint({ x * mTilewidth + (mTilewidth / 2.f), y * mTileheight + (mTileheight / 2.f) });
			if (!pCollider->contains(coords.x, coords.y))
			{
				seedNode.setIndex(0);
				seedNode.setPosition(coords);
			}
			else
			{
				++x;
				if (x == mWidth)
				{
					x = 0;
					++y;
				}
			}
		}

		SparseGraph<NavGraphNode, NavGraphEdge>* pGraph = new SparseGraph<NavGraphNode, NavGraphEdge>();
		if (seedNode.getIndex() != -1)
		{
			int seedIndex = pGraph->addNode(seedNode);

			std::vector<NavGraphNode> allNodes;
			std::map<sf::Vector2f, int> assigned;
			assigned.insert(std::make_pair(pGraph->getNode(seedIndex).getPosition(), seedIndex));
			auto flood = [&, this](int startIndex) {
				sf::Vector2f pos = pGraph->getNode(startIndex).getPosition();
				std::vector<int> newIndices;

				const std::array<sf::Vector2f, 4> offsets = {
					transform.transformPoint(sf::Vector2f((float)mTilewidth, 0)),
					transform.transformPoint(sf::Vector2f(-(float)mTilewidth, 0)),
					transform.transformPoint(sf::Vector2f(0, (float)mTileheight)),
					transform.transformPoint(sf::Vector2f(0, -(float)mTileheight))
				};
				std::for_each(offsets.begin(), offsets.end(), [&](const sf::Vector2f& offset) {
					sf::Vector2f newPos = offset + pos;
					sf::Vector2f max = transform.transformPoint((float)mWidth * mTilewidth, (float)mHeight * mTileheight);
					if (assigned.find(newPos) == assigned.end() && !pCollider->contains(newPos.x, newPos.y) && newPos.x > 0 && newPos.x < max.x && newPos.y > 0 && newPos.y < max.y)
					{
						NavGraphNode newNode;
						newNode.setPosition(newPos);
						int newIndex = pGraph->addNode(newNode);
						//graph.addEdge(NavGraphEdge(startIndex, newIndex, length(graph.getNode(startIndex).getPosition() - newPos)));

						assigned.insert(std::make_pair(newPos, newIndex));
						newIndices.push_back(newIndex);
					}
				});

				// Add diagonal edges
				std::for_each(newIndices.begin(), newIndices.end(), [&](int newIndex) {
					sf::Vector2f newPosition = pGraph->getNode(newIndex).getPosition();

					const std::array<sf::Vector2f, 8> neighbors = {
						transform.transformPoint(sf::Vector2f((float)mTilewidth, 0)),
						transform.transformPoint(sf::Vector2f(-(float)mTilewidth, 0)),
						transform.transformPoint(sf::Vector2f(0, (float)mTileheight)),
						transform.transformPoint(sf::Vector2f(0, -(float)mTileheight)),
						transform.transformPoint(sf::Vector2f((float)mTilewidth, (float)mTileheight)),
						transform.transformPoint(sf::Vector2f(-(float)mTilewidth, (float)mTileheight)),
						transform.transformPoint(sf::Vector2f(-(float)mTilewidth, -(float)mTileheight)),
						transform.transformPoint(sf::Vector2f((float)mTilewidth, -(float)mTileheight))
					};
					std::for_each(neighbors.begin(), neighbors.end(), [&](sf::Vector2f offset) {
						sf::Vector2f neighbor = newPosition + offset;
						auto neighborIndexIter = assigned.find(neighbor);
						if (neighborIndexIter != assigned.end())
						{
							pGraph->addEdge(NavGraphEdge(newIndex, neighborIndexIter->second, length(pGraph->getNode(newIndex).getPosition() - neighbor)));
						}
					});
				});
				return newIndices;
			};

			auto flatMapFlood = [&](std::vector<int> indices) {
				std::vector<int> newIndices;
				std::for_each(indices.begin(), indices.end(), [&](int index) {
					std::vector<int> currIndices = flood(index);
					newIndices.insert(newIndices.end(), currIndices.begin(), currIndices.end());
				});
				return newIndices;
			};

			std::vector<int> newIndices = flatMapFlood({ seedIndex });
			while (newIndices.size() > 0)
			{
				newIndices = flatMapFlood(newIndices);
			}
		}
		pGraph->pruneEdges();
		return pGraph;
	}

	TMX::Orientation TMX::getOrienation() const
	{
		return mOrientation;
	}

	int TMX::getWidth() const
	{
		return mWidth;
	}

	int TMX::getHeight() const
	{
		return mHeight;
	}

	int TMX::getTileWidth() const
	{
		return mTilewidth;
	}

	int TMX::getTileHeight() const
	{
		return mTileheight;
	}

	sf::Transform TMX::getTileToPixelTransform() const
	{
		switch (mOrientation)
		{
		case Orientation::Orthogonal:
			return sf::Transform{}.scale((float)mTilewidth, (float)mTileheight);
		case Orientation::Isometric:
		{
			float rotationAngle = std::atan((float)mTileheight / mTilewidth);
			float shearSlope = std::tan(2.f * rotationAngle);
			float finalScale = std::sqrt(0.25f * mTilewidth * mTilewidth + 0.25f * mTileheight * mTileheight);
			sf::Transform transform{};
			transform
				.rotate(rotationAngle * 180.f / PI)
				.scale(finalScale, finalScale);
			transform *= sf::Transform{ 1.f,-1.f / (float)shearSlope,0,0,1.f,0,0,0,1.f };
			transform.scale(1.f, std::sin(2 * rotationAngle));

			return transform;
		}
		default:
			throw std::runtime_error{"TMX::getTileToPixelTransform: Unsupported orienation."};
		}
	}

	std::vector<TMX::ObjectGroup> TMX::getObjectGroups() const
	{
		return mObjectGroups;
	}
}
