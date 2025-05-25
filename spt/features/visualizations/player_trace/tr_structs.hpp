#pragma once

#include <span>
#include <tuple>
#include <algorithm>
#include <compare>

#include "tr_config.hpp"
#include "spt/features/ent_props.hpp"
#include "spt/utils/interfaces.hpp"

#undef min
#undef max

#ifdef SPT_PLAYER_TRACE_ENABLED

/*
* Welcome to the funny over-engineered player trace feature. The trace is stored as a bunch of
* flattened vectors of the various structures below. Instead of pointers, objects reference other
* objects with templated indices (TrIdx). As a rule of thumb - unless otherwise noted, all TrIdx
* fields in the structs below are assumed to be valid.
*
* To make the trace recording (relatively) small in memory, objects are only stored when there is
* a change in state, for example:
* 
* - player data is only stored when it's different from the previous state
* - entities are delta'd to the previous state, and again only recorded when there's an actual delta
* - portals are only recorded if any portals changed
* 
* This means that if you want to get the state on any particular 'tick', you have to do a binary
* search into one of the vectors that has a 'tick' field. The 'tick' field is monotonically
* increasing and records the number of times data has been collected in TickSignal.
*/

namespace player_trace
{
	class TrRecordingCache;
	class TrRenderingCache;
	class TrEntityCache;
	struct TrPlayerTrace;

	// a scope within which we can deref TrIdx & TrSpan
	class TrReadContextScope
	{
		inline static const TrPlayerTrace* trCtx = nullptr;
		const TrPlayerTrace* oldCtx;

	public:
		TrReadContextScope(const TrPlayerTrace& newCtx) : oldCtx{trCtx}
		{
			trCtx = &newCtx;
		}

		~TrReadContextScope()
		{
			trCtx = oldCtx;
		}

		inline static const TrPlayerTrace& Current()
		{
			AssertMsg(trCtx, "Setup a TrReadContextScope");
			return *trCtx;
		}

	private:
		friend TrRecordingCache;

		// yes this is stupid, no I don't care
		inline static TrPlayerTrace& CurrentModifiable()
		{
			return const_cast<TrPlayerTrace&>(Current());
		}
	};

	/*
	* Every vector in the trace will be written to a "lump" when serialized. We define two values
	* to help with backwards compatibility during deserialization: the name & version. Backwards
	* compatiblity is handled in tr_binary_read_upgrade.cpp. All structs here should have an
	* associated TR_DEFINE_LUMP and have a _v1, _v2, _v3, etc. suffix. A type alias without the
	* version suffix is used to refer to the most up-to-date version of the struct. Older versions
	* may be used during deserialization. Structs that are not defined here but are used as vectors
	* in the trace storage must also have an associated TR_DEFINE_LUMP.
	* 
	* The lump version doesn't necessarily have to match the struct_vXXX version. New struct
	* versions indicate a change in the actual fields, but new lump versions can be used for
	* example to specify that a certain field is handled slightly differently.
	*/

	using tr_struct_version = uint32_t;
	constexpr tr_struct_version TR_INVALID_STRUCT_VERSION = 0;
	constexpr size_t TR_MAX_LUMP_NAME_LEN = 32;

	template<utils::_tstring NAME, tr_struct_version VERSION>
	struct _TrLumpNameShouldBeUniquePerVersion;

	template<typename T>
	struct TrLumpInfo;

#define TR_DEFINE_LUMP(type, lump_serialize_name, lump_version) \
	static_assert(lump_version != TR_INVALID_STRUCT_VERSION, \
	              "Cannot use TR_INVALID_STRUCT_VERSION for player trace structures"); \
	template<> \
	struct _TrLumpNameShouldBeUniquePerVersion<lump_serialize_name, lump_version> \
	{ \
	}; \
	template<> \
	struct TrLumpInfo<type> \
	{ \
		static constexpr const char name[TR_MAX_LUMP_NAME_LEN] = lump_serialize_name; \
		static constexpr tr_struct_version version = lump_version; \
	};

#define TR_LUMP_NAME(type) (TrLumpInfo<type>::name)
#define TR_LUMP_VERSION(type) (TrLumpInfo<type>::version)

