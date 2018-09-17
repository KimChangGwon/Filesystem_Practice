#include "NTFS_MFT_ENTRY_HEADER.h"

using namespace std;

VolStruct gVol;

int get_MFTEntry(int MFTNum, MFTEntry * curMFTEntry);
void show_mft(MFTEntry mft);
void show_attr(MFTEntry mft);
void attr_FILENAME(AttrFILENAME* filename, char * fin);
void attr_STD_INFO(AttrSTDINFO * stdInfo);
void attr_ATTR_LIST(ListEntry * pEntry, U32 AttrLen);
void get_BPB_info(NTFS_BPB * boot, VolStruct * gVol);
void convert_order(U64 & time);
U64 windowsTime(U64 time);

int main(int argc, char ** argv) {

	if (argc != 2 || strlen(argv[1]) > 8) {
		fprintf(stderr, "Invalid Input\n");
		fprintf(stderr, "<usage> FileSystem.exe [Filesystem Name]");
		exit(0);
	}
	NTFS_BPB boot;
	MFTEntry mft;

	gVol.Drive = 0x00;

	if ((gVol.VolBeginSec = EFI_Sector_Finder(argv[1])) == 0) {
		fprintf(stderr, "File System doesn't exist");
	}

	if ((HDD_read(gVol.Drive, gVol.VolBeginSec * 512, 1, (U8*)&boot)) == 0) {
		fprintf(stderr, "Boot Sector Read Failed \n");
		return 1;
	}

	get_BPB_info(&boot, &gVol);
	get_MFTEntry(1, &mft);
	show_mft(mft);
	show_attr(mft);
	
	return 0;
}

int get_MFTEntry(int MFTNum, MFTEntry * curMFTEntry) {
	U64 readSec = gVol.MFTStartSec + (MFTNum * 2);
	if (HDD_read(gVol.Drive, readSec * 512, 1, (U8*)curMFTEntry) == 0) {
		printf("MFT Read Failed\n");
		exit(1);
	}

	return 0;
}

void show_mft(MFTEntry mft) {
	printf("<<<<<<<<<< MFT Entry Info >>>>>>>>>>>>\n");
	printf("LSN = %d \n", mft.Header.LSN);
	printf("Sequence Value = %d\n", mft.Header.SequenceValue);
	printf("Link Count = %d\n", mft.Header.LinkCnt);
	printf("First Attr Offset = %d\n", mft.Header.AddrFirstArr);
	printf("Flags = %x\n", mft.Header.Flags);
	printf("Used Size of MFT = %d\n", mft.Header.UsedSizeOfEntry);
	printf("Alloc size of MFT = %d\n", mft.Header.AllocSizeOfEntry);
	printf("File Referrence to base record = %d\n", mft.Header.FileRefer);
	printf("Next Attr ID = %d\n", mft.Header.NextAttrID);
}

void convert_order(U64 & time) {
	U64 converted_time = 0;
	for (int a = 0; a < 64; a = a + 8) {
		converted_time |= ((time >> a) & 0xFF) << (56 - a);
	}
}

U64 windowsTime(U64 time) {
	convert_order(time);

	return time;
}

void show_attr(MFTEntry mft) {
	int offset = mft.Header.AddrFirstArr;
	void * pAttr;
	AttrHeader * header = (AttrHeader *)&mft.Data[offset];

	do {
		if (header->non_resident_flag == 0) {
			pAttr = &mft.Data[offset + header->Res.AttrOffset];
		}
		else {
			pAttr = &mft.Data[offset + header->NonRes.RunListOffset];
		}

		if (header->AttrTypeID == 48) {
			char fin[512] = { 0, };
			attr_FILENAME((AttrFILENAME*)pAttr, fin);
		}
		else if (header->AttrTypeID == 32) {
			attr_ATTR_LIST((ListEntry*)pAttr, header->Res.AttrSize);
		}
		else if (header->AttrTypeID == 16) {
			attr_STD_INFO((AttrSTDINFO*)pAttr);
		}

		offset = offset + header->Length;
		header = (AttrHeader*)&mft.Data[offset];
	} while (header->AttrTypeID != -1);
 }

void attr_ATTR_LIST(ListEntry*pEntry, U32 AttrLen) {
	U32 offset = 0;
	printf("---- ATTR List ATTR ---- \n");

	do {
		printf("<<<< Type = %d >>>> \n", pEntry->Type);
		printf("Entry Len = %d \n", pEntry->EntryLen);
		printf("Name Len = %d\n", pEntry->NameLen);
		printf("Name Offset = %d\n", pEntry->NameOffset);
		printf("Start VCN = %I64d\n", pEntry->StartVCN);
		printf("MFT ATtr = %I6d\n", pEntry->BaseMFTAddr & 0x0000FFFFFFFFFFFF);
		printf("Attr ID = %d\n", pEntry->AttrID);
	}	while (offset < AttrLen);
}

void attr_STD_INFO(AttrSTDINFO * stdInfo) {
	printf("---- STD INFO ATTR ----\n");
	printf("Created time = %I64X\n", windowsTime(stdInfo->CreateTime));
	printf("Modified time = %I64d\n", windowsTime(stdInfo->ModifiedTime));
	printf("MFT Modified time = %I64d\n", windowsTime(stdInfo->MFTmodifiedTime));
	printf("Accessd time = %I64d\n", windowsTime(stdInfo->AccessTime));
	printf("Flag = %x\n", stdInfo->Flags);
}

void attr_FILENAME(AttrFILENAME * filename, char * fin) {
	wcstombs(fin, &filename->FileName, filename->LenOfName);
	fin[filename->LenOfName] = 0;

	printf("---- FILE NAME ATTR ---- \n");
	printf("Flags = %x\n", filename->Flags);
	printf("Name Length = %d\n", filename->LenOfName);
	printf("Name Space = %d\n", filename->NameSpace);
	printf("Name = %s\n", fin);
}

void get_BPB_info(NTFS_BPB * pBPB, VolStruct * pVol) {
	pVol->ClusSize = pBPB->SecPerClus * pBPB->BytesPerSec;
	pVol->SecPerClus = pBPB->SecPerClus;
	pVol->MFTStartSec = pBPB->StartOfMFT * pBPB->SecPerClus + pVol->VolBeginSec;
	pVol->MFTMirrStartSec = pBPB->StartOfMFTMirr * pBPB->SecPerClus + pVol->VolBeginSec;
	pVol->TotalSec = pBPB->TotalSector;
	
	if (pBPB->SizeOfMFTEntry < 0) {
		pVol->SizeOfMFTEntry = 1 << (pBPB->SizeOfMFTEntry - 1);
	}
	else {
		pVol->SizeOfMFTEntry = pBPB->SizeOfMFTEntry * pVol->ClusSize;
	}

	pVol->SizeOfIndexRecord = pBPB->SizeOfIndexRecord * pVol->ClusSize;
}