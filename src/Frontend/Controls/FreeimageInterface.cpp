#include "FreeimageInterface.h"
#include <windows.h>
#include "freeimage.h"
extern "C"
{
    BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                           DWORD reason        /* Reason this function is being called. */ ,
                           LPVOID reserved     /* Not used. */ )
    {
        switch (reason)
        {
          case DLL_PROCESS_ATTACH:
            break;
    
          case DLL_PROCESS_DETACH:
            break;
    
          case DLL_THREAD_ATTACH:
            break;
    
          case DLL_THREAD_DETACH:
            break;
        }
    
        /* Returns TRUE on success, FALSE on failure */
        return TRUE;
    }
    
    
    //byte* DLLIMPORT LoadBitmap32(char* filename,BITMAPINFOHEADER* bmih)
    byte* DLLIMPORT LoadBitmap32(char* filename,int* width,int* height)
    {
        byte* result=0;
        if(!filename)
            return result;        
        FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
        fif = FreeImage_GetFileType(filename, 0);
        
        if(fif == FIF_UNKNOWN)
            fif = FreeImage_GetFIFFromFilename(filename);

        if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif))
        {
            FIBITMAP *dib = FreeImage_Load(fif, filename,0);//, flag);
            if(dib)
            {
                FIBITMAP *dib2=FreeImage_ConvertTo32Bits(dib);
                BITMAPINFOHEADER* dibbitinf=FreeImage_GetInfoHeader(dib2);

                byte* dibbytes=FreeImage_GetBits(dib2);
                if(dibbytes)
                {
                    if(width)
                        *width=dibbitinf->biWidth;
                    if(height)
                        *height=dibbitinf->biHeight;
//                        bmih->biHeight=dibbitinf->biHeight;
//                        bmih->biWidth=dibbitinf->biWidth;
                    result=new byte[dibbitinf->biWidth*dibbitinf->biHeight*4];
                    memcpy(result,dibbytes,dibbitinf->biWidth*dibbitinf->biHeight*4);
                }                
        
                FreeImage_Unload(dib);
                FreeImage_Unload(dib2);
            }
        }
        return result;
    }
    
    byte* DLLIMPORT LoadPicture(char* filename,BITMAPINFOHEADER* bmih)
    {
        byte* result=0;
        FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
//        fif = FreeImage_GetFileType(filename, 0);
        
        if(fif == FIF_UNKNOWN)
            fif = FreeImage_GetFIFFromFilename(filename);
        
        if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif))
        {
        
            FIBITMAP *dib = FreeImage_Load(fif, filename,0);//, flag);
            if(dib)
            {
                FIBITMAP *dib2=FreeImage_ConvertTo32Bits(dib);
                BITMAPINFOHEADER* dibbitinf=FreeImage_GetInfoHeader(dib2);
                               
                byte* dibbytes=FreeImage_GetBits(dib2);
                if(dibbytes)
                {
                    bmih->biHeight=dibbitinf->biHeight;
                    bmih->biWidth=dibbitinf->biWidth;                            
                    result=new byte[dibbitinf->biWidth*dibbitinf->biHeight*4];
                    memcpy(result,dibbytes,dibbitinf->biWidth*dibbitinf->biHeight*4);
                }                
        
                FreeImage_Unload(dib);
                FreeImage_Unload(dib2);
            }
        }
        return result;
    }    

//    byte* LoadPicture (char* imagefile,int* width,int* height)
//    {
////        BITMAPINFOHEADER Bmih;
////        ZeroMemory( &Bmih, sizeof(BITMAPINFOHEADER) );
////        Bmih.biWidth=width;
////        Bmih.biHeight=height;
////        Bmih.biPlanes=1;
////        Bmih.biBitCount=32;
////        Bmih.biSize=sizeof(BITMAPINFOHEADER);        
////        byte* result=LoadBitmap32(imagefile,&Bmih);
////        return result;
//    }

    //bool DLLIMPORT SaveBitmapAny(char* filename,byte* BmBits, BITMAPINFOHEADER* bmih,int ifif)
    bool DLLIMPORT SaveBitmapAny(char* filename,byte* BmBits, int width,int height,int ifif)    
    {    
        if(!filename) 
            return 0;
        FREE_IMAGE_FORMAT fif=FIF_JPEG;
        if(ifif==FIF_BITMAP24)
            fif=FIF_BMP;
        if(ifif==FIF_BITMAP32)
            fif=FIF_BMP;
        if(ifif==FIF_BITMAP8)
            fif=FIF_BMP;
        if(ifif==FIF_BITMAP8GRAY)
            fif=FIF_BMP;
        if(ifif==FIF_JPEGCOLOR)
            fif=FIF_JPEG;
        if(ifif==FIF_GIFCOLOR)
            fif=FIF_GIF;
        if(ifif==FIF_GIFGRAY)
            fif=FIF_GIF;                              
                
        bool result=false; 
        FIBITMAP *dib = FreeImage_ConvertFromRawBits(BmBits, width,height,width*4,32, 0x000000ff, 0x0000ff00,0x00ff0000, 1 );//=0
        
        if(dib)
        {               
            FIBITMAP *dib32=0;
            FIBITMAP *dib24=0;
            FIBITMAP *dib8=0;
            FIBITMAP *dibtosave;     //reference only
            if(ifif==FIF_BITMAP32)
            {
                dibtosave=dib;
            }
            else
            {
                if((ifif!=FIF_BITMAP8GRAY)&&(ifif!=FIF_GIFGRAY))
                    dib24=FreeImage_ConvertTo24Bits(dib);
                dibtosave=dib24;
                if((ifif==FIF_BITMAP8)||(ifif==FIF_GIFCOLOR))
                {
                    dib8=FreeImage_ColorQuantize(dib24,FIQ_WUQUANT);
                    dibtosave=dib8;
                }
                if((ifif==FIF_BITMAP8GRAY)||(ifif==FIF_GIFGRAY))
                {
                    dib8=FreeImage_ConvertTo8Bits(dib);
                    dibtosave=dib8;
                }            
            }
            result=FreeImage_Save(fif, dibtosave, filename,0);
            
            if(dib32)
                FreeImage_Unload(dib32);
            if(dib24)
                FreeImage_Unload(dib24);
            if(dib8)
                FreeImage_Unload(dib8);
            FreeImage_Unload(dib);            
        }
        return result;
    }    
}
