#include "NTFS_LAST_HEADER.hpp"

VolStruct gVol;

int get_MFTEntry(int MFTNum, MFTEntry * curMFTEntry);
void show_mft(MFTEntry mft);
void show_attr(MFTEntry mft);
void attr_FILENAME(AttrFILENAME* filename, char * fin);
void attr_STD_INFO(AttrSTDINFO * stdInfo);
void attr_ATTR_LIST(ListEntry * pEntry, U32 AttrLen);
void get_BPB_info(NTFS_BPB * boot, VolStruct * gVol);
void Input_Parser(char * input);
//void ChangeFixupData(U16 * data, U32 ArrayOffset, U32 ArrayCnt, U32 Size);
void read_file(MFTEntry ft, U32 read_size, U8 * read_buf);
S32 find_file(U32 MFTNum, char * filename, MFTEntry * findMft);
U64 fileFile_inNode(U8* p, U64 * child_vcn, char * filename);
void get_RunList(U8 * data, RunData * runData);
S32 readCluster(U64 vcn, U32 ClusCnt, RunData * runData, U8 * buf);
void * getAttr(U32 AttrNum, MFTEntry * mft);
void ChangetoUpper(char * name);
U64 EFI_Sector_Finder(char * FileSystem);
U32 HDD_read(U8 drv, U64 SecAddr, U32 blocks, U8 * buf);


int main(int argc, char ** argv) {

	if (argc != 2 || strlen(argv[1]) > 8) {
		fprintf(stderr, "Invalid Input\n");
		fprintf(stderr, "<usage> FileSystem.exe [Filesystem Name]");
		exit(0);
	}
	NTFS_BPB boot;
	MFTEntry mft;
	char filename[128];
	U8 buf[4096] = { 0 };

	gVol.Drive = 0x00;

	if ((gVol.VolBeginSec = EFI_Sector_Finder(argv[1])) == 0) {
		fprintf(stderr, "File System doesn't exist");
	}

	strcpy(filename, "gettysburg address.txt");

	if ((HDD_read(gVol.Drive, gVol.VolBeginSec * 512, 1, (U8*)&boot)) == 0) {
		fprintf(stderr, "Boot Sector Read Failed \n");
		return 1;
	}

	get_BPB_info(&boot, &gVol);
	find_file(5, filename, &mft);
	read_file(mft, 4096, buf);
	printf("%s\n", buf);


	return 0;
}

void read_file(MFTEntry mft, U32 read_size, U8 * read_buf) {
	RunData runData[MAX_CLUS_RUN] = { 0 };
	U32 ClusCnt;

	ClusCnt = (read_size + gVol.ClusSize - 1) / gVol.ClusSize;

	get_RunList((U8*)getAttr(128, &mft), runData);
	readCluster(0, ClusCnt, runData, read_buf);

	return;
}

void * getAttr(U32 AttrNum, MFTEntry* mft) {
	AttrHeader * header;
	U32 offset;
	void * pAttr;

	offset = mft->Header.AddrFirstArr;
	header = (AttrHeader*)&mft->Data[offset];

	do {
		if (header->non_resident_flag == 0) pAttr = &mft->Data[offset + header->Res.AttrOffset];
		else pAttr = &mft->Data[offset + header->NonRes.RunListOffset];

		if (header->AttrTypeID == AttrNum) return pAttr;

		offset = offset + header->Length;
		header = (AttrHeader*)&mft->Data[offset];
	} while (header->AttrTypeID != -1);
}

S32 find_file(U32 MFTNum, char * filename, MFTEntry * findMft) {
	MFTEntry mft;
	RunData runData[MAX_CLUS_RUN] = { 0 };
	U8 IndexRecord[4096];
	U64 child_vcn;
	U32 IndexRecordClusSize;
	U64 ret;

	AttrINDEX_ROOT * pRoot;
	INDEX_RECORD_HEADER * pIndexRecHeader;

	ChangetoUpper(filename);

	get_MFTEntry(MFTNum, &mft);

	pRoot = (AttrINDEX_ROOT*)getAttr(144, &mft);
	IndexRecordClusSize = pRoot->IndexRecordClusSize;

	ret = fileFile_inNode((U8*)pRoot + sizeof(AttrINDEX_ROOT), &child_vcn, filename);

	if (ret == 0) return 0;
	else if (ret > 1) {
		get_MFTEntry(ret, findMft);

		return 1;
	}

	get_RunList((U8*)getAttr(160, &mft), runData);

	do {
		readCluster(child_vcn, IndexRecordClusSize, runData, IndexRecord);

		pIndexRecHeader = (INDEX_RECORD_HEADER*)IndexRecord;

		ret = fileFile_inNode((U8*)IndexRecord + sizeof(INDEX_RECORD_HEADER), &child_vcn, filename);

		switch (ret) {
		case 0:
			return 0;
		case 1:
			break;
		default:
			get_MFTEntry(ret, findMft);

			return 1;
		}
	} while (1);

	return 0;
}

