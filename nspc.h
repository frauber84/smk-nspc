#define SWAP(a,b)   { int t; t=a; a=b; b=t; }  /* Macro for Bubble Sort */

#define FIRST_TRACK             0x60
#define TRACK_LENGHT            0x12
#define DEFAULT_VELOCITY        0x54
#define DEFAULT_GLOBAL_VOLUME   0x70
#define DEFAULT_TRACK_VOLUME    0x70
#define DEFAULT_GATE_TIME       0x20
#define DEFAULT_TEMPO           120
#define DEFAULT_PAN             0x7f
#define MAX_LOOP_DATA           0x1000
#define MAX_CH                  16  /* old */

/* Zelda specific */
#define ZELDA_GLOBAL_VOLUME     0x50
#define ZELDA_TRACK_VOLUME      0x50


#define false 0
#define true (!false)
typedef unsigned int bool;

struct Track
{
    int TrackPointer;        /* track-data pointer (in header) */
    int MusicTrackPointer;   /* muisic-track-data pointer (in track data) */
    int Timestamp;
    int Instrument;
    int Channel;
    signed char Pan;
    int Volume;
    int CurVelocity;
    int CurGateTime;
    int CurVibrato;          /* 0xd8 command (not inplemented yet) */
} TrackData[16];

struct Loop
{
    char   *BufferData;
    int    SourceOffset;
    int    Count;
	int    Size;
	unsigned short AsteriskSourceOffset[100];
    int    TargetOffset;   /* old  0xFC jump address */
    int    Label;
    struct Loop *next; 
} *FirstLoop, *CurrentLoop;

struct LabelLoop
{
    int  Label;
    int  SourceOffset;
} LabeledLoops[200];


/* More loop stuff */

int LoopStatus = 0;
int LoopCount = 0;
int AsteriskCount = 0;
unsigned int ptrloop = 0;

int ChanFlagBits[16] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4192,8192,16384, 32768};

char * out_file;
char * in_file;

int HighestTimestamp[16];

// global settings

int GlobalTranspose = 0;


/* n-spc-specific functions */
void bubble_srt( int a[], int n );
int AddLoop();
void my_fputc(int _ch, FILE *fptr);
void Write16(int u16, FILE *fp);
void WriteTimestamp(unsigned int timestamp, FILE *fp);

/* Original Addmusic functions */
int convert(const char *FileName, const int MusicNum, int Num);
void mml_to_nspc(void);
int GetInt(bool msg);
int GetHex(bool msg);
int GetPitch(int i);
int GetNoteValue(int i);
void error(const char *msg, bool show_line);

FILE *ROMfp;
char *ROM;
unsigned int ROMsize;
char *ROMname;
char *IniPath;

unsigned int FileSize;	//txt file size
char *buf;		//txt buffer
unsigned int ptr;
unsigned int line;

unsigned int ch;
int LastNoteValue;
int DefNoteValue;
bool triplet;
int octave;

