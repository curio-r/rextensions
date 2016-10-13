#pragma once
#include <vector>

class CommonObject
{
	std::vector<char*> m_deleteList;
public:
	CommonObject();
	~CommonObject();
	void Clear();
	bool TokenFileToPCharList(std::vector<char const*> &pCharList, const char *fName, int type);
};

