//===========================================================================//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//
//***************************************************************************
//
//								ENVIRON.H
//
//***************************************************************************

#ifndef ABLENV_H
#define ABLENV_H

#include <stdio.h>

#ifndef DABLENV_H
#include "dablenv.h"
#endif

#ifndef ABLGEN_H
#include "ablgen.h"
#endif

#ifndef ABLSYMT_H
#include "ablsymt.h"
#endif

#ifndef ABLEXEC_H
#include "ablexec.h"
#endif

#ifndef DABLDBUG_H
#include "dabldbug.h"
#endif

#ifndef ABLSCAN_H
#include "ablscan.h"
#endif

//***************************************************************************

typedef struct _SourceFile {
	char					fileName[MAXLEN_FILENAME];
	uint8_t			fileNumber;
	ABLFile*				filePtr;
	long					lineNumber;
} SourceFile;

//---------------------------------------------------------------------------

#define	MAX_USER_FILES			6
#define MAX_USER_FILE_LINES		50
#define	MAX_USER_FILE_LINELEN	200

class UserFile {

	public:

		long					handle;
		bool					inUse;
		char					fileName[MAXLEN_FILENAME];
		ABLFile*				filePtr;
		long					numLines;
		long					totalLines;
		char					lines[MAX_USER_FILE_LINES][MAX_USER_FILE_LINELEN];

		static UserFilePtr		files[MAX_USER_FILES];

	public:

		void* operator new (size_t mySize);

		void operator delete (void* us);
			
		void init (void) {
			handle = -1;
			inUse = false;
			fileName[0] = NULL;
			filePtr = NULL;
			numLines = 0;
			totalLines = 0;
			for (long i = 0; i < MAX_USER_FILE_LINES; i++)
				lines[i][0] = NULL;
		}

		UserFile (void) {
			init();
		}

		void destroy (void);

		~UserFile (void) {
			destroy();
		}

		void dump (void);

		void close (void);

		long open (PSTR fileName);

		void write (PSTR s);

		static void setup (void);

		static void cleanup (void);

		static UserFilePtr getNewFile (void);
};

//---------------------------------------------------------------------------

#define	MAX_ABLMODULE_NAME				5
#define	MAX_SOURCE_FILES				256			// per module
#define	MAX_LIBRARIES_USED				25			// per module
#define	MAX_STATE_HANDLES_PER_MODULE	10

typedef struct {
	char					name[128];
	long					size;
} VariableInfo;

typedef struct {
	char					name[128];
	long					codeSegmentSize;
} RoutineInfo;

typedef struct {
	char					name[128];
	char					fileName[128];
	long					numStaticVars;
	long					totalSizeStaticVars;
	VariableInfo			largestStaticVar;
	long					totalCodeSegmentSize;
	long					numRoutines;
	RoutineInfo				routineInfo[128];
} ModuleInfo;

typedef struct {
	PSTR					fileName;
	SymTableNodePtr			moduleIdPtr;
	long					numSourceFiles;
	PSTR*					sourceFiles;
	long					numLibrariesUsed;
	ABLModulePtr*			librariesUsed;
	long					numStaticVars;
	long					numOrderCalls;
	long					numStateHandles;
	StateHandleInfoPtr		stateHandles;
	long*					sizeStaticVars;
	long					totalSizeStaticVars;
	long					numInstances;
} ModuleEntry;

typedef ModuleEntry* ModuleEntryPtr;

class ABLModule {

	private:

		long					id;
		char					name[MAX_ABLMODULE_NAME];
		long					handle;
		StackItemPtr			staticData;
		size_t*			orderCallFlags;
		StackItem				returnVal;
		bool					initCalled;
		SymTableNodePtr			prevState;
		SymTableNodePtr			state;
		WatchManagerPtr			watchManager;
		BreakPointManagerPtr	breakPointManager;
		bool					trace;
		bool					step;
		bool					traceEntry;
		bool					traceExit;

		//static long				numModules;

