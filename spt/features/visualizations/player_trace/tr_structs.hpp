#pragma once

#include <span>
#include <tuple>
#include <algorithm>
#include <compare>

#include "tr_config.hpp"
#include "spt/utils/interfaces.hpp"

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
	constexpr size_t TR_MAX_TRACE_NAME_LEN = 64;
	constexpr size_t TR_MAX_SPT_VERSION_LEN = 32;

// for caching purposes I'm using memcmp so there better not be any implicit padding
#pragma warning(push)
#pragma warning(error : 4820)

	struct TrPlayerTrace;

	// a scope within which we can deref TrIdx & TrSpan
	class TrReadContextScope
	{
		inline static const TrPlayerTrace* trDataCtx = nullptr;
		const TrPlayerTrace* oldCtx;

		template<typename T>
		friend struct TrIdx;

		template<typename T>
		friend struct TrSpan;

	public:
		TrReadContextScope(const TrPlayerTrace& newCtx) : oldCtx{trDataCtx}
		{
			trDataCtx = &newCtx;
		}

		~TrReadContextScope()
		{
			trDataCtx = oldCtx;
		}
	};

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
	};

	using TrStr = TrIdx<char>;
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

	struct TrLandmark_v1
	{
		TrStr nameIdx;
		TrIdx<Vector> posIdx;
	};

	struct TrMap_v1
	{
		TrStr nameIdx;
		TrIdx<Vector> landmarkDeltaToFirstMapIdx;
		TrSpan<TrLandmark_v1> landmarkSp;
	};

	struct TrMapTransition_v1
	{
		uint32_t tick;
		TrIdx<TrMap_v1> fromMapIdx, toMapIdx;
	};

	struct TrSegmentStart_v1
	{
		uint32_t tick;
		TrSegmentReason reason;
	};

	struct TrAbsBox_v1
	{
		TrIdx<Vector> minsIdx, maxsIdx;
	};

	struct TrTransform_v1
	{
		TrIdx<Vector> posIdx;
		TrIdx<QAngle> angIdx;
	};

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

	/*
	* Like an entity snapshot, but just for portals. Since portals will rarely
	* change and there's usually only a few of them, we don't use deltas.
	*/
	struct TrPortalSnapshot_v1
	{
		uint32_t tick;
		TrSpan<TrIdx<TrPortal_v1>> portalsSp;
	};

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

	struct TrPhysMesh_v1
	{
		float ballRadius; // positive ball radius means this is a ball, otherwise it's a polygonal mesh
		TrSpan<TrIdx<Vector>> vertIdxSp; // triangle indices of polygonal mesh, possibly invalid
	};

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

	struct TrEntTransform_v1
	{
		// keep this here because multi-phys ents constantly change their OBB
		TrIdx<TrAbsBox_v1> obbIdx;
		TrIdx<TrTransform_v1> obbTransIdx;
		TrSpan<TrIdx<TrTransform_v1>> physTransSp;
	};

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

	struct TrEntTransformDelta_v1
	{
		TrIdx<TrEnt_v1> entIdx;
		TrIdx<TrEntTransform_v1> fromTransIdx, toTransIdx;
	};

	struct TrEntDeleteDelta_v1
	{
		TrIdx<TrEnt_v1> entIdx;
		TrIdx<TrEntTransform_v1> oldTransIdx;
	};

	struct TrEntSnapshotDelta_v1
	{
		uint32_t tick;
		TrSpan<TrEntCreateDelta_v1> createSp;
		TrSpan<TrEntTransformDelta_v1> deltaSp;
		TrSpan<TrEntDeleteDelta_v1> deleteSp;
	};

	struct TrEntSnapshot_v1
	{
		uint32_t tick;
		TrSpan<TrEntCreateDelta_v1> createSp;
		// the corresponding delta is included here for slightly faster ent iteration (possibly invalid)
		TrIdx<TrEntSnapshotDelta_v1> snapDeltaIdx;
	};

	struct TrPlayerContactPoint_v1
	{
		TrIdx<Vector> posIdx, normIdx;
		float force;
		TrStr objNameIdx;
		bool playerIsObj0;
		char _pad[3];
	};

	struct TrPlayerData_v1
	{
		uint32_t tick;
		TrIdx<Vector> qPosIdx, qVelIdx;
		TrIdx<TrTransform_v1> transEyesIdx, transSgEyesIdx, transVPhysIdx;
		TrSpan<TrIdx<TrPlayerContactPoint_v1>> contactPtsSp;

		int m_fFlags;
		uint32_t fov : 8;
		uint32_t m_iHealth : 8;
		uint32_t m_lifeState : 4;
		uint32_t m_CollisionGroup : 6; // Collision_Group_t
		uint32_t m_MoveType : 6;       // MoveType_t
	};

	struct TrServerState_v1
	{
		uint32_t tick;
		int serverTick;
		bool paused;
		bool hostTickSimulating;
		char _pad[2]{};

		int GetServerTickFromThisAsLastState(uint32_t atTick) const
		{
			return paused ? serverTick : serverTick + atTick - tick;
		}
	};

	struct TrTraceState_v1
	{
		uint32_t tick;
		TrIdx<TrAbsBox_v1> entCollectBboxAroundPlayerIdx;
	};

