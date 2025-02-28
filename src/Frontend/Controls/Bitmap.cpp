#ifndef WINVER
#define WINVER 0x0500
#endif
#include <stdio.h>
#include <windows.h>
#include "fileio2.h"
#include "bitmap.h"
#include "math.h"

typedef byte* (*LoadBitmap32_PROCTYPE)(char* a_filename, BITMAPINFOHEADER* bmih);
typedef bool  (*SaveBitmap32_PROCTYPE)(char* a_filename, byte* BmBits, int width, int height, int ifif);
LoadBitmap32_PROCTYPE  LoadBitmap32_PROC = 0;
SaveBitmap32_PROCTYPE  SaveBitmap32_PROC = 0;

int** g_buff = 0;

void InitBitmapLib()
{
    if (CheckFile("Freeimage.dll"))
    {
        HMODULE hMod5 = LoadLibrary("FreeimageInterface.dll");
        if (hMod5)
        {
            LoadBitmap32_PROC = (LoadBitmap32_PROCTYPE) GetProcAddress(hMod5, TEXT  ("LoadPicture"));
            SaveBitmap32_PROC = (SaveBitmap32_PROCTYPE)GetProcAddress(hMod5, TEXT  ("SaveBitmapAny"));
        }
    }
}

inline void alphaBlend32(int* bg, const unsigned int src, int antialpha)
{
    if (antialpha < 0)
        antialpha = 0;
    if (antialpha > 255)
        antialpha = 255;
    unsigned int rb = (((src & 0x00ff00ff) * (0xff - antialpha)) + ((*bg & 0x00ff00ff) * antialpha)) & 0xff00ff00;
    unsigned int g  = (((src & 0x0000ff00) * (0xff - antialpha)) + ((*bg & 0x0000ff00) * antialpha)) & 0x00ff0000;
    *bg = (rb | g) >> 8;
}

bool aalinecheck32(int &x1, int &y1, int &x2, int &y2, int maxx, int maxy)
{
    float fx1 = x1;
    float fy1 = y1;
    float fx2 = x2;
    float fy2 = y2;
    float fmaxx = maxx;
    float fmaxy = maxy;
    float fxmarg = 5;
    float fymarg = 5;

    if ((fx1 < fxmarg) && (fx2 < fxmarg))
        return false;
    if ((fy1 < fymarg) && (fy2 < fymarg))
        return false;
    if ((fx1 >= fmaxx - fxmarg) && (fx2 >= fmaxx - fxmarg))
        return false;
    if ((fy1 >= fmaxy - fymarg) && (fy2 >= fmaxy - fymarg))
        return false;

    if (fx1 < fxmarg)
    {
        if (!(fx2 - fx1))
            return false;
        fy1 = fy2 - ((fx2 - fxmarg) * (fy2 - fy1) / (fx2 - fx1));
        fx1 = fxmarg;
    }
    if (fx2 < fxmarg)
    {
        if (!(fx1 - fx2))
            return false;
        fy2 = fy1 - ((fx1 - fxmarg) * (fy1 - fy2) / (fx1 - fx2));
        fx2 = fxmarg;
    }
    if (fy1 < fymarg)
    {
        if (!(fy2 - fy1))
            return false;
        fx1 = fx2 - ((fy2 - fymarg) * (fx2 - fx1) / (fy2 - fy1));
        fy1 = fymarg;
    }
    if (fy2 < fymarg)
    {
        if (!(fy1 - fy2))
            return false;
        fx2 = fx1 - ((fy1 - fymarg) * (fx1 - fx2) / (fy1 - fy2));
        fy2 = fymarg;
    }
    if (fx2 >= fmaxx - fxmarg)
    {
        if (!(fx2 - fx1))
            return false;
        fy2 = fy1 + (((fmaxx - fxmarg) - fx1) * (fy2 - fy1) / (fx2 - fx1));
        fx2 = fmaxx - fxmarg - 1;
    }
    if (fx1 >= fmaxx - fxmarg)
    {
        if (!(fx1 - fx2))
            return false;
        fy1 = fy2 + (((fmaxx - fxmarg) - fx2) * (fy1 - fy2) / (fx1 - fx2));
        fx1 = fmaxx - fxmarg - 1;
    }
    if (fy2 >= fmaxy - fymarg)
    {
        if (!(fy2 - fy1))
            return false;
        fx2 = fx1 + (((fmaxy - fymarg) - fy1) * (fx2 - fx1) / (fy2 - fy1));
        fy2 = fmaxy - fymarg - 1;
    }
    if (fy1 >= fmaxy - fymarg)
    {
        if (!(fy1 - fy2))
            return false;
        fx1 = fx2 + (((fmaxy - fymarg) - fy2) * (fx1 - fx2) / (fy1 - fy2));
        fy1 = fmaxy - fymarg - 1;
    }
    x1 = (int)fx1;
    y1 = (int)fy1;
    x2 = (int)fx2;
    y2 = (int)fy2;
    return true;
}

