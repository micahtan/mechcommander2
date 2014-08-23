//===========================================================================//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#pragma once

#ifndef ABUTTON_H
#define ABUTTON_H

#include <mechgui/asystem.h>
#include <mechgui/afont.h>
#include <mechgui/aanim.h>

namespace mechgui {

	class FitIniFile;


	class aButton : public aObject
	{
	public:
		aButton();
		int32_t	init(int32_t xPos, int32_t yPos, int32_t w, int32_t h);
		void	destroy();
		virtual void	update();
		virtual void	render();

		void	init( FitIniFile& file, PCSTR headerName, HGOSFONT3D font = 0 );

		aButton& operator=( const aButton& src);
		aButton( const aButton& src);

		virtual void move( float offsetX, float offsetY );

		void	setHoldTime( float newTime ) { holdTime = newTime; }

		virtual bool		pointInside(int32_t xPos, int32_t yPos) const;



		void press( bool );
		void toggle();

		void disable( bool );
		bool isEnabled();
		bool isPressed() { return state == PRESSED; }
		void makeAmbiguous( bool bAmbiguous );
		void setSinglePress(){ singlePress = true; }
		void setMessageOnRelease() { messageOnRelease = true; }

		void hide( bool );

		int getID();
		void setID( int ID );
		void setText( int newID ) { data.textID = newID; }

		void setPressFX( int newFX ){ clickSFX = newFX; }
		void setHighlightFX( int newFX ){ highlightSFX = newFX; }
		void setDisabledFX( int newFX ){ disabledSFX = newFX; }

		enum States
		{
			ENABLED = 0,
			PRESSED,
			DISABLED,
			AMBIGUOUS,
			HIGHLIGHT,
			HIDDEN
		};




	protected:

		struct	aButtonData
		{
			int32_t			ID;
			int32_t			textID;
			int32_t			textColors[5];
			HGOSFONT3D		textFont;
			int32_t			textSize; // for ttf

			char			fileName[32];
			int32_t			stateCoords[5][2];	
			int32_t			textureWidth;
			int32_t			textureHeight;
			int				fileWidth;
			int				fileHeight;
			bool			textureRotated;
			RECT		textRect; // center text in here
			bool			outlineText; // draw empty square around text rect
			bool			outline;		// draw empty square around button
			int32_t			textAlign;
		};

		aButtonData		data;
		int				state;
		bool			toggleButton;
		bool			singlePress;
		bool			messageOnRelease;
		float			holdTime;		

		int32_t			clickSFX;
		int32_t			highlightSFX;
		int32_t			disabledSFX;


		static void makeUVs( gos_VERTEX* vertices, int State, aButtonData& data );

	private:
		void	copyData( const aButton& src );


	};

	class aAnimButton : public aButton
	{
	public:

		aAnimButton();
		void	init( FitIniFile& file, PCSTR headerName, HGOSFONT3D font = 0 );
		virtual void update();
		virtual void render();

		void destroy();

		aAnimButton& operator=( const aAnimButton& src);
		aAnimButton( const aAnimButton& src );

		void setAnimationInfo( aAnimation* normal, aAnimation* highlight,
			aAnimation* pressed, aAnimation* disabled );


		void animateChildren( bool bAnimate ){ bAnimateChildren = bAnimate; }

	private:

		aAnimation	normalData;
		aAnimation	highlightData;
		aAnimation	pressedData;
		aAnimation	disabledData;

		void update( const aAnimation& data );

		bool		animateText;
		bool		animateBmp;

		bool		bAnimateChildren;

		void copyData( const aAnimButton& src );



	};

#endif