//---------------------------------------------------------------------------
//
// file.h - This file contains the class declaration for File
//
//				The File class simply calls the Windows 32bit file functions.
//				It is purely a wrapper.
//
//				The mmFile Class is a wrapper for the Win32 Memory Mapped
//				file functionality.  It is used exactly the same as above class.
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#ifndef FILE_H
#define FILE_H
//---------------------------------------------------------------------------
// Include files

#ifndef DSTD_H
#include "dstd.h"
#endif

#ifndef DFILE_H
#include "dfile.h"
#endif

#ifndef DFFILE_H
#include "dffile.h"
#endif

#ifndef HEAP_H
#include "heap.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
//---------------------------------------------------------------------------
// Enums
enum FileMode
{
	NOMODE = 0,
	READ,
	CREATE,
	MC2_APPEND,
	WRITE,
	RDWRITE
};

enum FileClass
{
	BASEFILE = 0,
	INIFILE,
	PACKETFILE,
	CSVFILE
};

//---------------------------------------------------------------------------
// Function Declarations
//Returns 1 if file is on HardDrive and 2 if file is in FastFile
long fileExists(PSTR fName);
long fileExistsOnCD(PSTR fName);
bool file1OlderThan2(PSTR file1, PSTR file2);

//---------------------------------------------------------------------------
// Macro Definitions
#ifndef	NO_ERR
#define	NO_ERR					0x00000000
#endif

#define	DISK_FULL_ERR			0xBADF0001
#define	SHARE_ERR				0xBADF0002
#define	FILE_NOT_FOUND_ERR		0xBADF0003
#define	PACKET_OUT_OF_RANGE		0xBADF0004
#define	PACKET_WRONG_SIZE		0xBADF0005
#define	READ_ONLY_ERR			0xBADF0006
#define	TOO_MANY_FILES_ERR		0xBADF0007
#define	READ_PAST_EOF_ERR		0xBADF0008
#define	INVALID_SEEK_ERR		0xBADF0009
#define	BAD_WRITE_ERR			0xBADF000A
#define	BAD_PACKET_VERSION		0xBADF000B
#define	NO_RAM_FOR_SEEK_TABLE	0xBADF000C
#define	NO_RAM_FOR_FILENAME		0xBADF000D
#define	PARENT_NULL				0xBADF000E
#define	TOO_MANY_CHILDREN		0xBADF000F
#define	FILE_NOT_OPEN			0xBADF0010
#define	CANT_WRITE_TO_CHILD		0xBADF0011
#define	NO_RAM_FOR_CHILD_LIST	0xBADF0012
#define MAPPED_WRITE_NOT_SUPPORTED	0xBADF0013
#define COULD_NOT_MAP_FILE		0xBADF0014

//---------------------------------------------------------------------------
//									File
//---------------------------------------------------------------------------
class File
{
	// Data Members
	//--------------
	protected:
	
		char 					*fileName;
		FileMode				fileMode;
	
		long					handle;

		FastFilePtr				fastFile;
		long					fastFileHandle;

		size_t 			length;
		size_t 			logicalPosition;

		size_t			bufferResult;

		FilePtr					*childList;
		size_t			numChildren;
		size_t			maxChildren;
				
		FilePtr					parent;
		size_t			parentOffset;
		size_t			physicalLength;

		bool					inRAM;
		MemoryPtr				fileImage;

	public:

		static bool				logFileTraffic;
		static size_t	lastError;

	// Member Functions
	//------------------
		protected:

			void setup (void);

		public:

			void *operator new (size_t mySize);
			void operator delete (void *us);
			
			File (void);
			~File (void);

			bool eof (void);

			virtual long open ( PCSTR fName, FileMode _mode = READ, long numChildren = 50);
			virtual long open( PCSTR buffer, int bufferLength ); // for streaming from memory

			virtual long create (PCSTR fName);
			virtual long createWithCase( PSTR fName ); // don't strlwr for me please!

			virtual void close (void);

			virtual long open (File *_parent, size_t fileSize, long numChildren = 50);
			
			void deleteFile (void);

			long seek (long pos, long from = SEEK_SET);
			void seekEnd (void);
			void skip (long bytesToSkip);

			long read (size_t pos, MemoryPtr buffer, long length);
			long read (MemoryPtr buffer, long length);

			//Used to dig the LZ data directly out of the fastfiles.
			// For textures.
			long readRAW (size_t * &buffer, UserHeapPtr heap);

			uint8_t readByte (void);
			short readWord (void);
			short readShort (void);
			long readLong (void);
			float readFloat( void );

			long readString (MemoryPtr buffer);
			long readLine (MemoryPtr buffer, long maxLength);
			long readLineEx (MemoryPtr buffer, long maxLength);

			long write (size_t pos, MemoryPtr buffer, long bytes);
			long write (MemoryPtr buffer, long bytes);

			long writeByte (uint8_t value);
			long writeWord (short value);
			long writeShort (short value);
			long writeLong (long value);
			long writeFloat (float value);

			long writeString (PSTR buffer);
			long writeLine (PSTR buffer);

			bool isOpen (void);

			virtual FileClass getFileClass (void)
			{
				return BASEFILE;
			}

			PSTR getFilename (void);

			size_t getLength (void);
			size_t fileSize (void);
			size_t getNumLines (void);

			size_t getLastError (void) {
				return(lastError);
			}

			size_t getLogicalPosition (void)
			{
				return logicalPosition;
			}
			
			FilePtr getParent (void)
			{
				return parent;
			}

			FileMode getFileMode (void)
			{
				return(fileMode);
			}
			
			long getFileHandle (void)
			{
				return(handle);
			}

			time_t getFileMTime (void);
			
			long addChild (FilePtr child);
			void removeChild (FilePtr child);

};

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
#endif

//---------------------------------------------------------------------------
//
// Edit Log
//
//
//---------------------------------------------------------------------------