	// for caching purposes I'm using memcmp so there better not be any implicit padding
#pragma warning(push)
#pragma warning(error : 4820)

	// an index into one of the trace's vectors, dereference within a TrReadContextScope
	template<typename T>
	struct TrIdx
	{
		using idx_to = T;
		using idx_type = uint32_t;

		idx_type _val;

		// clang-format off
		TrIdx() : _val{std::numeric_limits<idx_type>::max()} {}
		TrIdx(idx_type v) : _val{v} {}
		void Invalidate() { *this = {}; }
		operator idx_type() const { return _val; }
		TrIdx operator+(idx_type b) const { return TrIdx{_val + b}; }
		TrIdx operator+(int b)      const { return TrIdx{_val + b}; }
		TrIdx operator-(idx_type b) const { return TrIdx{_val - b}; }
		TrIdx operator-(int b)      const { return TrIdx{_val - b}; }
		TrIdx& operator+=(idx_type b) { _val += b; return *this; }
		TrIdx& operator+=(int b)      { _val += b; return *this; }
		TrIdx& operator-=(idx_type b) { _val -= b; return *this; }
		TrIdx& operator-=(int b)      { _val -= b; return *this; }
		auto operator<=>(const TrIdx&) const = default;
		friend auto operator<=>(TrIdx a, size_t b) { return a._val <=> b; }
		TrIdx& operator++() { ++_val; return *this; }
		TrIdx& operator--() { --_val; return *this; }
		TrIdx operator++(int) { TrIdx tmp = *this; ++_val; return tmp; }
		TrIdx operator--(int) { TrIdx tmp = *this; --_val; return tmp; }
		// clang-format on

		bool IsValid() const;
		const T* operator*() const;

		auto operator->() const
		{
			return operator*();
		}

		const T& GetOrDefault(const T& o = T{}) const
		{
			return IsValid() ? ***this : o; // :)
		}
	};

	TR_DEFINE_LUMP(QAngle, "angle", 1);
	TR_DEFINE_LUMP(Vector, "point", 2);
	TR_DEFINE_LUMP(TrIdx<Vector>, "point_idx", 1);

	using TrStr = TrIdx<char>;
	TR_DEFINE_LUMP(TrStr::idx_to, "string", 1);
} // namespace player_trace

namespace std
{
	template<typename T>
	struct hash<player_trace::TrIdx<T>>
	{
		size_t operator()(const player_trace::TrIdx<T>& i) const
		{
			return hash<decltype(i._val)>{}(i._val);
		}
	};
}; // namespace std

namespace player_trace
{
	// a span of one of the trace's vectors, dereference within a TrReadContextScope
	template<typename T>
	struct TrSpan
	{
		TrIdx<T> start;
		uint32_t n;

		// a *default constructed* span must be invalid for the cache to work, so don't initialize n to 0
		TrSpan() : start{}, n{std::numeric_limits<uint32_t>::max()} {}
		TrSpan(TrIdx<T> idx, uint32_t n) : start{idx}, n{n} {}

		bool IsValid() const
		{
			return n == 0 || (start.IsValid() && (start + n - 1).IsValid());
		}

		auto operator<=>(const TrSpan& o) const
		{
			auto sp1 = operator*();
			auto sp2 = *o;
			return std::lexicographical_compare_three_way(sp1.begin(), sp1.end(), sp2.begin(), sp2.end());
		}

		std::span<const T> operator*() const;
	};

	using tr_tick = uint32_t;
	constexpr tr_tick TR_INVALID_TICK = std::numeric_limits<tr_tick>::max();

	struct TrLandmark_v1
	{
		TrStr nameIdx;
		TrIdx<Vector> posIdx;
	};
	TR_DEFINE_LUMP(TrLandmark_v1, "landmark", 1);

	using TrLandmark = TrLandmark_v1;

	struct TrMap_v1
	{
		TrStr nameIdx;
		TrIdx<Vector> landmarkDeltaToFirstMapIdx;
		TrSpan<TrLandmark> landmarkSp;
	};
	TR_DEFINE_LUMP(TrMap_v1, "map", 1);

	using TrMap = TrMap_v1;

