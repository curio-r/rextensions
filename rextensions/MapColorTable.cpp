#include "stdafx.h"
#include "MapColorTable.h"
#include "CommonObject.h"
#include <algorithm>
#include <vector>

typedef std::pair<char const *, size_t> MapColorInfo;

static std::vector<MapColorInfo> s_mapColorTable{};
static CommonObject s_obj{};

static size_t aToDW_HEX(char const* hexNumber)
{
	size_t color = 0;

	auto len = strlen(hexNumber);
	int i = 0;

	for (auto c = &hexNumber[len - 1]; c >= hexNumber + 2; c--, i++)
	{
		switch (*c)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				color |= (*c - '0') << (4 * i);
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				color |= (*c - 'a' + 10) << (4 * i);
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				color |= (*c - 'A' + 10) << (4 * i);
				break;
		}
	}

	return color;
}

bool InitMapColorTable()
{
	std::vector<char const*> pCharList;

	pCharList.reserve(50);

	auto result = s_obj.TokenFileToPCharList(pCharList, "mapcolortable.txt", 0);

	for (auto it = pCharList.begin(); it != pCharList.end(); it += 2)
	{
		s_mapColorTable.push_back({ it[0], (size_t)aToDW_HEX(it[1]) });
	}

	std::sort(
		s_mapColorTable.begin(),
		s_mapColorTable.end(),
		[](const MapColorInfo& a, const MapColorInfo& b)
		{
			return strcmp(a.first, b.first) < 0;
		}
	);

	return result;
}

bool GetMapColor(char const *mapName, size_t &color)
{
	color = 0x00000000u;

	auto it = std::lower_bound(
		s_mapColorTable.begin(),
		s_mapColorTable.end(),
		MapColorInfo (mapName, 0x00000000u),
		[](const MapColorInfo &a, const MapColorInfo &b)
		{
			return strcmp(a.first, b.first) < 0;
		}
	);

	if (it == s_mapColorTable.end() || strcmp(it->first, mapName) != 0)
	{
		return false;
	}
	else
	{
		color = it->second;

		return true;
	}
}
