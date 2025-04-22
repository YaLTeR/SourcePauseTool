#pragma once

#include <unordered_set>
#include <map>
#include <string>
#include <span>

#include "tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

class CPortalSimulator;

namespace player_trace
{
	constexpr uint32_t MAX_DELTAS_WITHOUT_SNAPSHOT = 100;

	class TrRecordingCache
	{
		friend struct TrPlayerTrace;
		TrPlayerTrace* tr;

	private:
		/*
		* One of the main goals of the recording cache is to reuse indices that we've seen before.
		* Say we recording a new player data object, and the player is looking in the same
		* direction as they were last tick. Ideally, no new QAngle object is added. Instead, the
		* TrIdx<QAngle> into the std::vector<QAngle> is the same. The naive way to do this is to
		* do something like: std::unordered_map<QAngle, TrIdx<QAngle>>, and the logic would
		* look like this:
		* 
		* 1) try to insert a new QAngle
		* 2) if the key exists then reuse the TrIdx value
		* 3) if the key is new, set the value to be the size of the trace's std::vector<QAngle>
		* 4) add the QAngle to the std::vector<QAngle> in the trace
		* 
		* The overhead here is that the QAngle is stored twice in two locations. But this is not
		* necessary, since we can get the QAngle if we just have a TrIdx to it. So really, we don't
		* need a map, it's sufficient to have a set!
		* 
		* 1) try to insert a new MemKey<QAngle> (initialized with a QAngle&)
		* 2) if the key exists, we'll get back an iterator to a MemKey<QAngle> with a valid idx
		* 3) if the key is new, set the idx so to be the size of the trace's std::vector<QAngle>
		* 4) add the QAngle to the std::vector<QAngle> in the trace
		* 
		* Just like that, we don't need to store the QAngle in the cache because it already exists
		* in the trace's std::vector.
		*/
		template<typename T>
		struct MemKey
		{
			const T& ref;
			mutable TrIdx<T> idx;

			MemKey(const T& ref) : ref{ref}, idx{} {}

			inline const T* GetPtr() const
			{
				return idx.IsValid() ? *idx : &ref;
			}

			bool operator==(const MemKey& o) const
			{
				return !memcmp(GetPtr(), o.GetPtr(), sizeof(T));
			}

			struct Hasher
			{
				size_t operator()(const MemKey& o) const
				{
					return std::hash<std::string_view>{}(std::string_view{
					    reinterpret_cast<const char*>(o.GetPtr()),
					    sizeof(T),
					});
				}
			};
		};

		// pretty much the same logic that's used for MemKey is used for this
		template<typename T>
		struct MemSpanKey
		{
			std::span<const T> span;
			mutable TrSpan<T> trSpan;

			MemSpanKey(const std::span<const T>& sp) : span{sp}, trSpan{} {}

			inline std::span<const T> GetStdSpan() const
			{
				return trSpan.IsValid() ? *trSpan : span;
			}

			bool operator==(const MemSpanKey& o) const
			{
				auto sp1 = GetStdSpan();
				auto sp2 = o.GetStdSpan();
				return sp1.size() == sp2.size() && !memcmp(sp1.data(), sp2.data(), sp1.size_bytes());
			}

			struct Hasher
			{
				size_t operator()(const MemSpanKey& o) const
				{
					auto sp = o.GetStdSpan();
					return std::hash<std::string_view>{}(std::string_view{
					    reinterpret_cast<const char*>(sp.data()),
					    sp.size_bytes(),
					});
				}
			};
		};

		template<typename T>
		using MemKeySet = std::unordered_set<MemKey<T>, typename MemKey<T>::Hasher>;

		template<typename T>
		using MemSpanKeySet = std::unordered_set<MemSpanKey<T>, typename MemSpanKey<T>::Hasher>;

		std::tuple<

		    MemKeySet<Vector>,
		    MemKeySet<QAngle>,
		    MemKeySet<TrTransform_v1>,
		    MemKeySet<TrPortal_v1>,
		    MemKeySet<TrAbsBox_v1>,
		    MemKeySet<TrEnt_v1>,
		    MemKeySet<TrPhysicsObject_v1>,
		    MemKeySet<TrPhysMesh_v1>,
		    MemKeySet<TrEntTransform_v1>,
		    MemKeySet<TrPlayerContactPoint_v1>

		    >
		    idxSets;

		std::tuple<

