#ifndef WINVER
#define WINVER 0x0500
#endif
#include <stdio.h>
#include <windows.h>
#include <shlobj.h>
#include <commctrl.h>
#include <string>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "bitmap.h"
#include "Defines.h"
#include "Control_Base.h"
#include "CommonVisualComponents.h"
#include "math.h"

#define IFVERT (Orientation&VERTICAL)?

void DrawBitmap(HDC hdcWindow, HBITMAP hBMP, int posx, int posy, int w, int h, int alpha = 255, HBRUSH brush = 0)
{
    BITMAP bmpp;
    GetObject(hBMP, sizeof(BITMAP), &bmpp);
    static int bmw, bmh;
    bmw = bmpp.bmWidth;
    bmh = bmpp.bmHeight;

    static HDC hdcMemory;
    hdcMemory = CreateCompatibleDC(hdcWindow);
    static HGDIOBJ oldbmp;
    HBITMAP bmx = 0;
    if (hBMP)
        oldbmp = SelectObject(hdcMemory, hBMP);
    else
    {
        bmx = CreateCompatibleBitmap(hdcWindow, w, h);
        oldbmp = (HBITMAP)SelectObject(hdcMemory, bmx);
        HBRUSH oldbrush = 0;
        if (brush)
            oldbrush = (HBRUSH)SelectObject(hdcMemory, brush);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(3, 12, 40));
        HPEN oldpen = (HPEN)SelectObject(hdcMemory, pen);
        Rectangle(hdcMemory, 0, 0, w, h);
        SelectObject(hdcMemory, oldpen);
        if (oldbrush)
            SelectObject(hdcMemory, oldbrush);
        DeleteObject(pen);
        bmw = w;
        bmh = h;
    }

    static BLENDFUNCTION m_bf;
    m_bf.BlendOp = AC_SRC_OVER;
    m_bf.BlendFlags = 0;
    m_bf.SourceConstantAlpha = alpha;
    m_bf.AlphaFormat = 0;

    if (alpha == 255)
    {
        SetStretchBltMode(hdcWindow, STRETCH_HALFTONE);
        StretchBlt(hdcWindow, posx, posy,  w, h, hdcMemory, 0, 0, bmw, bmh, SRCCOPY);
    }
    else
    {
        SetStretchBltMode(hdcWindow, STRETCH_HALFTONE);
        AlphaBlend(hdcWindow, posx, posy,  w, h, hdcMemory, 0, 0, bmw, bmh, m_bf);
    }

    if (hBMP)
        SelectObject(hdcMemory, oldbmp);
    if (bmx)
        DeleteObject(bmx);
    DeleteDC(hdcMemory);
}

