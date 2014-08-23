#define DROPZONEBRUSH_CPP
/*************************************************************************************************\
dropZoneBrush.cpp			: Implementation of the dropZoneBrush component.
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//
\*************************************************************************************************/

#include "dropZoneBrush.h"
#include "editorobjectmgr.h"

DropZoneBrush::DropZoneBrush(int32_t align, bool bVtol)
{
	alignment = align;
	bVTol = bVtol;
}

bool DropZoneBrush::paint( Stuff::Vector3D& worldPos, int32_t screenX, int32_t screenY )
{
	EditorObject* pInfo = EditorObjectMgr::instance()->addDropZone( worldPos, alignment, bVTol );
	if ( pInfo && pAction )
	{
		pAction->addBuildingInfo( *pInfo );
		return true;
	}

	return false;
}

bool DropZoneBrush::canPaint( Stuff::Vector3D& worldPos, int32_t screenX, int32_t screenY, int32_t flags )
{
	return EditorObjectMgr::instance()->canAddDropZone( worldPos, alignment, bVTol );
}
//*************************************************************************************************
// end of file ( dropZoneBrush.cpp )