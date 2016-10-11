#pragma once
#include "win32.h"
#include <vector>
#include <functional>

/* Helper class for searching process memory */

class ProcMem
{
protected:

	void *m_baseAddress;
	IMAGE_DOS_HEADER *m_dosHeader;
	IMAGE_NT_HEADERS *m_ntHeaders;
	IMAGE_SECTION_HEADER *m_sectionHeaders;
	size_t m_numSections;

public:

	struct SEGMENT
	{
		BYTE *start;
		BYTE *end;

		inline SEGMENT(BYTE *_Start, BYTE *_End) :
			start(_Start),
			end(_End)
		{
		}

		inline bool valid() const
		{
			return start != nullptr &&
				end != nullptr &&
				start < end;
		}
	};

	enum SEARCH_DIRECTION
	{
		SEARCH_DOWN,
		SEARCH_UP
	};

protected:

	size_t RvaToVa(size_t rva) const;
	size_t AlignSectionSize(size_t size) const;
	SEGMENT MakeSection(size_t rva, size_t size) const;

public:

	ProcMem(HMODULE hModule);

	/*
	 * Decode virtual address from call instruction
	 */
	static size_t DecodeCallAddress(BYTE * instr);

	/*
	 * Get memory segment of PE section
	 */
	SEGMENT GetSectionByName(const std::string &name) const;
	
	/*
	 * Find all instances of byte pattern
	 */
	std::vector<BYTE *> FindAllBytes(const BYTE *bytes, size_t size, const SEGMENT &segment, bool wildcard = false, SEARCH_DIRECTION direction = SEARCH_DOWN, std::function<bool(BYTE *)> filter = nullptr);

	/*
	 * Find all instances of byte pattern
	 */
	std::vector<BYTE *> FindAllBytes(const std::vector<BYTE> &bytes, const SEGMENT &segment, bool wildcard = false, SEARCH_DIRECTION direction = SEARCH_DOWN, std::function<bool(BYTE *)> filter = nullptr);
	
	/*
	 * Find first occurence of byte pattern
	 */
	BYTE* FindBytes(const BYTE *bytes, size_t size, const SEGMENT &segment, bool wildcard = false, SEARCH_DIRECTION direction = SEARCH_DOWN) const;

	/*
	 * Find first occurence of byte pattern
	 */
	BYTE* FindBytes(const std::vector<BYTE> &bytes, const SEGMENT &segment, bool wildcard = false, SEARCH_DIRECTION direction = SEARCH_DOWN) const;

	/*
	 * Get address of function pointer in the import address table (.idata)
	 */
	BYTE * GetImportTableAddress(const char * moduleName, const char *procName);

	/*
	 * Find first occurence of reference to pointer
	 * Equivalent to performing a search for the LE byte pattern of the address
	 */
	BYTE * FindReference(BYTE *refAddress, SEGMENT segment, SEARCH_DIRECTION direction = SEARCH_DOWN);

	/*
	 * Find all references to pointer
	 * Equivalent to performing a search for the LE byte pattern of the address
	 */
	std::vector<BYTE *> FindAllReferences(BYTE *refAddress, SEGMENT segment, SEARCH_DIRECTION direction = SEARCH_DOWN);

	/*
	 * Find all occurences of function calls to a given address
	 */
	std::vector<BYTE *> FindCallsTo(BYTE *procAddress, SEGMENT segment, SEARCH_DIRECTION direction = SEARCH_DOWN);

	/*
	 * Find string in data segments
	 */
	BYTE * FindDataString(const std::string &string);
};
