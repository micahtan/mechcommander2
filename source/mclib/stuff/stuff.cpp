//===========================================================================//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#include "stdafx.h"
#include "stuffheaders.hpp"
#include "eh.h"
#include "trace.cpp"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void Stuff::Flood_Memory_With_NAN(
	void *where,
	size_t how_much)
{
	Verify(!(reinterpret_cast<int>(where)&3));
	Check_Pointer(where);
	long *filler = Cast_Pointer(long *, where);
	for (size_t i = how_much >> 2; i; --i)
	{
		*filler++ = SNAN_NEGATIVE_LONG;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void
	__cdecl Terminate_Handler(void)
{
	STOP(("Unhandled exception"));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
static UCHAR __stdcall Check_0(void) {return UCHAR((ArmorLevel == 0) ? 1 : 0);}
static UCHAR __stdcall Check_1(void) {return UCHAR((ArmorLevel == 1) ? 1 : 0);}
static UCHAR __stdcall Check_2(void) {return UCHAR((ArmorLevel == 2) ? 1 : 0);}
static UCHAR __stdcall Check_3(void) {return UCHAR((ArmorLevel == 3) ? 1 : 0);}
static UCHAR __stdcall Check_4(void) {return UCHAR((ArmorLevel == 4) ? 1 : 0);}

static void __stdcall Activate_0(void) {ArmorLevel = 0;}
static void __stdcall Activate_1(void) {ArmorLevel = 1;}
static void __stdcall Activate_2(void) {ArmorLevel = 2;}
static void __stdcall Activate_3(void) {ArmorLevel = 3;}
static void __stdcall Activate_4(void) {ArmorLevel = 4;}

static UCHAR __stdcall Greyed(void)
{
	#if defined(_ARMOR)
		return UCHAR(0);
	#else
		return UCHAR(1);
	#endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
static UCHAR __stdcall Check_4hz() {return UCHAR((Environment.MaxTimeDelta == 0.25f) ? 1 : 0);}
static UCHAR __stdcall Check_40hz() {return UCHAR((Environment.MaxTimeDelta == 0.025f) ? 1 : 0);}
static UCHAR __stdcall Check_400hz() {return UCHAR((Environment.MaxTimeDelta == 0.0025f) ? 1 : 0);}

static void __stdcall Activate_4hz() {Environment.MaxTimeDelta = Environment.MinimumTimeDelta = 0.25f;}
static void __stdcall Activate_40hz() {Environment.MaxTimeDelta = Environment.MinimumTimeDelta = 0.025f;}
static void __stdcall Activate_400hz() {Environment.MaxTimeDelta = Environment.MinimumTimeDelta = 0.0025f;}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void Stuff::InitializeClasses(void)
{
	set_terminate(Terminate_Handler);

	//
	//-------------------------------------------------
	// First, Initialize all the non registered classes
	//-------------------------------------------------
	//
	Verify(FirstFreeStuffClassID <= LastStuffClassID);
	Verify(!FileStreamManager::Instance);
	FileStreamManager::Instance = new FileStreamManager;
	RegisteredClass::InitializeClass();
	Plug::InitializeClass();
	Node::InitializeClass();

	SlotLink::InitializeClass();
	ChainLink::InitializeClass();
	SafeChainLink::InitializeClass();

	Random::InitializeClass();

	//
	//----------------------------------------------
	// Now, initialize all of the registered classes
	//----------------------------------------------
	//
	MemoryStream::InitializeClass();
	FileStream::InitializeClass();
	#if defined(TRACE_ENABLED)
		TraceManager::InitializeClass();
	#endif


	//
	//------------------------------------
	// Build the quatenion fast lerp table
	//------------------------------------
	//
	

	UnitQuaternion::InitializeClass();

	//
	//-------------------
	// Add the armor menu
	//-------------------
	//
	AddDebuggerMenuItem("Libraries\\Stuff\\Armor Level 0", Check_0, Activate_0, Greyed );
	AddDebuggerMenuItem("Libraries\\Stuff\\Armor Level 1", Check_1, Activate_1, Greyed );
	AddDebuggerMenuItem("Libraries\\Stuff\\Armor Level 2", Check_2, Activate_2, Greyed );
	AddDebuggerMenuItem("Libraries\\Stuff\\Armor Level 3", Check_3, Activate_3, Greyed );
	AddDebuggerMenuItem("Libraries\\Stuff\\Armor Level 4", Check_4, Activate_4, Greyed );

	AddDebuggerMenuItem("Libraries\\Stuff\\4hz min", Check_4hz, Activate_4hz, NULL );
	AddDebuggerMenuItem("Libraries\\Stuff\\40hz min", Check_40hz, Activate_40hz, NULL );
	AddDebuggerMenuItem("Libraries\\Stuff\\400hz min", Check_400hz, Activate_400hz, NULL );
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void Stuff::TerminateClasses(void)
{
	//
	//-----------------------------------------------
	// First, terminate all of the registered classes
	//-----------------------------------------------
	//
	if ( !FileStream::DefaultData ) // yet again, nobody every checks for NULL pointers
		return;

	UnitQuaternion::TerminateClass();

	#if defined(TRACE_ENABLED)
		TraceManager::TerminateClass();
	#endif
	FileStream::TerminateClass();
	MemoryStream::TerminateClass();

	//
	//-----------------------------------------------
	// Then, terminate all the non registered classes
	//-----------------------------------------------
	//
	Random::TerminateClass();

	SafeChainLink::TerminateClass();
	ChainLink::TerminateClass();
	SlotLink::TerminateClass();

	Node::TerminateClass();
	Plug::TerminateClass();
	RegisteredClass::TerminateClass();
	Check_Pointer(FileStreamManager::Instance);
	delete FileStreamManager::Instance;
	FileStreamManager::Instance = NULL;
}