	struct TrMapTransition_v1
	{
		tr_tick tick;
		TrIdx<TrMap_v1> fromMapIdx, toMapIdx;
	};
	TR_DEFINE_LUMP(TrMapTransition_v1, "map_transition", 1);

	using TrMapTransition = TrMapTransition_v1;

	struct TrSegmentStart_v1
	{
		tr_tick tick;
		TrSegmentReason reason = TR_SR_NONE;
	};
	TR_DEFINE_LUMP(TrSegmentStart_v1, "segment", 2);

	using TrSegmentStart = TrSegmentStart_v1;

	struct TrAbsBox_v1
	{
		TrIdx<Vector> minsIdx, maxsIdx;
	};
	TR_DEFINE_LUMP(TrAbsBox_v1, "aabb", 1);

	using TrAbsBox = TrAbsBox_v1;

	struct TrTransform_v1
	{
		TrIdx<Vector> posIdx;
		TrIdx<QAngle> angIdx;
	};
	TR_DEFINE_LUMP(TrTransform_v1, "transform", 1);
	TR_DEFINE_LUMP(TrIdx<TrTransform_v1>, "transform_idx", 1);

	using TrTransform = TrTransform_v1;

	struct TrPortal_v1
	{
		/*
		* We can't store an index to the linked portal because of how the index cache works,
		* so we'll have to do with the linked handle instead. I don't think we're gonna
		* end up needing to know the linked portal anyways.
		*/
		CBaseHandle handle, linkedHandle;
		TrIdx<TrTransform_v1> transIdx;
		int linkageId; // only recorded if the server is active
		bool isOrange, isOpen, isActivated;
		char pad = {};
	};
	TR_DEFINE_LUMP(TrPortal_v1, "portal", 1);
	TR_DEFINE_LUMP(TrIdx<TrPortal_v1>, "portal_idx", 1);

	using TrPortal = TrPortal_v1;

	/*
	* Like an entity snapshot, but just for portals. Since portals will rarely
	* change and there's usually only a few of them, we don't use deltas.
	*/
	struct TrPortalSnapshot_v1
	{
		tr_tick tick;
		TrSpan<TrIdx<TrPortal_v1>> portalsSp;
	};
	TR_DEFINE_LUMP(TrPortalSnapshot_v1, "portal_snap", 1);

	using TrPortalSnapshot = TrPortalSnapshot_v1;

	enum TrPhysicsObjectFlags : uint32_t
	{
		TR_POF_NONE = 0,
		TR_POF_ASLEEP = 1,
		TR_POF_MEOVEABLE = 1 << 1,
		TR_POF_IS_TRIGGER = 1 << 2,
		TR_POF_GRAVITY_ENABLED = 1 << 3,
	};

	struct TrPhysicsObjectInfo_v1
	{
		/*
		* The IPhysicsObject* is not a unique-enough identifier for determining a mesh. This
		* caused a bug where shadow clones would appear to have a mesh from the wrong physics
		* object (e.g. player shadow clone has the mesh of a radio). All shadow clone phys objects
		* have the same name (which is NULL), and it seems like they can sometimes get destroyed
		* and reallocated in the same place in memory, so we have to add some extra data (the
		* source entity) to ensure uniqueness.
		* 
		* Furthermore, portalsimulator_collisionentity's are tied to a single portal (so the handle
		* and IServerEntity* stay the same) and their collision prop claims they have a OBB of size
		* 0 and are always at the origin. To ensure uniqueness for them, we have to tie them to a
		* specific portal index AND the associated linked portal index (which will be different if
		* either portal changes location or angles).
		*/
		struct SourceEntity
		{
			union Id
			{
				IServerEntity* pServerEnt;
				struct PortalIdxInfo
				{
					TrIdx<TrPortal_v1>::idx_type primaryPortal : 16;
					TrIdx<TrPortal_v1>::idx_type linkedPortal : 16;
				} portalIdxIfThisIsCollisionEntity;
			} extraId;
			CBaseHandle handle;
		} sourceEntity;

		union
		{
			IPhysicsObject* pPhysicsObject;
		} extraId;
		TrStr nameIdx;
		uint32_t flags; // TrPhysicsObjectFlags
	};
	TR_DEFINE_LUMP(TrPhysicsObjectInfo_v1, "physics_object_info", 1);

