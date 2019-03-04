#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "nspc.h"

#define VERSION "v0.01 alpha"
#define    NSPC_RAM_OFFSET    0xD000
#define    HEADER_LENGHT      0x4
#define    SMW_REST  0xc7
#define    SMK_REST  0xc9
#define    SMW_TIE   0xc6
#define    SMK_TIE   0xc8

int ch_tag[8];

int AddLoop()
{
    int i;
    ptrloop = 0;
    
    if (LoopCount == 0)
    {

	    /* First Loop */
        CurrentLoop = (struct Loop*)malloc ( sizeof(struct Loop) );
        FirstLoop->next = CurrentLoop;
    
        CurrentLoop->BufferData = (char *)calloc(MAX_LOOP_DATA + 1, 1);
  	    CurrentLoop->SourceOffset = 0;
  	    CurrentLoop->Label = 0;
  	    
	    
        for (i = 0; i < 100; i++)
        {
            CurrentLoop->AsteriskSourceOffset[i] = 0;
        }
        CurrentLoop->next = NULL;
    }
    else
    {
        
        /* New Loop */
        struct Loop *NewLoop;
        NewLoop = (struct Loop*)calloc( sizeof(struct Loop), 1);
      
        if (NewLoop == 0)
        {
            printf("\nOut of memory for new Loop node");
            exit(-1);
        }
    
        CurrentLoop->next = NewLoop;
        CurrentLoop = NewLoop;    
    
        CurrentLoop->BufferData = (char *)calloc(MAX_LOOP_DATA + 1, 1);
	    CurrentLoop->SourceOffset = 0;
   	    CurrentLoop->Label = 0;
	    
        for (i = 0; i < 100; i++)
        {
            CurrentLoop->AsteriskSourceOffset[i] = 0;
        }
        CurrentLoop->next = NULL;
    }
        
    return 0;
}

void my_fputc(int _ch, FILE *fptr) /* wrapper-function for loops */
{
    if (LoopStatus == 0) fputc(_ch, fptr);
	
	if (LoopStatus == 1)
	{
	    fputc(_ch, fptr);	  
	    CurrentLoop->BufferData[ptrloop] = _ch;
	    ptrloop++;
	}
}

void WritePointer(int u16, FILE *fp)
{
      /* write u16 in little endian */
  	  fputc(u16, fp);     
	  fputc( (u16 >> 8), fp);


	if (LoopStatus == 1)
	{
	  CurrentLoop->BufferData[ptrloop+1] = u16;
	  CurrentLoop->BufferData[ptrloop] = u16 >> 8;
	  ptrloop += 2;
	}
}