U64 fileFile_inNode(U8* p, U64 * child_vcn, char * filename) {
	NodeHeader * pNode;
	IndexEntry * pEntry;
	AttrFILENAME * pAttrName;
	char fin[512] = { 0, };

	pNode = (NodeHeader*)p;
	pEntry = (IndexEntry*)((char*)pNode + pNode->AddrFirstIndex);

	while (1) {
		if (pEntry->Header.Flags & 0x02) {
			if(pEntry->Header.Flags & 0x01) {
				*child_vcn = *((U64*)&pEntry->Data[pEntry->Header.LenOfEntry - 8]);
				return 1;
			}
			else return 0;
		}
		pAttrName = (AttrFILENAME*)&pEntry->Data[16];
		wcstombs(fin, &pAttrName->FileName, pAttrName->LenOfName);
		fin[pAttrName->LenOfName] = 0;

		ChangetoUpper(fin);

		if (strcmp(fin, filename) > 0) {
			if (pEntry->Header.Flags & 0x01) {
				*child_vcn = *((U64*)&pEntry->Data[pEntry->Header.LenOfEntry - 8]);
				return 1;
			}
		}
		else {
			return 0;
		}

		if (!strcmp(filename, fin)) return (pEntry->Header.FileReferrence & 0x0000FFFFFFFFFFFF);

		pEntry = (IndexEntry*)((char*)pEntry + pEntry->Header.LenOfEntry);
	}	
}

int readCluster(U64 vcn, U32 ClusCnt, RunData * runData, U8 * buf) {
	U32 i;
	U32 curVCN = 0;
	U32 len = 0;
	U32 bufOffset = 0;
	U32 readLen = 0, readLCN = 0;
	U32 readSec = 0, readSecCnt = 0;

	for (i = 0; i < MAX_CLUS_RUN; i++) {

		if (runData[i].Len == 0) return 1;

		if (curVCN == vcn) {
			readLen = runData[i].Len;
			readLCN = runData[i].Offset;
		}

		else if (vcn < runData[i].Len + curVCN) {
			int temp;
			temp = vcn - curVCN;

			readLen = runData[i].Len - temp;
			readLCN = runData[i].Offset + temp;

		}
		else {
			curVCN = runData[i].Len + curVCN;
			continue;
		}
	}

	curVCN = runData[i].Len + curVCN;

	readSec = (readLCN * gVol.SecPerClus) + gVol.VolBeginSec;

	if (ClusCnt > readLen) {
		readSecCnt = readLen * gVol.SecPerClus;

		HDD_read(gVol.Drive, readSec, readSecCnt, (U8*)&buf[bufOffset]);

		bufOffset = bufOffset + (readLen * gVol.ClusSize);
		vcn = vcn + readLen;
		ClusCnt = ClusCnt - readLen;
	}
	else if (ClusCnt <= readLen) {
		readSecCnt = ClusCnt * gVol.SecPerClus;
		HDD_read(gVol.Drive, readSec, readSecCnt, (U8*)&buf[bufOffset]);
		bufOffset = bufOffset + (ClusCnt * gVol.ClusSize);
		return 0;
	}
}

void ChangetoUpper(char * name) {
	int i = 0;
	while (name[i] != 0) name[i++] = toupper(name[i]);
}

void get_RunList(U8* data, RunData*runData) {
	int offset = 0;
	int LenSize;
	int OffsetSize;
	int i = 0, runCnt = 0;
	int prevRunOffset = 0;

	while (data[offset] != 0) {
		OffsetSize = data[offset] >> 4;
		LenSize = data[offset++] & 0x0F;

		for(i = 0; LenSize != 0; LenSize--, i += 8) {
			runData[runCnt].Len |= data[offset++] << i;

			if (OffsetSize == 0) {
				runData[runCnt].Offset = 0;
				continue;
			}

			for (i = 4; i > 0; i--) {
				runData[runCnt].Offset = runData[runCnt].Offset >> 8;

				if (OffsetSize != 0) {
					runData[runCnt].Offset = runData[runCnt].Offset & 0x00FFFFFF;
					runData[runCnt].Offset |= data[offset++] << 24;

					OffsetSize--;
				}
			}
		}

		runData[runCnt].Offset = prevRunOffset + runData[runCnt].Offset;
		prevRunOffset = runData[runCnt].Offset;
		runCnt++;
	}
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
	} while (offset < AttrLen);
}

void attr_STD_INFO(AttrSTDINFO * stdInfo) {
	printf("---- STD INFO ATTR ----\n");
	printf("Created time = %I64X\n", stdInfo->CreateTime);
	printf("Modified time = %I64d\n", stdInfo->ModifiedTime);
	printf("MFT Modified time = %I64d\n", stdInfo->MFTmodifiedTime);
	printf("Accessd time = %I64d\n", stdInfo->AccessTime);
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