	using TrPhysicsObjectInfo = TrPhysicsObjectInfo_v1;

	struct TrPhysMesh_v1
	{
		float ballRadius; // positive ball radius means this is a ball, otherwise it's a polygonal mesh
		TrSpan<TrIdx<Vector>> vertIdxSp; // triangle indices of polygonal mesh, possibly invalid
	};
	TR_DEFINE_LUMP(TrPhysMesh_v1, "physics_mesh", 1);

	using TrPhysMesh = TrPhysMesh_v1;

	/*
	* An equivalent of a CPhysicsObject - entities that have collision and stuff will generally
	* be composed of one or more physics objects. Because it's relatively expensive to regenerate
	* the mesh every tick to see if it's been cached before (and everything in the info struct is
	* pretty cheap to get), the info is used as a key to the mesh to check if it's new.
	*/
	struct TrPhysicsObject_v1
	{
		TrIdx<TrPhysicsObjectInfo_v1> infoIdx;
		TrIdx<TrPhysMesh_v1> meshIdx;
	};
	TR_DEFINE_LUMP(TrPhysicsObject_v1, "physics_object", 1);
	TR_DEFINE_LUMP(TrIdx<TrPhysicsObject_v1>, "physics_object_idx", 1);

	using TrPhysicsObject = TrPhysicsObject_v1;

	struct TrEnt_v1
	{
		union
		{
			IServerEntity* pServerEnt;
		} extraId;
		CBaseHandle handle;
		TrStr networkClassNameIdx;
		TrStr classNameIdx;
		TrStr nameIdx;
		TrSpan<TrIdx<TrPhysicsObject_v1>> physSp;

		uint32_t m_nSolidType : 4;     // SolidType_t
		uint32_t m_usSolidFlags : 11;  // SolidFlags_t
		uint32_t m_CollisionGroup : 6; // Collision_Group_t
		uint32_t _pad : 11;
	};
	TR_DEFINE_LUMP(TrEnt_v1, "ent", 1);

	using TrEnt = TrEnt_v1;

	struct TrEntTransform_v1
	{
		// keep this here because multi-phys ents constantly change their OBB
		TrIdx<TrAbsBox_v1> obbIdx;
		TrIdx<TrTransform_v1> obbTransIdx;
		TrSpan<TrIdx<TrTransform_v1>> physTransSp;
	};
	TR_DEFINE_LUMP(TrEntTransform_v1, "ent_transform", 1);

	using TrEntTransform = TrEntTransform_v1;

	/*
	* Instead of storing entity positions for every tick, entity positions are stored as
	* bi-directional deltas (which allows iterating through the trace both forwards & backwards).
	* To allow fast jumping to an arbitrary point in the trace, occasionally full snapshots of the
	* entity state are stored. This is basically the same technique that's used to send entity
	* info from server to client in e.g. demos, and in cases with lots of entities can result in
	* 1/3 of the memory usage for entity transforms compared to using snapshots alone.
	* 
	* Entity deltas consist of:
	* 
	* - new entities or entities that just entered the entity record radius
	* - entities that dissapeared or left the record radius
	* - entities whos positions changed
	* 
	* For full snapshots, it's sufficient to record all entities as 'new'.
	*/

	struct TrEntCreateDelta_v1
	{
		TrIdx<TrEnt_v1> entIdx;
		TrIdx<TrEntTransform_v1> transIdx;
	};
	TR_DEFINE_LUMP(TrEntCreateDelta_v1, "ent_create_delta", 1);

	using TrEntCreateDelta = TrEntCreateDelta_v1;

	struct TrEntTransformDelta_v1
	{
		TrIdx<TrEnt_v1> entIdx;
		TrIdx<TrEntTransform_v1> fromTransIdx, toTransIdx;
	};
	TR_DEFINE_LUMP(TrEntTransformDelta_v1, "ent_trans_delta", 1);

	using TrEntTransformDelta = TrEntTransformDelta_v1;

	struct TrEntDeleteDelta_v1
	{
		TrIdx<TrEnt_v1> entIdx;
		TrIdx<TrEntTransform_v1> oldTransIdx;
	};
	TR_DEFINE_LUMP(TrEntDeleteDelta_v1, "ent_delete_delta", 1);