		    MemSpanKeySet<char>,
		    MemSpanKeySet<TrIdx<Vector>>,
		    MemSpanKeySet<TrIdx<TrPhysicsObject_v1>>,
		    MemSpanKeySet<TrIdx<TrTransform_v1>>,
		    MemSpanKeySet<TrIdx<TrPortal_v1>>,
		    MemSpanKeySet<TrIdx<TrPlayerContactPoint_v1>>

		    >
		    spanSets;

		template<typename T>
		constexpr MemKeySet<T>& GetKeySet()
		{
			return std::get<MemKeySet<T>>(idxSets);
		}

		template<typename T>
		constexpr MemSpanKeySet<T>& GetSpanSet()
		{
			return std::get<MemSpanKeySet<T>>(spanSets);
		}

	public:
		TrRecordingCache(TrPlayerTrace& tr) : tr{&tr} {};
		TrRecordingCache(const TrRecordingCache&) = delete;

		struct EntSnapshotEntry
		{
			TrIdx<TrEnt_v1> entIdx;
			TrIdx<TrEntTransform_v1> entTransIdx;
			// a small optimization to allow overlapping snapshot deltas & full snapshot spans
			bool loggedAsCreateInLastDelta = false;

			auto operator<=>(const EntSnapshotEntry& o) const
			{
				return std::tie(entIdx, entTransIdx) <=> std::tie(o.entIdx, o.entTransIdx);
			}

			bool operator==(const EntSnapshotEntry& o) const
			{
				return operator<=>(o) == 0;
			}
		};

		std::unordered_map<CPortalSimulator*, TrIdx<TrPortal_v1>> simToPortalMap;
		std::vector<EntSnapshotEntry> entSnapshot;
		uint32_t nEntDeltasWithoutSnapshot = 0;

		// map IPhysicsObject* -> mesh_data so that we don't have to rebuild entity meshes every tick
		std::unordered_map<MemKey<TrPhysicsObjectInfo_v1>,
		                   TrIdx<TrPhysMesh_v1>,
		                   MemKey<TrPhysicsObjectInfo_v1>::Hasher>
		    entMeshMap;

		Vector landmarkDeltaToFirstMap = vec3_origin;

		struct
		{
			TrIdx<Vector> zeroVec{};
			TrIdx<Vector> invalidVec{};
			TrIdx<QAngle> invalidAng{};
			TrIdx<TrTransform_v1> invalidTrans{};
		} specialIdxs;

		void StartRecording()
		{
			specialIdxs.zeroVec = GetCachedIdx(vec3_origin);
			specialIdxs.invalidVec = GetCachedIdx(vec3_invalid);
			specialIdxs.invalidAng = GetCachedIdx(QAngle{NAN, NAN, NAN});
			specialIdxs.invalidTrans = GetCachedIdx(TrTransform_v1{
			    specialIdxs.invalidVec,
			    specialIdxs.invalidAng,
			});
		}

		void StopRecording()
		{
			CollectEntitySnapshot();
		}

		template<typename T>
		TrIdx<T> GetCachedIdx(const T& t)
		{
			TrReadContextScope ctx{*tr};
			auto& set = GetKeySet<T>();
			auto [it, new_elem] = set.emplace(t);
			if (new_elem)
			{
				auto& vec = tr->Get<T>();
				it->idx = vec.size();
				vec.push_back(t);
			}
			return it->idx;
		}

		template<typename T>
		TrSpan<T> GetCachedSpan(std::span<const T> sp)
		{
			TrReadContextScope ctx{*tr};
			auto& set = GetSpanSet<T>();
			auto [it, new_elem] = set.emplace(sp);
			if (new_elem)
			{
				auto& vec = tr->Get<T>();
				it->trSpan = TrSpan<T>{vec.size(), sp.size()};
				vec.insert(vec.cend(), sp.begin(), sp.end());
			}
			return it->trSpan;
		}

		template<typename T>
		TrSpan<T> GetCachedSpan(const std::vector<T>& vec)
		{
			return GetCachedSpan(std::span<const T>{vec.data(), vec.size()});
		}

		TrStr GetStringIdx(const char* s)
		{
			s = s ? s : "<NULL>";
			return GetCachedSpan(std::span<const char>{s, strlen(s) + 1}).start;
		}

		TrStr GetStringIdx(const std::string& s)
		{
			return GetCachedSpan(std::span<const char>{s.c_str(), s.size() + 1}).start;
		}

		void CollectEntityDelta(std::vector<EntSnapshotEntry>& newSnapshot);
		// delta must be recorded first!
		void CollectEntitySnapshot();
	};

} // namespace player_trace

#endif
