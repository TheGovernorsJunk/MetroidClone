namespace te
{
	template <typename Resource>
	ResourceManager<Resource>::ResourceManager()
		: m_NextID{1}
		, m_ResourceMap{}
	{
	}

	template <typename Resource>
	template <typename... Args>
	ResourceID<Resource> ResourceManager<Resource>::load(const std::string& filename, Args&&... args)
	{
		auto found = m_FilenameMap.find(filename);
		if (found != m_FilenameMap.end()) return {found->second};

		auto pResource = std::make_unique<Resource>(std::forward<Args>(args)...);
		if (!pResource->loadFromFile(filename))
			throw std::runtime_error{"ResourceManager::load: failed to load " + filename};

		auto id = insertResource(std::move(pResource));
		insertFilename(filename, id.value);
		return id;
	}

	template <typename Resource>
	ResourceID<Resource> ResourceManager<Resource>::store(std::unique_ptr<Resource>&& pResource)
	{
		return insertResource(std::move(pResource));
	}

	template <typename Resource>
	Resource& ResourceManager<Resource>::get(ResourceID<Resource> id)
	{
		auto found = m_ResourceMap.find(id.value);
		assert(found != m_ResourceMap.end());
		return *found->second;
	}

	template <typename Resource>
	const Resource& ResourceManager<Resource>::get(ResourceID<Resource> id) const
	{
		auto found = m_ResourceMap.find(id.value);
		assert(found != m_ResourceMap.end());
		return *found->second;
	}

	template <typename Resource>
	ResourceID<Resource> ResourceManager<Resource>::insertResource(std::unique_ptr<Resource>&& pResource)
	{
		auto inserted = m_ResourceMap.insert(std::make_pair(m_NextID, std::move(pResource)));
		assert(inserted.second);
		return { m_NextID++ };
	}

	template <typename Resource>
	void ResourceManager<Resource>::insertFilename(const std::string& filename, int idValue)
	{
		auto inserted = m_FilenameMap.insert(std::make_pair(filename, idValue));
		assert(inserted.second);
	}
}