CBitButton::CBitButton(int posx, int posy, int width, int height, const char* caption, CControl_Base* ParentObject, const char* bmp, const char* bmpdown, const char* bmphl, int transpar, int a_EID)
    : CControl_Base(0, 0, caption, 0, posx, posy, width, height, ParentObject, false, 0, 0, 0, false, a_EID)
{
    tristate = false;
    m_white_brush = 0;
    m_white_pen = 0;
    m_blue_brush = 0;
    ButtonFont = 0;

    TrackingEnabled = true;
    BtnState = 0;
    bkgrnd = 0;
    hBMP = 0;
    hBMPdown = 0;
    hBMPhl = 0;
    if (bmp)
        hBMP = (HBITMAP)LoadImage(GetModuleHandle(0), bmp, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (bmpdown)
        hBMPdown = (HBITMAP)LoadImage(GetModuleHandle(0), bmpdown, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (bmphl)
        hBMPhl = (HBITMAP)LoadImage(GetModuleHandle(0), bmphl, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    if ((!hBMPdown) && (!hBMPdown))
    {
        Transpareny[1] = 170;
        Transpareny[0] = 230;
        Transpareny[2] = 210;

        if (hBMP)
        {
            HDC hdc = GetDC(hWnd);
            RECT rc;
            GetClientRect(hWnd, &rc);
            bkgrnd = CreateCompatibleDC(hdc);
            HBITMAP bmp1 = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(bkgrnd, bmp1);
            DrawBitmap(bkgrnd, hBMP, 0, 0, rc.right, rc.bottom);
            DeleteObject(bmp1);
        }
    }
    else
    {
        Transpareny[0] = transpar;
        Transpareny[1] = transpar;
        Transpareny[2] = transpar;
    }
    if (!hBMP)
    {
        Transpareny[1] = 90;
        Transpareny[0] = 230;
        Transpareny[2] = 110;
    }

    m_white_brush = CreateSolidBrush(RGB(255, 255, 255));
    m_white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    m_blue_brush = CreateSolidBrush(RGB(182, 189, 210));

    ButtonFont = CreateFont(14, 5, 0, 0, 700, 0, 0, 0, EASTEUROPE_CHARSET, 255, 255, /*19*/PROOF_QUALITY, 0, 0);

    Refresh();
}

void CBitButton::Obj_OnResize(int x, int y, int width, int height)
{
    Refresh();
    return CControl_Base::Obj_OnResize(x, y, width, height);
}

CBitButton::~CBitButton()
{
    if (bkgrnd)
        DeleteDC(bkgrnd);
    DeleteObject(hBMP);
    DeleteObject(hBMPdown);
    DeleteObject(hBMPhl);
    DeleteObject(m_white_brush);
//    DeleteObject(m_white_pen);
    DeleteObject(m_blue_brush);
    DeleteObject(ButtonFont);
}

void CBitButton::Obj_OnMouseDown(int button, int x, int y, int client)
{
    CControl_Base::Obj_OnMouseDown(button, x, y, client);
    SetCapture(hWnd);
    BtnState = 1;
    Refresh();
}

void CBitButton::Obj_OnMouseUp(int button, int x, int y, int client)
{
    CControl_Base::Obj_OnMouseUp(button, x, y, client);
    BtnState = 0;
    ReleaseCapture();
    Refresh();
}

void CBitButton::Obj_OnMouseMove(int button, int x, int y)
{
    CControl_Base::Obj_OnMouseMove(button, x, y);
    int temstate;
    if (IsCursorOnWindow())
        temstate = 1;
    else
        temstate = 0;
    if (MouseIsOverControl != temstate)
    {
        MouseIsOverControl = temstate;
        Refresh();
    }
}

void CBitButton::RebuildBkg(HDC hdc, int px, int py)
{
    RECT rc;
    GetClientRect(hWnd, &rc);

    if (hdc)
    {
        if (!bkgrnd)
        {
            bkgrnd = CreateCompatibleDC(hdc);
            HBITMAP bmp1 = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(bkgrnd, bmp1);
            BitBlt(bkgrnd, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
            DeleteObject(bmp1);
        }
        BitBlt(bkgrnd, 0, 0, rc.right, rc.bottom, hdc, px, py, SRCCOPY);
    }
    else
    {
        if (bkgrnd)
            DeleteDC(bkgrnd);
        bkgrnd = 0;
    }
    Refresh();
}

int CBitButton::Obj_OnEraseBkgnd (HDC hdc)
{
    RECT rc;
    GetClientRect(hWnd, &rc);

    HBRUSH olbdbrush = 0;
    HPEN olbdpen = 0;
    if (bkgrnd)
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, bkgrnd, 0, 0, SRCCOPY);
    else
    {
        olbdbrush = (HBRUSH)SelectObject(hdc, m_white_brush);
        olbdpen = (HPEN)SelectObject(hdc, m_white_pen);
        Rectangle(hdc, -1, -1, rc.right + 1, rc.bottom + 1);
    }
    int dxy = 0;
    if (MouseIsOverControl)
    {
        if (BtnState)
        {
            DrawBitmap(hdc, hBMPdown, 0, 0, rc.right, rc.bottom, Transpareny[0], m_blue_brush);
            dxy = 1;
        }
        else
        {
            if (tristate == 2)
                DrawBitmap(hdc, hBMP, 0, 0, rc.right, rc.bottom, Transpareny[2], m_blue_brush);
            else
                DrawBitmap(hdc, hBMPhl, 0, 0, rc.right, rc.bottom, Transpareny[1], m_blue_brush);
        }
    }
    else if (BtnState)
        DrawBitmap(hdc, hBMPhl, 0, 0, rc.right, rc.bottom, Transpareny[1], m_blue_brush);
    else
        DrawBitmap(hdc, hBMP, 0, 0, rc.right, rc.bottom, Transpareny[2], m_blue_brush);

    char tx1[200];
    GetWindowText(hWnd, tx1, 200);
    SetBkMode (hdc, TRANSPARENT);
    HFONT olbfont = (HFONT)SelectObject(hdc, ButtonFont);
    SetTextColor(hdc, RGB(40, 50, 120));
    TextOut(hdc, rc.right / 2 - strlen(tx1) * 7 / 2 + dxy - 0, rc.bottom / 2 - 8 + dxy, tx1, strlen(tx1));
    SelectObject(hdc, olbfont);
    if (olbdbrush)
    {
        SelectObject(hdc, olbdbrush);
        SelectObject(hdc, olbdpen);
    }
    return 1;
}

int NrCombinations(int n, int r)
{
    int result = 0;
    if (--r - 1 < 0)
        return 1;
    if (r - 1)
        for (int i = 0; i < n + 1; i++)
            result += NrCombinations(n - i, r);
    else
        result += n + 1;
    return result;
}

int** makeintmx(int height, int width)
{
    if (!width | !height)
        return 0;
    int** result = new int*[height];
    int*  rawdat = new int [height * width];
    for (int i = 0; i < height; i++)
        result[i] = &rawdat[i * width];
    return result;
}

void deleteintmx(int** intmx)
{
    if (intmx)
    {
        delete[] intmx[0];
        delete[] intmx;
    }
}

int FillCombs(int n, int r, int** rows2, int nn = -1)
{
    static int rows[10];
    static int rowscount = 0;
    static int rr = 0;
    if (nn == -1)
    {
        rowscount = 0;
        nn = n;
        rr = r;
        for (int x = 0; x < r; x++)
            rows[x] = 0;
    }
    int result = 0;
    if (--r - 1 < 0)
    {
        rows2[0][0] = nn;
        return 1;
    }
    if (r - 1)
        for (int i = 0; i < n + 1; i++)
        {
            rows[r - 1] = i;
            result += FillCombs(n - i, r, rows2, nn);
        }
    else
    {
        result += n + 1;
        for (int x = 0; x < n + 1; x++)
        {
            rows[0] = x;
            rows2[rowscount][0] = nn;
            for (int i = 1; i < rr; i++)
            {
                rows2[rowscount][i] = rows[i - 1];
                rows2[rowscount][0] -= rows[i - 1];
            }
            rowscount++;
        }
    }
    return result;
}

int*** makecomblock(int nrelms)
{
    int*** comblock = new int**[nrelms];
    for (int rrows = 1; rrows < nrelms + 1; rrows++)
    {
        int   nrelms2 = nrelms - rrows;
        int   combs = NrCombinations(nrelms2, rrows);
        int** commx = makeintmx(combs, rrows);
        FillCombs(nrelms2, rrows, commx);
        comblock[rrows - 1] = commx;
    }
    return comblock;
}

void deletecomblock(int*** comblock, int nrelms)
{
    for (int i = 0; i < nrelms; i++)
        deleteintmx(comblock[i]);
    delete[]comblock;
}

CToolBar::~CToolBar()
{
}

void CToolBar::RefreshWinnerRowMinSizes()
{
    ControlRects.Rebuild(Controls.m_size);
    rowminsizes.Rebuild(Controls.m_size);
    winnerindxs.Rebuild(Controls.m_size);

    block1 = makecomblock(Controls.m_size);

    for (unsigned int k = 0; k < Controls.m_size; k++)
    {
        GetWindowRect(Controls.m_data[k]->hWnd, &ControlRects.m_data[k]);
        ControlRects.m_data[k].right = ControlRects.m_data[k].right - ControlRects.m_data[k].left;
        ControlRects.m_data[k].bottom = ControlRects.m_data[k].bottom - ControlRects.m_data[k].top;
    }
    for (unsigned int rrows = 0; rrows < Controls.m_size; rrows++)
    {
        int nrelms2 = Controls.m_size - rrows - 1;
        int combs = NrCombinations(nrelms2, rrows + 1);
        int sizz;
        winnerindxs.m_data[rrows] = findminindx(block1[rrows], combs, rrows + 1, &sizz);
        rowminsizes.m_data[rrows] = sizz;
    }
}

int CToolBar::Refresh()
{
    RefreshWinnerRowMinSizes();
    Hide();

    Obj_OnSizing(0, WMSZ_RIGHT);
    Show();
    Obj_OnSizing(0, WMSZ_RIGHT);
    return 1;
}

CToolBar::CToolBar(int posx, int posy, int width, int height, CControl_Base* ParentObject, bool floating, int style)
    : CControl_Base(POPUP_EXSTYLES, 0, "CToolBarom", POPUP_STYLES, posx, posy, width * 0, height * 0, ParentObject, floating)
{
    MoveEngineIndx = 0;
    SizingMargin = 5;
    int nrcontrols = 6;
    avgcontrolh = 34;
    bordw = 4;
    bordh = 22;
    OverCursor = SIZEALL;
    MoveCursor = SIZEALL;
    onedgenow = true;

    block1 = 0;

    HRGN hRgn2 = CreateRegion("c_reg3.bmp");

    for (int i = 0; i < nrcontrols; i++)
    {
        int increm = 15 + (int)((1 + sin((float)i / 2)) * 20);
        int increm2 = 15 + (int)((1 + sin((float)i / 2)) * 20);
        static int lastposs = 0;
        CControl_Base* tcb = new CBitButton(250, 400, increm, increm2, "A", this, "tb_.bmp");
        tcb = tcb;
        lastposs += increm;
    }

    DeleteObject(hRgn2);

    buttondownw = -1;
    buttondownh = -1;
    RECT rr;
    rr.right = 1800;
    rr.left = 0;
    rr.top = 0;

    RefreshWinnerRowMinSizes();
    Obj_OnSizing(&rr, 0 | WMSZ_RIGHT);
    Move(posx, posy, rr.right, rr.bottom, true);
    OnMoved += &CControl_Base::DefaultDragHandler;
    MoveButton =  MK_LBUTTON;
}

int findmin(int* vec, int w)
{
    int result = MAXINT;
    for (int i = 0; i < w; i++)
    {
        if (result > vec[i])
            result = vec[i];
    }
    return result;
}

int findmax(int* vec, int w)
{
    int result = MININT;
    for (int i = 0; i < w; i++)
    {
        if (result < vec[i])
            result = vec[i];
    }
    return result;
}

int findmax(RECT* vec, int w)
{
    int result = MININT;
    for (int i = 0; i < w; i++)
    {
        if (result < vec[i].right)
            result = vec[i].right;
    }
    return result;
}

int CToolBar::findminindx(int** combsmx, int nrcombs, int nrrows, int* sizz)
{
    int* rowhosszak = 0;
    int winnersize = MAXINT;
    int winnerindx = 0;
    for (int i = 0; i < nrcombs; i++)
    {
        if (rowhosszak)
            delete[]rowhosszak;
        rowhosszak = new int[nrrows];
        int controlcount = 0;
        for (int j = 0; j < nrrows; j++)
        {
            rowhosszak[j] = 0;
            for (int k = 0; k < combsmx[i][j] + 1; k++)
                rowhosszak[j] += ControlRects.m_data[controlcount++].right;
        }
        int localmin = findmax(rowhosszak, nrrows);
        if (winnersize > localmin)
        {
            winnersize = localmin;
            *sizz = localmin;
            winnerindx = i;
        }
    }
    if (rowhosszak)
        delete[]rowhosszak;
    return winnerindx;
}

void CToolBar::Obj_OnMouseDown(int button, int x, int y, int client)
{
    buttondownw = -1;
    buttondownh = -1;
    CControl_Base::Obj_OnMouseDown(button, x, y, client);
}

void PutPx(HDC hdc, int x, int y)
{
    SetPixel(hdc, x, y, RGB(219, 216, 209));
    SetPixel(hdc, x + 1, y, RGB(219, 216, 209));
    SetPixel(hdc, x, y + 1, RGB(219, 216, 209));
    SetPixel(hdc, x + 1, y + 1, RGB(219, 216, 209));
}

int CToolBar::Obj_OnEraseBkgnd(HDC hdc)
{
    RECT cr;
    GetClientRect(hWnd, &cr);
    HBRUSH white_brush = CreateSolidBrush(RGB(155, 155, 255));
    HBRUSH GrayBrush = CreateSolidBrush(RGB(128, 128, 128));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(128, 128, 128));
    HPEN origpen = (HPEN)SelectObject(hdc, pen);
    HBRUSH origbrush = (HBRUSH)SelectObject(hdc, white_brush);
    Rectangle(hdc, 1, 1, cr.right, cr.bottom);
    SelectObject(hdc, origbrush);
    origbrush = (HBRUSH)SelectObject(hdc, GrayBrush);
    if (IsPopup())
    {
        Rectangle(hdc, 4, 4, cr.right - 3, 18);
        char tx1[200];
        GetWindowText(hWnd, tx1, 200);
        SetBkMode (hdc, TRANSPARENT);
        int txlen = strlen(tx1);
        if (cr.right < (int)strlen(tx1) * 11)
            txlen = (cr.right / 11);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOut(hdc, 7, 3, tx1, txlen);
        TextOut(hdc, cr.right - 13, 2, "x", 1);
        SetTextColor(hdc, RGB(0, 0, 0));
    }
    else
    {
        Rectangle(hdc, 2, 2, 9, cr.bottom - 0);
        PutPx(hdc, 5, 8);
        PutPx(hdc, 5, 13);
        PutPx(hdc, 5, 18);
    }
    SelectObject(hdc, origpen);
    SelectObject(hdc, origbrush);
    DeleteObject(pen);
    DeleteObject(GrayBrush);
    DeleteObject(white_brush);
    return 1;
}

int  CToolBar::Obj_OnSizing(RECT* newsizerect, int fwside)
{
    if (!Controls.m_size)
        return CControl_Base::Obj_OnSizing(newsizerect, fwside);

    ControlRects.Rebuild(Controls.m_size);

    avgcontrolh = 0;
    for (unsigned int k = 0; k < Controls.m_size; k++)
    {
        GetWindowRect(Controls.m_data[k]->hWnd, &ControlRects.m_data[k]);
        ControlRects.m_data[k].right = ControlRects.m_data[k].right - ControlRects.m_data[k].left;
        ControlRects.m_data[k].bottom = ControlRects.m_data[k].bottom - ControlRects.m_data[k].top;

        avgcontrolh += ControlRects.m_data[k].bottom;
    }
    avgcontrolh /= Controls.m_size;

    RECT junkrect;
    bool wasnotnewsizerect = false;
    if (!newsizerect)
    {
        wasnotnewsizerect = true;
        newsizerect = &junkrect;
        GetClientRect(hWnd, newsizerect);
    }
    RECT tr1;
    GetScreenCoordinates(&tr1);
    int actualwidth = tr1.right - tr1.left;
    int actualheight = tr1.bottom - tr1.top;

    int xxx = 11;
    int topdownflag = 0;

    if (fwside == WMSZ_BOTTOM)
    {
        topdownflag = WMSZ_BOTTOM;
        fwside = WMSZ_RIGHT;
        xxx = 0;
    }

    if (fwside == WMSZ_TOP)
    {
        topdownflag = WMSZ_TOP;
        fwside = WMSZ_RIGHT;
        xxx = 0;
    }

    if (buttondownw == -1)
    {
        buttondownw = newsizerect->right - newsizerect->left;
        buttondownh = newsizerect->bottom - newsizerect->top;
    }

    int offset1 = 0;
    if (!topdownflag)
    {
        if (fwside == WMSZ_RIGHT)
            newsizerect->right -= xxx;
        else
            newsizerect->left += xxx;

        if (buttondownw < newsizerect->right - newsizerect->left)
            offset1 = 1;
    }

    int possiblew;
    int possibleh;
    int width2 = newsizerect->right - newsizerect->left;

    if (topdownflag)
    {
        unsigned int indx1 = (newsizerect->bottom - newsizerect->top) / avgcontrolh - 2;
        if (indx1 >= Controls.m_size)
            indx1 = Controls.m_size - 1;
        if (indx1 < 0)
            width2 = 50000;
        else
            width2 = rowminsizes.m_data[indx1];
    }

    int minw = findmax(ControlRects.m_data, Controls.m_size);
    if (width2 <= minw)
        width2 = minw + 1;

    int winner1indx = 0;
    for (unsigned int rrows = 0; rrows < Controls.m_size; rrows++)
    {
        int minw2 = findmin(rowminsizes.m_data, Controls.m_size) + 1;
        if (width2 - xxx * offset1 + xxx / 2 > minw2)
            minw2 = width2 - xxx * offset1 + xxx / 2;
        if (minw2 > rowminsizes.m_data[rrows])
        {
            winner1indx = rrows - offset1;
            if ((offset1) && (winner1indx > 2))
                winner1indx--;
            goto ex1;
        }
    }
ex1:
    if (winner1indx < 0)
        winner1indx = 0;

    possiblew = rowminsizes.m_data[winner1indx] + bordw + 3;
    int ypluss = bordh;
    if (IsChild())
    {
        SizingMargin = 0;
        ypluss = 5;
        possiblew += 5;
    }
    else
        SizingMargin = 5;

    CVector <int> rowheights;

    rowheights.Rebuild(winner1indx + 1);
    {
        int cc = 0;
        possibleh = 0;
        for (int j = 0; j < winner1indx + 1; j++)
        {
            rowheights.m_data[j] = 0;
            static int last1;
            last1 = 0;
            for (int k = 0; k < block1[winner1indx][winnerindxs.m_data[winner1indx]][j] + 1; k++)
            {
                if (rowheights.m_data[j] < ControlRects.m_data[cc].bottom)
                    rowheights.m_data[j] = ControlRects.m_data[cc].bottom;
                last1 += ControlRects.m_data[cc].right;
                cc++;
            }
            possibleh += rowheights.m_data[j];
        }
        possibleh += ypluss;
    }

    if ((newsizerect->right - newsizerect->left) != possiblew)
    {
        if (fwside == WMSZ_RIGHT)
            newsizerect->right = newsizerect->left + possiblew;
        else
            newsizerect->left = newsizerect->right - possiblew;
    }

    if ((newsizerect->bottom - newsizerect->top) != possibleh)
    {
        if ((!topdownflag) || (topdownflag == WMSZ_BOTTOM))
            newsizerect->bottom = newsizerect->top + possibleh;
        else
            newsizerect->top = newsizerect->bottom - possibleh;
    }

    int sizemaxoptimalrow = rowminsizes.m_data[Controls.m_size - 1];
    int nrmaxoptimalrow = Controls.m_size - 1 + 1;
    for (int i = Controls.m_size - 1; i > -1; i--)
    {
        if (sizemaxoptimalrow == rowminsizes.m_data[i])
            nrmaxoptimalrow--;
    }

    if (nrmaxoptimalrow > 2)
    {
        if ((winner1indx == 0) || (winner1indx == nrmaxoptimalrow))
        {
            onedgenow = 1;
        }
        else
        {
            if (onedgenow)
            {
                if ((winner1indx == 2) || (winner1indx == nrmaxoptimalrow - 2))
                {
                    onedgenow = 0;
                    buttondownw = -1;
                    buttondownh = -1;
                }
            }
        }
    }

    int buttondownw2 = newsizerect->right - newsizerect->left;
    int buttondownh2 = newsizerect->bottom - newsizerect->top;

    if ((actualwidth != buttondownw2) || (actualheight != buttondownh2) || (wasnotnewsizerect))
    {
        HDWP hdwp = BeginDeferWindowPos(Controls.m_size);
        int xoff = bordw / 2 + 2;
        int yoff = bordh - bordw / 2 - 1;
        if (IsChild())
        {
            xoff = 10;
            yoff = 3;
        }
        int cc = 0;
        int temy = 0;
        for (int j = 0; j < winner1indx + 1; j++)
        {
            static int last1;
            last1 = 0;

            for (int k = 0; k < block1[winner1indx][winnerindxs.m_data[winner1indx]][j] + 1; k++)
            {
                int centerit = (rowheights.m_data[j] - ControlRects.m_data[cc].bottom) / 2;
                DeferWindowPos(hdwp, Controls.m_data[cc]->hWnd, 0, last1 + xoff, temy + yoff + centerit, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW );
                last1 += ControlRects.m_data[cc].right;
                cc++;
            }
            temy += rowheights.m_data[j];
        }
        EndDeferWindowPos(hdwp);
        InvalidateRect(hWnd, 0, false);
    }
    if (wasnotnewsizerect)
        Move(NOCHANGE, NOCHANGE, newsizerect->right, newsizerect->bottom, false);

    return CControl_Base::Obj_OnSizing(newsizerect, fwside);
}

int __stdcall CShellBrowser::BrowseCallbackProc(HWND  hwnd, UINT  uMsg, LPARAM  lParam, LPARAM  lpData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        CControl_Base* BrowserParent;
        AccessBrowserParent(&BrowserParent, false);
        ((CShellBrowser*)BrowserParent)->TreeV = hwnd;
        SetWindowLong(hwnd, GWL_USERDATA, (int)BrowserParent);
        EnableWindow(BrowserParent->hWnd, true);
        EnableWindow(BrowserParent->ParentWnd, true);
        SetWindowLong(hwnd, GWL_STYLE, 0x500B0000);
        SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
        MoveWindow(hwnd, 150, 150, 00, 00, true);
        InvalidateRect(hwnd, 0, true);
        HWND CListView = FindWindowEx(hwnd, NULL, "SysTreeView32", NULL);
        SetParent(CListView, BrowserParent->hWnd);
        BrowserParent->Refresh();
        RECT wnrc;
        GetWindowRect(BrowserParent->hWnd, &wnrc);
        MoveWindow(CListView, 0, 0, wnrc.right - wnrc.left, wnrc.bottom - wnrc.top, true);
    }
    if (uMsg == BFFM_SELCHANGED)
    {
        CShellBrowser* This = (CShellBrowser*) GetWindowLong(hwnd, GWL_USERDATA);
        if (This)
        {
            bool t = SHGetPathFromIDList((ITEMIDLIST*)lParam, This->Path);
            t = t;
            This->Obj_OnSelectionChanged(This->Path);
        }
    }
    return 0;
}

ITEMIDLIST* CShellBrowser::ParsePath(const char *szPath)
{
    IShellFolder* lpsf;
    ITEMIDLIST*   lpidlOut;
    ITEMIDLIST*   lpidl;
    IMalloc *     lpml;
    LPOLESTR   lpolestr;
    int        i;
    HRESULT    hr;
    unsigned long ul;

    hr = SHGetDesktopFolder(&lpsf);
    i  = MultiByteToWideChar(CP_ACP, 0, szPath, -1, NULL, 0);
    lpolestr = (LPOLESTR) malloc(i * sizeof(*lpolestr));

    i = MultiByteToWideChar(CP_ACP, 0, szPath, -1, lpolestr, i);
    if (!i)
        free(lpolestr);

    hr = lpsf->ParseDisplayName(NULL, NULL, lpolestr, &ul, &lpidl, NULL);
    free(lpolestr);
    if (FAILED(hr))
        lpsf->Release();

    int m_size = 0;
    while ((i = ((ITEMIDLIST*)((char*)lpidl + m_size))->mkid.cb) != 0 )
        m_size += i;

    lpidlOut = (ITEMIDLIST *)malloc(m_size + 2);
    if (m_size)
        memcpy(lpidlOut, lpidl, m_size);
    memset((char*)lpidlOut  + m_size, 0, 2);

    hr = SHGetMalloc(&lpml);
    if (lpml)
        lpml->Free(lpidl);

    return (lpidlOut);
}

void CShellBrowser::SetPath(char* path)
{
    SendMessage(TreeV, BFFM_SETSELECTION, 1, (WPARAM)path);
}

void CShellBrowser::Obj_OnSelectionChanged(char* selection)
{
    for (unsigned int i = 0; i < OnSelectionChanged.m_size; i++)
        (ParentObj->*OnSelectionChanged.m_data[i])(this, selection);
}

void CShellBrowser::GetPath(char* path)
{
    if (path)
        strcpy(path, Path);
}

char* CShellBrowser::GetPath()
{
    return CopyString(Path);
}

int CShellBrowser::AccessBrowserParent(CControl_Base** shb, bool set)
{
    static CControl_Base* Shb;
    if (set)
        Shb = *shb;
    else
        *shb = Shb;
    return 0;
}

CShellBrowser::CShellBrowser(int posx, int posy, int width, int height, CControl_Base* ParentObject, char* rootpath, bool floating, int style)
    : CControl_Base(0, 0, "", 0, posx, posy, width, height, ParentObject, floating)
{
    if (rootpath)
        RootPIDL = ParsePath(rootpath);
    else
        RootPIDL = 0;
    TreeV = 0;
    Path[0] = 0;
    DrawLevel = 1;
    dontdeleteondestroy = true;
    const UINT idTimer1 = 1;
    UINT nTimerDelay = 10;
    SetTimer(hWnd, idTimer1, nTimerDelay, NULL);
}

int CShellBrowser::Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    if (a_message == WM_TIMER)
    {
        const UINT idTimer1 = 1;
        KillTimer(a_hwnd, idTimer1);
        CControl_Base* BrowserParent = this;
        AccessBrowserParent(&BrowserParent, true);
        TCHAR dname[MAX_PATH];
        IMalloc *imalloc;
        SHGetMalloc(&imalloc);
        BROWSEINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.hwndOwner = 0; //hwnd;
        bi.pszDisplayName = dname;
        bi.lpszTitle = TEXT("Click the check box to HIDE/SHOW the edit contol.....");
        if (RootPIDL)
            bi.pidlRoot = RootPIDL;
        bi.ulFlags = BIF_RETURNONLYFSDIRS;
        bi.lpfn = BrowseCallbackProc;
        ITEMIDLIST *pidl = SHBrowseForFolder(&bi);
        imalloc->Free(pidl);
        imalloc->Release();
    }
    return CControl_Base::Obj_OnMessage (a_hwnd, a_message, a_wparam, a_lparam);
}

