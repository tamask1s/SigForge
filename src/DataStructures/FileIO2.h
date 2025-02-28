#ifndef FILEIO2_H_
#define FILEIO2_H_

#ifndef byte
#define byte unsigned char
#endif

#ifndef UInt32
//#define UInt32 unsigned int
#endif

#define ENDOFFILE -1  //appends always to the end of file
#define CONTINUE  -2  //continues writing from the last position
#define WHOLEFILE -1  //loads the whole file into the buffer (from offset)

bool   CheckFile (const char* Filename);
int    GetFileLen(const char* Filename);

int    SaveBuffer(const char Filename[], byte* buffer, int length, int offset = 0, bool rewrite = false); //ENDOFFILE/OFFSET
int    SaveBuffer(FILE* f, byte* buffer, int length, int offset = ENDOFFILE); //ENDOFFILE/CONTINUE/OFFSET

byte*  LoadBuffer(const char Filename[], int offset = 0, int* nrofbytesreaded = 0, int nrbytestoread = WHOLEFILE); //WHOLEFILE/NROFBYTESTOREAD
byte*  LoadBuffer(FILE* f, int offset = 0, int* nrofbytesreaded = 0, int nrbytestoread = WHOLEFILE); //WHOLEFILE/NROFBYTESTOREAD

byte   LoadByte  (FILE* f, int offset);
int    LoadInt   (FILE* f, int offset);
unsigned int LoadUInt  (FILE* f, int offset);

class IFileIO
{
public:
    virtual int   GetSerializedLen  () = 0;
    virtual byte* Serialize         (unsigned int* nrbytes = 0) = 0;
    virtual bool  Deserialize       (byte* buffer, int* nrbytes = 0) = 0;

    int LoadFromFile     (char Filename[], int offset = 0, int*totalstreamlen = 0);
    int LoadFromFile     (FILE*f, int offset = 0, int*totalstreamlen = 0);

    int SaveToFile       (char Filename[], int offset = 0, bool rewrite = false, int packer = 0, int additionalbytes = 0); //ENDOFFILE/OFFSET
    int SaveToFile       (FILE*f, int offset = ENDOFFILE, int packer = 0, int additionalbytes = 0); //ENDOFFILE/CONTINUE/OFFSET

    void Equal           (IFileIO& fioin);
};

class CBuffer
{
    int offset;
    int bufferlen;
    bool buffergetted;
    byte* buffer;

public:
    CBuffer();
    void Deinitialize();
    ~CBuffer();
    void Create(int nrelements);
    bool CatBuffer(byte* bufftoadd, int bufftoaddlen);
    bool CatBuffer(IFileIO* inbuffer);
    bool Finished();
    byte* GetBuffer();
};

#endif // FILEIO2_H_
