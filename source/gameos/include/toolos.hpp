#pragma once
#include "GameOS.hpp"
//===========================================================================//
// File:	 ToolOS.hpp														 //
// Contents: External API not for Game use, but for tools					 //
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//



//
// Returns the highest res timer possible (in seconds from start of game)
//
double __stdcall gos_GetHiResTime();



//
// Creates a 'worker thread' - this API returns a handle to the thread.
//
// You must pass the address of the worker thread routine to this function.
//
DWORD __stdcall gos_CreateThread( void (__stdcall *ThreadRoutine)(void*) );

//
// Waits until the thread is not executing anymore and then deletes the thread
//
void __stdcall gos_DeleteThread( DWORD ThreadHandle );


enum gosThreadPriority
{ 
	ThreadPri_Lowest=1,
	ThreadPri_BelowNormal=2,
	ThreadPri_Normal=3,
	ThreadPri_AboveNormal=4,
	ThreadPri_Highest=5
};
//
// Set the thread priority
//
void __stdcall gos_SetThreadPriority( DWORD ThreadHandle, gosThreadPriority Priority );

//
// Starts the created thread executing
//
// 'ThreadFinished' will be set when the thread has finished executing
// 'Context' will be passed to the thread function
//
void __stdcall gos_TriggerThread( DWORD ThreadHandle, bool* ThreadFinished, void* Context ); 



//
//
// LZ Compression - returns length of compressed destination buffer
//
//
DWORD __stdcall gos_LZCompress( PUCHAR dest, PUCHAR src, DWORD srcLen, DWORD destLen=0 );
//
//
// LZ Decompression routine, returns length to decompressed output in dest buffer
//
//
DWORD __stdcall gos_LZDecompress( PUCHAR dest, PUCHAR src, DWORD srcLen );



//
// Gets a pointer to text data in the windows clip board (NULL=No text)
//
DWORD __stdcall gos_GetClipboardText( PSTR Buffer, DWORD BufferSize );

//
// Sets the windows clipboard to the current text string
//
void __stdcall gos_SetClipboardText( PSTR Text );



//
// Gets a list of valid drives, returned as a list of strings, ending in 0,0
//
// eg:  "a:\","c:\","d:\","",""
//
// Returns the length of the string
//
DWORD __stdcall gos_GetValidDrives( PSTR Buffer, DWORD buf_len );

//
// Returns the drive label for a root directory specified. eg:  "c:\" might return "DriveC"
//
bool __stdcall gos_GetDriveLabel( PCSTR RootName, PSTR DriveLabel, DWORD DriveLabelBufferLen );

//
// Get the space available on the drive specified, either "x:" or "x:\"
//
__int64 __stdcall gos_GetDriveFreeSpace( PSTR Path );


//
// Gets the current path
//
void __stdcall gos_GetCurrentPath( PSTR Buffer,int buf_len );


//
// Create directory. - Returns TRUE for sucess
//
bool __stdcall gos_CreateDirectory( PCSTR FileName );
//
// Delete directory. - Returns TRUE for sucess  (Directories must be empty)
//
bool __stdcall gos_DeleteDirectory( PCSTR DirectoryName );

//
// Rename file/directory. - Returns TRUE for sucess
//
bool __stdcall gos_RenameFile( PCSTR FileNameFrom, PCSTR FileNameTo );
//
// Delete file. - Returns TRUE for sucess
//
bool __stdcall gos_DeleteFile( PCSTR FileName );

//
// Find files matching pattern - returns NULL when no more files (Can be used too see if a directory is empty : ie: 0==gos_FindFiles( "\\graphics\\*.*" )
//
PSTR __stdcall gos_FindFiles( PCSTR PathFileName );
//
// Continues the previous gos_FindFiles
//
PSTR __stdcall gos_FindFilesNext();
//
// Stop the previous gos_FindFiles
//
void __stdcall gos_FindFilesClose();

//
// Find directories matching pattern - returns NULL when no more directories
//
PSTR __stdcall gos_FindDirectories( PCSTR DirectoryName );
//
// Continues the previous gos_FindDirectoriesNext
//
PSTR __stdcall gos_FindDirectoriesNext();
//
// Stop the previous gos_FindDirectories
//
void __stdcall gos_FindDirectoriesClose();

//
// Return full path name of file - FullPath should point to a 256 byte buffer
//
void __stdcall gos_GetFullPathName( PSTR FullPath, PCSTR FileName );

//
// Get file size information (-1 if error)
//
DWORD __stdcall gos_FileSize( PCSTR FileName );
//
// Get file date/time information (-1 if error) - this can be compared directly, and decoded using gos_FileTimeString
//
__int64 __stdcall gos_FileTimeStamp( PCSTR FileName );
//
// Get current date/time information (only updates one per game logic)
//
__int64 __stdcall gos_GetTimeDate();
//
// Returns an ASCII time/date string from a file time
//
PSTR __stdcall gos_FileTimeString( __int64 Time );

//
// Get file read only attribute information
//
bool __stdcall gos_FileReadOnly( PCSTR FileName );
//
// Set file to read/write
//
void __stdcall gos_FileSetReadWrite( PCSTR FileName );
//
// Set file to read only
//
void __stdcall gos_FileSetReadOnly( PCSTR FileName );











//////////////////////////////////////////////////////////////////////////////////
// Ways to specify how to open file.
typedef enum gosEnum_FileWriteStatus 
{
	READONLY,				// Open for READ
	READWRITE,				// Open for READ and WRITE - but clears the file on open
	APPEND					// Open for READ and WRITE - leaves the file alone if it already exists
};

//////////////////////////////////////////////////////////////////////////////////
// Different ways to seek within a file.
typedef enum gosEnum_FileSeekType 
{
	FROMSTART,
	FROMCURRENT,
	FROMEND
};

//////////////////////////////////////////////////////////////////////////////////
// If the file or directory exists, return TRUE
bool __stdcall gos_DoesFileExist( PCSTR FileName );

//////////////////////////////////////////////////////////////////////////////////
// Open the file found at <path> with the method <writeable> and return a handle
// hfile to it.
void __stdcall gos_OpenFile( HGOSFILE* hfile, PCSTR path, gosEnum_FileWriteStatus );

//////////////////////////////////////////////////////////////////////////////////
// Close the file specified by handle <hfile>.
void __stdcall gos_CloseFile( HGOSFILE hfile );

//////////////////////////////////////////////////////////////////////////////////
// Read <size> bytes from the file specified by handle <hfile> into the buffer
// pointed to by <buf>.
DWORD __stdcall gos_ReadFile( HGOSFILE hfile, void* buf, DWORD size );

//////////////////////////////////////////////////////////////////////////////////
// Write <size> bytes to the file specified by handle <hfile> from the buffer
// pointed to by <buf>.
DWORD __stdcall gos_WriteFile( HGOSFILE hfile, const void* buf, DWORD size );

//////////////////////////////////////////////////////////////////////////////////
// Move the current file position in filestream <hfile> to offset <offset> using
// the seek type specified by <type>.
DWORD __stdcall gos_SeekFile( HGOSFILE hfile, gosEnum_FileSeekType type, int offset );