int CShellBrowser::Refresh()
{
    return CControl_Base::Refresh();
}

int CShellBrowser::Move(int x, int y, int width, int height, bool refresh)
{
    HWND CListView = FindWindowEx(hWnd, NULL, "SysTreeView32", NULL);
    MoveWindow(CListView, 0, 0, width, height, refresh);
    return CControl_Base::Move(x, y, width, height, refresh);
}

CShellBrowser::~CShellBrowser()
{}

CScrollBar::CScrollBar(int posx, int posy, int width, int height, CControl_Base* ParentObject, int orientation, int Exstyle, int style)
    : CControl_Base(0, 0, "", 0x50000000, posx, posy, width, height, ParentObject, false)
{
    DrawLevel = 1;
    LineStep = 1;
    ActualPos = 0;
    UpSideDown = false;
    FloatngPoint = false;
    LastPos = MININT;
    ScrBar = new CControl_Base(Exstyle, "scrollbar", "", (orientation & VERTICAL) ? (style | 1) : (style & 0xfffffffe), 0, 0, width, height, this, false);
    Orientation = VERTICAL;

    memset(&ScrollInfo, 0, sizeof(ScrollInfo));
    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_ALL;
    ScrollInfo.nMin = 0;
    ScrollInfo.nPos = 0;
    SetScrollInfo(ScrBar->hWnd, SB_CTL, &ScrollInfo, true);

    Min = 0;
    Max = 500;
    ScrollRealMax = 50000;
    TrackerSize = AUTO;
    SetTrackerSize(TrackerSize);
}

