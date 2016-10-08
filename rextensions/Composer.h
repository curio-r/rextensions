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
		std::vector<BYTE *> m_curResults;

		std::vector<BYTE *> Expand(const std::function<std::vector<BYTE *>(class ProcMem *, BYTE *)>& fn)
		{
			std::vector<BYTE *> newResults;

			for (auto curRef : m_curResults)
			{
				auto newRefs = fn(m_mem, curRef);

				newResults.insert(newResults.end(), newRefs.begin(), newRefs.end());
			}

			return newResults;
		}

		void Propagate(const std::function<std::vector<BYTE *>(class ProcMem *, BYTE *)>& fn)
		{
			m_curResults = Expand(fn);
		}

	public:

		static const size_t DEFAULT_SEARCH_RANGE = 200;

	private:

		State& FindString(const std::string &string)
		{
			auto result = m_mem->FindDataString(string);

			if (result != nullptr)
			{
				m_curResults.push_back(result);
			}

			return *this;
		}

	public:

		State& FindReferences()
		{
			Propagate([](class ProcMem *mem, BYTE *ref)
			{
				return mem->FindAllReferences(ref);
			});

			return *this;
		}

		State& Find(const std::vector<BYTE> &bytes, SEARCH_DIRECTION direction, size_t range = DEFAULT_SEARCH_RANGE, bool wildcard = false)
		{
			Propagate([&bytes, range, direction, wildcard](class ProcMem *mem, BYTE *ref)
			{
				auto down = direction == ProcMem::SEARCH_DOWN;

				ProcMem::SEGMENT segment{ down ? ref : ref - range, down ? ref + range : ref };

				return mem->FindAllBytes(bytes, segment, wildcard, direction);
			});

			return *this;
		}

		State& FindStartOfFunction(bool requireCall = false, bool requireReference = false)
		{
			auto mem = m_mem;

			std::transform(m_curResults.begin(), m_curResults.end(), m_curResults.begin(), [mem](BYTE *curRef) -> BYTE *
			{
				SEGMENT s{ curRef - DEFAULT_SEARCH_RANGE, curRef };
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

			std::remove_if(m_curResults.begin(), m_curResults.end(), [](BYTE *curRef) -> bool
			{
				return curRef == nullptr;
			});

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

			if (requireCall)
			{
				std::remove_if(m_curResults.begin(), m_curResults.end(), [mem](BYTE *refAddr) -> bool
				{
					return mem->FindCallsTo(refAddr).size() == 0;
				});
			}

			if (requireReference)
			{
				std::remove_if(m_curResults.begin(), m_curResults.end(), [mem](BYTE *refAddr) -> bool
				{
					return mem->FindReference(refAddr) == nullptr;
				});
			}

			return *this;
		}

		State& FindCallsTo()
		{
			Propagate([](class ProcMem *mem, BYTE *ref)
			{
				return mem->FindCallsTo(ref);
			});

			return *this;
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
			if (m_curResults.size() > N)
			{
				auto nth = m_curResults[N];

				m_curResults.clear();
				m_curResults.push_back(nth);
			}

			return *this;
		}

		State& First()
		{
			return Nth(0);
		}

		State& Last()
		{
			if (m_curResults.size() > 0)
			{
				return Nth(m_curResults.size() - 1);
			}

			return *this;
		}

		size_t Count()
		{
			return m_curResults.size();
		}

		BYTE * GetMostCommon()
		{
			auto &results = m_curResults;

			if (results.size() == 0)
			{
				return nullptr;
			}

			return *std::max_element(m_curResults.begin(), m_curResults.end(), [results](BYTE *lhs, BYTE *rhs) -> bool
			{
				auto count1 = std::count_if(results.begin(), results.end(), [lhs](BYTE *addr)
				{
					return *(size_t *)lhs == *(size_t *)addr;
				});

				auto count2 = std::count_if(results.begin(), results.end(), [rhs](BYTE *addr)
				{
					return *(size_t *)rhs == *(size_t *)addr;
				});

				return count1 < count2;
			});
		}

		BYTE * GetOne()
		{
			if (m_curResults.size() == 0)
			{
				return nullptr;
			}

			return m_curResults[0];
		}

		std::vector<BYTE *> GetAll()
		{
			return m_curResults;
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

		s.m_curResults.push_back((BYTE *)offset);

		return s;
	}

	State FromCode(const std::vector<BYTE> &bytes, bool wildcard = false)
	{
		State s{ m_mem };

		s.m_curResults = m_mem->FindAllBytes(bytes, m_mem->GetSectionByName(".text"), wildcard);

		return s;
	}

	State FromCallTo(BYTE *function)
	{
		return FromOffset(function).FindCallsTo();
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