void AALine2(int x1, int y1, int x2, int y2, int color, int** rgbquads, int widthx, int heighty)
{
    int nadx = x2 - x1;
    int nady = y2 - y1;

    int adx = abs(nadx);
    int ady = abs(nady);

    int y   = y1;
    int x   = x1;

    if (adx > ady)
    {
        if (nadx < 0)
        {
            y1 = y2;
            y2 = y;
            y = y1;
            x1 = x2;
            x2 = x;
        }
    }
    else
    {
        if (nady < 0)
        {
            y1 = y2;
            y2 = y;
            y = y1;
            x1 = x2;
            x2 = x;
        }
    }

    if (!aalinecheck32(x1, y1, x2, y2, widthx, heighty))
        return;

    y   = y1;
    x   = x1;

    nadx = x2 - x1;
    nady = y2 - y1;

    int eps = 0;
    int sz  = 2048 * 1024 * 1;
    int invD;
    if (adx > ady)
        invD  = sz / adx;
    else
        invD  = sz / ady;
    int invD2du ;

    int yinc = 1;
    int xinc = 1;

    if (nady < 0)
        yinc = -1;
    if (nadx < 0)
        xinc = -1;

    if (adx > ady)
        invD2du = (int) (1.0 * ((float)adx * (1.0 / (1.0 * sqrt((float)adx * (float)adx + (float)ady * (float)ady)) * 1.0)) * (float)sz);
    else
        invD2du = (int) (1.0 * ((float)ady * (1.0 / (1.0 * sqrt((float)adx * (float)adx + (float)ady * (float)ady)) * 1.0)) * (float)sz);

    int enigma = 58 * 4;

    if (adx > ady)
    {
        for ( int x = x1; x <= x2; x++ )
        {
            int invdXeps = invD * eps;
            int antialpha;

            antialpha = (abs((invdXeps * enigma)) >> 22);
            alphaBlend32(&rgbquads[y][x], color, antialpha);

            antialpha = (abs((invD2du - invdXeps)) * enigma >> 22);
            alphaBlend32(&rgbquads[y + 1 * yinc][x], color, antialpha);

            antialpha = (abs((invD2du + invdXeps)) * enigma >> 22);
            alphaBlend32(&rgbquads[y - 1 * yinc][x], color, antialpha);

            antialpha = (abs((invD2du * 2 - invdXeps)) * enigma >> 22);
            alphaBlend32(&rgbquads[y + 2 * yinc][x], color, antialpha);

            antialpha = (abs((invD2du * 2 + invdXeps)) * enigma >> 22);
            alphaBlend32(&rgbquads[y - 2 * yinc][x], color, antialpha);

            if ((x != x1) && (x != x2))
            {
                antialpha = (abs((invD2du * 3 - invdXeps)) * enigma >> 22);
                alphaBlend32(&rgbquads[y + 3 * yinc][x], color, antialpha);

                antialpha = (abs((invD2du * 3 + invdXeps)) * enigma >> 22);
                alphaBlend32(&rgbquads[y - 3 * yinc][x], color, antialpha);
            }
            else
            {
                antialpha = 240;
                alphaBlend32(&rgbquads[y][x + 3 * xinc], color, antialpha);
                alphaBlend32(&rgbquads[y][x - 3 * xinc], color, antialpha);
            }

            eps += ady;
            if ((eps << 1) >= adx)
            {
                y += yinc;
                eps -= adx;
            }
        }
    }
    else // adx<=ady
    {
        for ( int y = y1; y <= y2; y++ )
        {
            int invdXeps = invD * eps;

            int antialpha = abs(invdXeps * enigma) >> 22;
            alphaBlend32(&rgbquads[y][x], color, antialpha);

            antialpha = abs(invD2du - invdXeps) * enigma >> 22;
            alphaBlend32(&rgbquads[y][x + 1 * xinc], color, antialpha);

            antialpha = abs(invD2du + invdXeps) * enigma >> 22;
            alphaBlend32(&rgbquads[y][x - 1 * xinc], color, antialpha);

            antialpha = abs(invD2du * 2 - invdXeps) * enigma >> 22;
            alphaBlend32(&rgbquads[y][x + 2 * xinc], color, antialpha);

            antialpha = abs(invD2du * 2 + invdXeps) * enigma >> 22;
            alphaBlend32(&rgbquads[y][x - 2 * xinc], color, antialpha);

            if ((y != y1) && (y != y2))
            {
                antialpha = abs(invD2du * 3 - invdXeps) * enigma >> 22;
                alphaBlend32(&rgbquads[y][x + 3 * xinc], color, antialpha);

                antialpha = abs(invD2du * 3 + invdXeps) * enigma >> 22;
                alphaBlend32(&rgbquads[y][x - 3 * xinc], color, antialpha);
            }
            else
            {
                antialpha = 240;
                alphaBlend32(&rgbquads[y][x + 3 * xinc], color, antialpha);
                alphaBlend32(&rgbquads[y][x - 3 * xinc], color, antialpha);
            }
            eps += adx;
            if ( (eps << 1 ) >= ady )
            {
                x += xinc;
                eps -= ady;
            }
        }
    }
}

