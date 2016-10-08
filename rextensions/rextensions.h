// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the REXTENSIONS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// REXTENSIONS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef REXTENSIONS_EXPORTS
#define REXTENSIONS_API __declspec(dllexport)
#else
#define REXTENSIONS_API __declspec(dllimport)
#endif

// This class is exported from the rextensions.dll
class REXTENSIONS_API Crextensions {
public:
	Crextensions(void);
	// TODO: add your methods here.
};

extern REXTENSIONS_API int nrextensions;

REXTENSIONS_API int fnrextensions(void);
