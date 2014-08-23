/*************************************************************************************************\
MechIcon.h			: Interface for the MechIcon component.
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//
\*************************************************************************************************/

#pragma once

#ifndef MECHICON_H
#define MECHICON_H

#include <mclib.h>
#include <mechgui/afont.h>
#include "mc2movie.h"

//*************************************************************************************************
class Mover;
class MechWarrior;

#define NUM_DEATH_INFOS 6
/**************************************************************************************************
CLASS DESCRIPTION
MechIcon:
**************************************************************************************************/
class PilotIcon
{

public:

	static const int32_t DEAD_PILOT_INDEX;

	PilotIcon();

	static void swapResolutions(bool bForce = 0);

	void setPilot( MechWarrior* pWarrior );
	void setTextureIndex(  int32_t newIndex ) { pilotTextureIndex = newIndex; }
	void render( float left, float top, float right, float bottom );


private:

	uint32_t pilotTextureIndex;
	
	static float			pilotIconX;
	static float			pilotIconY;

	static uint32_t	s_pilotTextureHandle;
	static uint32_t	s_pilotTextureWidth;

	friend class ForceGroupIcon;
	friend class MechIcon;
	friend class Mission;
};


class ForceGroupIcon
{
public:

		ForceGroupIcon();
		virtual ~ForceGroupIcon();

		virtual void update() = 0;
		virtual void render();
		void renderUnitIcon( float left, float top, float right, float bottom );
		virtual void renderUnitIconBack( float left, float top, float right, float bottom );
		void renderPilotIcon( float left, float top, float right, float bottom );
		virtual bool			init( Mover* pMover ){ return false; }
		void init();

		


		bool inRegion( int32_t x, int32_t y );

		void click( bool shiftDn );
		void rightClick();

		virtual void setDrawBack( bool bSet) { };

		static int32_t __cdecl sort( PCVOID p1, PCVOID p2 );

		PCSTR getPilotName();

		static void init( FitIniFile& file, int32_t which );
		void swapResolutions(bool bForce);
		static void resetResolution(bool bForce);

		void setLocationIndex( int32_t i );
		void beginDeathAnimation( ){ deathAnimationTime = .0001f; } 
		bool deathAnimationOver(){ return deathAnimationTime > animationInfos[NUM_DEATH_INFOS-1].time; }
		bool isAnimatingDeath() { return deathAnimationTime ? 1 : 0; }

protected:

		static int32_t		damageColors[4][3];

		static uint32_t	s_textureHandle[5];
		static bool				s_slotUsed[240];
		static TGAFileHeader*	s_textureMemory;

		static float			unitIconX;
		static float		 	unitIconY;
		static int32_t				curScreenWidth;

		static StaticInfo*		jumpJetIcon;

		static aFont*			gosFontHandle;
		
		static gos_VERTEX	bmpLocation[17][5]; // in screen coords
		
		static RECT textArea[17];
		static RECT pilotRect[17];
		static RECT selectionRect[17];
		static RECT healthBar[17];
		static int32_t		pilotTextTop[17];

		static float	pilotLocation[17][4];
		Mover*		unit;

		static int32_t		ForceGroupColors[11];

		void drawBar( float barStatus );
		static void setIconVariables();


		friend class ForceGroupBar;
		friend void __stdcall TerminateGameEngine(void);

		bool	bDrawBack;

		int32_t		damageIconIndex;
		int32_t		backDamageIndex;
		int32_t		locationIndex;

		float	deathAnimationTime;

		struct AnimationInfo
		{
			float	time;
			int32_t	color;
		};

		static AnimationInfo animationInfos[NUM_DEATH_INFOS];

		void drawDeathEffect();

		PilotIcon	pilotIcon;

		float		msgPlayTime;

		static		MC2MoviePtr 	bMovie;
		static		uint32_t			pilotVideoTexture;
		static		MechWarrior*	pilotVideoPilot;
		
};

class MechIcon : public ForceGroupIcon
{
	public:

		MechIcon(){}
		~MechIcon(){}

		virtual bool			init( Mover* pMover );
		virtual void			update();

		static TGAFileHeader* s_MechTextures;

		void doDraw( PSTR newDamage, PSTR oldDamage, uint32_t handle, uint32_t where);
		virtual void setDrawBack( bool bSet);

		

		

	private:

		char	damage[8];
		char	backDamage[8];
		bool init( int32_t whichIndex );
		bool initTextures();

		
};

class VehicleIcon : public ForceGroupIcon
{
public:

	VehicleIcon(){}
	~VehicleIcon(){}

	virtual void renderUnitIconBack( float left, float top, float right, float bottom ){}
	virtual void	update();
	virtual bool			init( Mover* pMover );

	static TGAFileHeader* s_VehicleTextures;

private:

		char	damage[5];

};



//*************************************************************************************************
#endif  // end of file ( MechIcon.h )
