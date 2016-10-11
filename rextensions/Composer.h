#include "ProcMem.h"
#include <algorithm>

class BinarySearchComposer
{
	using SEARCH_DIRECTION = ProcMem::SEARCH_DIRECTION;
	using SEGMENT = ProcMem::SEGMENT;

	ProcMem *m_mem;

	struct State
	{
		friend class BinarySearchComposer;

	private:

		State(class ProcMem *mem) : m_mem(mem)
		{
		}

		class ProcMem *m_mem;
		std::vector<BYTE *> m_results;

		enum MODE
		{
			REPLACE,
			WHERE,
			NOT_WHERE
		} m_mode;

		bool m_isDont = false;

		std::vector<BYTE *> Apply(const std::function<std::vector<BYTE *>(class ProcMem *, BYTE *)>& fn)
		{
			std::vector<BYTE *> results;

			for (auto current : m_results)
			{
				auto candidates = fn(m_mem, current);
				
				switch(m_mode)
				{
					case WHERE:
						if (candidates.size() > 0)
						{
							results.push_back(current);
						}
						break;
					case NOT_WHERE:
						if (candidates.size() == 0)
						{
							results.push_back(current);
						}
					default:
						results.insert(results.end(), candidates.begin(), candidates.end());
				}
			}

			// Reset mode
			m_mode = REPLACE;

			return results;
		}

		void Propagate(const std::function<std::vector<BYTE *>(class ProcMem *, BYTE *)>& fn)
		{
			m_results = Apply(fn);
		}

		static SEGMENT MakeSegmentByRange(BYTE *ref, size_t range, SEARCH_DIRECTION direction)
		{
			auto down = direction == ProcMem::SEARCH_DOWN;

			return{ down ? ref : ref - range, down ? ref + range : ref };
		}

		State& FindString(const std::string &string)
		{
			auto result = m_mem->FindDataString(string);

			if (result != nullptr)
			{
				m_results.push_back(result);
			}

			return *this;
		}

	public:
		
		static const size_t DEFAULT_SEARCH_RANGE = 200;

		State& FindReferences()
		{
			Propagate([](class ProcMem *mem, BYTE *ref)
			{
				return mem->FindAllReferences(ref, mem->GetSectionByName(".text"));
			});

			return *this;
		}

		State& Find(const std::vector<BYTE> &bytes, SEARCH_DIRECTION direction, size_t range = DEFAULT_SEARCH_RANGE, bool wildcard = false)
		{
			Propagate([&bytes, range, direction, wildcard](class ProcMem *mem, BYTE *ref)
			{
				auto segment = MakeSegmentByRange(ref, range, direction);
				
				return mem->FindAllBytes(bytes, segment, wildcard, direction);
			});

			return *this;
		}

		State& FindStartOfFunctionByCall(size_t align, size_t numCallsRequired = 1, size_t range = DEFAULT_SEARCH_RANGE)
		{
			Propagate([align, numCallsRequired, range](class ProcMem *mem, BYTE *ref)->std::vector<BYTE *>
			{
				auto alignPointer = [align](BYTE *ptr)
				{
					return (BYTE *)((size_t)ptr & ~(align - 1));
				};

				auto base = ref - range;
				auto segment = mem->GetSectionByName(".text");

				do
				{
					ref = alignPointer(ref);

					if (mem->FindCallsTo(ref, segment).size() >= numCallsRequired)
					{
						return { ref };
					}

				} while (ref-- > base);

				return {};
			});

			return *this;
		}

