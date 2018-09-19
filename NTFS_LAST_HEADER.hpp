/*
1. usage : FileSystem.exe [FS name]
2. EFI_Offset_Finder : finds a number of the sector that matches the input fs name
3. HDD_read : copies the fs boot record using the offset from EFI_Offset_Finder function return value
*/

#pragma once
#include <cstdio>
#include <windows.h>
#include <stdlib.h>
#include <direct.h>
#include <locale.h>
#include <ctype.h>

#define U8 unsigned char
#define S8 char
#define U16 unsigned short
#define U32 unsigned int
#define U64 unsigned __int64
#define S32 int

#pragma pack(1)
typedef struct __Attr_INDEX_ROOT {
	U32 AttrType;
	U32 CollationRule;
	U32 IndexRecorSize;
	U8 IndexRecordClusSize;
	U8 Padding[3];
} AttrINDEX_ROOT;
#pragma pack()

#pragma pack(1)
typedef struct __INDEX_RECORD_HEADER {
	S8 Sign[4];
	U16 AddrFixUpArray;
	U16 CountFixUpArray;
	U64 LSN;
	U64 ThisIndexVCN :
} INDEX_RECORD_HEADER;
#pragma pack()

#pragma pack(1)
typedef struct __NODE_HEADER {
	U32 AddrFirstIndex;
	U32 UsedSizeOfIndex;
	U32 AllocSizeOfIndex;
	U8 Flags;
	U8 Padding[3];
}NodeHeader;
#pragma pack()

#pragma pack(1)
typedef struct __INDEX_ENTRY_HEADER {
	U64 FileReferrence;
	U16 LenOfEntry;
	U16 LenOfContent;
	U16 Flags;
}IndexEntryHeader;
#pragma pack()

typedef struct _RunData {
	U32 Len;
	S32 Offset;
} RunData;

typedef union __Index_Entry {
	IndexEntryHeader Header;
	U8 Data[];
} IndexEntry;


#pragma pack(1)
typedef struct __MFTEntry_Header_Struct {
	S8 Signature[4];
	U16 AddrFixUpArray;
	U16 CountFixArray;
	U64 LSN;
	U16 SequenceValue;
	U16 LinkCnt;
	U16 AddrFirstArr;
	U16 Flags;
	U16 UsedSizeOfEntry;
	U32 AllocSizeOfEntry;
	U64 FileRefer;
	U16 NextAttrID;
}MFTEntry_Header;
#pragma pack()

typedef union __MFTEntry {
	MFTEntry_Header Header;
	U8 Data[1024];
}MFTEntry;

#pragma pack(1)
typedef struct __Attr_Resident {
	U32 AttrSize;
	U16 AttrOffset;
	U8 IndexedFlag;
	U8 Padding;
} AttrRes;
#pragma pack()

#pragma pack(1)
typedef struct __Attr_Non_Resident {
	U64 StartVCNOfRunList;
	U64 EndVCNOfRunList;
	U16 RunListOffset;
	U16 CompressUnitSize;
	U32 Unused;
	U64 AllocSizeOfAttrContent;
	U64 ActualSizeOfAttrContent;
	U64 InitialSizeOfAttrContent;
}AttrNonRes;
#pragma pack()

#pragma pack(1)
typedef struct __Attr_Header {
	U32 AttrTypeID;
	U32 Length;
	U8 non_resident_flag;
	U8 LenOfName;
	U16 OffsetOfName;
	U16 Flags;
	U16 AttrID;
	union {
		AttrRes Res;
		AttrNonRes NonRes;
	};
}AttrHeader;
#pragma pack()

#pragma pack(1)
typedef struct __Attr_FILENAME {
	U64 FileReferOfParentDIR;
	U64 CreateTime;
	U64 ModTime;
	U64 MFTmodTime;
	U64 AccTime;
	U64 AllocSize;
	U64 UsedSize;
	U32 Flags;
	U32 ReparseValue;
	U8 LenOfName;
	U8 NameSpace;
	wchar_t FileName;
}AttrFILENAME;
#pragma pack()

#pragma pack(1)
typedef struct __Attr_STDINFO {
	U64 CreateTime;
	U64 ModifiedTime;
	U64 MFTmodifiedTime;
	U64 AccessTime;
	U32 Flags;
	U32 MaxVer;
	U32 CurVer;
	U32 ClassID;
	U32 OwnerID;
	U32 SecurityID;
	U64 Quata;
	U64 USN;
}AttrSTDINFO;
#pragma pack()

#pragma pack(1)
typedef struct __ATTR_LIST_ENTRY {
	U32 Type;
	U16 EntryLen;
	U8 NameLen;
	U8 NameOffset;
	U64 StartVCN;
	U64 BaseMFTAddr;
	U64 AttrID;
	wchar_t ATtrName;
}ListEntry;
#pragma pack()

#pragma pack(1)
typedef struct _NTFS_BPB_struct {
	U8 JumpBoot[3];
	U8 OEMName[8];
	U16 BytesPerSec;
	U8 SecPerClus;
	U16 RsvdSecCnt;
	U8 Unused1[5];
	U8 Media;
	U8 Unused2[18];
	U64 TotalSector;
	U64 StartOfMFT;
	U64 StartOfMFTMirr;
	S8 SizeOfMFTEntry;
	U8 Unused3[3];
	S8 SizeOfIndexRecord;
	U8 Unused4[3];
	U64 SerialNumber;
	U32 Unused;

	U8 BootCodeArea[426];

	U16 Signature;
}NTFS_BPB;

typedef struct _VOL_struct {
	U32 Drive;
	U64 VolBeginSec;
	U64 MFTStartSec;
	U64 MFTMirrStartSec;
	U32 ClusSize;
	U32 SecPerClus;
	U64 TotalSec;
	U32 SizeOfMFTEntry;
	U32 SizeOfIndexRecord;
}VolStruct;