	using TrEntDeleteDelta = TrEntDeleteDelta_v1;

	struct TrEntSnapshotDelta_v1
	{
		tr_tick tick;
		TrSpan<TrEntCreateDelta_v1> createSp;
		TrSpan<TrEntTransformDelta_v1> deltaSp;
		TrSpan<TrEntDeleteDelta_v1> deleteSp;
	};
	TR_DEFINE_LUMP(TrEntSnapshotDelta_v1, "ent_snap_delta", 1);

	using TrEntSnapshotDelta = TrEntSnapshotDelta_v1;

	struct TrEntSnapshot_v1
	{
		tr_tick tick;
		TrSpan<TrEntCreateDelta_v1> createSp;
		// the corresponding delta is included here for slightly faster ent iteration (possibly invalid)
		TrIdx<TrEntSnapshotDelta_v1> snapDeltaIdx;
	};
	TR_DEFINE_LUMP(TrEntSnapshot_v1, "ent_snap", 1);

	using TrEntSnapshot = TrEntSnapshot_v1;

	struct TrPlayerContactPoint_v1
	{
		TrIdx<Vector> posIdx, normIdx;
		float force;
		TrStr objNameIdx;
		bool playerIsObj0;
		char _pad[3];
	};
	TR_DEFINE_LUMP(TrPlayerContactPoint_v1, "contact_point", 1);
	TR_DEFINE_LUMP(TrIdx<TrPlayerContactPoint_v1>, "contact_point_idx", 1);

	using TrPlayerContactPoint = TrPlayerContactPoint_v1;

	struct TrPlayerData_v1
	{
		TrPlayerData_v1() = default;
		TrPlayerData_v1(tr_tick tick) : tick{tick} {}

		tr_tick tick = TR_INVALID_TICK;
		TrIdx<Vector> qPosIdx, qVelIdx;
		TrIdx<TrTransform_v1> transEyesIdx, transSgEyesIdx, transVPhysIdx;
		TrSpan<TrIdx<TrPlayerContactPoint_v1>> contactPtsSp;

		int m_fFlags = 0;
		uint32_t fov : 8 = 0;
		uint32_t m_iHealth : 8 = 0;
		uint32_t m_lifeState : 4 = 0;
		uint32_t m_CollisionGroup : 6 = 0; // Collision_Group_t
		uint32_t m_MoveType : 6 = 0;       // MoveType_t
	};
	TR_DEFINE_LUMP(TrPlayerData_v1, "player_data", 1);

	struct TrPlayerData_v2 : TrPlayerData_v1
	{
		TrPlayerData_v2() = default;
		TrPlayerData_v2(tr_tick tick) : TrPlayerData_v1{tick} {};

		TrPlayerData_v2(const TrPlayerData_v1& v1)
		{
			memcpy(this, &v1, sizeof v1);
		}

		TrIdx<Vector> vVelIdx;
		CBaseHandle envPortalHandle{};
	};
	TR_DEFINE_LUMP(TrPlayerData_v2, "player_data", 2);

	using TrPlayerData = TrPlayerData_v2;

	struct TrServerState_v1
	{
		tr_tick tick;
		int serverTick;
		bool paused;
		bool hostTickSimulating;
		char _pad[2]{};

		int GetServerTickFromThisAsLastState(tr_tick atTick) const
		{
			return paused ? serverTick : serverTick + atTick - tick;
		}
	};
	TR_DEFINE_LUMP(TrServerState_v1, "server_state", 1);

	using TrServerState = TrServerState_v1;

	struct TrTraceState_v1
	{
		tr_tick tick;
		TrIdx<TrAbsBox_v1> entCollectBboxAroundPlayerIdx;
	};
	TR_DEFINE_LUMP(TrTraceState_v1, "trace_state", 1);

	using TrTraceState = TrTraceState_v1;

#pragma warning(pop) // all trace objects should be above this pragma

	/*
	* Behold all the data in the player trace. Everything that is stored inside the struct is all
	* that is needed for serialization & deserialization. Most of that stuff is going to be in one
	* of the storage lists. All of the caching associated with recording & rendering is stored
	* separately.
	*/
	struct TrPlayerTrace
	{
		// I might be going to hell for this
		std::tuple<
		    // :)

