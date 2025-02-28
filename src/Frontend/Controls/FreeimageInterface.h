#ifndef _FreeimageInterface_H_
#define _FreeimageInterface_H_
#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */

#define FIF_BITMAP24       1
#define FIF_BITMAP8        2
#define FIF_BITMAP8GRAY    3
#define FIF_JPEGCOLOR      4
#define FIF_GIFCOLOR       5
#define FIF_GIFGRAY        6
#define FIF_BITMAP32       7

//typedef struct tagBITMAPINFOHEADER{
//	unsigned int	biSize;
//	int             biWidth;
//	int             biHeight;
//	short           biPlanes;
//	short           biBitCount;
//	unsigned int    biCompression;
//	unsigned int    biSizeImage;
//	int             biXPelsPerMeter;
//	int             biYPelsPerMeter;
//	unsigned int    biClrUsed;
//	unsigned int    biClrImportant;
//} BITMAPINFOHEADER,*LPBITMAPINFOHEADER,*PBITMAPINFOHEADER;

//extern "C"
//{
//    byte* DLLIMPORT LoadBitmap32(char* filename,BITMAPINFOHEADER* bmih);
//}

#endif /* _FreeimageInterface_H_ */