double CScrollBar::GetMinF()
{
    return Min;
}

double CScrollBar::GetMaxF()
{
    return Max;
}

int CScrollBar::GetMin()
{
    return (int)Min;
}

int CScrollBar::GetMax()
{
    return (int)Max;
}

void CScrollBar::SetFloatingPointStyle(bool floatingp)
{
    FloatngPoint = floatingp;
}

double CScrollBar::GetPosF()
{
    CheckBounds(&ActualPos);
    return ActualPos;
}

int CScrollBar::GetPos()
{
    CheckBounds(&ActualPos);
    return (int)ActualPos;
}

void CScrollBar::SetTrackerSize(int trackersize)
{
    ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
    if (trackersize < -3)
        trackersize = 0;
    if (trackersize == FIXED)
    {
        trackersize = 0;
        TrackerSize = trackersize;
    }

    if (trackersize > 0)
    {
        if (trackersize > 99)
            trackersize = MAXINT / 2;
        else
            trackersize = (int)(ScrollRealMax / (double)(100 - trackersize) * (double)(trackersize));
        TrackerSize = trackersize;
    }

    if (trackersize == DEFAULT)
    {
        TrackerSize = trackersize;
        trackersize = (int)ScrollRealMax / 20;
    }

    if (trackersize == AUTO)
    {
        TrackerSize = trackersize;
        RECT WindowRect;
        GetClientCoordinates(&WindowRect);
        if (Max - Min < ScrollRealMax / 5)
        {
            if (Max - Min > 0)
                trackersize = (int)((ScrollRealMax) / ((Max - Min) * 10) * (double)(IFVERT (WindowRect.bottom - WindowRect.top): (WindowRect.right - WindowRect.left)));
            else
                trackersize = MAXINT / 2;
        }
        else
            trackersize = 5 / 1 * (IFVERT (WindowRect.bottom - WindowRect.top): (WindowRect.right - WindowRect.left));
        PageStep = trackersize * (Max - Min) / ScrollRealMax;
        if (!FloatngPoint)
            PageStep = (int)PageStep;
    }

    ScrollInfo.nPage = trackersize;
    if (trackersize == 0)
        ScrollInfo.nMax = (int)ScrollRealMax + ScrollInfo.nPage ;
    else
        ScrollInfo.nMax = (int)ScrollRealMax + ScrollInfo.nPage - 1;

    SetScrollInfo(ScrBar->hWnd, SB_CTL, &ScrollInfo, true);
}

double CScrollBar::GetPageStep()
{
    return PageStep;
}

double CScrollBar::GetLineStep ()
{
    return LineStep;
}

void CScrollBar::SetPageStep(double page)
{
    if (!FloatngPoint)
        page = (int)page;
    if (page < 0)
        return;
    if (Max >= 0)
    {
        if (page > Max)
            page = Max;
    }
    else
    {
        if (page < Max)
            page = Max;
    }
    PageStep = page;
    ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
    int tempage;
    if (Max - Min > 0)
        tempage = (int)(ScrollRealMax / (Max - Min) * page);
    else
        tempage = MAXINT / 2;
    ScrollInfo.nPage = tempage;
    if (ScrollInfo.nPage < 1000)
        ScrollInfo.nPage = 1000;
    if (tempage == 0)
        ScrollInfo.nMax = (int)ScrollRealMax + ScrollInfo.nPage ;
    else
        ScrollInfo.nMax = (int)ScrollRealMax + ScrollInfo.nPage - 1;
    SetScrollInfo(ScrBar->hWnd, SB_CTL, &ScrollInfo, true);
}

void CScrollBar::SetRange(double topleft, double bottomright)
{
    if (!FloatngPoint)
    {
        topleft = (int)topleft;
        bottomright = (int)bottomright;
    }

    if (bottomright > topleft)
    {
        UpSideDown = false;
        Min = topleft;
        Max = bottomright;
    }

    else
    {
        UpSideDown = true;
        Min = bottomright;
        Max = topleft;
    }
    SetTrackerSize(TrackerSize);
}