void DrawBitmap32(HDC hdcWindow, HBITMAP hBMP, int posx, int posy, int w, int h, int alpha, bool stretch)
{
    BITMAP bmpp;
    GetObject(hBMP, sizeof(BITMAP), &bmpp);
    static int bmw, bmh;
    bmw = bmpp.bmWidth;
    bmh = bmpp.bmHeight;

    static HDC hdcMemory;
    hdcMemory = CreateCompatibleDC(hdcWindow);
    static HGDIOBJ oldbmp;
    if (hBMP)
        oldbmp = SelectObject(hdcMemory, hBMP);
    else
    {
        HBITMAP bmx = CreateCompatibleBitmap(hdcWindow, w, h);
        SelectObject(hdcMemory, bmx);
        HBRUSH brush = CreateSolidBrush(RGB(200, 200, 255));
        SelectObject(hdcMemory, brush );
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(30, 62, 90));
        SelectObject(hdcMemory, pen);
        Rectangle(hdcMemory, 0, 0, w, h);
        SetBkMode (hdcMemory, TRANSPARENT);
        TextOut(hdcMemory, 10, 10, "NO IMAGE TO DISPLAY", strlen("NO IMAGE TO DISPLAY"));
        DeleteObject(pen);
        DeleteObject(brush);
        DeleteObject(bmx);
        bmw = w;
        bmh = h;
    }

    static BLENDFUNCTION m_bf;
    m_bf.BlendOp = AC_SRC_OVER;
    m_bf.BlendFlags = 0;
    m_bf.SourceConstantAlpha = alpha;
    m_bf.AlphaFormat = 0;

    if (stretch)
    {
        if (alpha == 255)
            StretchBlt(hdcWindow, posx, posy,  w, h, hdcMemory, 0, 0, bmw, bmh, SRCCOPY);
        else
            AlphaBlend(hdcWindow, posx, posy,  w, h, hdcMemory, 0, 0, bmw, bmh, m_bf);
    }
    else
    {
        BitBlt(hdcWindow, posx, posy, w, h, hdcMemory, 0, 0, SRCCOPY);
    }

    if (hBMP)
        SelectObject(hdcMemory, oldbmp);
    DeleteDC(hdcMemory);
}

bool Bitmap32::LoadFreeimage(char* filename)
{
    bool result = false;
    byte* bmbits = 0;
    if (LoadBitmap32_PROC)
        bmbits = LoadBitmap32_PROC(filename, &Bmi.bmiHeader);
    if (bmbits)
    {
        Rebuild(0, bmbits);
        delete[]bmbits;
        result = true;
    }
    return result;
}