U64 EFI_Sector_Finder(char * FileSystem);
U32 HDD_read(U8 drv, U64 SecAddr, U32 blocks, U8 * buf);
void Input_Parser(char * input);

U32 HDD_write(U8 drv, U32 SecAddr, U32 blocks, U8 * buf) {
	_wsetlocale(LC_ALL, L"koeran");
	U32 ret = 0;
	U32 ldistanceLow, ldistanceHigh, dwpointer, bytestoread, numread;

	char cur_drv[100];
	HANDLE g_hDevice;

	sprintf(cur_drv, "E:");
	g_hDevice = CreateFile(cur_drv, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (g_hDevice == INVALID_HANDLE_VALUE) return 0;

	ldistanceLow = SecAddr;
	ldistanceHigh = SecAddr >> (32 - 9);
	dwpointer = SetFilePointer(g_hDevice, ldistanceLow, (long*)&ldistanceHigh, FILE_BEGIN);

	if (dwpointer != 0xFFFFFFFF) {
		bytestoread = blocks * 512;
		ret = WriteFile(g_hDevice, buf, bytestoread, (unsigned long*)&numread, NULL);

		if (ret) ret = 1;
		else ret = 0;
	}

	CloseHandle(g_hDevice);
	return ret;
}

U32 HDD_read(U8 drv, U64 SecAddr, U32 blocks, U8 * buf) {
	U32 ret;
	U32 ldistanceLow, ldistanceHigh, dwpointer, bytestoread, numread;
	U8 * cur_path = (U8*)malloc(200);
	char cur_drv[100];
	HANDLE g_hDevice;

	sprintf(cur_drv, "\\\\.\\PhysicalDrive%d", (U32)drv);
	g_hDevice = CreateFile(cur_drv, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (g_hDevice == INVALID_HANDLE_VALUE) {
		puts("INVALID HANDLE");
		return 0;
	}

	ldistanceLow = SecAddr & (0xFFFFFFFF);
	ldistanceHigh = SecAddr >> 32;
	dwpointer = SetFilePointer(g_hDevice, ldistanceLow, (long*)&ldistanceHigh, FILE_BEGIN);

	if (dwpointer != 0xFFFFFFFF) {
		bytestoread = blocks * 512;
		ret = ReadFile(g_hDevice, buf, bytestoread, (unsigned long*)&numread, NULL);

		if (ret) ret = 1;
		else ret = 0;
	}
	else {
		puts("File open Fail");
		ret = 0;
	}
	CloseHandle(g_hDevice);
	return ret;
}

void HexDump(U8 * addr, U32 len) {
	U8 *s = addr, *endPtr = (U8*)((U32)addr + len);
	U32 i, remainder = len % 16;

	printf("\nOffset       Hex Value	                                      Ascii value \n");

	while (s + 16 <= endPtr) {

		printf("0x%08lx   ", (long)(s - addr));

		for (i = 0; i < 16; i++) printf("%02x ", s[i]);

		printf(" ");

		for (i = 0; i < 16; i++) {
			if (s[i] >= 32 && s[i] <= 125) printf("%c", s[i]);
			else printf(".");
		}

		s += 16;
		printf("\n");
	}

	if (remainder) {
		printf("0x%08lx   ", (long)(s - addr));

		for (i = 0; i < remainder; i++) printf("%02x ", s[i]);

		for (i = 0; i < (16 - remainder); i++) printf("     ");

		printf(" ");

		for (i = 0; i < remainder; i++) {
			if (s[i] >= 32 && s[i] <= 125) printf("%c", s[i]);
			else printf(".");
		}

		for (i = 0; i < (16 - remainder); i++) printf(" ");
	}

	return;
}

void Input_Parser(char * input) {
	char * temp = (char*)malloc(sizeof(char) * 8);
	memset(temp, 0x20, sizeof(char) * 8);
	memcpy(temp, input, strlen(input));
	memcpy(input, temp, sizeof(char) * 8);
}

/*
EFI_Sector_Finder
This function receives a FileSystem name from user's command line
then, parses the input to 8 bytes FS name for being used to find the offset 'sector number' from GPT structure.
returns a sector number of FS BR sector number.
the Sector number is used in HDD_read function to have a sector that contains 'filesystem boot record'
*/

U64 EFI_Sector_Finder(char * FileSystem)
{
	U64 ret = 0;
	U8 Entry_Buf[1024];
	bool pass = true;
	memset(Entry_Buf, 0, sizeof(U8) * 1024);
	Input_Parser(FileSystem);

	if (!HDD_read(0, (U64)1024, 2, Entry_Buf)) {
		printf("Cannot read EFI_Entry Header");
		exit(0);
	}

	for (int i = 0; i < 8; i++) {
		U8 tmp_Buf[128];
		char VBR[512];
		memset(tmp_Buf, 0, sizeof(U8) * 128);
		memcpy(tmp_Buf, Entry_Buf + (i * 128), sizeof(U8) * 128);
		if (tmp_Buf[0] == 0x00) return ret;

		U64 offset = ((U64)tmp_Buf[35] * 16777216 + (U64)tmp_Buf[34] * 65536 + (U64)tmp_Buf[33] * 256 + (U64)tmp_Buf[32]);
		HDD_read(0, offset * 512, 1, (unsigned char*)VBR);
		if (!strncmp(VBR + 3, FileSystem, 8)) {
			if (!strncmp(FileSystem, "NTFS", 4) && pass) {
				pass = false;
				continue;
			}
			ret = offset;
			break;
		}
	}

	return ret;
}