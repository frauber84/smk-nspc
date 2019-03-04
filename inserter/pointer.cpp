#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys\stat.h>
#include <windows.h>
#include "LunarDLL.h"

#define POINTER_TABLE    0x1f47b
#define NUM_TRACK        15

FILE *out;
unsigned char buffer[0x10000];
unsigned int FileSize;
unsigned int TrackToInsert;
unsigned int InsertFileSize;
int TrackPointers[15];

char *TrackNames[15] = { "Player Select [dummy]",          /* 0 */
                          "Mario Kart Theme",
                          "Player Select",
                          "Mario Circuit",
                          "Donut Plains",
                          "Choco Island",                  /* 5 */
                          "Koopa Beach",
                          "Vanilla Lake",
                          "Ghost House",
                          "Bowser",
                          "Battle Mode",                   /* 10 */
                          "Finish Podium",
                          "Finish Podium [4th place]",
                          "Ending",
                          "Rainbow Road" };

int LoadPointers(FILE* ROM, int address);
int ReadPointer(FILE *ROM);

int LoadPointers(FILE* ROM, int address)
{
    int i;
    
    fseek(ROM, address, SEEK_SET);
    
    for (i = 0; i < NUM_TRACK; i++)
    {
        TrackPointers[i] = ReadPointer(ROM);
    }
    return 0;

}

int ReadPointer(FILE* ROM)
{

    int pointer;
    unsigned char byte1, byte2, byte3;

    byte1 = fgetc(ROM);
    byte2 = fgetc(ROM);
    byte3 = fgetc(ROM);

    pointer = byte1 + (byte2 << 8) + (  (byte3 & 0x0F) << 16 ) ;
    
    return pointer;

}

int main (int argc, char *argv[])
{

    FILE *fp;
	FILE *track;
    char filename[14];
	int i;

    if ( argc < 3 )
	{
		printf("Usage: SMK.exe [-d] [rom]  (Decompress all tracks)\n"
			   "       SMK.exe [-e] [file] [rom] [track number] (Compress and replace track)\n");
		return -1;
	}

    if (!LunarLoadDLL())
    {
	    printf("\nAin't a secret to everybody: go get Lunar Compress.dll!");
        return -1;
    }

    if (strcmp(argv[1], "-d") == 0)
	{
		fp = fopen(argv[2], "rb");

		if (!fp) 
		{
			printf("File %s not found\n", argv[2]);
			return -1;
		}

        LoadPointers(fp, POINTER_TABLE);
		fclose(fp);

	    if (!LunarOpenFile(argv[2] ,LC_READONLY))
        {
            printf("Could not open file smk.smc for reading.\n");
            LunarUnloadDLL();
            return -1;
        }

        for (i = 0; i < NUM_TRACK; i++)
        { 
           printf("\nTrack #%02d (%s) --> 0x%06x ", i, TrackNames[i], TrackPointers[i]);
        }
        
       for (i = 1; i < NUM_TRACK; i++)
       {
           sprintf(filename, "Track%02d", i);
           FileSize = LunarDecompress(buffer, TrackPointers[i], 0x10000, 4, 0, 0);
           out = fopen(filename,"wb");  

		   if (!out)
		   {
			   printf("Error writing file %s, aborting.\n", filename);
			   LunarUnloadDLL();
			   return -1;
		   }

           fseek(out, 0, 0);
           fwrite(buffer, FileSize, 1, out);
           fclose(out);
       
       } 
       LunarCloseFile();

	}
    if (strcmp(argv[1], "-e") == 0)
	{

        if ( argc < 5 )
	    {
		    printf("Missing arguments!\n"
                   "Usage: SMK.exe [-d] [rom]  (Decompress all tracks)\n"
			       "       SMK.exe [-e] [file] [rom] [track] (Compress track and replace track #)\n");
		    return -1;

		}

        if ( strcmp(argv[2],argv[3]) == 0 )
        {
			printf("Error: file names are identical.\n");
            return -1;
        }

		fp = fopen(argv[3], "rb");
        if (!fp) 
		{
			printf("File %s not found\n", argv[3]);
			return -1;
		}
		LoadPointers(fp, POINTER_TABLE);
		fclose(fp);

		sscanf(argv[4], "%i", &TrackToInsert);
		printf("TrackToInsert = %d\n", TrackToInsert);

		if (TrackToInsert < 1 || TrackToInsert > 14)
		{
			printf("Invalid track number\n");
			return -1;
		}

        track = fopen(argv[2], "rb");
        if (!track) 
		{
			printf("File %s not found\n", argv[2]);
			return -1;
		}
		fseek(track, 0, SEEK_END);
		InsertFileSize = ftell(track);
        fseek(track, 0, SEEK_SET);
		fread(buffer, InsertFileSize, 1, track);

		fclose(track);

		FileSize = LunarRecompress(buffer, buffer, InsertFileSize, 0x10000, 4, 0);

		if (!FileSize)
        {
            printf("Compression failed.\n");
            return -1;
        }
        printf("Compressed Size    %6X\nUncompressed Size  %6X\n", FileSize, InsertFileSize);

		fp = fopen(argv[3], "r+b");
        fseek(fp,TrackPointers[TrackToInsert],0);
        fwrite(buffer,FileSize,1,fp);
        fclose(fp);

        printf("\nTrack %d (%s) inserted!\n", TrackToInsert, TrackNames[TrackToInsert]);
        return 1;   
	}
   
   /* terminate */
   LunarUnloadDLL();

    return 0;
}