bool Bitmap32::LoadPicture (char* imagefile, bool forcefreeimage)
{
    HBITMAP hBMPloaded = 0;
    if (!forcefreeimage)
        hBMPloaded = (HBITMAP)LoadImage(GetModuleHandle(0), imagefile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (hBMPloaded)
    {
        BITMAP bmpp;
        GetObject(hBMPloaded, sizeof(BITMAP), &bmpp);
        ZeroMemory( &Bmi.bmiHeader, sizeof(BITMAPINFOHEADER) );
        Bmi.bmiHeader.biWidth = bmpp.bmWidth;
        Bmi.bmiHeader.biHeight = bmpp.bmHeight;
        Bmi.bmiHeader.biPlanes = bmpp.bmPlanes;
        Bmi.bmiHeader.biBitCount = 32;
        Bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        Rebuild ();
        DrawBitmap32(Hdc, hBMPloaded, 0, 0, bmpp.bmWidth, bmpp.bmHeight, 255, false);
        DeleteObject(hBMPloaded);
        return true;
    }
    else
        return LoadFreeimage(imagefile);
}

bool Bitmap32::SavePicture(char* a_filename, int format)
{
    bool result = false;
    if (Hdc)
        result = SaveBitmap32_PROC(a_filename, BmBits, Bmi.bmiHeader.biWidth, Bmi.bmiHeader.biHeight, format);
    return result;
}

bool Bitmap32::Rebuild (BITMAPINFOHEADER* inbmih, byte* inbmbits, int fill)
{
    static BITMAPINFOHEADER temih;

    if (!inbmih)
        memcpy(&temih, &Bmi.bmiHeader, sizeof(BITMAPINFOHEADER));

    DeinitializeObject();
    InitializeObject();
    if (!inbmih)
        memcpy(&Bmi.bmiHeader, &temih, sizeof(BITMAPINFOHEADER));

    if (!inbmih)
        inbmih = &temih;
    else
        memcpy(&Bmi.bmiHeader, inbmih, sizeof(BITMAPINFOHEADER));

    if (inbmih->biHeight * inbmih->biWidth == 0)
        return false;
    Hdc = CreateCompatibleDC(NULL);
    HBmpp = CreateDIBSection (Hdc, &Bmi, DIB_RGB_COLORS, (VOID**)&BmBits, NULL, 0);
    oldmbpp = (HBITMAP)SelectObject(Hdc, HBmpp);
    DeleteObject(HBmpp);
    BmRGBLayers   = new byte**[inbmih->biHeight];
    byte** rawdat = new byte* [inbmih->biHeight * inbmih->biWidth];

    for (int i = 0; i < inbmih->biHeight; i++)
    {
        static int i2;
        i2 = inbmih->biHeight - i - 1;
        BmRGBLayers[i] = &rawdat[i * inbmih->biWidth];
        for (int j = 0; j < inbmih->biWidth; j++)
            BmRGBLayers[i][j] = &BmBits[(i2 * inbmih->biWidth + j) << 2];
    }

    BmRGBQuads = new int*[inbmih->biHeight];
    for (int i = 0; i < inbmih->biHeight; i++)
        BmRGBQuads[inbmih->biHeight - i - 1] = (int*)&BmBits[i * inbmih->biWidth << 2];

    if (inbmbits)
        memcpy(BmBits, inbmbits, inbmih->biHeight * inbmih->biWidth * 4);

    if (fill != NOCOLOR)
        for (int i = 0; i < inbmih->biHeight; i++)
            for (int j = 0; j < inbmih->biWidth; j++)
                BmRGBQuads[i][j] = fill;
    return true;
}

void Bitmap32::Fill(int fill)
{
    if (fill == 0x00000000)
    {
        memset(BmBits, 0, Bmi.bmiHeader.biHeight * Bmi.bmiHeader.biWidth * 4);
        return;
    }
    if (fill == (int)0xffffffff)
    {
        memset(BmBits, 0xffffffff, Bmi.bmiHeader.biHeight * Bmi.bmiHeader.biWidth * 4);
        return;
    }
    for (int i = 0; i < Bmi.bmiHeader.biHeight; i++)
        for (int j = 0; j < Bmi.bmiHeader.biWidth; j++)
            BmRGBQuads[i][j] = fill;
}

void Bitmap32::InitializeObject()
{
    oldmbpp = 0;
    HBmpp = 0;
    Hdc = 0;
    BmBits = 0;
    BmRGBLayers = 0;
    BmRGBQuads = 0;
    ZeroMemory(&Bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
    Bmi.bmiHeader.biPlanes = 1;
    Bmi.bmiHeader.biBitCount = 32;
    Bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
}

HBITMAP Bitmap32::ToHBitmap()
{
    if (!Hdc)
        return 0;
    VOID* tbits;
    HBITMAP resultHBmp = CreateDIBSection (0, &Bmi, DIB_RGB_COLORS, &tbits, 0, 0);
    memcpy(tbits, BmBits, Bmi.bmiHeader.biWidth * Bmi.bmiHeader.biHeight * Bmi.bmiHeader.biBitCount / 8);
    return resultHBmp;
}

Bitmap32::Bitmap32()
{
    InitializeObject();
}

Bitmap32::Bitmap32(char* imagefile)
{
    InitializeObject();
    LoadPicture(imagefile);
}

Bitmap32::Bitmap32 (int width, int height, int fill, byte* inbmbits)
{
    InitializeObject();
    Rebuild(width, height, fill, inbmbits);
}

bool Bitmap32::Rebuild (int width, int height, int fill, byte* inbmbits)
{
    Bmi.bmiHeader.biWidth = width;
    Bmi.bmiHeader.biHeight = height;
    return Rebuild(0, inbmbits, fill);
}

void Bitmap32::DeinitializeObject()
{
    if (Hdc)
    {
        if (oldmbpp)
        {
            HBITMAP bmx2 = (HBITMAP )SelectObject(Hdc, oldmbpp);
            if (bmx2)
                bmx2 = bmx2;
            DeleteObject(HBmpp);
        }
        //DeleteDC(Hdc);
    }
    Hdc = 0;
    if (BmRGBLayers)
    {
        delete[]BmRGBLayers[0];
        delete[]BmRGBLayers;
    }
    BmRGBLayers = 0;
    if (BmRGBQuads)
        delete[]BmRGBQuads;
    BmRGBQuads = 0;
    BmBits = 0;
}

Bitmap32::~Bitmap32()
{
    DeinitializeObject();
}

void Bitmap32::Stretch(Bitmap32* dstbitmap, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int alpha, int transparentcolor)
{
    if (w == MAXIMUMSIZE)
        w = dstbitmap->Bmi.bmiHeader.biWidth - x;
    if (h == MAXIMUMSIZE)
        h = dstbitmap->Bmi.bmiHeader.biHeight - y;
    if (w <= 0)
        return;
    if (h <= 0)
        return;
    if (x + w > dstbitmap->Bmi.bmiHeader.biWidth)
        return;
    if (y + h > dstbitmap->Bmi.bmiHeader.biHeight)
        return;
    Stretch(dstbitmap->BmRGBQuads, x, y, w, h, sx, sy, sw, sh, alpha, transparentcolor);
}

void Bitmap32::Blt (Bitmap32* dstbitmap, int x, int y, int w, int h, int sx, int sy, int alpha, int transparentcolor)
{
    if (w == MAXIMUMSIZE || w == 0)
        w = dstbitmap->Bmi.bmiHeader.biWidth;
    if (h == MAXIMUMSIZE || h == 0)
        h = dstbitmap->Bmi.bmiHeader.biHeight;
    if (x + w > dstbitmap->Bmi.bmiHeader.biWidth)
        w = dstbitmap->Bmi.bmiHeader.biWidth - x;
    if (y + h > dstbitmap->Bmi.bmiHeader.biHeight)
        h = dstbitmap->Bmi.bmiHeader.biHeight - y;
    Blt (dstbitmap->BmRGBQuads, x, y, w, h, sx, sy, alpha, transparentcolor);
}

void Bitmap32::Blt (int** dst, int x, int y, int w, int h, int sx, int sy, int alpha, int transparentcolor)
{
    if (sx < 0)
        sx = 0;
    if (sy < 0)
        sy = 0;

    if (x < 0)
    {
        w += x;
        sx -= x;
        x = 0;
    }

    if (y < 0)
    {
        h += y;
        sy -= y;
        y = 0;
    }

    if (w <= 0)
        return;
    if (h <= 0)
        return;

    if (sx + w > Bmi.bmiHeader.biWidth)
        w = Bmi.bmiHeader.biWidth - sx;
    if (sy + h > Bmi.bmiHeader.biHeight)
        h = Bmi.bmiHeader.biHeight - sy;

    if (w <= 0)
        return;
    if (h <= 0)
        return;

    if (transparentcolor == NOCOLOR) /*-1*/
    {
        if (alpha == OPAQUEALPHA) /*255*/
        {
            for (int i = y; i < y + h; i++)
                memcpy(dst[i] + x, BmRGBQuads[sy + i - y] + sx, w * sizeof(int));
            return;
        }
        if (alpha != ALPHACHANNEL) /*-2*/
        {
            int rb;
            int g;
            for (int i = y; i < y + h; i++)
                for (int j = 0; j < w; j++)
                {
                    rb = (((BmRGBQuads[sy + i - y][sx + j] & 0x00ff00ff) * alpha) + ((dst[i][x + j] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                    g  = (((BmRGBQuads[sy + i - y][sx + j] & 0x0000ff00) * alpha) + ((dst[i][x + j] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                    dst[i][x + j] = (rb | g) >> 8;
                }
            return;
        }
        if (alpha == ALPHACHANNEL) /*-2*/
        {
            int rb;
            int g;
            for (int i = y; i < y + h; i++)
                for (int j = 0; j < w; j++)
                {
                    alpha  = BmRGBQuads[sy + i - y][sx + j] >> 24;
                    rb = (((BmRGBQuads[sy + i - y][sx + j] & 0x00ff00ff) * alpha) + ((dst[i][x + j] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                    g  = (((BmRGBQuads[sy + i - y][sx + j] & 0x0000ff00) * alpha) + ((dst[i][x + j] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                    dst[i][x + j] = (rb | g) >> 8;
                }
            return;
        }
    }
    else
    {
        if (alpha == OPAQUEALPHA) /*255*/
        {
            for (int i = y; i < y + h; i++)
                for ( int j = 0; j < w; j++)
                    if ((BmRGBQuads[sy + i - y][sx + j] & 0x00ffffff) != transparentcolor)
                        dst[i][x + j] = BmRGBQuads[sy + i - y][sx + j];
            return;
        }
        if (alpha != ALPHACHANNEL) /*-2*/
        {
            int rb;
            int g;
            for (int i = y; i < y + h; i++)
                for (int j = 0; j < w; j++)
                {
                    if ((BmRGBQuads[sy + i - y][sx + j] & 0x00ffffff) != transparentcolor)
                    {
                        rb = (((BmRGBQuads[sy + i - y][sx + j] & 0x00ff00ff) * alpha) + ((dst[i][x + j] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                        g  = (((BmRGBQuads[sy + i - y][sx + j] & 0x0000ff00) * alpha) + ((dst[i][x + j] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                        dst[i][x + j] = (rb | g) >> 8;
                    }
                }
            return;
        }
        if (alpha == ALPHACHANNEL) /*-2*/
        {
            int rb;
            int g;
            for (int i = y; i < y + h; i++)
                for (int j = 0; j < w; j++)
                {
                    if ((BmRGBQuads[sy + i - y][sx + j] & 0x00ffffff) != transparentcolor)
                    {
                        alpha  = BmRGBQuads[sy + i - y][sx + j] >> 24;
                        rb = (((BmRGBQuads[sy + i - y][sx + j] & 0x00ff00ff) * alpha) + ((dst[i][x + j] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                        g  = (((BmRGBQuads[sy + i - y][sx + j] & 0x0000ff00) * alpha) + ((dst[i][x + j] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                        dst[i][x + j] = (rb | g) >> 8;
                    }
                }
            return;
        }
    }
}

void Bitmap32::Stretch(int** dst, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int alpha, int transparentcolor)
{
    if (sw == MAXIMUMSIZE)
        sw = Bmi.bmiHeader.biWidth - sx;
    if (sh == MAXIMUMSIZE)
        sh = Bmi.bmiHeader.biHeight - sy;
    if (sw <= 0)
        return;
    if (sh <= 0)
        return;
    if (w <= 0)
        return;
    if (h <= 0)
        return;
    if (sx + sw > Bmi.bmiHeader.biWidth)
        return;
    if (sy + sh > Bmi.bmiHeader.biHeight)
        return;

    bool shwasbiggerthanh = true;
    if (sw < w)
    {
        sw--;
    }
    if (sh < h)
    {
        shwasbiggerthanh = false;
        sh--;
    }

    if (!dst || !BmRGBQuads)
        return;

    int yeps = 0;
    int say = sy;
    if (!g_buff)
    {
        int maxdim = w;
        if (maxdim < h)
            maxdim = h;
        if (maxdim < sw)
            maxdim = sw;
        if (maxdim < sh)
            maxdim = sh;
        if (maxdim < 10000)
            maxdim = 10000;

        g_buff = new int* [maxdim];
        for (int i = 0; i < maxdim; i++)
            g_buff[i] = new int [maxdim];
    }

    if (transparentcolor == NOCOLOR) /*-1*/
    {
        if (alpha == OPAQUEALPHA) /*255*/
        {
            float radx = 1;
            int antialphavector [w];
            int saxvector       [w];
            int epssignvector   [w];
            if (!shwasbiggerthanh)
                sh++;
            {
                float eps = 0;
                int sax = sx;
                float fsw = sw + (float)sw / (float)w;
                for (int ax = x; ax < x + w; ax++)
                {
                    if (eps)
                    {
                        if (eps > 0)
                        {
                            epssignvector[ax - x] = 1;
                        }
                        else
                        {
                            epssignvector[ax - x] = -1;
                        }
                    }
                    else
                        epssignvector[ax - x] = 0;

                    float val2 = eps / (float)w;
                    antialphavector[ax - x] = 255 - (int)(val2 * val2 * radx * 510.0f);

                    if (sax >= Bmi.bmiHeader.biWidth)
                        sax = Bmi.bmiHeader.biWidth - 1;
                    saxvector[ax - x] = sax;

                    eps += fsw;
                    while((eps * 2) >= w)
                    {
                        sax++;
                        eps -= w;
                    }
                }
            }

            int say = sy;
            for (int ay = sy; ay < sy + sh; ay++)
            {
                for (int ax = x; ax < x + w; ax++)
                {
                    int sax = saxvector[ax - x];
                    int alpha = antialphavector[ax - x];
                    int rb = (((BmRGBQuads[say][sax] & 0x00ff00ff) * (alpha)) + ((BmRGBQuads[say][sax + epssignvector[ax - x]] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                    int g  = (((BmRGBQuads[say][sax] & 0x0000ff00) * (alpha)) + ((BmRGBQuads[say][sax + epssignvector[ax - x]] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                    g_buff[ay][ax] = (rb | g) >> 8;
                }
                say++;
            }

            if (!shwasbiggerthanh)
                sh--;

            int antialphavector2 [h];
            int sayvector        [h];
            int epssignvector2   [h];

            {
                float eps = 0;
                int say = sy;
                float fsh = sh + (float)sh / (float)h;
                for (int ay = y; ay < y + h; ay++)
                {
                    if (eps)
                    {
                        if (eps > 0)
                        {
                            epssignvector2[ay - y] = 1;
                        }
                        else
                        {
                            epssignvector2[ay - y] = -1;
                        }
                    }
                    else
                        epssignvector2[ay - y] = 0;

                    float val2 = eps / (float)h;
                    antialphavector2[ay - y] = 255 - (int)(val2 * val2 * radx * 510.0f);

                    if (say >= Bmi.bmiHeader.biHeight)
                        say = Bmi.bmiHeader.biHeight - 1;
                    sayvector[ay - y] = say;
                    eps += fsh;
                    while((eps * 2) >= h)
                    {
                        say += 1;
                        eps -= h;
                    }
                }
            }

            int sax = sx;
            for (int ax = x; ax < x + w; ax++)
            {
                for (int ay = y; ay < y + h; ay++)
                {
                    int say = sayvector[ay - y];
                    int alpha = antialphavector2[ay - y];
                    int rb = (((g_buff[say][sax] & 0x00ff00ff) * (alpha)) + ((g_buff[say + epssignvector2[ay - y]][sax] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                    int g  = (((g_buff[say][sax] & 0x0000ff00) * (alpha)) + ((g_buff[say + epssignvector2[ay - y]][sax] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                    dst[ay][ax] = (rb | g) >> 8;
                }
                sax++;
            }


            return;
        }
        if (alpha != ALPHACHANNEL) /*-2*/
        {
            for (int ay = y; ay < y + h; ay++)
            {
                int eps = 0;
                int sax = sx;
                for (int ax = x; ax < x + w; ax++)
                {
                    int rb = (((BmRGBQuads[say][sax] & 0x00ff00ff) * alpha) + ((dst[ay][ax] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                    int g  = (((BmRGBQuads[say][sax] & 0x0000ff00) * alpha) + ((dst[ay][ax] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                    dst[ay][ax] = (rb | g) >> 8;

                    eps += sw;
                    while((eps << 1) >= w)
                    {
                        sax += 1;
                        eps -= w;
                    }
                }
                yeps += sh;
                while((yeps << 1) >= h)
                {
                    say += 1;
                    yeps -= h;
                }
            }
            return;
        }
        if (alpha == ALPHACHANNEL) /*-2*/
        {
            for (int ay = y; ay < y + h; ay++)
            {
                int eps = 0;
                int sax = sx;
                for (int ax = x; ax < x + w; ax++)
                {
                    alpha  = BmRGBQuads[say][sax] >> 24;
                    int rb = (((BmRGBQuads[say][sax] & 0x00ff00ff) * alpha) + ((dst[ay][ax] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                    int g  = (((BmRGBQuads[say][sax] & 0x0000ff00) * alpha) + ((dst[ay][ax] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                    dst[ay][ax] = (rb | g) >> 8;

                    eps += sw;
                    while((eps << 1) >= w)
                    {
                        sax += 1;
                        eps -= w;
                    }
                }
                yeps += sh;
                while((yeps << 1) >= h)
                {
                    say += 1;
                    yeps -= h;
                }
            }
            return;
        }
    }
    else
    {
        if (alpha == OPAQUEALPHA) /*255*/
        {
            for (int ay = y; ay < y + h; ay++)
            {
                int eps = 0;
                int sax = sx;
                for (int ax = x; ax < x + w; ax++)
                {
                    if ((BmRGBQuads[say][sax] & 0x00ffffff) != transparentcolor)
                        dst[ay][ax] = BmRGBQuads[say][sax];

                    eps += sw;
                    while((eps << 1) >= w)
                    {
                        sax += 1;
                        eps -= w;
                    }
                }
                yeps += sh;
                while((yeps << 1) >= h)
                {
                    say += 1;
                    yeps -= h;
                }
            }
            return;
        }
        if (alpha != ALPHACHANNEL) /*-2*/
        {
            for (int ay = y; ay < y + h; ay++)
            {
                int eps = 0;
                int sax = sx;
                for (int ax = x; ax < x + w; ax++)
                {
                    if ((BmRGBQuads[say][sax] & 0x00ffffff) != transparentcolor)
                    {
                        int rb = (((BmRGBQuads[say][sax] & 0x00ff00ff) * alpha) + ((dst[ay][ax] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                        int g  = (((BmRGBQuads[say][sax] & 0x0000ff00) * alpha) + ((dst[ay][ax] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                        dst[ay][ax] = (rb | g) >> 8;
                    }
                    eps += sw;
                    while((eps << 1) >= w)
                    {
                        sax += 1;
                        eps -= w;
                    }
                }
                yeps += sh;
                while((yeps << 1) >= h)
                {
                    say += 1;
                    yeps -= h;
                }
            }
            return;
        }
        if (alpha == ALPHACHANNEL) /*-2*/
        {
            for (int ay = y; ay < y + h; ay++)
            {
                int eps = 0;
                int sax = sx;
                for (int ax = x; ax < x + w; ax++)
                {
                    if ((BmRGBQuads[say][sax] & 0x00ffffff) != transparentcolor)
                    {
                        alpha  = BmRGBQuads[say][sax] >> 24;
                        int rb = (((BmRGBQuads[say][sax] & 0x00ff00ff) * alpha) + ((dst[ay][ax] & 0x00ff00ff) * (0xff - alpha))) & 0xff00ff00;
                        int g  = (((BmRGBQuads[say][sax] & 0x0000ff00) * alpha) + ((dst[ay][ax] & 0x0000ff00) * (0xff - alpha))) & 0x00ff0000;
                        dst[ay][ax] = (rb | g) >> 8;
                    }
                    eps += sw;
                    while((eps << 1) >= w)
                    {
                        sax += 1;
                        eps -= w;
                    }
                }
                yeps += sh;
                while((yeps << 1) >= h)
                {
                    say += 1;
                    yeps -= h;
                }
            }
            return;
        }
    }
}

int Bitmap32::GetSerializedLen ()
{
    int bufflen = 12;
    bufflen += sizeof(BITMAPINFO);
    if (Hdc)
        bufflen += Bmi.bmiHeader.biWidth * Bmi.bmiHeader.biHeight * Bmi.bmiHeader.biBitCount / 8;
    return bufflen;
}

byte* Bitmap32::Serialize  (unsigned int* nrbytes)
{
    int bufflen = Bitmap32::GetSerializedLen();

    byte* result = new byte [bufflen];
    *((int*)result) = bufflen;
    *((int*)(&result[4])) = Bmi.bmiHeader.biWidth;
    *((int*)(&result[8])) = Bmi.bmiHeader.biHeight;
    memcpy(result + 12, &Bmi, sizeof(BITMAPINFO));
    if (Hdc)
        memcpy(result + 12 + sizeof(BITMAPINFO), BmBits, Bmi.bmiHeader.biWidth * Bmi.bmiHeader.biHeight * Bmi.bmiHeader.biBitCount / 8);
    if (nrbytes)
        *nrbytes = bufflen;
    return result;
}

bool Bitmap32::Deserialize(byte* buffer, int* nrbytes)
{
    if (!buffer)
        return false;
    DeinitializeObject();

    memcpy(&Bmi, buffer + 12, sizeof(BITMAPINFO));
    Rebuild (0, &buffer[12 + sizeof(BITMAPINFO)]);
    if (nrbytes)
        *nrbytes = 12 + sizeof(BITMAPINFO) + Bmi.bmiHeader.biWidth * Bmi.bmiHeader.biHeight * Bmi.bmiHeader.biBitCount / 8;
    return true;
}

void Bitmap32::AALine(int x1, int y1, int x2, int y2, int color, int width)
{
    AALine2(x1, y1, x2, y2, color, BmRGBQuads, Bmi.bmiHeader.biWidth, Bmi.bmiHeader.biHeight);
}

WTransform::WTransform()
{
    WTIndexes = 0;
    WTResetObject();
}

void WTransform::WTResetObject()
{
    Transform.eM11 = 1;
    Transform.eM12 = 0;
    Transform.eM21 = 0;
    Transform.eM22 = 1;
    Transform.eDx = 0;
    Transform.eDy = 0;
    WTdstw = 0;
    WTdsth = 0;
    WTsrcw = 0;
    WTsrch = 0;
    WTTransparentColor = NOCOLOR;
    if (WTIndexes)
        delete[]WTIndexes;
    WTIndexes = 0;
}

WTransform::~WTransform()
{
    if (WTIndexes)
        delete[]WTIndexes;
}

int WTransform::GetSerializedLen ()
{
    int bufflen = sizeof(WTRANSFORM) + 8 * 4;
    if (WTIndexes)
        bufflen += WTdstw * WTdsth * 4;
    return bufflen;
}

byte* WTransform::Serialize (unsigned int* nrbytes)
{
    byte* buff = 0;
    int bufflen = WTransform::GetSerializedLen ();
    buff = new byte[bufflen];
    *((int*)buff) = bufflen;
    if (WTIndexes)
        *((int*)&buff[4]) = WTdstw * WTdsth * 4;
    else
        *((int*)&buff[4]) = 0;
    *((int*)&buff[8]) = WTdstw;
    *((int*)&buff[12]) = WTdsth;
    *((int*)&buff[16]) = WTsrcw;
    *((int*)&buff[20]) = WTsrch;
    *((int*)&buff[24]) = WTTransparentColor;
    *((int*)&buff[28]) = 0; //reserved for future use
    memcpy(buff + 32, &Transform, sizeof(WTRANSFORM));
    if (WTIndexes)
        memcpy(buff + 32 + sizeof(WTRANSFORM), WTIndexes, WTdstw * WTdsth * 4);
    if (nrbytes)
        *nrbytes = bufflen;
    return buff;
}

bool WTransform::Deserialize (byte* buffer, int* nrbytes)
{
    if (!buffer)
    {
        WTResetObject();
        return false;
    }
    int WTIndexeslen = *((int*)&buffer[4]);
    WTdstw = *((int*)&buffer[8]);
    WTdsth = *((int*)&buffer[12]);
    WTsrcw = *((int*)&buffer[16]);
    WTsrch = *((int*)&buffer[20]);
    WTTransparentColor = *((int*)&buffer[24]);
    memcpy(&Transform, buffer + 32, sizeof(WTRANSFORM));
    if (WTIndexes)
        delete[] WTIndexes;
    if (WTIndexeslen)
    {
        WTIndexes = new int[WTIndexeslen / 4];
        memcpy(WTIndexes, buffer + 32 + sizeof(WTRANSFORM), WTIndexeslen);
    }
    if (nrbytes)
        *nrbytes = 32 + sizeof(WTRANSFORM) + WTIndexeslen;
    return true;
}
