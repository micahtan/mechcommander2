/*************************************************************************************************\
MPConnectionType.h			: Interface for the MPConnectionType component.
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//
\*************************************************************************************************/

#pragma once

#ifndef MPCONNECTIONTYPE_H
#define MPCONNECTIONTYPE_H

//#include <mechgui/logisticsscreen.h>
//#include <mechgui/asystem.h>
//#include <mechgui/abutton.h>
//#include <mechgui/alistbox.h>
//#include <mechgui/aanim.h>
//#include "mphostgame.h"

typedef enum __mpconnectiontype_const {
	ZONE_PANEL_FIRST_BUTTON_ID		= 1000100,
	LAN_PANEL_FIRST_BUTTON_ID		= 1000200,
	TCPIP_PANEL_FIRST_BUTTON_ID		= 1000300,
}; 

class aZonePanel : public aObject
{
public:
	void init(FitIniFile* pFile, LogisticsScreen* pParent);
	virtual int32_t			handleMessage( uint32_t, uint32_t );
	virtual void		update();
	virtual void		render();

private:
	LogisticsScreen *pParentScreen;

	aAnimButton button;
	aText text;

	bool	bShowWarning;
};

class aLanPanel : public aObject
{
public:
	aLanPanel(LogisticsScreen &refParentScreenParam)
	{
		pParentScreen = &refParentScreenParam;
	}
	void init(FitIniFile* pFile);
	virtual int32_t			handleMessage( uint32_t, uint32_t );
	virtual void update();
private:
	LogisticsScreen *pParentScreen;
	aAnimButton button0;
	aAnimButton button1;
	aText text;
};



class aTcpipPanel : public aObject
{
public:
	aTcpipPanel(LogisticsScreen &refParentScreenParam)
	{
		pParentScreen = &refParentScreenParam;
		connectingTime = 0.f;
	}
	void init(FitIniFile* pFile);
	virtual void destroy();
	virtual int32_t			handleMessage( uint32_t, uint32_t );
	virtual void		update();
	virtual void		render();
	virtual void		begin();
private:

	int32_t		getNum( PSTR pStr, int32_t index1, int32_t index2 );
	LogisticsScreen *pParentScreen;
	aAnimButton button0;
	aAnimButton button1;
	aText text0;
	aText text1;
	aRect helpRect;
	aComboBox comboBox;
	bool	bConnectingDlg;
	float	connectingTime;
	bool	bErrorDlg;
	bool			bExpanded;
	bool	bFoundConnection;

};

class MPConnectionType : public LogisticsScreen
{
public:
	
	MPConnectionType();
	virtual ~MPConnectionType();
	
	void init(FitIniFile* file);
	bool isDone();
	virtual void		begin();
	virtual void		end();
 	virtual void render( int32_t xOffset, int32_t yOffset );
	virtual void render();
	virtual void update();
	virtual int32_t			handleMessage( uint32_t, uint32_t );

	bool				bDone;

	PVOID*	ppConnectionScreen;
	PVOID	pLocalBrowserScreen;
	PVOID	pDirectTcpipScreen;
	PVOID	pMPPlaceHolderScreen;
	PVOID	pMPHostGame;

private:
	int32_t indexOfButtonWithID(int32_t id);

	aZonePanel zonePanel;
	aLanPanel lanPanel;
	aTcpipPanel tcpipPanel;
	aObject *pPanel;

	MPHostGame	hostDlg;

	bool		bHosting;
};

class MPPlaceHolderScreen : public LogisticsScreen
{
public:
	
	//MPPlaceHolderScreen();
	//virtual ~MPPlaceHolderScreen();
	
	virtual void		begin() { status = RUNNING; }
	//virtual void		end();
 	virtual void render( int32_t xOffset, int32_t yOffset ) {
		static int32_t lastXOffset = 0;
		static int32_t lastYOffset = 0;
		if ((0 == xOffset) && (0 == yOffset)) {
			if (xOffset < lastXOffset) {
				status = NEXT;
			} else if (xOffset > lastXOffset) {
				status = PREVIOUS;
			} else if (yOffset > lastYOffset) {
				status = UP;
			} else if (yOffset < lastYOffset) {
				status = DOWN;
			} else {
				assert(false);
				status = NEXT;
			}
		}
		lastXOffset = xOffset;
		lastYOffset = yOffset;
	}
	virtual void render() { render(0, 0); }
	//virtual void update();

private:
};



//*************************************************************************************************
#endif  // end of file ( MPConnectionType.h )
