#pragma once

#include "..\feature.hpp"
#include "datamap_wrapper.hpp"

enum class PropMode
{
	Server,
	Client,
	PreferServer,
	PreferClient,
	None
};

struct _InternalPlayerField
{
	int serverOffset = utils::INVALID_DATAMAP_OFFSET;
	int clientOffset = utils::INVALID_DATAMAP_OFFSET;

	void** GetServerPtr() const;
	void** GetClientPtr() const;

	bool ClientOffsetFound() const;
	bool ServerOffsetFound() const;
};

template<typename T>
struct PlayerField
{
	_InternalPlayerField field;
	PropMode mode = PropMode::PreferServer;

	T GetValue() const;
	T* GetPtr() const;
	bool ClientOffsetFound()
	{
		return field.clientOffset != utils::INVALID_DATAMAP_OFFSET;
	}
	bool ServerOffsetFound()
	{
		return field.serverOffset != utils::INVALID_DATAMAP_OFFSET;
	}
	bool Found()
	{
		return ClientOffsetFound() || ServerOffsetFound();
	}
};

// Initializes ent utils stuff
class EntProps : public FeatureWrapper<EntProps>
{
public:
	virtual bool ShouldLoadFeature()
	{
		return true;
	}

	virtual void LoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void UnloadFeature() override;

	int GetFieldOffset(const std::string& mapKey, const std::string& key, bool server);
	int GetPlayerOffset(const std::string& key, bool server);
	template<typename T>
	PlayerField<T> GetPlayerField(const std::string& key, PropMode mode = PropMode::PreferServer);
	PropMode ResolveMode(PropMode mode);
	void PrintDatamaps();
	void WalkDatamap(std::string key);
	void* GetPlayer(bool server);

protected:
	bool tablesProcessed = false;
	bool playerDatamapSearched = false;
	utils::DatamapWrapper* __playerdatamap = nullptr;

	void AddMap(datamap_t* map, bool server);
	_InternalPlayerField _GetPlayerField(const std::string& key, PropMode mode);
	utils::DatamapWrapper* GetDatamapWrapper(const std::string& key);
	utils::DatamapWrapper* GetPlayerDatamapWrapper();
	void ProcessTablesLazy();
	std::vector<patterns::MatchedPattern> serverPatterns;
	std::vector<patterns::MatchedPattern> clientPatterns;
	std::vector<utils::DatamapWrapper*> wrappers;
	std::unordered_map<std::string, utils::DatamapWrapper*> nameToMapWrapper;
};

extern EntProps spt_entprops;

template<typename T>
inline PlayerField<T> EntProps::GetPlayerField(const std::string& key, PropMode mode)
{
	PlayerField<T> field;
	field.field = _GetPlayerField(key, mode);
	field.mode = mode;
	return field;
}

template<typename T>
inline T PlayerField<T>::GetValue() const
{
	T* ptr = GetPtr();

	if (ptr)
		return *ptr;
	else
		return T();
}

template<typename T>
inline T* PlayerField<T>::GetPtr() const
{
	auto resolved = spt_entprops.ResolveMode(mode);
	if (resolved == PropMode::Server)
		return reinterpret_cast<T*>(field.GetServerPtr());
	else if (resolved == PropMode::Client)
		return reinterpret_cast<T*>(field.GetClientPtr());
	else
		return nullptr;
}