int convert(const char *FileName, const int MusicNum, int Num)
{
	FILE *fp;
	
	fp = fopen(FileName, "rb");
	
	if (fp == 0)
	{
		printf("Error opening file %s\n", FileName);
        printf("\nPress any key to return ..."); 
		return -1;
	}
	
	fseek(fp, 0, SEEK_END);
	
	if( !(FileSize = ftell(fp) ) )
    {	
		printf("File %s is an empty file\n", FileName);
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	buf = (char*)malloc(FileSize+1);
	
	if(!buf)
    {
		printf("Memory allocation failed\n");
		fclose(fp);
		return -1;
	}
	
	fread(buf, 1, FileSize, fp);
	fclose(fp);
	buf[FileSize]=(void *)NULL;

	printf("\n--> N-SPC importer %s \n", VERSION); 
	mml_to_nspc();
	free(buf); buf=NULL;

	return 0;
}


void mml_to_nspc(void)
{

	FILE *out;

	int num_ch = 0;       // channel counter
	int i,j, k = 0;
	int LoopQt = 0;   // for '*' command
	GlobalTranspose = 0;
	int DontLoop = 0;
	int LoopLabel = 0;
	int LabelCount = 0;
	int FileSize2 = 0;
	
	for (i = 0; i < 200; i++)
	{
        LabeledLoops[i].Label = 0;
        LabeledLoops[i].SourceOffset = 0;
    }

	printf("\nOut_file = %s", out_file);

    ptrloop = 0;
	ptr=0;			      // pointer of buffer
	line=1;			      // the line number of txt
	LoopStatus = 0;

    /* init loop linked list */
    FirstLoop = (struct Loop*)malloc( sizeof(struct Loop));
    FirstLoop->BufferData = (char*)calloc(MAX_LOOP_DATA + 1, 1);
	FirstLoop->Count = 0;
    FirstLoop->next = NULL;

	/* init track struct */
    for (i = 0; i <= 8; i++)
    {
	    TrackData[i].Timestamp = 0;
  	    TrackData[i].Instrument = 0;
        TrackData[i].Pan = DEFAULT_PAN;
	    TrackData[i].CurVibrato = 0;
        TrackData[i].Volume = DEFAULT_TRACK_VOLUME;
        TrackData[i].CurGateTime = DEFAULT_GATE_TIME;
	    TrackData[i].CurVelocity = DEFAULT_VELOCITY;
    }

	/* create seq file from template */
	out = fopen(out_file, "w+b");
	
	ch=0;
	octave=4;
	LastNoteValue= -1;
	DefNoteValue= 8;

	triplet=false;
	
	for (i = 0; i < 0x24; i++)
	{
        fputc(0, out);   // reserve space for pattern pointers + 'header'
    }
	
	while(ptr<FileSize){
		switch(buf[ptr]){
			case '?':
				ptr++;
				i=GetInt(false);
				if (i==-1) i=0;
				
				if (i == 0)
				{
					printf("\nSong won't be looped");
					DontLoop = 1; 
				}				
				continue;
			case '!':	// stop?
				ptr=0xFFFFFFFF;
				continue;
			case '%':  // global transpose;
				ptr++;
				i=GetInt(false);
				if ( -24 <= i && i <= 24)
				{
				    GlobalTranspose = i;
				    printf("\nGlobal transpose = %d semitones", GlobalTranspose);				
				}
				continue;
			case '#':	//ch number
				ptr++;
				i=GetInt(false);
				
				if( 0 <= i && i <=7)
                {
					ch=i;
					
					if (num_ch != 0) my_fputc(0x00, out);
					
					printf("\n\n"
						        "-----------\n"
								"Channel #%02d\n"
								"------------\n", ch);

					num_ch++;
					TrackData[num_ch].MusicTrackPointer = ftell(out) + NSPC_RAM_OFFSET - HEADER_LENGHT;
					
					ch_tag[num_ch] = ch;
					
					// To do: Check if channel has already been used

					LastNoteValue=-1;
					// octave = 4;

				}
                else error("\nInvalid channel number", true);
				continue;
			case 'l':	//default note value
            case 'L':
				ptr++;
				i=GetInt(false);
				if( 1<= i && i <= 192)
                {
					DefNoteValue=i;
				    printf("\nDefNoteValue = %x", i);
				}
				else printf("\nInvalid note value");
				continue;
			case 'w':	//global volume
			case 'W':
				ptr++;
				i=GetInt(false);
				if( 0<=i && i<=255)
                {
					my_fputc(0xE5, out);
					my_fputc(i, out);
                    printf("\nGlobal Volume = %d", i);
				}
                else printf("\nInvalid global volume");
				continue;
			case 'v':	//channel-specific volume
			case 'V':
				ptr++;
				i=GetInt(false);
				if( 0<=i && i<=255)
                {
					my_fputc(0xED, out);
					my_fputc(i, out);
					printf("\nChannel #%d Volume = %d", ch, i);
					
//					if ( i < 50 )
//					(
//					   printf("\nWarning, volume levels below 50 will be inaudible");
//					)
					TrackData[num_ch].Volume = i;
				}
                else printf("\nInvalid track volume");
				continue;

			case 'q':	//quantization
				ptr++;
				i=GetHex(false);
				printf("\nIgnored 'q' command at line %4i: ", line);
				continue;
			case 'y':	//pan
			case 'Y':
				ptr++;
				i=GetInt(false);
				if( 0<=i && i<=20 )   /* check values */
                {
					my_fputc(0xE1, out);
					my_fputc(i, out);
					TrackData[num_ch].Pan = i;
				}
                else printf("\nInvalid pan value");
				continue;
			case '(':
                ptr++;
				i=GetInt(false);
				if( i==-1){
					error("Specify label number", true);
					exit(-1);
					continue;					
				}else if(i==0 || i > 100){
					error("Label number can't be 0 or higher than 100", true);
					continue;
				}
				
                if( buf[ptr]== ')' )
                {
					ptr++;
					
					if(buf[ptr]=='[')
                    {
                    LoopLabel=i;
                    printf("\nLoopLabel = %d", LoopLabel);

                    }
					else{
						LastNoteValue=-1;
						
						/* To Do: Check if Label exists */
						
                        fputc(0xEF, out);
                        
                        LabeledLoops[LabelCount].SourceOffset = ftell(out);
                        LabeledLoops[LabelCount].Label = i; /* problems with this */
                        
                        printf("\nLabeledLoops[%d].SourceOffset = %x", LabelCount, LabeledLoops[LabelCount].SourceOffset);
                        printf("\nLabeledLoops[%d].Label = %d", LabelCount, LabeledLoops[LabelCount].Label);
                        
                        fputc(0x00, out); // this will be later be filled
                        fputc(0x00, out);
                        
                        LabelCount++;
						
						i=GetInt(false);
						
						if( i==-1) i=1;
						if( i<1 || 127<i){
							error("Invalid loop count", true);
							i=1;
						}
						fputc(i, out);
					}
				}
				else if(buf[ptr]== ')' ) ptr++;
				
				continue;

			case '/':
				printf("\nWarning: Intro command '/' not supported.");
				ptr++;
				continue;
			case 't':
			case 'T':
				ptr++;
				if(!strncmp(&buf[ptr], "uning[", 6)){
					ptr+=6;
					i=GetInt(false);
					if( 0<=i&&i<=255){
						if(strncmp(&buf[ptr], "]=", 2)){error("\nInvalid tuning", true); continue;}
						ptr+=2;
						while(1){
							bool plus=true;
							if(buf[ptr]=='+') ptr++;
							else if(buf[ptr]=='-'){ptr++, plus=false;}
							j=GetInt(false);
							if(j==-1){error("\nInvalid tuning value", true); break;}
							if(!plus) j=-j;
							if(buf[ptr]!=',') break;
							ptr++;
							if(++i==256){ error("\n[] exceeded the range", true); break;}
						}
					}else error("\n[] out of the range", true);
					continue;
				}
				i=GetInt(false);
				if( 0<=i && i<=255)
                {
					printf("\nTempo = %d", i);
					my_fputc(0xE7, out);
					my_fputc(i, out);
				}
                else error("\nInvalid tempo", true);
				continue;
			case 'o':
			case 'O':
				ptr++;
				i=GetInt(false);
				if( 1<=i && i<=6) octave=i;
				else error("\nInvalid octave", true);
				continue;
			case '@':{
				ptr++;
				if(buf[ptr]=='@') ptr++;
				i=GetInt(false);
				if( (0<=i && i<= 127))
				{
					my_fputc(0xE0, out);
					my_fputc(i, out);
				}
				else printf("\nInvalid intrument");
				continue;}
			case '[':
				ptr++;

                if (LoopStatus == 1 )
                {
                    printf("\nFatal error: missing loop terminator");
                    exit(-1);
                }
                
                AsteriskCount = 0;
				LoopStatus = 1;
				AddLoop();
				LoopCount++;
				
				if ( LoopLabel ) 
                {
                     CurrentLoop->Label = LoopLabel;
                     LoopLabel = 0;
                }
				
				CurrentLoop->SourceOffset = ftell(out);
				printf("\n\n--- Begin of loop segment %d ---", LoopCount);
							
				LastNoteValue=-1;
				continue;
			case ']':
				ptr++;

                
				if (LoopStatus == 0)
				{
					printf("\nError: Loop end '[' before actual Loop start ']'.");
					exit (-1);
				}
				CurrentLoop->BufferData[ptrloop] = (void*)NULL;				
				CurrentLoop->Size = ptrloop;
				
                LoopStatus = 0;

				i=GetInt(false);
				if( i==-1) i=1;
				if( i<1 || 127<i){
					error("\nInvalid loop count", true);
					i=1;
				}

				printf("\n--- End of loop segment %d (%d loops) ---\n", LoopCount, i);

				CurrentLoop->Count = i;

				fseek(out, CurrentLoop->SourceOffset, SEEK_SET);				

                fputc(0xEF, out);
                fputc(0x00, out); // this will be later be filled
                fputc(0x00, out);                                                
                fputc(CurrentLoop->Count, out);
                    
				continue;
			case '*':
				if (LoopCount == 0)
				{
					printf("nFatal Error: Loop command '*' used before assigning loop data.");
					exit(-1);

				}
				ptr++;
				LastNoteValue=-1;
				i=GetInt(false);
				if( i==-1) i=1;
				if( i<1 || 127<i){
					error("Invalid loop count", true);
					i=1;
				}

				LoopQt = i;

				printf("\nWriting Loop '*' (last loop) %d times", LoopQt);

		        fputc(0xEF,out);
		        CurrentLoop->AsteriskSourceOffset[AsteriskCount] = ftell(out);
		        WritePointer(0x0000, out); // * this will be later filled in */
		        fputc(LoopQt,out);
		        AsteriskCount++;
				
				continue;
			case 'p':{
				ptr++;
				i=GetInt(false);
				char temp=buf[ptr++];
				j=GetInt(false);
				continue;}
			case '{':
				ptr++;
				if(!triplet) triplet=true;
				else printf("\ntriplet-enable-command specified repeatedly");
				continue;
			case '}':
				ptr++;
				if(triplet) triplet=false;
				else printf("\ntriplet-disable-command specified repeatedly");
				continue;
			case '>':
				ptr++;
				if( ++octave > 7)
                {
					octave=7;
					printf("\nOctave is too high (octave = %d)", octave);
				}
				continue;
			case '<':
				ptr++;
				if( --octave < 0)
                {
					octave=0;
                    printf("\nOctave is too low (octave = %d)", octave);
				}
				continue;
			case '&':
				ptr++;
				continue;
			case '$':
				ptr++;
				i=GetHex(false);
				if( 0<=i && i<= 0xFF){
					my_fputc(i, out);
				}else error("Invalid hexadecimal value", true);
				continue;
				continue;
			case 'c':
			case 'C':
			case 'd':
			case 'D':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			case 'a':
			case 'A':
			case 'b':
			case 'B':
			case 'r':
			case 'R':
			case '^':
				j = buf[ptr++];
				i = (j=='r')?0xC9:
					(j=='^')?0xC8:GetPitch(j);
				if(i<0) i=0xC9; // in SMW, 0xc7
				
				j=GetNoteValue( GetInt(false) );

				if(j>=0x80){			//Note value was too long!
					my_fputc(0x60, out);
					
					my_fputc(i + GlobalTranspose,out);				//pitch
					j-=0x60;
					while(j>0x60){
						j-=0x60;
						my_fputc(0xC8,out);		//tie
					}
					if(j>0){
						if( j!=0x60) my_fputc(j, out);
						my_fputc(0xC8, out);
					}
					LastNoteValue=j;
					continue;
				}else if(j>0){			//note value needed to be updated
					my_fputc(j, out);
					LastNoteValue=j;
				}
				my_fputc(i + GlobalTranspose,out);					//pitch
				continue;
			case '\n':
				ptr++, line++;
				continue;
			case ';':
				while( buf[ptr]!='\n' && buf[ptr]!=NULL) ptr++;
				continue;
			default:
				ptr++;
				continue;
		}  //switch
	}    //while

	LoopStatus = 0;
	fputc(0x00, out);     
	
	/* write Tracks */
	
	for (i = 1; i <= num_ch; i++)
	{
		
		TrackData[i].TrackPointer = ftell(out) + NSPC_RAM_OFFSET - HEADER_LENGHT;

		printf ("\nChannel #%d Track Data Pointer: %x", TrackData[i].Channel, TrackData[i].MusicTrackPointer);

    }
 
 	if (LoopCount != 0) 
	{
		 
		CurrentLoop = FirstLoop->next;

		for (i = 1; i <= LoopCount; i++)
		{
            fseek(out, 0, SEEK_END);
            printf("\n--Loop %d--", i);
		    printf("\nSource: 0x%x", CurrentLoop->SourceOffset);
		    printf("\nCount: %d", CurrentLoop->Count);
		    printf("\nSize: %x", CurrentLoop->Size);
		    printf("\nLabel: %d", CurrentLoop->Label);		    
            CurrentLoop->TargetOffset = ftell(out) + NSPC_RAM_OFFSET - HEADER_LENGHT;
		    printf("\nTarget: 0x%x", CurrentLoop->TargetOffset);
		        
		    for (j = 0; j < CurrentLoop->Size; j++) 
		    {
  		        fputc(CurrentLoop->BufferData[j], out);
		    }

	        fputc(0x00, out); /* terminate loop */

		    fseek(out, CurrentLoop->SourceOffset, SEEK_SET);

		    fputc(0xEF,out);
		    WritePointer(CurrentLoop->TargetOffset, out);
		    fputc(CurrentLoop->Count,out);
		    
            /* deal with asterisks */		    
		    if (CurrentLoop->AsteriskSourceOffset[0] != 0)
		    {
               
                for ( k = 0; k < 100; k++)
                {
                    if (CurrentLoop->AsteriskSourceOffset[k] != 0)
                    {
                        fseek(out, CurrentLoop->AsteriskSourceOffset[k], SEEK_SET);
                        WritePointer(CurrentLoop->TargetOffset, out);
                    }
                    
                }
                
            }

	 	    CurrentLoop = CurrentLoop->next;   

        } /* for i */


    } /* if loop_count != 0 */
    
    if (LabeledLoops[0].Label == 0) goto Skip;
    
    /* Labeled Loops = warning, very inneficient! */
   	for (i = 0; i < 200; i++)
	{
        printf("\nLabeledLoops[%d].Label = %d", i, LabeledLoops[i].Label);
        printf("\nLabeledLoops[%d].SourceOffset = %x", i, LabeledLoops[i].SourceOffset);        

        if (LabeledLoops[i].Label == 0) goto Skip;
             
//            printf("\nLabeledLoops[%d].Label = %d", i, LabeledLoops[i].Label);             
//            printf("\nLabeledLoops[%d].SourceOffset = %x", i, LabeledLoops[i].SourceOffset);
		    CurrentLoop = FirstLoop->next;

		    for (k = 1; k <= LoopCount; k++)
		    {
              
//             printf("\nCurrentLoop->Label = %d, LabeledLoops[%d].Label = %d", CurrentLoop->Label, i, LabeledLoops[i].Label);
             if (CurrentLoop->Label == LabeledLoops[i].Label)
             {
                 fseek(out, LabeledLoops[i].SourceOffset, SEEK_SET);
                 WritePointer(CurrentLoop->TargetOffset, out);
             }
             CurrentLoop = CurrentLoop->next;       
            
            }
             
        }
    Skip:
         
    /* write header */
    fseek(out, 0, SEEK_END);
    FileSize2 = ( ftell(out) - HEADER_LENGHT );
    
    /* end SPC-RAM upload block */ 
    fputc(0x00, out);
    fputc(0x00, out);
    
    /* not sure why those are needed - according to smkdan, they are trashed anyway */
    fputc(0x00, out);
    fputc(0x08, out);
         
    fseek(out, 0, SEEK_SET);
    WritePointer(FileSize2, out); // ammount of data to be uploaded
    
    WritePointer(0xD000, out); // SPC-RAM destination
    
    // patterns
    
    WritePointer(0xD010, out);  // pattern #1
    
    if (DontLoop == 1) 
    {
        fputc(0x00, out);
        fputc(0x00, out);
    }
    else if (DontLoop == 0) 
    {
        fputc(0xff, out);
        fputc(0x00, out);
        WritePointer(0xD000, out);
    }
    
    fseek(out, 0x10 + HEADER_LENGHT, SEEK_SET);
    
    for (i = 1; i <= num_ch; i++)
    {
        fseek(out, 0x14 + (ch_tag[i] * 2), SEEK_SET);
        WritePointer( TrackData[i].MusicTrackPointer, out );
        
        printf("\nPointer 0x%x written at 0x%x [num_ch %d= ch_tag = %d]", TrackData[i].MusicTrackPointer, (0x14+ (ch_tag[i] * 2)), num_ch, ch_tag[i]);
    }

	printf("\n\n.bin file sucessfully written!");
	fclose(out);

	return;
}

int GetInt(bool msg)
{
	int i=0;	//number
	int d=0;	//digit

	while('0'<= buf[ptr] && buf[ptr] <= '9') d++, i = (i*10)+(buf[ptr++]-0x30);
	if(!d){
		if(msg) error("Invalid decimal number", true);
		return -1;
	}

	return i;
}

int GetHex(bool msg)
{
	int i=0;
	int d=0;
	int j;

	while(1){
		if('0'<=buf[ptr]&&buf[ptr]<='9') j=buf[ptr++]-0x30;
		else if('A'<=buf[ptr]&&buf[ptr]<='F') j=buf[ptr++]-0x37;
		else if('a'<=buf[ptr]&&buf[ptr]<='f') j=buf[ptr++]-0x57;
		else break;
		d++, i=(i*16)+j;
	}
	if(!d){
		if(msg) error("Invalid hexadecimal number", true);
		return -1;
	}
	return i;
}
int GetPitch(int i)
{   
	static const int Pitch[]={ 9, 11, 0, 2, 4, 5, 7 };

	i= Pitch[i-0x61] + (octave-1)*12 + 0x80;
	if(buf[ptr]=='+') i++, ptr++;
	else if(buf[ptr]=='-') i--, ptr++;

    if( i<0x80){
        printf("\nMusical interval is too low, i = %x", i);
		return -1;
       }
	else if(i>=0xc7){ // before 0xc6
		printf("\nMusical interval is too high, i = %x", i);
		return -1;  // before = -1
	}

	return i;
}
int GetNoteValue(int i)
{
	bool still = true;
	if(i==-1 && buf[ptr]=='='){
		ptr++;
		i=GetInt(false);
		if( 1<=i && i<=192 ) still=false;
	}
	if(still){
		if( !(1 <= i && i <= 192) ) i=DefNoteValue;
		i = 192/i;
		if(buf[ptr]=='.'){
			if(buf[++ptr]=='.') ptr++, i = i*7/4;
			else i = i*3/2;
		}
		if(triplet) i = i*2/3;
	}
	
	if(i==LastNoteValue) return 0;
	
	return i;
}
void error(const char *msg, bool show_line)
{

    printf("ERROR : ");
	if(show_line) printf("\n%4i: ", line);
	printf("%s", msg);
	return;

}

int main(int argc, char *argv[])
{
	
	if ( argc == 2)
	{
     
	    in_file = (char*)calloc(strlen(argv[1]), 1);
	    strcpy(in_file, argv[1]);

        out_file = (char*)calloc( strlen(argv[1]) + 4 + 1, 1);
  	    strcpy( out_file, argv[1] );
  	    
	    strcat( out_file, ".bin" );
	    printf("\nOutFile = %s", out_file);

        convert(in_file, 1, 1);

	}
	else
	{
	    printf("\n--> Usage: nspc file.mml");
        return -1;
	}
	
	return 0;
}
