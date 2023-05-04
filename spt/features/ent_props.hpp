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
	* Always asserts that the field exists. Optionally, calls Error() if it does not.
	* additionalOffset can be used when the exact field you're looking for does not exist;
	* you can instead reference a nearby field and add an offset from that in bytes.
	*/
	template<typename T, _tstring map, _tstring field, bool server, bool error, int additionalOffset = 0>
	struct CachedField
	{
		int _off = INVALID_DATAMAP_OFFSET;

		int Get()
		{
			if (_off != INVALID_DATAMAP_OFFSET)
				return _off + additionalOffset;
			_off = spt_entprops.GetFieldOffset(map.val, field.val, server);
			if (_off == INVALID_DATAMAP_OFFSET)
			{
				char errStr[256];
				snprintf(errStr,
				         sizeof(errStr),
				         "spt: %s::%s (%s) does not exist",
				         map.val,
				         field.val,
				         (server) ? "server" : "client");
				AssertMsg1(0, "%s", errStr);
				if (error)
					Error("%s", errStr);
				return INVALID_DATAMAP_OFFSET;
			}
			else
			{
				return _off + additionalOffset;
			}
		}

		bool Exists()
		{
			return Get() != INVALID_DATAMAP_OFFSET;
		}

		T* GetPtr(const void* ent)
		{
			Assert(ent);
			if (!ent)
				return nullptr;
			if (!Exists())
				return nullptr;
			return reinterpret_cast<T*>(reinterpret_cast<uint32_t>(ent) + _off + additionalOffset);
		}

		T* GetPtrPlayer()
		{
			return GetPtr(spt_entprops.GetPlayer(server));
		}
	};
} // namespace utils