void CScrollBar::SetLineStep(double page)
{
    if (!FloatngPoint)
        page = (int)page;

    if (Max >= 0)
    {
        if (page > Max)
            page = Max;
    }
    else
    {
        if (page < Max)
            page = Max;
    }
    LineStep = page;
}

void CScrollBar::Obj_OnChange(double pos, int button)
{
    CheckBounds(&pos);
    for (unsigned int i = 0; i < OnChange.m_size; i++)
        (ParentObj->*OnChange.m_data[i])(this, pos, button);
}

void  CScrollBar::Obj_OnEndScroll(double pos)
{
    if (LastPos == MININT)
        LastPos = pos;
    for (unsigned int i = 0; i < OnEndScroll.m_size; i++)
        (ParentObj->*OnEndScroll.m_data[i])(this, pos, LastPos);
    LastPos = pos;
}

void CScrollBar::CheckBounds(double*pos)
{
    if (*pos < Min)
        *pos = Min;
    if (*pos > Max)
        *pos = Max;
}

void CScrollBar::SetPos(double pos, bool refresh)
{
    if (!FloatngPoint)
        pos = (int)pos;
    CheckBounds(&pos);
    ActualPos = pos;
    int curpos = (int)(ScrollRealMax * (pos - Min) / (Max - Min));
    if (curpos < 0)
        curpos = 0;
    if ((int)curpos > ScrollRealMax)
        curpos = (int)ScrollRealMax;
    if (UpSideDown)
        curpos = (int)ScrollRealMax - curpos;
    SetScrollPos(ScrBar->hWnd, SB_CTL, (int)curpos, refresh);
    Obj_OnChange (ActualPos, 0);
}

int CScrollBar::Obj_OnMessage(HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    switch (a_message)
    {
    case WM_HSCROLL:
    case WM_VSCROLL:
    {
        int CurPos = GetScrollPos(ScrBar->hWnd, SB_CTL);
        double NextActualPos = ActualPos;
        switch (LOWORD(a_wparam))
        {
        case SB_ENDSCROLL:
            Obj_OnEndScroll(ActualPos);
            break;
        case SB_THUMBPOSITION:
            break;
        case SB_LINEDOWN:
            if (UpSideDown)
                NextActualPos -= LineStep;
            else
                NextActualPos += LineStep;
            break;
        case SB_LINEUP:
            if (UpSideDown)
                NextActualPos += LineStep;
            else
                NextActualPos -= LineStep;
            break;
        case SB_PAGEDOWN:
            if (UpSideDown)
                NextActualPos -= PageStep;
            else
                NextActualPos += PageStep;
            break;
        case SB_PAGEUP:
            if (UpSideDown)
                NextActualPos += PageStep;
            else
                NextActualPos -= PageStep;
            break;
        case SB_THUMBTRACK:
        {
            CurPos = HIWORD(a_wparam);
            if (UpSideDown)
                CurPos = (int)ScrollRealMax - CurPos;
            if (Max - Min > 0)
                NextActualPos = ((Max - Min) / ScrollRealMax * (double)CurPos + Min);
            else
                NextActualPos = Max;
            if (!FloatngPoint)
            {
                if (((int)NextActualPos) != ((int)ActualPos))
                    Obj_OnChange ((int)NextActualPos, 1);
            }
            else if (NextActualPos != ActualPos)
                Obj_OnChange (NextActualPos, 1);

            CheckBounds(&NextActualPos);

            if (NextActualPos != ActualPos)
                SetPos(NextActualPos, false);
            NextActualPos = ActualPos;
        }
        break;
        }
        CheckBounds(&NextActualPos);
        if (NextActualPos != ActualPos)
        {
            SetPos   (NextActualPos, true);
            Obj_OnChange (ActualPos, 1);
        }
        break;
    }
    }
    return CControl_Base::Obj_OnMessage(a_hwnd, a_message, a_wparam, a_lparam);
}

int CScrollBar::Move(int x, int y, int width, int height, bool refresh)
{
    ScrBar->Move(0, 0, width, height, true);
    return CControl_Base::Move(x, y, width, height, refresh);
}

CScrollBar::~CScrollBar()
{}

void CSplitter::SetWindows(CControl_Base* window1, CControl_Base* window2)
{
    if (window1)
        Window1 = window1;
    if (window2)
        Window2 = window2;
    Refresh();
}
CControl_Base* CSplitter::GetWindow(int windowindx)
{
    if (windowindx - 1)
        return Window2;
    else
        return Window1;
}

void CSplitter::GetRect(WINDOWRECT* rect)
{
    if (rect)
    {
        rect->height = SpRect.height;
        rect->width = SpRect.width;
        rect->left = SpRect.left;
        rect->top = SpRect.top;
    }
}

int CSplitter::Refresh()
{
    RECT WindowRect;
    GetClientCoordinates(&WindowRect);
    return Obj_OnMove(WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top);
}

int CSplitter::SetPos(int pos)
{
    SpPos = (IFVERT (SpRect.left): (SpRect.top)) + pos;
    return Move(NOCHANGE, NOCHANGE, NOCHANGE, NOCHANGE, true);
}

int CSplitter::GetPos()
{
    return SpPos;
}

int CSplitter::Move(int x, int y, int width, int height, bool refresh)
{
    if (x != NOCHANGE)
        SpRect.left = x;
    if (y != NOCHANGE)
        SpRect.top = y;
    if (width != NOCHANGE)
        SpRect.width = width;
    if (height != NOCHANGE)
        SpRect.height = height;
    return CControl_Base::Move(IFVERT SpPos: (SpRect.left), IFVERT (SpRect.top): SpPos, IFVERT SpWeight: (SpRect.width), IFVERT (SpRect.height): SpWeight, refresh);
}

void CSplitter::MoveWindows(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    if (Window1)
        Window1->Move(x1, y1, w1, h1, true);
    if (Window2)
        Window2->Move(x2, y2, w2, h2, true);
}

int CSplitter::MoveObjects(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    if (Window1)
        if (Window1->Obj_OnMove(x1, y1, w1, h1))
            return 1;
    if (Window2)
        if (Window2->Obj_OnMove(x2, y2, w2, h2))
            return 1;
    return 0;
}

int CSplitter::Obj_OnMove(int x, int y, int w, int h)
{
    int tresult = -100;
    if (Orientation & VERTICAL)
        if (x > 30000)
            if ((SpPos > x - 32768 + MinSizes) && (SpPos + SpWeight < x - 32768 + w - MinSizes))
            {
                x -= 32768;
                tresult = MoveObjects(32768 + x, y, SpPos - x - SpSpace, h, 32768 + SpPos + SpWeight + SpSpace, y, w - SpPos + x - SpWeight - SpSpace, h);
                if (!tresult)
                    MoveWindows(x, y, SpPos - x - SpSpace, h, SpPos + SpWeight + SpSpace, y, w - SpSpace - SpPos + x - SpWeight, h);
            }
            else
                tresult = 1;
        else if ((0 > SpRect.left - x + MinSizes) && (0 < -x + SpRect.left + SpRect.width - MinSizes - SpWeight))
        {
            tresult = MoveObjects(32768 + SpRect.left, SpRect.top, x - SpRect.left - SpSpace, h, 32768 + x + SpWeight + SpSpace, SpRect.top, SpRect.width - SpSpace - x + SpRect.left - SpWeight, h);
            if (!tresult)
            {
                SpPos = x;
                MoveWindows(SpRect.left, SpRect.top, x - SpRect.left - SpSpace, h, x + SpWeight + SpSpace, SpRect.top, SpRect.width - SpSpace - x + SpRect.left - SpWeight, h);
            }
        }
        else
            tresult = 1;
    else if (x > 30000)
        if ((SpPos > y + MinSizes) && (SpPos + SpWeight < y + h - MinSizes))
        {
            x -= 32768;
            tresult = MoveObjects(32768 + x, y, w, SpPos - y - SpSpace, 32768 + x, SpPos + SpWeight + SpSpace, w, h - SpPos + y - SpWeight - SpSpace);
            if (!tresult)
                MoveWindows(x, y, w, SpPos - y - SpSpace, x, SpPos + SpWeight + SpSpace, w, h - SpSpace - SpPos + y - SpWeight);
        }
        else
            tresult = 1;
    else if ((0 > SpRect.top - y + MinSizes) && (0 < -y + SpRect.top + SpRect.height - MinSizes - SpWeight))
    {
        tresult = MoveObjects(32768 + SpRect.left, SpRect.top, w, y - SpRect.top - SpSpace, 32768 + SpRect.left, y + SpWeight + SpSpace, w, SpRect.height - SpSpace - y + SpRect.top - SpWeight);
        if (!tresult)
        {
            SpPos = y;
            MoveWindows(SpRect.left, SpRect.top, w, y - SpRect.top - SpSpace, SpRect.left, y + SpWeight + SpSpace, w, SpRect.height - SpSpace - y + SpRect.top - SpWeight);
        }
    }
    else
        tresult = 1;

    int ttresult = CControl_Base::Obj_OnMove(x, y, w, h);
    if (tresult == -100)
        tresult = ttresult;
    return tresult;
}

