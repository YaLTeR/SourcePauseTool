#pragma once

#include <vector>
#include <algorithm>

#include "iserverentity.h"
#include "icliententity.h"
#include "basehandle.h"

namespace utils
{
	struct PortalInfo
	{
		void* pEnt; // client/server entity (depends on which ent list you used)
		CBaseHandle handle, linkedHandle;
		Vector pos, linkedPos;
		QAngle ang, linkedAng;
		bool isOrange, isActivated, isOpen;
		unsigned char linkageId = 0; // only exists on server

		inline const PortalInfo* GetLinkedPortal() const;

		void Invalidate()
		{
			handle = linkedHandle = CBaseHandle{};
			pos = linkedPos = Vector{NAN, NAN, NAN};
			ang = linkedAng = QAngle{NAN, NAN, NAN};
		}

		bool operator==(const PortalInfo& other) const
		{
			return !memcmp(this, &other, sizeof other);
		}

		bool operator!=(const PortalInfo& other) const
		{
			return !!memcmp(this, &other, sizeof other);
		}
	};

	/*
	* A server/client entity list abstraction. Stores an internal list for all entities which is
	* rebuilt every tick/frame for server/client. Also does the same for portals. All portal info
	* is returned by POINTERS which should NOT be stored inside a feature. All pointers are invalid
	* on the next tick/frame. If required, portal info can be copied and stored by value, but this
	* should not be necessary for most features.
	*/
	template<bool SERVER>
	class SptEntList
	{
		using ent_type = std::conditional_t<SERVER, IServerEntity*, IClientEntity*>;

	private:
		std::vector<ent_type> _entList;
		std::vector<PortalInfo> _portalList;
		int lastUpdatedAt = -666;

		void FillPortalInfo(ent_type ent, PortalInfo& info);

		SptEntList() = default;
		SptEntList(SptEntList&) = delete;
		SptEntList(SptEntList&&) = delete;

	public:
		static SptEntList& GetSingleton()
		{
			static SptEntList entList;
			return entList;
		}

		void CheckRebuildLists(bool force = false);

		const auto& GetEntList()
		{
			CheckRebuildLists(false);
			return _entList;
		}

		const auto& GetPortalList()
		{
			CheckRebuildLists(false);
			return _portalList;
		}

		static ent_type GetPlayer()
		{
			return GetEnt(1);
		};

		static bool EntIsPortal(ent_type ent)
		{
			const char* name = NetworkClassName(ent);
			return name && !strcmp(name, "CProp_Portal");
		}

		const PortalInfo* GetPortalAtIndex(int index)
		{
			auto& portalList = GetPortalList();
			auto it = std::lower_bound(portalList.cbegin(),
			                           portalList.cend(),
			                           index,
			                           [](const PortalInfo& portal, int x)
			                           { return portal.handle.GetEntryIndex() < x; });
			return it != portalList.cend() && it->handle.GetEntryIndex() == index ? &*it : nullptr;
		}

		static bool Valid()
		{
			return !!GetEnt(0);
		}

		const PortalInfo* GetEnvironmentPortal();
		static ent_type GetEnt(int index);
		static const char* NetworkClassName(ent_type ent);
	};

	using SptEntListServer = SptEntList<true>;
	using SptEntListClient = SptEntList<false>;

	// (references to) the singleton objects
	inline SptEntListServer& spt_serverEntList = SptEntListServer::GetSingleton();
	inline SptEntListClient& spt_clientEntList = SptEntListClient::GetSingleton();

	/*
	* Portal wrappers that can be used directly from the 'utils' namespace. The portal info struct
	* is server/client agnostic by design with some slight differences:
	* - the server position is more accurate
	* - entity handle serial numbers are different (TODO check)
	* 
	* This logic prefers getting portal info from the server when available, otherwise gets it from
	* the client.
	*/

	// avoid copying by value :p
	static auto& GetPortalList()
	{
		return spt_serverEntList.Valid() ? spt_serverEntList.GetPortalList()
		                                 : spt_clientEntList.GetPortalList();
	}

	static auto GetEnvironmentPortal()
	{
		return spt_serverEntList.Valid() ? spt_serverEntList.GetEnvironmentPortal()
		                                 : spt_clientEntList.GetEnvironmentPortal();
	}

	static const PortalInfo* GetPortalAtIndex(int index)
	{
		return spt_serverEntList.Valid() ? spt_serverEntList.GetPortalAtIndex(index)
		                                 : spt_clientEntList.GetPortalAtIndex(index);
	}

	inline const PortalInfo* PortalInfo::GetLinkedPortal() const
	{
		return handle.IsValid() && linkedHandle.IsValid() ? GetPortalAtIndex(linkedHandle.GetEntryIndex())
		                                                  : nullptr;
	}

} // namespace utils