		    std::vector<Vector>,
		    std::vector<QAngle>,
		    std::vector<TrTransform>,
		    // player stuff!
		    std::vector<TrPlayerData>,
		    std::vector<TrPlayerContactPoint>,
		    std::vector<TrIdx<TrPlayerContactPoint>>,
		    // map stuff, first map is at index 0
		    std::vector<TrMap>,
		    std::vector<TrLandmark>,
		    std::vector<TrMapTransition>,
		    // game and trace recording state switching
		    std::vector<TrSegmentStart>,
		    std::vector<TrServerState>,
		    std::vector<TrTraceState>,
		    // strings
		    std::vector<TrStr::idx_to>,
		    // portal storage
		    std::vector<TrPortal>,
		    std::vector<TrIdx<TrPortal>>,
		    std::vector<TrPortalSnapshot>,
		    // entity stuff
		    std::vector<TrEnt>,
		    std::vector<TrAbsBox>,
		    std::vector<TrEntSnapshot>,
		    std::vector<TrEntCreateDelta>,
		    std::vector<TrEntTransformDelta>,
		    std::vector<TrEntSnapshotDelta>,
		    std::vector<TrEntDeleteDelta>,
		    std::vector<TrIdx<TrTransform>>,
		    std::vector<TrPhysicsObjectInfo>,
		    std::vector<TrPhysMesh>,
		    std::vector<TrPhysicsObject>,
		    std::vector<TrIdx<Vector>>,
		    std::vector<TrIdx<TrPhysicsObject>>,
		    std::vector<TrEntTransform>

		    // :)
		    >
		    _storage;

		tr_tick numRecordedTicks = 0;
		TrIdx<TrAbsBox_v1> playerStandBboxIdx{}, playerDuckBboxIdx{};

		struct
		{
			std::string gameName;
			std::string gameModName;
			std::string playerName;
			bool playerNameInitialized = false;
			int32_t gameVersion = -666;

			void Clear()
			{
				gameName.clear();
				gameModName.clear();
				playerName.clear();
				playerNameInitialized = false;
				gameVersion = -666;
			}
		} firstRecordedInfo;

		bool hasStartRecordingBeenCalled = false;

	private:
		/*
		* A little bit of boilerplate to create a corresponding tuple of versions for each type in
		* the storage. Say our storage is std::tuple<std::vector<A>, std::vector<B>>, then we want
		* a corresponding std::tuple<VersionHolder<A>, VersionHolder<B>>.
		*/
		template<typename T>
		struct VersionHolder
		{
			using type = T;
			tr_struct_version firstExportVersion;
		};

		template<typename... Ts>
		struct StorageToVersionHolder;

		template<typename... Ts>
		struct StorageToVersionHolder<std::tuple<Ts...>>
		{
			using type = std::tuple<VersionHolder<typename Ts::value_type>...>;
		};

		StorageToVersionHolder<decltype(_storage)>::type versions;

		std::unique_ptr<TrRecordingCache> recordingCache;
		mutable std::unique_ptr<TrRenderingCache> renderingCache;
		mutable std::unique_ptr<TrEntityCache> entityCache;

		TrRecordingCache& GetRecordingCache();

		void CollectServerState(bool hostTickSimulating);
		void CollectPlayerData();
		void CollectPortals();
		void CollectEntities(float entCollectRadius);
		void CollectTraceState(float entCollectRadius);
		void CollectMapTransition();

		template<typename T>
		static bool MemSameExceptTick(const T& a, const T& b)
		{
			constexpr size_t offsetAfterTick = offsetof(T, tick) + sizeof(a.tick);
			return !memcmp(&a, &b, offsetof(T, tick))
			       && !memcmp((char*)&a + offsetAfterTick,
			                  (char*)&b + offsetAfterTick,
			                  sizeof(T) - offsetAfterTick);
		}

	public:
		void Clear();
		void StartRecording();
		void StopRecording();

		bool IsRecording() const
		{
			return recordingCache.get();
		}

		bool IsRendering() const
		{
			return renderingCache.get();
		}

		TrRenderingCache& GetRenderingCache() const;
		TrEntityCache& GetEntityCache() const;

