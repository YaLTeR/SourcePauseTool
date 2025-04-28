#pragma once

#include "..\feature.hpp"
#include "datamap_wrapper.hpp"
#include "spt\utils\ent_list.hpp"

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

	static void ImGuiEntInfoCvarCallback(ConVar& var);
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

namespace utils
{
	// horrible, but the only way I know of to get C-style strings into templates
	template<size_t N>
	struct _tstring
	{
		constexpr _tstring(const char (&str)[N])
		{
			std::copy_n(str, N, val);
		}
		char val[N];
	};

	/*
	* A small wrapper of spt_entprops.GetFieldOffset() that caches the offset.
	* AdditionalOffset can be used when the exact field you're looking for does not exist;
	* you can instead reference a nearby field and add an offset from that in bytes.
	*/
	template<typename T, _tstring map, _tstring field, bool server, int additionalOffset = 0>
	struct CachedField
	{
		int _off = INVALID_DATAMAP_OFFSET;

		int Get()
		{
			if (_off != INVALID_DATAMAP_OFFSET)
				return _off + additionalOffset;
			_off = spt_entprops.GetFieldOffset(map.val, field.val, server);
			return _off == INVALID_DATAMAP_OFFSET ? INVALID_DATAMAP_OFFSET : _off + additionalOffset;
		}

		bool Exists()
		{
			return Get() != INVALID_DATAMAP_OFFSET;
		}

		T* GetPtr(const void* ent)
		{
			if (!ent || !Exists()) [[unlikely]]
				return nullptr;
			return reinterpret_cast<T*>(reinterpret_cast<uint32_t>(ent) + _off + additionalOffset);
		}

		T& GetValueOrDefault(const void* ent, T def = T{})
		{
			T* ptr = GetPtr(ent);
			return ptr ? *ptr : def;
		}

		T* GetPtrPlayer()
		{
			if constexpr (server)
				return GetPtr(utils::spt_serverEntList.GetPlayer());
			return GetPtr(utils::spt_clientEntList.GetPlayer());
		}

		T& GetValueOrDefaultPlayer(T def = T{})
		{
			T* ptr = GetPtrPlayer();
			return ptr ? *ptr : def;
		}
	};

	/*
	* Usage:
	* static CachedField1<...> f1;
	* static CachedField1<...> f2;
	* static CachedField1<...> f3;
	* 
	* static CachedFields fs{f1, f2, f3};
	* if (!fs.HasAll()) return; // or whatever
	* auto [p1, p2, p3] = fs.GetAllPtrs(ent);
	*/
	template<typename... Fields>
	struct CachedFields
	{
		std::tuple<Fields&...> _fields;

		CachedFields(Fields&... fields) : _fields{fields...} {}

		bool HasAll()
		{
			return (std::get<Fields&>(_fields).Exists() && ...);
		}

		auto GetAllPtrs(const void* ent)
		{
			return std::make_tuple(std::get<Fields&>(_fields).GetPtr(ent)...);
		}

		auto GetAllPtrsPlayer()
		{
			return std::make_tuple(std::get<Fields&>(_fields).GetPtrPlayer()...);
		}
	};
} // namespace utils