		State& FindStartOfFunction(size_t numCallsRequired = 0, size_t numReferencesRequired = 0, size_t range = DEFAULT_SEARCH_RANGE)
		{
			auto mem = m_mem;

			std::transform(m_results.begin(), m_results.end(), m_results.begin(), [mem, range](BYTE *curRef) -> BYTE *
			{
				SEGMENT s { curRef - range, curRef };
				BYTE *addr;

				// Try to find frame size setup

				addr = mem->FindBytes({ 0x83, 0xEC }, s, false, ProcMem::SEARCH_UP); // sub esp, ?

				if (addr != nullptr)
				{
					return addr;
				}

				// Try to find Prologue

				addr = mem->FindBytes({ 0x55, 0x8B, 0xEC }, s, false, ProcMem::SEARCH_UP); // push ebp; mov ebp, esp;

				if (addr != nullptr)
				{
					return addr;
				}

				// Try to find alignment bytes

				addr = mem->FindBytes({ 0xCC, 0xCC, 0xCC, 0xCC }, s, false, ProcMem::SEARCH_UP);

				if (addr != nullptr)
				{
					return addr + 4;
				}

				addr = mem->FindBytes({ 0x90, 0x90, 0x90, 0x90 }, s, false, ProcMem::SEARCH_UP);

				if (addr != nullptr)
				{
					return addr + 4;
				}

				return nullptr;
			});

			m_results.erase(std::remove_if(m_results.begin(), m_results.end(), [](BYTE *curRef) -> bool
			{
				return curRef == nullptr;
			}), m_results.end());

			Propagate([](class ProcMem *mem, BYTE *ref) -> std::vector<BYTE *>
			{
				ProcMem::SEGMENT segment{ ref - 32, ref };

				// Try to find alignment bytes (0xCC or 0x90)

				BYTE *ptr = nullptr;
				BYTE *fnStart = nullptr;

				ptr = mem->FindBytes({ 0xCC, 0xCC }, segment, false, ProcMem::SEARCH_UP);

				if (ptr != nullptr)
				{
					while (*ptr == 0xCC) ptr++;

					fnStart = ptr;
				}

				ptr = mem->FindBytes({ 0x90, 0x90 }, segment, false, ProcMem::SEARCH_UP);

				if (ptr != nullptr)
				{
					while (*ptr == 0x90) ptr++;

					fnStart = ptr;
				}

				// Try to find function prologue

				segment.end += 3;

				ptr = mem->FindBytes({ 0x55, 0x8B, 0xEC }, segment, false, ProcMem::SEARCH_UP); // push ebp; mov ebp, esp;

				if (ptr != nullptr)
				{
					fnStart = ptr;
				}

				if (fnStart != nullptr)
				{
					return{ fnStart };
				}

				return{};
			});

			auto segment = m_mem->GetSectionByName(".text");

			if (numCallsRequired)
			{
				m_results.erase(std::remove_if(m_results.begin(), m_results.end(), [mem, segment, numCallsRequired](BYTE *refAddr) -> bool
				{
					return mem->FindCallsTo(refAddr, segment).size() < numCallsRequired;
				}), m_results.end());
			}

			if (numReferencesRequired)
			{
				m_results.erase(std::remove_if(m_results.begin(), m_results.end(), [mem, segment, numReferencesRequired](BYTE *refAddr) -> bool
				{
					return mem->FindAllReferences(refAddr, segment).size() < numReferencesRequired;
				}), m_results.end());
			}

			return *this;
		}

		State& FindCallsToCurrentAddress()
		{
			Propagate([](class ProcMem *mem, BYTE *ref)
			{
				return mem->FindCallsTo(ref, mem->GetSectionByName(".text"));
			});

			return *this;
		}

		State& FindCallsTo(BYTE *address, size_t range, SEARCH_DIRECTION direction)
		{
			auto segment = MakeSegmentByRange(address, range, direction);
			
			Propagate([segment, direction](class ProcMem *mem, BYTE *ref)
			{
				return mem->FindCallsTo(ref, segment, direction);
			});

			return *this;
		}

		State& FindCallsToAbove(BYTE *address, size_t range)
		{
			return FindCallsTo(address, range, SEARCH_DIRECTION::SEARCH_UP);
		}

		State& FindCallsToBelow(BYTE *address, size_t range)
		{
			return FindCallsTo(address, range, SEARCH_DIRECTION::SEARCH_DOWN);
		}

		State& FindReferencesToAddress(BYTE *address, size_t range, SEARCH_DIRECTION direction)
		{
			auto segment = MakeSegmentByRange(address, range, direction);

			Propagate([segment](class ProcMem *mem, BYTE *ref)
			{
				return mem->FindAllReferences(ref, segment);
			});

			return *this;
		}

		State& FollowCall()
		{
			Propagate([](class ProcMem *mem, BYTE *ref) -> std::vector<BYTE *>
			{
				return { (BYTE *)mem->DecodeCallAddress(ref) };
			});

			return *this;
		}

		State& FindReferencesToAddressAbove(BYTE *address, size_t range = DEFAULT_SEARCH_RANGE)
		{
			return FindReferencesToAddress(address, range, SEARCH_DIRECTION::SEARCH_UP);
		}