CSplitter::CSplitter(int posx, int posy, int width, int height, CControl_Base* ParentObject, int orientation, const char* caption, const char* classname, int Exstyle, int style)
{
    Orientation = orientation;
    DrawLevel = 1;
    SpSpace = 1;
    Window1 = 0;
    Window2 = 0;
    MinSizes = 20;
    SpWeight = 10;
    Ratio = 50;
    SpRect.left = posx;
    SpRect.top = posy;
    SpRect.width = width;
    SpRect.height = height;
    SpPos = (IFVERT posx: posy) + (IFVERT width: height) * Ratio / 100;
    Create(Exstyle, classname, caption, style, IFVERT SpPos: posx, IFVERT posy: SpPos, IFVERT SpWeight: width, IFVERT height: SpWeight, ParentObject, false);
    MoveButton = MK_LBUTTON;
AlowMove = IFVERT HORIZONTAL:
               VERTICAL;
OverCursor = IFVERT SIZEWE:
                 SIZENS;
}

CSplitter::~CSplitter()
{}

CBButton::CBButton(int posx, int posy, int width, int height, char* caption, CControl_Base* ParentObject, char* btnresfilename)
    : CControl_Base(0, 0, caption, 0, posx, posy, width, height, ParentObject, false)
{
    BtnResReference = 0;
    TrackingEnabled = true;
    BtnState = 0;
    bkgrnd = 0;

    int transpar = 200;
    if (!BtnResource.m_data[1]->Hdc)
    {
        Transpareny[1] = 170;
        Transpareny[0] = 230;
        Transpareny[2] = 210;

        if (BtnResource.m_data[3]->Hdc)
        {
        }
    }
    else
    {
        Transpareny[0] = transpar;
        Transpareny[1] = transpar;
        Transpareny[2] = transpar;
    }
    if (!BtnResource.m_data[0]->Hdc)
    {
        Transpareny[1] = 90;
        Transpareny[0] = 190;
        Transpareny[2] = 110;
    }

    m_white_brush = CreateSolidBrush(RGB(255, 255, 255));
    m_white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    m_blue_brush = CreateSolidBrush(RGB(182, 189, 210));

    ButtonFont = CreateFont(14, 5, 0, 0, 700, 0, 0, 0, EASTEUROPE_CHARSET, 255, 255, /*19*/PROOF_QUALITY, 0, 0);

    Refresh();
}

CBButton::CBButton(int posx, int posy, int width, int height, char* caption, CControl_Base* ParentObject, CVectorRS <Bitmap32> * btnresreference, int btnresreferenceindex)
    : CControl_Base(0, 0, caption, 0, posx, posy, width, height, ParentObject, false)
{
    BtnResReference = btnresreference;
    BtnResReferenceIndex = btnresreferenceindex;
    TrackingEnabled = true;
    BtnState = 0;
    bkgrnd = 0;

    int transpar = 200;
    if ((*BtnResReference).m_data[btnresreferenceindex + 1]->Hdc)
    {
        Transpareny[1] = 170;
        Transpareny[0] = 230;
        Transpareny[2] = 210;

        if ((*BtnResReference).m_data[btnresreferenceindex + 3]->Hdc)
        {
        }
    }
    else
    {
        Transpareny[0] = transpar;
        Transpareny[1] = transpar;
        Transpareny[2] = transpar;
    }
    if (!(*BtnResReference).m_data[btnresreferenceindex + 0]->Hdc)
    {
        Transpareny[1] = 90;
        Transpareny[0] = 190;
        Transpareny[2] = 110;
    }

    m_white_brush = CreateSolidBrush(RGB(255, 255, 255));
    m_white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    m_blue_brush = CreateSolidBrush(RGB(182, 189, 210));
    ButtonFont = CreateFont(14, 5, 0, 0, 700, 0, 0, 0, EASTEUROPE_CHARSET, 255, 255, PROOF_QUALITY, 0, 0);

    Refresh();
}

CBButton::~CBButton()
{
    if (bkgrnd)
        DeleteDC(bkgrnd);
    DeleteObject(m_white_brush);
    DeleteObject(m_white_pen);
    DeleteObject(m_blue_brush);
    DeleteObject(ButtonFont);
}

void CBButton::Obj_OnMouseDown(int button, int x, int y, int client)
{
    CControl_Base::Obj_OnMouseDown(button, x, y, client);
    SetCapture(hWnd);
    BtnState = 1;
    Refresh();
}

void CBButton::Obj_OnMouseUp(int button, int x, int y, int client)
{
    CControl_Base::Obj_OnMouseUp(button, x, y, client);
    BtnState = 0;
    ReleaseCapture();
    Refresh();
}

void CBButton::Obj_OnMouseMove(int button, int x, int y)
{
    CControl_Base::Obj_OnMouseMove(button, x, y);
    int temstate;
    if (IsCursorOnWindow())
        temstate = 1;
    else
        temstate = 0;
    if (MouseIsOverControl != temstate)
    {
        MouseIsOverControl = temstate;
        Refresh();
    }
}

void CBButton::RebuildBkg(HDC hdc, int px, int py)
{
    RECT rc;
    GetClientRect(hWnd, &rc);

    if (hdc)
    {
        if (!bkgrnd)
        {
            bkgrnd = CreateCompatibleDC(hdc);
            HBITMAP bmp1 = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(bkgrnd, bmp1);
            BitBlt(bkgrnd, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
            DeleteObject(bmp1);
        }
        BitBlt(bkgrnd, 0, 0, rc.right, rc.bottom, hdc, px, py, SRCCOPY);
    }
    else
    {
        if (bkgrnd)
            DeleteDC(bkgrnd);
        bkgrnd = 0;
    }
    Refresh();
}

int CBButton::Obj_OnEraseBkgnd (HDC hdc)
{
    CControl_Base::Obj_OnEraseBkgnd(hdc);
    RECT rc;
    GetClientRect(hWnd, &rc);

    if (bkgrnd)
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, bkgrnd, 0, 0, SRCCOPY);
    else
    {
        bkgrnd = hdc;
    }
    int dxy = 0;
    if (MouseIsOverControl)
    {
        if (BtnState)
        {
            if (BtnResReference)
                StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, (*BtnResReference).m_data[BtnResReferenceIndex + 1]->Hdc, 0, 0, (*BtnResReference).m_data[BtnResReferenceIndex + 1]->Bmi.bmiHeader.biWidth, (*BtnResReference).m_data[BtnResReferenceIndex + 1]->Bmi.bmiHeader.biHeight, SRCCOPY);
            else
                StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, BtnResource.m_data[1]->Hdc, 0, 0, BtnResource.m_data[1]->Bmi.bmiHeader.biWidth, BtnResource.m_data[1]->Bmi.bmiHeader.biHeight, SRCCOPY);
            dxy = 1;
        }
        else
        {
            if (tristate == 2)
            {
                if (BtnResReference)
                    StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, (*BtnResReference).m_data[BtnResReferenceIndex + 0]->Hdc, 0, 0, (*BtnResReference).m_data[BtnResReferenceIndex + 0]->Bmi.bmiHeader.biWidth, (*BtnResReference).m_data[BtnResReferenceIndex + 0]->Bmi.bmiHeader.biHeight, SRCCOPY);
                else
                    StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, BtnResource.m_data[0]->Hdc, 0, 0, BtnResource.m_data[0]->Bmi.bmiHeader.biWidth, BtnResource.m_data[0]->Bmi.bmiHeader.biHeight, SRCCOPY);
            }
            else
            {
                if (BtnResReference)
                    StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, (*BtnResReference).m_data[BtnResReferenceIndex + 2]->Hdc, 0, 0, (*BtnResReference).m_data[BtnResReferenceIndex + 2]->Bmi.bmiHeader.biWidth, (*BtnResReference).m_data[BtnResReferenceIndex + 2]->Bmi.bmiHeader.biHeight, SRCCOPY);
                else
                    StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, BtnResource.m_data[2]->Hdc, 0, 0, BtnResource.m_data[2]->Bmi.bmiHeader.biWidth, BtnResource.m_data[2]->Bmi.bmiHeader.biHeight, SRCCOPY);
            }
        }
    }
    else if (BtnState)
    {
        if (BtnResReference)
            StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, (*BtnResReference).m_data[BtnResReferenceIndex + 2]->Hdc, 0, 0, (*BtnResReference).m_data[BtnResReferenceIndex + 2]->Bmi.bmiHeader.biWidth, (*BtnResReference).m_data[BtnResReferenceIndex + 2]->Bmi.bmiHeader.biHeight, SRCCOPY);
        else
            StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, BtnResource.m_data[2]->Hdc, 0, 0, BtnResource.m_data[2]->Bmi.bmiHeader.biWidth, BtnResource.m_data[2]->Bmi.bmiHeader.biHeight, SRCCOPY);
    }
    else
    {
        if (BtnResReference)
            StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, (*BtnResReference).m_data[BtnResReferenceIndex + 0]->Hdc, 0, 0, (*BtnResReference).m_data[BtnResReferenceIndex + 0]->Bmi.bmiHeader.biWidth, (*BtnResReference).m_data[BtnResReferenceIndex + 0]->Bmi.bmiHeader.biHeight, SRCCOPY);
        else
            StretchBlt(bkgrnd, 0, 0, rc.right, rc.bottom, BtnResource.m_data[0]->Hdc, 0, 0, BtnResource.m_data[0]->Bmi.bmiHeader.biWidth, BtnResource.m_data[0]->Bmi.bmiHeader.biHeight, SRCCOPY);
    }

    char tx1[200];
    GetWindowText(hWnd, tx1, 200);
    SetBkMode (hdc, TRANSPARENT);
    HFONT olbfont = (HFONT)SelectObject(hdc, ButtonFont);
    SetTextColor(hdc, RGB(40, 50, 120));
    TextOut(hdc, rc.right / 2 - strlen(tx1) * 7 / 2 + dxy - 0, rc.bottom / 2 - 8 + dxy, tx1, strlen(tx1));
    SelectObject(hdc, olbfont);
    bkgrnd = 0;
    return 1;
}

