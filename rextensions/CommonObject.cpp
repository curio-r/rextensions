#include "stdafx.h"
#include "CommonObject.h"
#include "ProxyImpl.h"

CommonObject::CommonObject()
{
	Clear();
}

CommonObject::~CommonObject()
{
	Clear();
}

void CommonObject::Clear()
{
	for (auto i : m_deleteList)
	{
		delete[] i;
	}

	m_deleteList.clear();
}

bool CommonObject::TokenFileToPCharList(std::vector<char const*> &pCharList, const char *fName, int type)
{
	CFile fp;
	
	pCharList.clear();
	pCharList.reserve(4096);

	const char* name = fName;
	
	if (!proxy(CFile)::Open(&fp, name, 0))
	{
		return false;
	}

	auto size = fp.m_size;
	char *buf = new char[fp.m_size + 1];

	fp.Read(buf, size);
	
	if (proxy(CFile)::Close.IsCallable())
	{
		proxy(CFile)::Close(&fp);
	}

	buf[fp.m_size] = 0;

	char* cur = buf;

	for (unsigned i = 0; i < size; i++)
	{
		auto c = buf[i];

		if (c == '\n')
		{
			cur++;
			continue;
		}

		if (c == '\r')
		{
			buf[i] = 0;

			if (type != 0 && strncmp(cur, "//", 2u) != 0 && cur[0] != 0)
			{
				pCharList.push_back(cur);
			}

			cur = &buf[i + 1];
			continue;
		}
		
		if (c == '#')
		{
			buf[i] = 0;
			pCharList.push_back(cur);

			if (type == 2)
			{
				pCharList.push_back("");
			}

			cur = &buf[i + 1];
		}
	}

	m_deleteList.push_back(buf);
	
	return true;
}
