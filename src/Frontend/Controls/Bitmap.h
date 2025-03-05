#ifndef BITMAP_H_
#define BITMAP_H_

#define MAXIMUMSIZE -1
#define MINIMUMSIZE -2

#define NOCOLOR            -2
#define ALPHACHANNEL       -2
#define OPAQUEALPHA        255

#define FIF_BITMAP24       1
#define FIF_BITMAP8        2
#define FIF_BITMAP8GRAY    3
#define FIF_JPEGCOLOR      4
#define FIF_GIFCOLOR       5
#define FIF_GIFGRAY        6
#define FIF_BITMAP32       7

void InitBitmapLib();

class  Bitmap32: public IFileIO
{
    HBITMAP HBmpp;
    HBITMAP oldmbpp;
    void    InitializeObject  ();
    void    DeinitializeObject();
    bool    LoadFreeimage(char* filename);

public:
    /*------------These variables are references only. They are public
    because we need direct access, so you may change the data they are
    pointing to, but don't even think about rebuilding them!!!------*/
    HDC        Hdc;
    BITMAPINFO Bmi;
    unsigned char*      BmBits;
    unsigned char***    BmRGBLayers;
    int**      BmRGBQuads;
    /*--------------------------------------------------------------*/
    Bitmap32   (char* imagefile);
    void       Blt       (int** dst, int x, int y, int w, int h, int sx = 0, int sy = 0, int alpha = OPAQUEALPHA, int transparentcolor = NOCOLOR);
    void       Blt       (Bitmap32* dstbitmap, int x = 0, int y = 0, int w = MAXIMUMSIZE, int h = MAXIMUMSIZE, int sx = 0, int sy = 0, int alpha = OPAQUEALPHA, int transparentcolor = NOCOLOR);
    void       Stretch   (int** dst, int x, int y, int w, int h, int sx = 0, int sy = 0, int sw = MAXIMUMSIZE, int sh = MAXIMUMSIZE, int alpha = OPAQUEALPHA, int transparentcolor = NOCOLOR);
    void       Stretch   (Bitmap32* dstbitmap, int x = 0, int y = 0, int w = MAXIMUMSIZE, int h = MAXIMUMSIZE, int sx = 0, int sy = 0, int sw = MAXIMUMSIZE, int sh = MAXIMUMSIZE, int alpha = OPAQUEALPHA, int transparentcolor = NOCOLOR);
    void       AALine    (int x1, int y1, int x2, int y2, int color, int width = 3);
    virtual ~Bitmap32  ();
    Bitmap32   ();
    Bitmap32   (int width, int height, int fill = NOCOLOR, unsigned char* inbmbits = 0);
    bool       Rebuild (BITMAPINFOHEADER* inbmih = 0, unsigned char* inbmbits = 0, int fill = NOCOLOR);
    bool       Rebuild2(BITMAPINFOHEADER* inbmih = 0, unsigned char* inbmbits = 0, int fill = NOCOLOR);
    void       Fill(int fill = NOCOLOR);
    bool       Rebuild (int width, int height, int fill = NOCOLOR, unsigned char* inbmbits = 0);
    bool       LoadPicture (char* imagefile, bool forcefreeimage = false);
    HBITMAP    ToHBitmap();
    virtual    int   GetSerializedLen ();
    virtual    unsigned char* Serialize        (unsigned int* nrbytes = 0);
    virtual    bool  Deserialize      (unsigned char* buffer, int* nrbytes = 0);
    bool       SavePicture(char* a_filename, int format = FIF_JPEGCOLOR);
};

typedef struct _WTRANSFORM
{
    float eM11;
    float eM12;
    float eM21;
    float eM22;
    float eDx;
    float eDy;
} WTRANSFORM;

class WTransform: public IFileIO
{
public:
    WTRANSFORM Transform;
    int WTdstw;
    int WTdsth;
    int WTsrcw;
    int WTsrch;
    int WTTransparentColor;
    int* WTIndexes;
    WTransform();
    void WTResetObject();
    virtual ~WTransform();
    int GetSerializedLen ();
    unsigned char* Serialize (unsigned int* nrbytes = 0);
    bool Deserialize (unsigned char* buffer, int* nrbytes = 0);
};

#endif // BITMAP_H_
