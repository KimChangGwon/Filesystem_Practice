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
#define MAX_CLUS_RUN 4096

#pragma pack(1)
typedef struct __Attr_INDEX_ROOT {
	U32 AttrType;
	U32 CollationRule; // 해당 인덱스가 담고 있는 형식 (0x00 : Binary, 0x01 File Name, 0x02 Unicode string, 0x10 Unsigned Long ...)
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
	U64 ThisIndexVCN;
}INDEX_RECORD_HEADER;
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