CBitBtn::CBitBtn(int posx, int posy, int width, int height, const char* caption, CControl_Base* ParentObject, const char* bmp, const char* bmpdown, const char* bmphl, const char* rbmp, int transpar, int a_EID)
    : CBitButton(posx, posy, width, height, caption, ParentObject, bmp, bmpdown, bmphl, transpar, a_EID)
{
    tristate = 1;
    if (rbmp)
    {
        HRGN hRgn = CreateRegion(rbmp);
        SetRegion(hRgn);
        OverCursor = HAND;
        DeleteObject(hRgn);
        hBMPchacked = (HBITMAP)LoadImage(GetModuleHandle(0), rbmp, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    }
    else
        hBMPchacked = 0;
}

void CBitBtn::UnCheck()
{
    if (tristate == 2)
        ToggleCheck();
}

void CBitBtn::Check()
{
    if (tristate == 1)
        ToggleCheck();
}

void CBitBtn::ToggleCheck()
{
    if (tristate == 1)
        tristate = 2;
    else if (tristate == 2)
        tristate = 1;

    if (tristate)
    {
        HBITMAP tembmp = hBMP;
        hBMP = hBMPchacked;
        hBMPchacked = tembmp;

        int tt = Transpareny[2];
        Transpareny[2] = Transpareny[0];
        Transpareny[0] = tt;
    }
}

void CBitBtn::Obj_OnClick(int button, int x, int y)
{
    ToggleCheck();
    return CControl_Base::Obj_OnClick(button, x, y);
}

CBitBtn::~CBitBtn()
{
    DeleteObject(hBMPchacked);
}

BBtn::BBtn(int posx, int posy, int width, int height, char* caption, CControl_Base* ParentObject, char* btnresfilename, int regionpicureindex)
    : CBButton(posx, posy, width, height, caption, ParentObject, btnresfilename)
{
    tristate = 1;
    if (regionpicureindex >= 0)
        if (BtnResource.m_data[regionpicureindex]->Hdc)
        {
            HRGN hRgn = CreateRegion(BtnResource.m_data[regionpicureindex], 0x00ffffff);
            SetRegion(hRgn);
            OverCursor = HAND;
            DeleteObject(hRgn);
        }
}

bool BBtn::IsChecked()
{
    if (tristate == 2)
        return true;
    else
        return false;
}

void BBtn::UnCheck()
{
    if (tristate == 2)
    {
        ToggleCheck();
        Refresh();
    }
}

void BBtn::Check()
{
    if (tristate == 1)
    {
        ToggleCheck();
        Refresh();
    }
}

void BBtn::ToggleCheck()
{
    if (tristate == 1)
        tristate = 2;
    else if (tristate == 2)
        tristate = 1;
    if (tristate)
    {
        Bitmap32* tembmp = BtnResource.m_data[0];
        BtnResource.m_data[0] = BtnResource.m_data[3];
        BtnResource.m_data[3] = tembmp;

        int tt = Transpareny[2];
        Transpareny[2] = Transpareny[0];
        Transpareny[0] = tt;
    }
}

void BBtn::Obj_OnClick(int button, int x, int y)
{
    ToggleCheck();
    return CControl_Base::Obj_OnClick(button, x, y);
}

BBtn::~BBtn()
{
    DeleteObject(hBMPchacked);
}

CheckBox::CheckBox(CControl_Base* parent, char* caption, int posx, int posy, int width, int height, bool state)
{
    CControl_Base::Create(0, "button", caption, 0x50010003, posx, posy, width, height, parent, false);
    SetState(state);
    ButtonFont = CreateFont(14, 5, 0, 0, 700, 0, 0, 0, EASTEUROPE_CHARSET, 255, 255, /*19*/PROOF_QUALITY, 0, 0);
    SendMessage(hWnd, WM_SETFONT, (int)ButtonFont, 0);
}

void CheckBox::Obj_OnClick(int button, int x, int y)
{
    if (GetState())
        SetState(false);
    else
        SetState(true);
    CControl_Base::Obj_OnClick(button, x, y);
}

void CheckBox::SetState(bool state)
{
    SendMessage(this->hWnd, BM_SETCHECK, (WPARAM)state, 0);
}

bool CheckBox::GetState()
{
    int result = SendMessage(this->hWnd, BM_GETCHECK, 0, 0);
    if (result == BST_CHECKED)
        return true;
    else
        return false;
}

CheckBox::~CheckBox()
{
    DeleteObject(ButtonFont);
}

CEdit::CEdit(CControl_Base* parent, const char* caption, int x, int y, int w, int h)
{
    CControl_Base::Create(0x00000000, "edit", caption, 0x50810080, x, y, w, h, parent, false);
}

void CEdit::GetText(char* string, int maxlen)
{
    if (!string)
        return;
    GetWindowText(hWnd, string, maxlen);
}

void CEdit::SetText(const char* string)
{
    if (!string)
        return;
    SetWindowText(hWnd, string);
}

CListView::CListView(CControl_Base* parent, int x, int y, int w, int h)
    : CControl_Base(0x00000200, "SysCListView32", "listview", 0x50010001 | LVS_NOSORTHEADER, x, y, w, h, parent, false)
{
    SendMessage(hWnd, 4096 + 54, 0, (32 | 1 | 256) );

    flipped = false;
    m_data = 0;
    lastheight = 0;
    ConstructMe(0);
}

void CListView::Redisplay()
{
    SendMessage(hWnd, LVM_DELETEALLITEMS, 0, 0);
    for (int i = 0; i <= lastheight; i++)
        SendMessage(hWnd, LVM_DELETECOLUMN, (WPARAM)1, 0);

    ConstructMe(1);
}

void CListView::Redisplay(CStringMX*  data)
{
    ListView_DeleteAllItems(hWnd);
    for (int i = 0; i <= lastheight; i++)
        SendMessage(hWnd, LVM_DELETECOLUMN, (WPARAM)1, 0);
    m_data = data;
    ConstructMe(1);
    if (data)
        lastheight = m_data->m_width;
    else
        lastheight = 0;
}

int CListView::GetSel()
{
    return ListView_GetNextItem(hWnd, (unsigned int) - 1, LVNI_SELECTED);
}

void CListView::SelectItem(int indx)
{
    if (indx < 0)
        indx = 0;
    ListView_SetItemState(hWnd, indx, LVIS_SELECTED, LVIS_SELECTED);
    ListView_SetItemState(hWnd, indx,  LVNI_FOCUSED, LVNI_FOCUSED );
    SetFocus(hWnd);
}

void CListView::UnselectItem(int indx)
{
    ListView_SetItemState(hWnd, indx,  0, LVIS_SELECTED  );
    ListView_SetItemState(hWnd, indx,  0, LVNI_FOCUSED );
    SetFocus(hWnd);
}

void CListView::ConstructMe(int start)
{
    LVCOLUMN lvcol;
    lvcol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvcol.cx = 0x28;
    lvcol.pszText = (char*)"";
    lvcol.cx = 40;
    if (start == 0)
        SendMessage(hWnd, LVM_INSERTCOLUMN, 0, (LPARAM)&lvcol);
    lvcol.cx = 0x42;
    char atm[20];
    if (m_data)
    {
        if (!flipped)
        {
            for (unsigned int i = 0; i < m_data->m_width; i++)
            {
                sprintf(atm, "%u", i);
                lvcol.pszText = atm;
                SendMessage(hWnd, LVM_INSERTCOLUMN, i + 1, (LPARAM)&lvcol);
            }
            for (unsigned int i = 0; i < m_data->m_height; i++)
            {
                AddRow(i, m_data->m_width);
            }
        }
        else
        {
            for (unsigned int i = 0; i < m_data->m_height; i++)
            {
                sprintf(atm, "%u", i);
                lvcol.pszText = atm;
                SendMessage(hWnd, LVM_INSERTCOLUMN, i + 1, (LPARAM)&lvcol);
            }
            for (unsigned int i = 0; i < m_data->m_height; i++)
            {
                AddColumn(i, m_data->m_width, i);
            }
        }
    }
}

void CListView::AddRow(int StrListindx, int listheight)
{
    int RowNr = (int) SendMessage(hWnd, LVM_GETITEMCOUNT, 0, 0);
    LVITEM LvItem;
    LvItem.mask = LVIF_TEXT;
    LvItem.cchTextMax = 256;
    LvItem.iItem = RowNr;
    LvItem.iSubItem = 0;
    char atm[20];
    sprintf(atm, "%u", RowNr);
    LvItem.pszText = atm;
    SendMessage(hWnd, LVM_INSERTITEM, 0, (LPARAM)&LvItem);
    if (m_data)
        for (int i = 0; i < listheight; i++)
        {
            LvItem.iSubItem = i + 1;
            LvItem.pszText = m_data->m_data[StrListindx][i];
            SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);
        }
}