#pragma warning(pop) // all trace objects should be above this pragma

	struct ITrWriter
	{
		virtual bool Write(std::span<const char> sp) = 0;
		virtual void Finish() {};

		bool Write(const void* what, size_t nBytes)
		{
			return Write(std::span<const char>{(char*)what, nBytes});
		}

		template<typename T>
		bool Write(const T& o)
		{
			return Write(&o, sizeof o);
		}
	};

	struct ITrReader
	{
		virtual bool ReadTo(std::span<char> sp, uint32_t at) = 0;
		virtual void Finish() {};

		template<typename T>
		bool ReadTo(T& o, uint32_t at)
		{
			return ReadTo(std::span<char>{(char*)&o, sizeof o}, at);
		}
	};

	class TrRenderingCache;
	class TrRecordingCache;

	/*
	* Behold all the data in the player trace. Everything that is stored inside the struct is all
	* that is needed for serialization & deserialization. Most of that stuff is going to be in one
	* of the storage lists. All of the caching associated with recording & rendering is stored
	* separately.
	*/
	struct TrPlayerTrace
	{
	private:
		// I might be going to hell for this
		std::tuple<
		    // :)

		    std::vector<Vector>,
		    std::vector<QAngle>,
		    std::vector<TrTransform_v1>,
		    // player stuff!
		    std::vector<TrPlayerData_v1>,
		    std::vector<TrPlayerContactPoint_v1>,
		    std::vector<TrIdx<TrPlayerContactPoint_v1>>,
		    // map stuff, first map is at index 0
		    std::vector<TrMap_v1>,
		    std::vector<TrLandmark_v1>,
		    std::vector<TrMapTransition_v1>,
		    // game and trace recording state switching
		    std::vector<TrSegmentStart_v1>,
		    std::vector<TrServerState_v1>,
		    std::vector<TrTraceState_v1>,
		    // strings
		    std::vector<TrStr::idx_to>,
		    // portal storage
		    std::vector<TrPortal_v1>,
		    std::vector<TrIdx<TrPortal_v1>>,
		    std::vector<TrPortalSnapshot_v1>,
		    // entity stuff
		    std::vector<TrEnt_v1>,
		    std::vector<TrAbsBox_v1>,
		    std::vector<TrEntSnapshot_v1>,
		    std::vector<TrEntCreateDelta_v1>,
		    std::vector<TrEntTransformDelta_v1>,
		    std::vector<TrEntSnapshotDelta_v1>,
		    std::vector<TrEntDeleteDelta_v1>,
		    std::vector<TrIdx<TrTransform_v1>>,
		    std::vector<TrPhysicsObjectInfo_v1>,
		    std::vector<TrPhysMesh_v1>,
		    std::vector<TrPhysicsObject_v1>,
		    std::vector<TrIdx<Vector>>,
		    std::vector<TrIdx<TrPhysicsObject_v1>>,
		    std::vector<TrEntTransform_v1>

		    // :)
		    >
		    storage;

		std::unique_ptr<TrRecordingCache> recordingCache;
		std::unique_ptr<TrRenderingCache> renderingCache;

	public:
		uint32_t numRecordedTicks = 0;
		TrIdx<TrAbsBox_v1> playerStandBboxIdx{}, playerDuckBboxIdx{};

		using storageType = decltype(storage);

	private:
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

		void StopRendering();

		TrRenderingCache& GetRenderingCache();

		size_t GetMemoryUsage() const
		{
			auto capacityFunc = [](auto&... vecs)
			{ return (0u + ... + (vecs.capacity() * sizeof(std::decay_t<decltype(vecs)>::value_type))); };
			return sizeof(*this) + std::apply(capacityFunc, storage);
		}

		template<typename T>
		constexpr std::vector<T>& Get()
		{
			return std::get<std::vector<T>>(storage);
		}

		template<typename T>
		constexpr const std::vector<T>& Get() const
		{
			return std::get<std::vector<T>>(storage);
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
		TrIdx<T> GetAtTick(uint32_t atTick) const
		{
			const std::vector<T>& vec = Get<T>();
			if (vec.empty())
				return TrIdx<T>{};
			auto it =
			    std::upper_bound(vec.cbegin(),
			                     vec.cend(),
			                     atTick,
			                     [](uint32_t targetTick, const T& o) { return targetTick < o.tick + 1; });
			TrIdx<T> idx = std::distance(vec.cbegin(), it);
			return (it != vec.cbegin()) && (it == vec.cend() || it->tick > atTick) ? idx - 1 : idx;
		}

		int GetServerTickAtTick(uint32_t atTick) const;
		TrIdx<TrMap_v1> GetMapAtTick(uint32_t atTick) const;
		void HostTickCollect(bool simulated, TrSegmentReason segmentReason, float entCollectRadius);
		Vector GetAdjacentLandmarkDelta(std::span<const TrLandmark_v1> fromLandmarkSp,
		                                std::span<const TrLandmark_v1> toLandmarkSp) const;

		bool WriteBinaryStream(ITrWriter& wr) const;
		bool ReadBinaryStream(ITrReader& rd, char sptVersion[TR_MAX_SPT_VERSION_LEN], std::string& errMsg);
	};

	template<typename T>
	inline bool TrIdx<T>::IsValid() const
	{
		AssertMsg(!!TrReadContextScope::trDataCtx, "SPT: set up a TrReadContextScope");
		return _val < TrReadContextScope::trDataCtx->Get<T>().size();
	}

	template<typename T>
	inline const T* TrIdx<T>::operator*() const
	{
		Assert(IsValid());
		return TrReadContextScope::trDataCtx->Get<T>().data() + _val;
	}
	template<typename T>
	inline std::span<const T> TrSpan<T>::operator*() const
	{
		if (n == 0)
			return {(T*)nullptr, 0};
		return {TrReadContextScope::trDataCtx->Get<T>().data() + start, n};
	}
} // namespace player_trace

#endif

bool SptGetActiveTracePos(Vector& pos, QAngle& ang, float& fov);
