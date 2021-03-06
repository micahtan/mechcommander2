//===========================================================================//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#include "stdinc.h"

//#include "gameos.hpp"
#include "mlr/gosvertex.h"
#include "mlr/gosvertexpool.h"
#include "mlr/mlrtexture.h"
#include "mlr/mlrtexturepool.h"
#include "mlr/mlrshape.h"
#include "mlr/mlrcliptrick.h"
#include "mlr/mlr_i_det_pmesh.h"

namespace MidLevelRenderer {

//#############################################################################

extern uint32_t gEnableDetailTexture;

#if defined(TRACE_ENABLED) && defined(MLR_TRACE)
BitTrace* MLR_I_DeT_PMesh_Clip;
#endif

//#############################################################################
//###### MLRIndexedPolyMesh with no color no lighting two texture layer ######
//#############################################################################

MLR_I_DeT_PMesh::ClassData* MLR_I_DeT_PMesh::DefaultData = nullptr;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void
MLR_I_DeT_PMesh::InitializeClass()
{
	_ASSERT(!DefaultData);
	// _ASSERT(gos_GetCurrentHeap() == StaticHeap);
	DefaultData = new ClassData(MLR_I_DeT_PMeshClassID, "MidLevelRenderer::MLR_I_DeT_PMesh",
		MLR_I_PMesh::DefaultData, (MLRPrimitiveBase::Factory)&Make);
	Register_Object(DefaultData);
#if defined(TRACE_ENABLED) && defined(MLR_TRACE)
	MLR_I_DeT_PMesh_Clip = new BitTrace("MLR_I_DeT_PMesh_Clip");
	Register_Object(MLR_I_DeT_PMesh_Clip);
#endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void
MLR_I_DeT_PMesh::TerminateClass()
{
	Unregister_Object(DefaultData);
	delete DefaultData;
	DefaultData = nullptr;
#if defined(TRACE_ENABLED) && defined(MLR_TRACE)
	Unregister_Object(MLR_I_DeT_PMesh_Clip);
	delete MLR_I_DeT_PMesh_Clip;
#endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
MLR_I_DeT_PMesh::MLR_I_DeT_PMesh(ClassData* class_data, std::iostream stream, uint32_t version) :
	MLR_I_PMesh(class_data, stream, version)
{
	// Check_Pointer(this);
	Check_Pointer(stream);
	// _ASSERT(gos_GetCurrentHeap() == Heap);
	referenceState2.Load(stream, version);
	*stream >> xOffset >> yOffset >> xScale >> yScale;
	noDetailDistance = 2 * xScale;
	referenceState2.SetZBufferWriteOff();
	referenceState.SetPriority(MLRState::DefaultPriority);
	referenceState2.SetPriority(MLRState::DefaultPriority);
#if 0
	referenceState.SetPriority(MLRState::DefaultPriority + 1);
	referenceState2.SetAlphaMode(MLRState::OneZeroMode);
	referenceState.SetAlphaMode(MLRState::AlphaInvAlphaMode);
#endif
	detTextureVisible = true;
	passes = 2;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
MLR_I_DeT_PMesh::MLR_I_DeT_PMesh(ClassData* class_data) :
	MLR_I_PMesh(class_data)
{
	// Check_Pointer(this);
	// _ASSERT(gos_GetCurrentHeap() == Heap);
	noDetailDistance = -1.0f;
	detTextureVisible = true;
	passes = 2;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void
MLR_I_DeT_PMesh::Copy(
	MLR_I_PMesh*, MLRState detailState, float xOff, float yOff, float xFac, float yFac)
{
	// Check_Pointer(this);
	(void)xOff;
	(void)yOff;
	(void)xFac;
	(void)yFac;
	detTextureVisible = true;
	// STOP(("No conversion implemented yet !?"));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
MLR_I_DeT_PMesh::~MLR_I_DeT_PMesh()
{
	// Check_Object(this);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
MLR_I_DeT_PMesh*
MLR_I_DeT_PMesh::Make(std::iostream stream, uint32_t version)
{
	Check_Object(stream);
#if CONSIDERED_OBSOLETE
	gos_PushCurrentHeap(Heap);
	MLR_I_DeT_PMesh* mesh = new MLR_I_DeT_PMesh(DefaultData, stream, version);
	gos_PopCurrentHeap();
#else
	MLR_I_DeT_PMesh* mesh = new MLR_I_DeT_PMesh(DefaultData, stream, version);
#endif
	return mesh;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void
MLR_I_DeT_PMesh::Save(std::iostream stream)
{
	// Check_Object(this);
	Check_Object(stream);
	MLR_I_PMesh::Save(stream);
	referenceState2.Save(stream);
	*stream << xOffset << yOffset << xScale << yScale;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
uint32_t
MLR_I_DeT_PMesh::GetNumPasses()
{
	// Check_Object(this);
	if (gEnableDetailTexture == 0 || detTextureVisible == false)
		return 1;
	return passes;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
void
MLR_I_DeT_PMesh::TestInstance(void) const
{
	_ASSERT(IsDerivedFrom(DefaultData));
}

#define I_SAY_YES_TO_DETAIL_TEXTURES
#undef I_SAY_YES_TO_DUAL_TEXTURES
#undef I_SAY_YES_TO_COLOR
#undef I_SAY_YES_TO_LIGHTING

#define CLASSNAME MLR_I_DeT_PMesh

#if defined(TRACE_ENABLED) && defined(MLR_TRACE)
#define SET_MLR_PMESH_CLIP() MLR_I_DeT_PMesh_Clip->Set()
#define CLEAR_MLR_PMESH_CLIP() MLR_I_DeT_PMesh_Clip->Clear()
#else
#define SET_MLR_PMESH_CLIP()
#define CLEAR_MLR_PMESH_CLIP()
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This include contains follwing functions:
// void MLR_I_DeT_PMesh::TransformNoClip(Matrix4D*, GOSVertexPool*);
// int32_t MLR_I_DeT_PMesh::Clip(MLRClippingState, GOSVertexPool*);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "mlr/mlrprimitiveclipping.inl"

#undef I_SAY_YES_TO_DETAIL_TEXTURES
#undef CLASSNAME

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
MLR_I_DeT_PMesh*
MidLevelRenderer::CreateIndexedCube_Nocolour_NoLit_DetTex(
	float half, MLRState* state, MLRState* state2, float xOff, float yOff, float xFac, float yFac)
{
#ifdef _GAMEOS_HPP_
	gos_PushCurrentHeap(Heap);
#endif
	MLR_I_DeT_PMesh* ret = new MLR_I_DeT_PMesh();
	Register_Object(ret);
	ret->SetDetailData(xOff, yOff, xFac, yFac);
	Stuff::Point3D* coords = new Stuff::Point3D[8];
	Register_Object(coords);
	coords[0] = Stuff::Point3D(half, -half, half);
	coords[1] = Stuff::Point3D(-half, -half, half);
	coords[2] = Stuff::Point3D(half, -half, -half);
	coords[3] = Stuff::Point3D(-half, -half, -half);
	coords[4] = Stuff::Point3D(-half, half, half);
	coords[5] = Stuff::Point3D(half, half, half);
	coords[6] = Stuff::Point3D(half, half, -half);
	coords[7] = Stuff::Point3D(-half, half, -half);
	uint8_t* lengths = new uint8_t[6];
	Register_Pointer(lengths);
	size_t i;
	for (i = 0; i < 6; i++)
	{
		lengths[i] = 4;
	}
	ret->SetSubprimitiveLengths(lengths, 6);
	ret->SetCoordData(coords, 8);
	uint16_t* index = new uint16_t[6 * 4];
	Register_Pointer(index);
	index[0] = 0;
	index[1] = 2;
	index[2] = 6;
	index[3] = 5;
	index[4] = 0;
	index[5] = 5;
	index[6] = 4;
	index[7] = 1;
	index[8] = 5;
	index[9] = 6;
	index[10] = 7;
	index[11] = 4;
	index[12] = 2;
	index[13] = 3;
	index[14] = 7;
	index[15] = 6;
	index[16] = 1;
	index[17] = 4;
	index[18] = 7;
	index[19] = 3;
	index[20] = 0;
	index[21] = 1;
	index[22] = 3;
	index[23] = 2;
	ret->SetIndexData(index, 6 * 4);
	ret->FindFacePlanes();
	Stuff::Vector2DScalar* texCoords = new Stuff::Vector2DScalar[16];
	Register_Object(texCoords);
	// warning C6386: Buffer overrun while writing to 'texCoords'
	texCoords[0] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[1] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[2] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[3] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[4] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[5] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[6] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[7] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[8] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[9] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[10] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[11] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[12] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[13] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[14] = Stuff::Vector2DScalar(0.0f, 0.0f);
	texCoords[15] = Stuff::Vector2DScalar(0.0f, 0.0f); // ?? index numbers typo?
	if (state)
	{
		ret->SetReferenceState(*state);
		if (state->GetTextureHandle() > 0)
		{
			texCoords[0] = Stuff::Vector2DScalar(0.0f, 0.0f);
			texCoords[1] = Stuff::Vector2DScalar(1.0f, 0.0f);
			texCoords[2] = Stuff::Vector2DScalar(0.25f, 0.25f);
			texCoords[3] = Stuff::Vector2DScalar(0.75f, 0.25f);
			texCoords[4] = Stuff::Vector2DScalar(1.0f, 1.0f);
			texCoords[5] = Stuff::Vector2DScalar(0.0f, 1.0f);
			texCoords[6] = Stuff::Vector2DScalar(0.25f, 0.75f);
			texCoords[7] = Stuff::Vector2DScalar(0.75f, 0.75f);
		}
	}
	if (state && state2)
	{
		ret->SetReferenceState(*state, 1);
		if (state2->GetTextureHandle() > 0)
		{
			texCoords[8] = Stuff::Vector2DScalar(0.0f, 0.0f);
			texCoords[9] = Stuff::Vector2DScalar(1.0f, 0.0f);
			texCoords[10] = Stuff::Vector2DScalar(0.25f, 0.25f);
			texCoords[11] = Stuff::Vector2DScalar(0.75f, 0.25f);
			texCoords[12] = Stuff::Vector2DScalar(1.0f, 1.0f);
			texCoords[13] = Stuff::Vector2DScalar(0.0f, 1.0f);
			texCoords[14] = Stuff::Vector2DScalar(0.25f, 0.75f);
			texCoords[15] = Stuff::Vector2DScalar(0.75f, 0.75f);
		}
	}
	ret->SetTexCoordData(texCoords, 16);
	Unregister_Object(texCoords);
	delete[] texCoords;
	Unregister_Pointer(index);
	delete[] index;
	Unregister_Pointer(lengths);
	delete[] lengths;
	Unregister_Object(coords);
	delete[] coords;
	gos_PopCurrentHeap();
	return ret;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
MLRShape*
MidLevelRenderer::CreateIndexedIcosahedron_Nocolour_NoLit_DetTex(
	IcoInfo& icoInfo, MLRState* state, MLRState* stateDet)
{
#ifdef _GAMEOS_HPP_
	gos_PushCurrentHeap(Heap);
#endif
	MLRShape* ret = new MLRShape(20);
	Register_Object(ret);
	size_t i, j, k;
	uint32_t nrTri = static_cast<uint32_t>(ceil(icoInfo.all * pow(4.0f, icoInfo.depth)));
	Stuff::Point3D v[3];
	if (3 * nrTri >= Limits::Max_Number_Vertices_Per_Mesh)
	{
		nrTri = Limits::Max_Number_Vertices_Per_Mesh / 3;
	}
	uint8_t* lengths = new uint8_t[nrTri];
	Register_Pointer(lengths);
	for (i = 0; i < nrTri; i++)
	{
		lengths[i] = 3;
	}
	Stuff::Point3D* coords = new Stuff::Point3D[nrTri * 3];
	Register_Pointer(coords);
	Stuff::Point3D* collapsedCoords = nullptr;
	if (icoInfo.indexed == true)
	{
		collapsedCoords = new Stuff::Point3D[nrTri * 3];
		Register_Pointer(collapsedCoords);
	}
	uint16_t* index = new uint16_t[nrTri * 3];
	Register_Pointer(index);
	Stuff::Vector2DScalar* texCoords = new Stuff::Vector2DScalar[nrTri * 3];
	Register_Pointer(texCoords);
	size_t uniquePoints = 0;
	for (k = 0; k < 20; k++)
	{
		triDrawn = 0;
		MLR_I_DeT_PMesh* mesh = new MLR_I_DeT_PMesh();
		Register_Object(mesh);
		// setup vertex position information
		for (j = 0; j < 3; j++)
		{
			v[j].x = vdata[tindices[k][j]][0];
			v[j].y = vdata[tindices[k][j]][1];
			v[j].z = vdata[tindices[k][j]][2];
		}
		subdivide(coords, v[0], v[1], v[2], icoInfo.depth, nrTri, icoInfo.radius);
		mesh->SetSubprimitiveLengths(lengths, nrTri);
		if (icoInfo.indexed == true)
		{
			uniquePoints = 1;
			collapsedCoords[0] = coords[0];
			index[0] = 0;
			for (i = 1; i < nrTri * 3; i++)
			{
				for (j = 0; j < uniquePoints; j++)
				{
					if (coords[i] == collapsedCoords[j])
					{
						break;
					}
				}
				if (j == uniquePoints)
				{
					collapsedCoords[uniquePoints++] = coords[i];
				}
				index[i] = static_cast<uint16_t>(j);
			}
			mesh->SetCoordData(collapsedCoords, uniquePoints);
		}
		else
		{
			uniquePoints = nrTri * 3;
			for (i = 0; i < nrTri * 3; i++)
			{
				index[i] = static_cast<uint16_t>(i);
			}
			mesh->SetCoordData(coords, nrTri * 3);
		}
		mesh->SetIndexData(index, nrTri * 3);
		mesh->FindFacePlanes();
		if (state == nullptr)
		{
			for (i = 0; i < uniquePoints; i++)
			{
				texCoords[i] = Stuff::Vector2DScalar(0.0f, 0.0f);
			}
		}
		else
		{
			mesh->SetReferenceState(*state);
			if (state->GetTextureHandle() > 0)
			{
				if (icoInfo.indexed == true)
				{
					for (i = 0; i < uniquePoints; i++)
					{
						texCoords[i] = Stuff::Vector2DScalar((1.0f + collapsedCoords[i].x) / 2.0f,
							(1.0f + collapsedCoords[i].y) / 2.0f);
					}
				}
				else
				{
					for (i = 0; i < nrTri; i++)
					{
						texCoords[3 * i] = Stuff::Vector2DScalar(
							(1.0f + coords[3 * i].x) / 2.0f, (1.0f + coords[3 * i].y) / 2.0f);
						texCoords[3 * i + 1] =
							Stuff::Vector2DScalar((1.0f + coords[3 * i + 1].x) / 2.0f,
								(1.0f + coords[3 * i + 1].y) / 2.0f);
						texCoords[3 * i + 2] =
							Stuff::Vector2DScalar((1.0f + coords[3 * i + 2].x) / 2.0f,
								(1.0f + coords[3 * i + 2].y) / 2.0f);
					}
				}
			}
			else
			{
				for (i = 0; i < uniquePoints; i++)
				{
					texCoords[i] = Stuff::Vector2DScalar(0.0f, 0.0f);
				}
			}
		}
		mesh->SetTexCoordData(texCoords, uniquePoints);
		mesh->SetDetailData(0.0f, 0.0f, 16.0f, 16.0f);
		mesh->SetReferenceState(*stateDet, 1);
		ret->Add(mesh);
		mesh->DetachReference();
	}
	Unregister_Pointer(texCoords);
	delete[] texCoords;
	Unregister_Pointer(index);
	delete[] index;
	if (icoInfo.indexed == true)
	{
		Unregister_Pointer(collapsedCoords);
		delete[] collapsedCoords;
	}
	Unregister_Pointer(coords);
	delete[] coords;
	Unregister_Pointer(lengths);
	delete[] lengths;
	gos_PopCurrentHeap();
	return ret;
}

} // namespace MidLevelRenderer