		State& FindReferencesToAddressBelow(BYTE *address, size_t range = DEFAULT_SEARCH_RANGE)
		{
			return FindReferencesToAddress(address, range, SEARCH_DIRECTION::SEARCH_DOWN);
		}
		
		State& FindReferencesToImport(const char *moduleName, const char *procName, size_t range, SEARCH_DIRECTION direction)
		{
			return FindReferencesToAddress(m_mem->GetImportTableAddress(moduleName, procName), range, direction);
		}

		State& FindReferencesToImportAbove(const char *moduleName, const char *procName, size_t range = DEFAULT_SEARCH_RANGE)
		{
			return FindReferencesToImport(moduleName, procName, range, SEARCH_DIRECTION::SEARCH_UP);
		}

		State& FindReferencesToImportBelow(const char *moduleName, const char *procName, size_t range = DEFAULT_SEARCH_RANGE)
		{
			return FindReferencesToImport(moduleName, procName, range, SEARCH_DIRECTION::SEARCH_DOWN);
		}

		State& FindAbove(const std::vector<BYTE> &bytes, size_t range = DEFAULT_SEARCH_RANGE, bool wildcard = false)
		{
			return Find(bytes, ProcMem::SEARCH_UP, range, wildcard);
		}

		State& FindBelow(const std::vector<BYTE> &bytes, size_t range = DEFAULT_SEARCH_RANGE, bool wildcard = false)
		{
			return Find(bytes, ProcMem::SEARCH_DOWN, range, wildcard);
		}

		State& Nth(size_t N)
		{
			N -= 1;

			if (m_results.size() > N)
			{
				auto nth = m_results[N];

				m_results.clear();
				m_results.push_back(nth);
			}

			return *this;
		}

		State& First()
		{
			return Nth(0);
		}

		State& Last()
		{
			if (m_results.size() > 0)
			{
				return Nth(m_results.size() - 1);
			}

			return *this;
		}

		State& If()
		{
			m_mode = WHERE;

			return *this;
		}

		State& Unless()
		{
			m_mode = NOT_WHERE;

			return *this;
		}

		size_t Count()
		{
			return m_results.size();
		}

		BYTE * GetMostCommon(std::function<bool(BYTE *, BYTE *)> cmp)
		{
			auto &results = m_results;

			if (results.size() == 0)
			{
				return nullptr;
			}

			return *std::max_element(results.begin(), results.end(), [results, cmp](BYTE *lhs, BYTE *rhs) -> bool
			{
				auto num_equals = [results, cmp](BYTE *cur)
				{
					return std::count_if(results.begin(), results.end(), [cmp, cur](BYTE *addr)
					{
						return cmp(cur, addr);
					});
				};

				return num_equals(lhs) < num_equals(rhs);
			});
		}

		BYTE * GetMostCommonBytes(size_t n = 1)
		{
			return GetMostCommon([n](BYTE *lhs, BYTE *rhs)
			{
				return std::memcmp(lhs, rhs, n) == 0;
			});
		}

		BYTE * GetMostCommonCall()
		{
			return GetMostCommon([](BYTE *lhs, BYTE *rhs)
			{
				return ProcMem::DecodeCallAddress(lhs) == ProcMem::DecodeCallAddress(rhs);
			});
		}

		BYTE * GetOne()
		{
			if (m_results.size() == 0)
			{
				return nullptr;
			}

			return m_results[0];
		}

		std::vector<BYTE *> GetAll()
		{
			return m_results;
		}

		BYTE * GetNth(size_t N)
		{
			N -= 1;

			if (m_results.size() > N)
			{
				return m_results[N];
			}

			return nullptr;
		}
	};

public:

	BinarySearchComposer(ProcMem *memSearcher)
		: m_mem(memSearcher)
	{
	}

	State FromOffset(void *offset)
	{
		auto s = State(m_mem);

		s.m_results.push_back((BYTE *)offset);

		return s;
	}

	State FromCode(const std::vector<BYTE> &bytes, bool wildcard = false)
	{
		State s{ m_mem };

		s.m_results = m_mem->FindAllBytes(bytes, m_mem->GetSectionByName(".text"), wildcard);

		return s;
	}

	State FromCallTo(BYTE *function)
	{
		return FromOffset(function).FindCallsToCurrentAddress();
	}

	State FromImport(char *moduleName, char *procName)
	{
		return FromOffset(m_mem->GetImportTableAddress(moduleName, procName));
	}

	State FromString(const std::string &string)
	{
		return State(m_mem).FindString(string);
	}
};
