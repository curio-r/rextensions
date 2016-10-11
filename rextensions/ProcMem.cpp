#include "stdafx.h"
#include "ProcMem.h"
#include <algorithm>

size_t ProcMem::RvaToVa(size_t rva) const
{
	return (size_t)m_baseAddress + rva;
}

ProcMem::SEGMENT ProcMem::MakeSection(size_t rva, size_t size) const
{
	auto start = (BYTE *)RvaToVa(rva);
	auto end = start + AlignSectionSize(size);

	return{ start, end };
}

size_t ProcMem::AlignSectionSize(size_t size) const
{
	auto align = m_ntHeaders->OptionalHeader.SectionAlignment;
	auto r = size % align;

	return r == 0 ? size : size + (align - r);
}

ProcMem::ProcMem(HMODULE hModule)
{
	m_baseAddress = hModule;
	m_dosHeader = (IMAGE_DOS_HEADER *)m_baseAddress;
	m_ntHeaders = (IMAGE_NT_HEADERS *)((size_t)m_dosHeader + (size_t)m_dosHeader->e_lfanew);
	m_numSections = m_ntHeaders->FileHeader.NumberOfSections;
	m_sectionHeaders = (IMAGE_SECTION_HEADER *)((size_t)m_ntHeaders +
		sizeof(m_ntHeaders->FileHeader) +
		m_ntHeaders->FileHeader.SizeOfOptionalHeader +
		sizeof(m_ntHeaders->Signature));
}

size_t ProcMem::DecodeCallAddress(BYTE * instr)
{
	instr += 1; // E8 .. .. .. ..

	return (size_t)instr + *(size_t *)instr + 4;
}

ProcMem::SEGMENT ProcMem::GetSectionByName(const std::string &name) const
{
	for (size_t i = 0; i < m_numSections; i++)
	{
		if (name == (char *)m_sectionHeaders[i].Name)
		{
			return MakeSection(m_sectionHeaders[i].VirtualAddress, m_sectionHeaders[i].Misc.VirtualSize);
		}
	}

	return{ nullptr, nullptr };
}

std::vector<BYTE *> ProcMem::FindAllBytes(const BYTE *bytes, size_t size, const SEGMENT &segment, bool wildcard, SEARCH_DIRECTION direction, std::function<bool(BYTE *)> filter)
{
	std::vector<BYTE *> results;
	BYTE *curResult;
	SEGMENT curSegment = segment;

	while ((curResult = FindBytes(bytes, size, curSegment, wildcard, direction)) != nullptr)
	{
		if (filter == nullptr || filter(curResult))
		{
			results.push_back(curResult);
		}

		if (direction == SEARCH_DOWN)
		{
			curSegment.start = std::min(curResult + 1, curSegment.end);
		}
		else
		{
			curSegment.end = curResult;
		}
	}

	return results;
}

std::vector<BYTE *> ProcMem::FindAllBytes(const std::vector<BYTE> &bytes, const SEGMENT &segment, bool wildcard, SEARCH_DIRECTION direction, std::function<bool(BYTE *)> filter)
{
	return FindAllBytes(bytes.data(), bytes.size(), segment, wildcard, direction, filter);
}

BYTE* ProcMem::FindBytes(const BYTE *bytes, size_t size, const SEGMENT &segment, bool wildcard, SEARCH_DIRECTION direction) const
{
	if (!segment.valid())
	{
		return nullptr;
	}

	auto begin = segment.start;
	auto last = std::max(segment.end - size, segment.start);
	auto cur = begin;

	if (direction == SEARCH_DOWN)
	{
		while (cur <= last)
		{
			for (size_t i = 0; i < size; i++)
			{
				if (cur[i] != bytes[i] && !(wildcard && bytes[i] == 0xCC))
				{
					break;
				}

				if (i == size - 1)
				{
					return cur;
				}
			}

			cur++;
		}
	}
	else
	{
		cur = last;

		while (cur >= begin)
		{
			for (size_t i = 0; i < size; i++)
			{
				if (cur[i] != bytes[i] && !(wildcard && bytes[i] == 0xCC))
				{
					break;
				}

				if (i == size - 1)
				{
					return cur;
				}
			}

			cur--;
		}
	}

	return nullptr;
}

BYTE* ProcMem::FindBytes(const std::vector<BYTE> &bytes, const SEGMENT &segment, bool wildcard, SEARCH_DIRECTION direction) const
{
	return FindBytes(bytes.data(), bytes.size(), segment, wildcard, direction);
}

BYTE * ProcMem::GetImportTableAddress(const char * moduleName, const char *procName)
{
	auto importDescriptor = (IMAGE_IMPORT_DESCRIPTOR *)RvaToVa(m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (importDescriptor->Name != 0)
	{
		char *importModuleName = (char *)RvaToVa(importDescriptor->Name);

		auto thunkData = (IMAGE_THUNK_DATA *)RvaToVa(importDescriptor->FirstThunk);
		auto importsByName = (IMAGE_IMPORT_BY_NAME **)RvaToVa(importDescriptor->Characteristics);

		if (_strcmpi(moduleName, importModuleName) == 0)
		{
			while (*importsByName != nullptr)
			{
				auto importProcName = (char *)(((IMAGE_IMPORT_BY_NAME *)RvaToVa((size_t)*importsByName))->Name);
				auto importAddress = (BYTE *)thunkData;

				if (_strcmpi(procName, importProcName) == 0)
				{
					return importAddress;
				}

				thunkData++;
				importsByName++;
			}
		}

		importDescriptor++;
	}

	return nullptr;
}

BYTE * ProcMem::FindReference(BYTE *refAddress, SEGMENT segment, SEARCH_DIRECTION direction)
{
	return FindBytes((BYTE *)&refAddress, sizeof(BYTE *), segment, false, direction);
}

std::vector<BYTE *> ProcMem::FindAllReferences(BYTE *refAddress, SEGMENT segment, SEARCH_DIRECTION direction)
{
	return FindAllBytes((BYTE *)&refAddress, sizeof(BYTE *), segment, false, direction);
}

std::vector<BYTE *> ProcMem::FindCallsTo(BYTE *procAddress, SEGMENT segment, SEARCH_DIRECTION direction)
{
	return FindAllBytes({ 0xE8 }, segment, false, direction, [procAddress](BYTE *refAddress)
	{
		return ProcMem::DecodeCallAddress(refAddress) == (size_t)procAddress;
	});
}

BYTE * ProcMem::FindDataString(const std::string &string)
{
	std::vector<BYTE> bytes(string.begin(), string.end());
	BYTE *result;

	result = FindBytes(bytes, GetSectionByName(".rdata"));

	if (result != nullptr)
	{
		return result;
	}

	return FindBytes(bytes, GetSectionByName(".data"));
}