	public:

		void* operator new (size_t mySize);
		void operator delete (void* us);
			
		void init (void) {
			id = -1;
			name[0] = NULL;
			handle = -1;
			staticData = NULL;
			returnVal.integer = 0;
			initCalled = false;
			prevState = NULL;
			state = NULL;
			watchManager = NULL;
			breakPointManager = NULL;
			trace = false;
			step = false;
			traceEntry = false;
			traceExit = false;
		}


		ABLModule (void) {
			init();
		}

		long init (long moduleHandle);
		
		void write (ABLFile* moduleFile);
		
		void read (ABLFile* moduleFile);

		long getId (void) {
			return(id);
		}

		long getRealId (void);

		long getHandle (void) {
			return(handle);
		}

		StackItemPtr getStaticData (void) {
			return(staticData);
		}

		void setInitCalled (bool called) {
			initCalled = called;
		}

		bool getInitCalled (void) {
			return(initCalled);
		}

		PSTR getFileName (void);

		PSTR getName (void) {
			return(name);
		}

		void setName (PSTR _name);

		size_t* getOrderCallFlags (void) {
			return(orderCallFlags);
		}

		void setPrevState (SymTableNodePtr stateSym) {
			prevState = stateSym;
		}

		SymTableNodePtr getPrevState (void) {
			return(prevState);
		}

		long getPrevStateHandle (void);

		void setState (SymTableNodePtr stateSym) {
			state = stateSym;
		}

		SymTableNodePtr getState (void) {
			return(state);
		}

		long getStateHandle (void);

		bool isLibrary (void);

		void resetOrderCallFlags (void);

		void setOrderCallFlag (uint8_t dword, uint8_t bit);

		void clearOrderCallFlag (uint8_t orderDWord, uint8_t orderBitMask);

		bool getOrderCallFlag (uint8_t dword, uint8_t bit) {
			return((orderCallFlags[dword] & (1 << bit)) != 0);
		}

		WatchManagerPtr getWatchManager (void) {
			return(watchManager);
		}

		BreakPointManagerPtr getBreakPointManager (void) {
			return(breakPointManager);
		}

		void setTrace (bool _trace) {
			trace = _trace;
			traceEntry = _trace;
			traceExit = _trace;
		}

		bool getTrace (void) {
			return(trace);
		}

		void setStep (bool _step) {
			step = _step;
		}

		bool getStep (void) {
			return(step);
		}

		long execute (ABLParamPtr paramList = NULL);
		long execute (ABLParamPtr moduleParamList, SymTableNodePtr functionIdPtr);

		SymTableNodePtr findSymbol (PSTR symbolName, SymTableNodePtr curFunction = NULL, bool searchLibraries = false);

		SymTableNodePtr findFunction (PSTR functionName, bool searchLibraries = false);

		SymTableNodePtr findState (PSTR stateName);

		long findStateHandle (PSTR stateName);

		PSTR getSourceFile (long fileNumber);

		PSTR getSourceDirectory (long fileNumber, PSTR directory);

		void getInfo (ModuleInfo* moduleInfo);

		float getReal (void) {
			return(returnVal.real);
		}
		
		long getInteger (void) {
			return(returnVal.integer);
		}

		long setStaticInteger (PSTR name, long value);

		long getStaticInteger (PSTR name);

		long setStaticReal (PSTR name, float value);
		
		float getStaticReal (PSTR name);

		long setStaticIntegerArray (PSTR name, long size, long* values);

		long getStaticIntegerArray (PSTR name, long size, long* values);
		
		long setStaticRealArray (PSTR name, long size, float* values);

		long getStaticRealArray (PSTR name, long size, float* values);

		void destroy (void);

		~ABLModule (void) {

			destroy();
		}

};

//*************************************************************************

void initModuleRegistry (long maxModules);
void destroyModuleRegistry (void);
void initLibraryRegistry (long maxLibraries);
void destroyLibraryRegistry (void);

//***************************************************************************

#endif