void CListView::AddColumn(int StrListindx, int listheight, int indx)
{
    LVITEM LvItem;
    LvItem.mask = LVIF_TEXT;
    LvItem.cchTextMax = 256;
    if (m_data)
    {
        if (indx == 0)
        {
            for (int i = 0; i < listheight; i++)
            {
                LvItem.iItem = i;
                char atm[20];
                sprintf(atm, "%u", i);
                LvItem.pszText = atm;
                LvItem.iSubItem = 0;
                SendMessage(hWnd, LVM_INSERTITEM, 0, (LPARAM)&LvItem);
                LvItem.iSubItem = 1;
                LvItem.pszText = m_data->m_data[StrListindx][i];
                SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);
            }
        }
        else
        {
            for (int i = 0; i < listheight; i++)
            {
                LvItem.iItem = i;
                LvItem.iSubItem = indx + 1;
                LvItem.pszText = m_data->m_data[StrListindx][i];;
                SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);
            }
        }
    }
}

CStringMXViewer::CStringMXViewer(CControl_Base* parent, int posx, int posy, char* caption, CStringMX*stringMXRef, bool floating)
    : CControl_Base(0, 0, caption, 0, posx, posy, 528, 370, parent, floating)
{
    StringMXRef = stringMXRef;
    InitializeComponents();
    MoveButton = MK_LBUTTON;
    Refresh();
}

int CStringMXViewer::Refresh()
{
    if (StringMXRef)
        if (StringMXRef->m_width != RecordEditors.m_size)
        {
            for (unsigned int i = 0; i < RecordEditors.m_size; i++)
                DestroyWindow(RecordEditors.m_data[i]->hWnd);
            RecordEditors.Rebuild(0);
            if (StringMXRef)
                if (StringMXRef->m_width)
                {
                    int cw = 500 / StringMXRef->m_width;
                    for (unsigned int i = 0; i < StringMXRef->m_width; i++)
                        RecordEditors += new CEdit(this, "Edit1", 10 + i * (cw + 5), 268, cw, 25);
                }
        }
    CListView1->Redisplay(StringMXRef);
    return CControl_Base::Refresh();
}

int CStringMXViewer::Obj_OnSizing(RECT* newsizerect, int fwside)
{
    RECT wr;
    GetWindowRect(hWnd, &wr);
    memcpy(newsizerect, &wr, sizeof(RECT));
    return CControl_Base::Obj_OnSizing(newsizerect, fwside);
}

void CStringMXViewer::Obj_OnChange()
{
    for (unsigned int i = 0; i < OnChange.m_size; i++)
        (ParentObj->*OnChange.m_data[i])(this);
}

void CStringMXViewer::InitializeComponents()
{
    CListView1 = new CListView(this, 10, 10, 500, 250);
    CListView1->OnMouseUp += &CStringMXViewer::CListView1_OnMouseUp;
    AddButton = new CBitButton (220, 300, 134, 28, "ADD", this);
    AddButton->OnClick += &CStringMXViewer::AddButton_OnClick;
    RemoveButton = new CBitButton (370, 300, 134, 28, "REMOVE", this);
    RemoveButton->OnClick += &CStringMXViewer::RemoveButton_OnClick;
    UpdateButton = new CBitButton (10, 300, 134, 28, "UPDATE", this);
    UpdateButton->OnClick += &CStringMXViewer::UpdateButton_OnClick;
}

void CStringMXViewer::CListView1_OnMouseUp (CControl_Base* a_sender, int button, int x, int y, int client)
{
    int sel = CListView1->GetSel();
    if (StringMXRef)
        if (sel > -1)
        {
            if (RecordEditors.m_size != StringMXRef->m_width)
            {
                return;
            }
            if (sel < (int)StringMXRef->m_height)
                for (unsigned int i = 0; i < RecordEditors.m_size; i++)
                    RecordEditors.m_data[i]->SetText(StringMXRef->m_data[sel][i]);
        }
}

void CStringMXViewer::UpdateButton_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    int sel = CListView1->GetSel();
    if (StringMXRef)
        if (sel > -1)
        {
            if (RecordEditors.m_size != StringMXRef->m_width)
            {
                return;
            }
            if (RecordEditors.m_size && (sel < (int)StringMXRef->m_height))
            {
                for (unsigned int i = 0; i < RecordEditors.m_size; i++)
                {
                    char string[1000];
                    RecordEditors.m_data[i]->GetText(string);
                    strcpy(StringMXRef->m_data[sel][i], string);
                }
                CListView1->Redisplay();
                CListView1->SelectItem(sel);
                Obj_OnChange ();
            }
        }
}

void CStringMXViewer::RemoveButton_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    if (StringMXRef)
    {
        int sel = CListView1->GetSel();
        CListView1->Redisplay();
        CListView1->SelectItem(sel - 1);
        Obj_OnChange ();
    }
}

void CStringMXViewer::AddButton_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    if (StringMXRef)
    {
        if (RecordEditors.m_size != StringMXRef->m_width)
        {
            return;
        }
        if (RecordEditors.m_size)
        {
            for (unsigned int i = 0; i < RecordEditors.m_size; i++)
            {
                char string[1000];
                RecordEditors.m_data[i]->GetText(string);
                strcpy(StringMXRef->m_data[StringMXRef->m_height - 1][i], string);
            }
            CListView1->Redisplay();
            CListView1->SelectItem(StringMXRef->m_height - 1);
            Obj_OnChange ();
        }
    }
}

void CFileChooser::Obj_OnFileChanged(char* aFile)
{
    SetWindowText(iEditBox->hWnd, aFile);
    for (unsigned int i = 0; i < OnFileChanged.m_size; i++)
        (ParentObj->*OnFileChanged.m_data[i])(this, aFile);
}

CFileChooser::CFileChooser(int aPosX, int aPosY, int aWidth, int aHeight, CControl_Base* aParentObject, int aStyle, int aExtStyle, bool aSaveOrLoad, const char* a_Filter, const char* a_DefaultFile)
    : CControl_Base(aExtStyle, 0, 0, aStyle, aPosX, aPosY, aWidth, aHeight, aParentObject, false), iFilter(0), iSaveOrLoad(aSaveOrLoad)
{
    iEditBox = new CEdit(this, a_DefaultFile, 5, 5, aWidth - 45, aHeight - 10);
    iChooseButton = new CBitButton(aWidth - 35, 5, 30, aHeight - 10, "...", this);
    Resize(aWidth + 1, aHeight, false);
    Resize(aWidth, aHeight, false);
    iChooseButton->OnClick += &CFileChooser::ChooseButton_OnClick;

    if (a_Filter)
    {
        int filterlen = 0;
        while (a_Filter[filterlen] | a_Filter[filterlen + 1])
            filterlen++;
        filterlen += 2;
        iFilter = new char [filterlen];
        memcpy(iFilter, a_Filter, filterlen);
    }
}

CFileChooser::~CFileChooser()
{
    if (iFilter)
        delete[]iFilter;
}

void CFileChooser::ChooseButton_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    char pszFileName2[MAX_PATH];
    pszFileName2[0] = 0;
    GetWindowText(iEditBox->hWnd, pszFileName2, MAX_PATH - 1);
    if (GetFileName(hWnd, iSaveOrLoad, pszFileName2, iFilter))
        Obj_OnFileChanged(pszFileName2);
}

void CFileChooser::Obj_OnResize(int x, int y, int width, int height)
{
    iEditBox->Move(5, 5, width - 45, height - 10);
    iChooseButton->Move(width - 35, 5, 30, height - 10);
    CControl_Base::Obj_OnResize(x, y, width, height);
}

BOOL CFileChooser::GetFileName(HWND hwnd, BOOL bSave, const char pszFileName2[], const char* filterr )
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);

    ofn.lpstrFile = (char*)pszFileName2;
    ofn.hwndOwner = hwnd;

    ofn.lpstrFilter = filterr;

    ofn.nMaxFile = MAX_PATH;

    if (bSave)
    {
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileName(&ofn))
            return FALSE;
    }
    else
    {
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        if (!GetOpenFileName(&ofn))
            return FALSE;
    }
    return TRUE;
}
