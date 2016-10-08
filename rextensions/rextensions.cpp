// rextensions.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "rextensions.h"


// This is an example of an exported variable
REXTENSIONS_API int nrextensions=0;

// This is an example of an exported function.
REXTENSIONS_API int fnrextensions(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see rextensions.h for the class definition
Crextensions::Crextensions()
{
	return;
}