		void KillRenderingCache();
		void KillEntityCache();

		size_t GetMemoryUsage() const
		{
			auto capacityFunc = [](auto&... vecs)
			{ return (0u + ... + (vecs.capacity() * sizeof(std::decay_t<decltype(vecs)>::value_type))); };
			return sizeof(*this) + std::apply(capacityFunc, _storage);
		}

		template<typename T>
		constexpr std::vector<T>& Get()
		{
			return std::get<std::vector<T>>(_storage);
		}

		template<typename T>
		constexpr const std::vector<T>& Get() const
		{
			return std::get<std::vector<T>>(_storage);
		}

		/*
		* If a trace was just recorded, the version of each type will be the most up-to-date one,
		* i.e. TR_LUMP_VERSION(T). If a trace was imported, this is used to get the version that
		* was *first* recorded for that type. Example use case:
		* 
		* 1) trace is recorded and exported
		* 2) trace is imported in newer spt version which has new fields (e.g. player shadow vel)
		* 3) during the import process, a compat handler initializes the player shadow vel to NAN
		* 4) we can use GetFirstExportVersion<TrPlayerData> to check if the vel was one of the
		*    things that was recorded when the trace was *first* written
		* 
		* If the lump for type T did not exist in the file OR if StartRecording hasn't been called
		* yet, GetFirstExportVersion<T> will return TR_INVALID_STRUCT_VERSION. Since
		* TR_INVALID_STRUCT_VERSION is just 0, we can then do:
		* 
		* if (GetFirstExportVersion<TrPlayerData>() < 2)
		*     Msg("Trace does not have player shadow vel");
		*/

		// only call from import code
		template<typename T>
		constexpr void SetFirstExportVersion(tr_struct_version v)
		{
			std::get<VersionHolder<T>>(versions).firstExportVersion = v;
		}

		template<typename T>
		constexpr tr_struct_version GetFirstExportVersion() const
		{
			return std::get<VersionHolder<T>>(versions).firstExportVersion;
		}

		/*
		* Gets a state at the given tick using a binary search.
		* Specifically, given a tick, finds either:
		* - the *first* element with the matching 'tick' field
		* - the *last* element with a smaller 'tick' field if no exact match exists
		* 
		* The result is clamped to be an element inside the vector. So long as the vector
		* is not empty, a non-end index is always returned.
		*/
		template<typename T>
		TrIdx<T> GetAtTick(tr_tick atTick) const
		{
			const std::vector<T>& vec = Get<T>();
			if (vec.empty())
				return TrIdx<T>{};

			AssertMsg(vec.size() < 2 || (vec[0].tick != vec[1].tick),
			          "SPT: ticks should be distinct in this struct");

			auto it =
			    std::upper_bound(vec.cbegin(),
			                     vec.cend(),
			                     atTick,
			                     [](tr_tick targetTick, const T& o) { return targetTick < o.tick + 1; });
			TrIdx<T> idx = std::distance(vec.cbegin(), it);
			return (it != vec.cbegin()) && (it == vec.cend() || it->tick > atTick) ? idx - 1 : idx;
		}

		int GetServerTickAtTick(tr_tick atTick) const;
		TrIdx<TrMap_v1> GetMapAtTick(tr_tick atTick) const;
		Vector GetAdjacentLandmarkDelta(std::span<const TrLandmark> fromLandmarkSp,
		                                std::span<const TrLandmark> toLandmarkSp) const;

		void HostTickCollect(bool simulated, TrSegmentReason segmentReason, float entCollectRadius);
	};

	template<typename T>
	inline bool TrIdx<T>::IsValid() const
	{
		return _val < TrReadContextScope::Current().Get<T>().size();
	}

	template<typename T>
	inline const T* TrIdx<T>::operator*() const
	{
		Assert(IsValid());
		return TrReadContextScope::Current().Get<T>().data() + _val;
	}

	template<typename T>
	inline std::span<const T> TrSpan<T>::operator*() const
	{
		if (n == 0 || !IsValid())
			return {};
		return std::span<const T>{TrReadContextScope::Current().Get<T>()}.subspan(start, n);
	}
} // namespace player_trace

#endif
