#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <string.h>
#include <set>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "bitmap.h"
#include "Defines.h"
#include "Control_Base.h"

HCURSOR  Cursors[CursorVectorSize];  /*BUILTIN CURSOR RESOURCE*/
char     Debugger::dpd[5][1000];     /*STATIC DEBUGGER BUFFER INITIALIZATION*/
static   HBITMAP g_bmp1 = 0;
static   RECT Rect;
static   PAINTSTRUCT ps;
static   HDC hdcWindow;
static   HDC memDC = CreateCompatibleDC(0);
static   HBITMAP oldbmp_memDC = 0;

typedef void   (*DEBUG_PROC) (char* dpv1, char* dpv2, char* dpv3, char* dpv4, char* dpv5);
typedef void   (*VISINT_PROC)(void* data, int datatype, int datalen, int tableindx, bool jumptoend);
DEBUG_PROC     DEBUGPRINT;
VISINT_PROC    VISINT;

void InitCBLib()
{
    static HMODULE hMod5 = 0;
    hMod5 = 0; // LoadLibrary("tamtable.dll");
    if (!hMod5)
    {
        DEBUGPRINT = 0;
        VISINT = 0;
    }
    else
    {
        DEBUGPRINT = (DEBUG_PROC)GetProcAddress(hMod5, TEXT("CBDebugPrint"));
        VISINT = (VISINT_PROC)GetProcAddress(hMod5, TEXT("CBDebugVisInt"));
    }
}

void Debugger::Watch(int* indv, int datalen, int tableindx, bool jumptoend)
{
    VISINT((void*)indv, 1, datalen, tableindx, jumptoend);
}

void Debugger::Watch(unsigned int* indv, int datalen, int tableindx, bool jumptoend)
{
    VISINT((void*)indv, 2, datalen, tableindx, jumptoend);
}

void Debugger::Watch(float* indv, int datalen, int tableindx, bool jumptoend)
{
    VISINT((void*)indv, 3, datalen, tableindx, jumptoend);
}

void Debugger::Watch(unsigned char* indv, int datalen, int tableindx, bool jumptoend)
{
    VISINT((void*)indv, 4, datalen, tableindx, jumptoend);
}

void Debugger::Watch(char** indv, int datalen, int tableindx, bool jumptoend)
{
    VISINT((void*)indv, 5, datalen, tableindx, jumptoend);
}

void Debugger::Watch(double* indv, int datalen, int tableindx, bool jumptoend) //!!!this is fake!!!
{
    float* copy1 = new float [datalen];
    for (int i = 0; i < datalen; i++)
        copy1[i] = indv[i];
    VISINT((void*)copy1, 3, datalen, tableindx, jumptoend);
}

void Debugger::DPrint (int indv, int indx)
{
    sprintf(dpd[indx], "%i", indv);
}

void Debugger::DPrint (double& indv, int indx)
{
    sprintf(dpd[indx], "%f", indv);
}

void Debugger::DPrint (float& indv, int indx)
{
    sprintf(dpd[indx], "%f", indv);
}

void Debugger::DPrint (char* indv, int indx)
{
    if (strlen(indv) < 1000)
        strcpy(dpd[indx], indv);
    else
    {
        memcpy(dpd[indx], indv, 999);
        dpd[indx][999] = 0;
    }
}

void Debugger::DPrint (const char*& indv, int indx)
{
    if (strlen(indv) < 1000)
        strcpy(dpd[indx], indv);
    else
    {
        memcpy(dpd[indx], indv, 999);
        dpd[indx][999] = 0;
    }
}

void Debugger::CDPrint()
{
    if (DEBUGPRINT)
        DEBUGPRINT(dpd[0], dpd[1], dpd[2], dpd[3], dpd[4]);
    else
        MessageBox(NULL, "CControl_Base cpp:", "Debugger not loaded", MB_OK);
}

void Debugger::Pause()
{
    static bool ignorepauses = false;
    if (!ignorepauses)
    {
        int retval = MessageBox(NULL, "After you pess any of the folowing buttons, your application will continue to run. Press 'Ok' or hit 'Enter' if you want your application to stop on the next 'Debug.Pause()', or press 'No' or hit 'Esc' if you want the further pause requests to be ignored.", "Debug.Pause()", MB_OKCANCEL | MB_ICONQUESTION);
        if (retval == IDCANCEL)
            ignorepauses = true;
    }
}

LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM a_lparam)
{
    return DefMDIChildProc(hwnd, Message, wParam, a_lparam);
}

void CControl_Base::InitCControl_Base(HINSTANCE HINST)
{
    static bool CControl_Base_Inited = false;
    if (!CControl_Base_Inited)
    {
        {
            WNDCLASSEX wc;
            if (GetClassInfoEx(NULL, CLASS_NAME, &wc))
                return;

            wc.cbClsExtra = 0;
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.cbWndExtra = 0;
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hIcon = NULL;
            wc.hIconSm = NULL;
            wc.hIcon = (HICON)LoadImage(HINST, MAKEINTRESOURCE(1123), IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), 0);
            wc.hIconSm = (HICON)LoadImage(HINST, MAKEINTRESOURCE(1123), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
            wc.hInstance = HINST;
            wc.lpfnWndProc = (WNDPROC) &CControl_Base::CControl_BaseWindowProc;
            wc.lpszClassName = CLASS_NAME;
            wc.lpszMenuName = NULL;
            wc.style = CS_GLOBALCLASS;
            RegisterClassEx(&wc);
        }
        {
            WNDCLASSEX wincl;

            wincl.hInstance = 0;
            wincl.lpszClassName = "MyMDIChild";
            wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof (WNDCLASSEX);
            wincl.hIcon = (HICON)LoadImage(HINST, MAKEINTRESOURCE(1123), IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), 0);
            wincl.hIconSm = (HICON)LoadImage(HINST, MAKEINTRESOURCE(1123), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
            wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
            wincl.cbClsExtra = 0;
            wincl.cbWndExtra = 0;
            wincl.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);

            wincl.lpfnWndProc     = MDIChildWndProc;
            wincl.lpszMenuName    = NULL;

            RegisterClassEx(&wincl);
        }
        ZeroMemory(Cursors, sizeof(HCURSOR)*CursorVectorSize);
        CControl_Base_Inited = true;
    }
}

void CControl_Base::TogglePopupStyle()
{
    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    if (dwStyle & WS_CHILD)
    {
        SetWindowLong(hWnd, GWL_STYLE,   POPUP_STYLES);
        SetWindowLong(hWnd, GWL_EXSTYLE, POPUP_EXSTYLES);
        SetParent    (hWnd, NULL);
    }
    else
    {
        SetWindowLong(hWnd, GWL_STYLE,   CHILD_STYLES);
        SetWindowLong(hWnd, GWL_EXSTYLE, CHILD_EXSTYLES);
        SetParent    (hWnd, GetWindow(hWnd, GW_OWNER));
    }
    SendMessage(hWnd, WM_NCCALCSIZE, 0, 0);
}

void CControl_Base::DefaultDragHandler(CControl_Base* a_sender, int x, int y, int width, int height)
{
    RECT wr;
    GetWindowRect(a_sender->hWnd, &wr);
    RECT wrcl;

    int ww = wr.right - wr.left;
    int hh = wr.bottom - wr.top;

    if (a_sender->IsPopup())
        a_sender->GetClientCoordinates(&wrcl);
    else
        GetWindowRect(a_sender->hWnd, &wrcl);

    if (wr.left > 200)
    {
        if (a_sender->IsPopup())
        {
            a_sender->Hide();
            a_sender->TogglePopupStyle();
            a_sender->Obj_OnSizing(0, WMSZ_RIGHT);
            a_sender->Move(wrcl.left, wrcl.top, ww, hh - 14, false);
            a_sender->Show();
            a_sender->Obj_OnSizing(0, WMSZ_RIGHT);
            a_sender->Refresh();
        }
        else
        {}
    }
    else
    {
        if (a_sender->IsChild())
        {
            a_sender->Hide();
            a_sender->TogglePopupStyle();
            a_sender->Move(wrcl.left, wrcl.top, ww, hh + 14, false);
            a_sender->Obj_OnSizing(0, WMSZ_RIGHT);
            a_sender->Show();
            a_sender->Obj_OnSizing(0, WMSZ_RIGHT);
        }
        else
        {}
    }
}

int CControl_Base::ChackMargins(RECT& rect, POINT& movepoint)
{
    if (!SizingMargin)
        return SizingMargin;
    if (rect.right - movepoint.x < SizingMargin)
        return RIGHT;
    if (rect.bottom - movepoint.y < SizingMargin)
        return BOTTOM;
    if (movepoint.x - rect.left < SizingMargin)
        return LEFT;
    if (movepoint.y - rect.top < SizingMargin)
        return TOP;
    return 0;
}

int CControl_Base::IsPopup()
{
    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    if (dwStyle & WS_POPUP)
        return true;
    else
        return false;
}

int CControl_Base::IsCursorOnWindow()
{
    int result = 0;
    HRGN hRgn = CreateRectRgn(0, 0, 1, 1);
    int e = GetWindowRgn(hWnd, hRgn);
    POINT mp;
    GetCursorPos(&mp);
    ScreenToClient(hWnd, &mp);

    if (e == ERROR || e == NULLREGION)
    {
        DeleteObject(hRgn);
        RECT rc;
        GetClientRect(hWnd, &rc);
        int mousex = mp.x;
        int mousey = mp.y;
        if ((rc.left < (mousex)) && (rc.right > (mousex)) && (rc.top < (mousey)) && (rc.bottom > (mousey)))
            return 1;
        else
            return 0;
    }

    if (PtInRegion(hRgn, mp.x, mp.y))
        result = true;
    DeleteObject(hRgn);
    return result;
}

int CControl_Base::IsChild()
{
    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    if (dwStyle & WS_CHILD)
        return true;
    else
        return false;
}

bool CControl_Base::HasControl(CControl_Base* control)
{
    for (unsigned int i = 0; i < Controls.m_size; i++)
        if (Controls.m_data[i] == control)
            return true;
    return false;
}
int dispmsg = false;

int __stdcall CControl_Base::CControl_BaseWindowProc(HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    static POINT ClickPoint;
    static POINT MovePoint;
    static HWND windowtosetactive = 0;
    static CControl_Base* NowStartedToMove = 0;

    try
    {
        CControl_Base* ObjectData = (CControl_Base*)GetWindowLong(a_hwnd, GWL_USERDATA);
        if (ObjectData)
        {
            int temresult = ObjectData->Obj_OnMessage(a_hwnd, a_message, a_wparam, a_lparam);
            if (temresult == 0)
                return 0;
            if (temresult == 1)
                return 1;
            if (temresult == 2)
            {
                if (ObjectData->OldWndProc)
                {
                    int result2 = CallWindowProc(ObjectData->OldWndProc, a_hwnd, a_message, a_wparam, a_lparam);
                    if (a_message == WM_NCDESTROY)
                    {
                        if (!ObjectData->dontdeleteondestroy)
                        {
                            SetWindowLong(a_hwnd, GWL_USERDATA, 0);
                            if (ObjectData->ParentObj)
                                ObjectData->ParentObj->Controls -= ObjectData;
                            delete ObjectData;

                            ObjectData = 0;
                        }
                        SetWindowLong(a_hwnd, GWL_USERDATA, 0);
                    }
                    return result2;
                }
                else
                {
                    if (a_message == WM_NCDESTROY)
                    {
                        if (!ObjectData->dontdeleteondestroy)
                        {
                            SetWindowLong(a_hwnd, GWL_USERDATA, 0);
                            if (ObjectData->ParentObj)
                                ObjectData->ParentObj->Controls -= ObjectData;
                            delete ObjectData;
                            ObjectData = 0;
                        }
                        SetWindowLong(a_hwnd, GWL_USERDATA, 0);
                    }
                    if (ObjectData->IsMdiChild)
                        return DefMDIChildProc(a_hwnd, a_message, a_wparam, a_lparam);
                    else
                        return DefWindowProc(a_hwnd, a_message, a_wparam, a_lparam);
                }
            }

            switch (a_message)
            {
            case WM_MOUSEHOVER:
            {
                if (ObjectData->IsCursorOnWindow())
                    ObjectData->MouseIsOverControl = true;
                else
                    ObjectData->MouseIsOverControl = false;

                ObjectData->Obj_OnMouseEnter();
                InvalidateRect(a_hwnd, 0, true);
                return 1;
            }

            case WM_MOUSELEAVE:
            {
                ObjectData->NowTracking = false;
                ObjectData->MouseIsOverControl = false;
                InvalidateRect(a_hwnd, 0, true);
                ObjectData->Obj_OnMouseLeave();
                return 0;
            }

            case WM_ERASEBKGND:
            {
                static int nowpainting = false;
                if (ObjectData->DrawLevel == 2)
                {
                    nowpainting = false;
                    return 1;
                }
                if (!nowpainting)
                {
                    static RECT Rect;
                    if (ObjectData->DrawLevel == 1)
                        return 1;
                    nowpainting = true;

                    if (ObjectData->DrawLevel == 3)
                    {
                        GetClientRect(a_hwnd, &Rect);
                        if (ObjectData->BackBitmap.Hdc)
                            BitBlt((HDC)a_wparam, 0, 0, Rect.right, Rect.bottom, ObjectData->BackBitmap.Hdc, 0, 0, SRCCOPY);
                        nowpainting = false;
                        return 1;
                    }
                    static int retval;

                    if (ObjectData->DrawLevel == 4)
                    {
                        RECT Rect;
                        GetClientRect(a_hwnd, &Rect);
                        if (ObjectData->BackBitmap.Hdc)
                        {
                            if (ObjectData->BackBitmap.Stretch)
                            {
                                SetStretchBltMode((HDC) a_wparam, HALFTONE);
                                StretchDIBits((HDC) a_wparam, 0, 0, Rect.right, Rect.bottom, 0, 0, ObjectData->BackBitmap.Bmi.bmiHeader.biWidth, ObjectData->BackBitmap.Bmi.bmiHeader.biHeight, ObjectData->BackBitmap.BmBits, &ObjectData->BackBitmap.Bmi, DIB_RGB_COLORS	, SRCCOPY);
                            }
                            else
                                BitBlt((HDC)a_wparam, 0, 0, Rect.right, Rect.bottom, ObjectData->BackBitmap.Hdc, 0, 0, SRCCOPY);
                        }
                        nowpainting = false;
                        return 1;
                    }

                    retval = 0;
                    if (!g_bmp1)
                    {
                        g_bmp1 = CreateCompatibleBitmap((HDC)a_wparam, 2000, 2000);
                        BeginPaint(a_hwnd, &ps);
                        oldbmp_memDC = (HBITMAP)SelectObject(memDC, g_bmp1);
                    }
                    static int temretval;
                    temretval = ObjectData->Obj_OnEraseBkgnd(memDC);
                    if (temretval == 1)
                    {
                        GetClientRect(a_hwnd, &Rect);
                        BitBlt((HDC)a_wparam, 0, 0, Rect.right, Rect.bottom, memDC, 0, 0, SRCCOPY);
                        retval = 1;
                    }
                    if (temretval == 2)
                        retval = 1;
                    nowpainting = false;
                    if (retval)
                        return 1;
                }
            }
            break;

            case WM_PAINT:
            {
                if (ObjectData->DrawLevel == 4)
                    break;
                if (ObjectData->DrawLevel == 3)
                    break;
                static int nowpainting = false;
                if (!nowpainting)
                {
                    nowpainting = true;
                    if (!g_bmp1)
                    {
                        g_bmp1 = CreateCompatibleBitmap((HDC)a_wparam, 2000, 2000);
                        BeginPaint(a_hwnd, &ps);
                        oldbmp_memDC = (HBITMAP)SelectObject(memDC, g_bmp1);
                    }
                    if (ObjectData->DrawLevel == 2)
                    {
                        GetClientRect(a_hwnd, &Rect);
                        hdcWindow = BeginPaint(a_hwnd, &ps);
                        if (ObjectData->BackBitmap.Hdc)
                            BitBlt(hdcWindow, 0, 0, Rect.right, Rect.bottom, ObjectData->BackBitmap.Hdc, 0, 0, SRCCOPY);
                        EndPaint(a_hwnd, &ps);
                        DeleteDC(hdcWindow);
                        nowpainting = false;
                        break;
                    }
                    if (ObjectData->Obj_OnPaint(memDC))
                    {
                        GetClientRect(a_hwnd, &Rect);
                        hdcWindow = BeginPaint(a_hwnd, &ps);
                        BitBlt(hdcWindow, 0, 0, Rect.right, Rect.bottom, memDC, 0, 0, SRCCOPY);
                        EndPaint(a_hwnd, &ps);
                        DeleteDC(hdcWindow);
                    }
                    nowpainting = false;
                }
            }
            break;

            case WM_CREATE:
                break;

            case WM_MOVE:
                ObjectData->Obj_OnMoved(LOWORD(a_lparam), HIWORD(a_lparam), 0, 0);
                break;

            case WM_MOUSEMOVE:
                if (ObjectData->OnMouseEnter.m_size || ObjectData->OnMouseLeave.m_size)
                    ObjectData->TrackingEnabled = true;
                if ((ObjectData->TrackingEnabled) && (!ObjectData->NowTracking))
                {
                    TRACKMOUSEEVENT tme;
                    memset(&tme, 0, sizeof(TRACKMOUSEEVENT));
                    tme.cbSize = sizeof(tme);
                    tme.hwndTrack = ObjectData->hWnd;
                    tme.dwFlags = TME_LEAVE | TME_HOVER;
                    tme.dwHoverTime = 1;
                    ObjectData->NowTracking = TrackMouseEvent(&tme);
                }

                if (ObjectData->NowStartedToSize)
                {
                    GetCursorPos(&MovePoint);
                    if (ObjectData->NowStartedToSize)
                    {
                        POINT WindowPos;
                        WindowPos.x = ObjectData->ButtonDownRect.left;
                        WindowPos.y = ObjectData->ButtonDownRect.top;

                        if (ObjectData->IsChild())
                            ScreenToClient(ObjectData->ParentWnd, &WindowPos);

                        int top = WindowPos.y;
                        int left = WindowPos.x;
                        int width = ObjectData->ButtonDownRect.right - ObjectData->ButtonDownRect.left;
                        int height = ObjectData->ButtonDownRect.bottom - ObjectData->ButtonDownRect.top;
                        int fws;
                        if (ObjectData->NowStartedToSize == TOP)
                        {
                            fws = WMSZ_TOP;
                            top += (MovePoint.y - ClickPoint.y);
                            height -= (MovePoint.y - ClickPoint.y);
                        }
                        if (ObjectData->NowStartedToSize == LEFT)
                        {
                            fws = WMSZ_LEFT;
                            left += (MovePoint.x - ClickPoint.x);
                            width -= (MovePoint.x - ClickPoint.x);
                        }
                        if (ObjectData->NowStartedToSize == RIGHT)
                        {
                            fws = WMSZ_RIGHT;
                            width += (MovePoint.x - ClickPoint.x);
                        }
                        if (ObjectData->NowStartedToSize == BOTTOM)
                        {
                            fws = WMSZ_BOTTOM;
                            height += (MovePoint.y - ClickPoint.y);
                        }
                        RECT newr;
                        newr.top = top;
                        newr.left = left;
                        newr.right = left + width;
                        newr.bottom = top + height;

                        RECT nowr;
                        GetWindowRect(a_hwnd, &nowr);

                        ObjectData->Obj_OnSizing(&newr, fws);
                        if (newr.bottom ^ nowr.bottom  ||   newr.top ^ nowr.top  || newr.right ^ nowr.right || newr.left ^ nowr.left)
                            MoveWindow(a_hwnd, newr.left, newr.top, newr.right - newr.left, newr.bottom - newr.top, true);
                    }
                }

                if (NowStartedToMove)
                {
                    GetCursorPos(&MovePoint);
                    int dX = 0;
                    int dY = 0;
                    POINT WindowPos;
                    WindowPos.x = ObjectData->ButtonDownRect.left;
                    WindowPos.y = ObjectData->ButtonDownRect.top;

                    if (ObjectData->IsChild())
                        ScreenToClient(ObjectData->ParentWnd, &WindowPos);

                    if (ObjectData->AlowMove & HORIZONTAL)
                        dX = WindowPos.x + MovePoint.x - ClickPoint.x;
                    else
                        dX = WindowPos.x;

                    if (ObjectData->AlowMove & VERTICAL)
                        dY = WindowPos.y + MovePoint.y - ClickPoint.y;
                    else
                        dY = WindowPos.y;

                    RECT nowr;
                    GetWindowRect(a_hwnd, &nowr);
                    if (!ObjectData->Obj_OnMove(dX, dY, nowr.right - nowr.left, nowr.bottom - nowr.top))
                    {
                        ObjectData = (CControl_Base*)GetWindowLong(a_hwnd, GWL_USERDATA);
                        if (ObjectData && (ObjectData == NowStartedToMove))
                        {
                            if (ObjectData->IsChild())
                            {
                                MoveWindow(a_hwnd, dX, dY, nowr.right - nowr.left, nowr.bottom - nowr.top, true);
                                InvalidateRect(a_hwnd, 0, false);
                            }
                            else
                            {
                                if (ObjectData->MoveEngineIndx == 0)
                                {
                                    MoveWindow(a_hwnd, dX, dY, nowr.right - nowr.left, nowr.bottom - nowr.top, true);
                                }
                                if (ObjectData->MoveEngineIndx == 1)
                                {
                                    NowStartedToMove = 0;
                                    PostMessage(a_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                                    return true;
                                }
                            }
                        }
                    }
                }
                if (ObjectData)
                    ObjectData->Obj_OnMouseMove(a_wparam, LOWORD(a_lparam), HIWORD(a_lparam));
                break;

            case WM_CLOSE:
                if (ObjectData->Obj_OnClose())
                    return 0;
                break;

            case WM_DESTROY:
                ObjectData->Obj_OnDestroy();
                break;

            case WM_SETCURSOR:
            {
                RECT wnr;
                GetWindowRect(a_hwnd, &wnr);
                GetCursorPos(&MovePoint);
                int margin = ObjectData->ChackMargins(wnr, MovePoint);
                if (margin)
                {
                    if (margin & (TOP | BOTTOM))
                        ObjectData->CBSetCursor(SIZENS);
                    else
                        ObjectData->CBSetCursor(SIZEWE);
                    return 0;
                }
                if (!margin)
                    if (!ObjectData->Obj_OnSetCursor(a_hwnd, a_wparam, a_lparam))
                        return 0;
            }
            break;

            case WM_NCDESTROY:
            {
                windowtosetactive = ObjectData->ParentWnd;
                SetWindowLong(a_hwnd, GWL_USERDATA, 0);
                WNDPROC OldWndProc2 = 0;
                if (ObjectData->OldWndProc)
                    OldWndProc2 = ObjectData->OldWndProc;
                if (!ObjectData->dontdeleteondestroy)
                {
                    SetWindowLong(a_hwnd, GWL_USERDATA, 0);
                    if (ObjectData->ParentObj)
                        ObjectData->ParentObj->Controls -= ObjectData;
                    delete ObjectData;
                    ObjectData = 0;
                }
                SetWindowLong(a_hwnd, GWL_USERDATA, 0);
                if (windowtosetactive)
                {
                    SetActiveWindow(windowtosetactive);
                    windowtosetactive = 0;
                }
                if (OldWndProc2)
                    return CallWindowProc(OldWndProc2, a_hwnd, a_message, a_wparam, a_lparam);
            }
            break;

            case WM_NCLBUTTONDOWN:
            {
                ObjectData->Obj_OnMouseDown(MK_LBUTTON, 0, 0, 0);
            }
            break;

            case WM_NCRBUTTONDOWN:
            {
                ObjectData->Obj_OnMouseDown(MK_RBUTTON, 0, 0, 0);
            }
            break;

            case WM_NCLBUTTONUP:
            {
                ObjectData->Obj_OnMouseUp(MK_LBUTTON, 0, 0, 0);
            }
            break;

            case WM_NCRBUTTONUP:
            {
                ObjectData->Obj_OnMouseUp(MK_RBUTTON, 0, 0, 0);
            }
            break;

            case WM_LBUTTONDOWN:
                if (!(ObjectData->MoveButton ^ a_wparam))
                {
                    GetWindowRect(a_hwnd, &ObjectData->ButtonDownRect);
                    GetCursorPos(&ClickPoint);

                    int margs = ObjectData->ChackMargins(ObjectData->ButtonDownRect, ClickPoint);
                    if (margs)
                    {
                        ObjectData->NowStartedToSize = margs;
                        SetCapture(a_hwnd);
                    }
                    else
                        NowStartedToMove = ObjectData;
                    if (!ObjectData->ChackMargins(ObjectData->ButtonDownRect, ClickPoint))
                        if (ObjectData->MoveCursor > -1)
                            ObjectData->CBSetCursor(ObjectData->MoveCursor);
                    if (!ObjectData->MoveEngineIndx || ObjectData->IsChild())
                        SetCapture(a_hwnd);
                }
                ObjectData->mousestate = 1;
                ObjectData->Obj_OnMouseDown(a_wparam, LOWORD(a_lparam), HIWORD(a_lparam), 1);
                break;

            case WM_LBUTTONUP:
                if (ObjectData->mousestate == 1)
                {
                    RECT rc;
                    GetClientRect(a_hwnd, &rc);
                    int temstate = 0;
                    int mousex = LOWORD(a_lparam);
                    int mousey = HIWORD(a_lparam);
                    if (ObjectData->IsCursorOnWindow())
                        temstate = 1;
                    if (temstate)
                        ObjectData->Obj_OnClick(0, mousex, mousey);
                }
                ObjectData = (CControl_Base*)GetWindowLong(a_hwnd, GWL_USERDATA);
                if (ObjectData)
                {
                    ObjectData->mousestate = 0;
                    if (NowStartedToMove)
                    {
                        NowStartedToMove = 0;
                        InvalidateRect(a_hwnd, 0, true);
                    }
                    ObjectData->NowStartedToSize = false;
                    ObjectData->Obj_OnMouseUp(a_wparam, LOWORD(a_lparam), HIWORD(a_lparam), 1);
                }
                ReleaseCapture();
                break;

            case WM_RBUTTONDOWN:
                if (!(ObjectData->MoveButton ^ a_wparam))
                {
                    GetWindowRect(a_hwnd, &ObjectData->ButtonDownRect);
                    GetCursorPos(&ClickPoint);
                    int margs = ObjectData->ChackMargins(ObjectData->ButtonDownRect, ClickPoint);
                    if (margs)
                    {
                        ObjectData->NowStartedToSize = margs;
                        SetCapture(a_hwnd);
                    }
                    else
                        NowStartedToMove = ObjectData;
                    if (!ObjectData->ChackMargins(ObjectData->ButtonDownRect, ClickPoint))
                        if (ObjectData->MoveCursor > -1)
                            ObjectData->CBSetCursor(ObjectData->MoveCursor);
                    if (!ObjectData->MoveEngineIndx || ObjectData->IsChild())
                        SetCapture(a_hwnd);
                }
                ObjectData->mousestate = 2;
                ObjectData->Obj_OnMouseDown(a_wparam, LOWORD(a_lparam), HIWORD(a_lparam), 1);
                break;

            case WM_RBUTTONUP:
                if (ObjectData->mousestate == 2)
                {
                    RECT rc;
                    GetClientRect(a_hwnd, &rc);
                    int temstate = 0;
                    int mousex = LOWORD(a_lparam);
                    int mousey = HIWORD(a_lparam);
                    if (ObjectData->IsCursorOnWindow())
                        temstate = 1;
                    if (temstate)
                        ObjectData->Obj_OnClick(1, mousex, mousey);
                }
                ObjectData = (CControl_Base*)GetWindowLong(a_hwnd, GWL_USERDATA);
                if (ObjectData)
                {
                    ObjectData->mousestate = 0;
                    ObjectData->NowStartedToSize = false;
                    ObjectData->Obj_OnMouseUp(a_wparam, LOWORD(a_lparam), HIWORD(a_lparam), 1);
                    if (NowStartedToMove)
                    {
                        NowStartedToMove = 0;
                        InvalidateRect(a_hwnd, 0, true);
                    }
                }
                ReleaseCapture();
                break;

            case WM_MBUTTONDOWN:
                if (!(ObjectData->MoveButton ^ a_wparam))
                {
                    GetWindowRect(a_hwnd, &ObjectData->ButtonDownRect);
                    GetCursorPos(&ClickPoint);
                    int margs = ObjectData->ChackMargins(ObjectData->ButtonDownRect, ClickPoint);
                    if (margs)
                    {
                        ObjectData->NowStartedToSize = margs;
                        SetCapture(a_hwnd);
                    }
                    else
                        NowStartedToMove = ObjectData;
                    if (!ObjectData->ChackMargins(ObjectData->ButtonDownRect, ClickPoint))
                        if (ObjectData->MoveCursor > -1)
                            ObjectData->CBSetCursor(ObjectData->MoveCursor);
                    if (!ObjectData->MoveEngineIndx || ObjectData->IsChild())
                        SetCapture(a_hwnd);
                }
                ObjectData->mousestate = 2;
                ObjectData->Obj_OnMouseDown(a_wparam, LOWORD(a_lparam), HIWORD(a_lparam), 1);
                break;

            case WM_MBUTTONUP:
                if (ObjectData->mousestate == 2)
                {
                    RECT rc;
                    GetClientRect(a_hwnd, &rc);
                    int temstate = 0;
                    int mousex = LOWORD(a_lparam);
                    int mousey = HIWORD(a_lparam);
                    if (ObjectData->IsCursorOnWindow())
                        temstate = 1;
                    if (temstate)
                        ObjectData->Obj_OnClick(2, mousex, mousey);
                }
                ObjectData = (CControl_Base*)GetWindowLong(a_hwnd, GWL_USERDATA);
                if (ObjectData)
                {
                    ObjectData->mousestate = 0;
                    ObjectData->NowStartedToSize = false;
                    ObjectData->Obj_OnMouseUp(a_wparam, LOWORD(a_lparam), HIWORD(a_lparam), 1);
                    if (NowStartedToMove)
                    {
                        NowStartedToMove = 0;
                        InvalidateRect(a_hwnd, 0, true);
                    }
                }
                ReleaseCapture();
                break;

            case WM_SIZE:
            {
                RECT wr1;
                GetWindowRect(a_hwnd, &wr1);
                POINT tp;
                tp.x = wr1.left;
                tp.y = wr1.top;
                ScreenToClient(ObjectData->ParentWnd, &tp);
                ObjectData->Obj_OnResize(tp.x, tp.y, LOWORD(a_lparam), HIWORD(a_lparam));
            }
            break;

            case WM_SIZING:
            {
                RECT* lprc = (LPRECT) a_lparam;
                ObjectData->Obj_OnSizing(lprc, a_wparam);
            }
            break;

            }
            if (ObjectData)
                if (ObjectData->OldWndProc)
                    return CallWindowProc(ObjectData->OldWndProc, a_hwnd, a_message, a_wparam, a_lparam);
        }
        else
        {
            if (a_message == WM_NCCREATE)
                SetWindowLong(a_hwnd, GWL_USERDATA, (long)((LPCREATESTRUCT(a_lparam))->lpCreateParams));
        }

        if (ObjectData)
        {
            if (ObjectData->IsMdiChild)
            {
                return DefMDIChildProc(a_hwnd, a_message, a_wparam, a_lparam);
            }
            else
            {
                return DefWindowProc(a_hwnd, a_message, a_wparam, a_lparam);
            }
        }
        else
return DefWindowProc(a_hwnd, a_message, a_wparam, a_lparam);

    }
    catch (const std::exception& e)
    {
        std::cout << "exception: " << e.what() << '\n';
    }
    return 0;
}

void CControl_Base::Obj_OnClick(int button, int x, int y)
{
    for (unsigned int i = 0; i < OnClick.m_size; i++)
        (ParentObj->*OnClick.m_data[i])(this, button, x, y);
}

int CControl_Base::Obj_OnSetCursor(HWND a_hwnd, int a_wparam, int a_lparam)
{
    int retval = 1;
    for (unsigned int i = 0; i < OnSetCursor.m_size; i++)
        if (!(ParentObj->*OnSetCursor.m_data[i])(this, a_wparam, a_lparam))
            retval = 0;
    if (OverCursor > -1)
    {
        CBSetCursor(OverCursor);
        return 0;
    }
    return retval;
}

int CControl_Base::Obj_OnEraseBkgnd(HDC hdc)
{
    int result = 0;
    for (unsigned int i = 0; i < OnEraseBkgnd.m_size; i++)
    {
        int tres = (ParentObj->*OnEraseBkgnd.m_data[i])(this, hdc);
        if (tres)
            result = tres;
    }
    return result;
}

int CControl_Base::Obj_OnPaint(HDC hdc)
{
    int result = 0;

    for (unsigned int i = 0; i < OnPaint.m_size; i++)
    {
        int tres = (ParentObj->*OnPaint.m_data[i])(this, hdc);
        if (tres)
            result = tres;
    }
    return result;

}

int CControl_Base::Obj_OnMessage(HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    int result = -1;
    int tresult = -1;
    for (unsigned int i = 0; i < OnMessage.m_size; i++)
    {
        tresult = (ParentObj->*OnMessage.m_data[i])(this, a_message, a_wparam, a_lparam);
        if (tresult > 0)
            result = tresult;
        if (tresult < 0)
            result = 0;
    }
    return result;
}

void CControl_Base::Obj_OnCreate(int a_wparam, int a_lparam)
{
    for (unsigned int i = 0; i < OnCreate.m_size; i++)
        (ParentObj->*OnCreate.m_data[i])(this, a_wparam, a_lparam);
}

void CControl_Base::Obj_OnDestroy()
{
    for (unsigned int i = 0; i < OnDestroy.m_size; i++)
        (ParentObj->*OnDestroy.m_data[i])(this);
}

void CControl_Base::Obj_OnMouseMove(int button, int x, int y)
{
    for (unsigned int i = 0; i < OnMouseMove.m_size; i++)
        (ParentObj->*OnMouseMove.m_data[i])(this, button, x, y);
}

void CControl_Base::Obj_OnResize(int x, int y, int width, int height)
{
    for (unsigned int i = 0; i < OnResize.m_size; i++)
        (ParentObj->*OnResize.m_data[i])(this, x, y, width, height);
}

int CControl_Base::Obj_OnSizing(RECT* newsizerect, int fwside)
{
    int result = 0;
    for (unsigned int i = 0; i < OnSizing.m_size; i++)
    {
        int tres = (ParentObj->*OnSizing.m_data[i])(this, newsizerect, fwside);
        if (tres)
            result = tres;
    }
    return result;
}

int CControl_Base::Obj_OnMove(int x, int y, int width, int height)
{
    int result = 0;
    if (x < 30000) // "+32768" reserved for internal use
        for (unsigned int i = 0; i < OnMove.m_size; i++)
        {
            int tres = (ParentObj->*OnMove.m_data[i])(this, x, y, width, height);
            if (tres)
                result = tres;
        }
    return result;
}

void CControl_Base::Obj_OnMoved(int x, int y, int width, int height)
{
    for (unsigned int i = 0; i < OnMoved.m_size; i++)
        (ParentObj->*OnMoved.m_data[i])(this, x, y, width, height);
}

void CControl_Base::Obj_OnMouseDown(int button, int x, int y, int client)
{
    for (unsigned int i = 0; i < OnMouseDown.m_size; i++)
        (ParentObj->*OnMouseDown.m_data[i])(this, button, x, y, client);
}

void CControl_Base::Obj_OnMouseUp(int button, int x, int y, int client)
{
    for (unsigned int i = 0; i < OnMouseUp.m_size; i++)
        (ParentObj->*OnMouseUp.m_data[i])(this, button, x, y, client);
}

void CControl_Base::Obj_OnMouseLeave()
{
    for (unsigned int i = 0; i < OnMouseLeave.m_size; i++)
        (ParentObj->*OnMouseLeave.m_data[i])(this);
}
void CControl_Base::Obj_OnMouseEnter()
{
    for (unsigned int i = 0; i < OnMouseEnter.m_size; i++)
        (ParentObj->*OnMouseEnter.m_data[i])(this);
}

bool CControl_Base::Obj_OnClose()
{
    int result = 0;
    for (unsigned int i = 0; i < OnClose.m_size; i++)
    {
        int tres = (ParentObj->*OnClose.m_data[i])(this);
        if (tres)
            result = tres;
    }
    return result;
}

void CControl_Base::CBSetCursor(int CursorIndx)
{
    if (!Cursors[CursorIndx])
        switch(CursorIndx)
        {
        case 0:
            Cursors[CursorIndx] = LoadCursor(0, IDC_APPSTARTING);
            break;
        case 1:
            Cursors[CursorIndx] = LoadCursor(0, IDC_ARROW);
            break;
        case 2:
            Cursors[CursorIndx] = LoadCursor(0, IDC_CROSS);
            break;
        case 3:
            Cursors[CursorIndx] = LoadCursor(0, IDC_IBEAM);
            break;
        case 4:
            Cursors[CursorIndx] = LoadCursor(0, IDC_ICON);
            break;
        case 5:
            Cursors[CursorIndx] = LoadCursor(0, IDC_NO);
            break;
        case 6:
            Cursors[CursorIndx] = LoadCursor(0, IDC_SIZE);
            break;
        case 7:
            Cursors[CursorIndx] = LoadCursor(0, IDC_SIZEALL);
            break;
        case 8:
            Cursors[CursorIndx] = LoadCursor(0, IDC_SIZENESW);
            break;
        case 9:
            Cursors[CursorIndx] = LoadCursor(0, IDC_SIZENS);
            break;
        case 10:
            Cursors[CursorIndx] = LoadCursor(0, IDC_SIZENWSE);
            break;
        case 11:
            Cursors[CursorIndx] = LoadCursor(0, IDC_SIZEWE);
            break;
        case 12:
            Cursors[CursorIndx] = LoadCursor(0, IDC_UPARROW);
            break;
        case 13:
            Cursors[CursorIndx] = LoadCursor(0, IDC_WAIT);
            break;
        case 14:
            Cursors[CursorIndx] = LoadCursor(0, IDC_HAND);
            break;
        }
    SetCursor(Cursors[CursorIndx]);
}

int CControl_Base::Resize(int width, int height, bool refresh)
{
    return Move(NOCHANGE, NOCHANGE, width, height, refresh);
}

int CControl_Base::Move(int x, int y, int width, int height, bool refresh)
{
    RECT wnr3;
    GetWindowRect(hWnd, &wnr3);

    if (width == NOCHANGE)
        width = wnr3.right - wnr3.left;
    if (height == NOCHANGE)
        height = wnr3.bottom - wnr3.top;

    if ((x == NOCHANGE) || (y == NOCHANGE))
    {
        POINT tp;
        tp.x = wnr3.left;
        tp.y = wnr3.top;

        if (ParentWnd)
            if (IsChild())// !!! (!ParentObj->IsChild())&&IsChild() ?
                ScreenToClient(ParentWnd, &tp);

        if (x == NOCHANGE)
            x = tp.x;
        if (y == NOCHANGE)
            y = tp.y;
    }
    if (!Obj_OnMove(x, y, width, height))
        MoveWindow(hWnd, x, y, width, height, refresh);
    else
        return 1;
    return 0;
}

int CControl_Base::EmptyControlsFromTo(int eidstart, int eidstorp)
{
    int result = 0;
    set<HWND> windows_2be_destroyed;
    for (unsigned int i = 0; i < Controls.m_size; i++)
        if (Controls.m_data[i]->EID >= eidstart && Controls.m_data[i]->EID < eidstorp)
        {
            windows_2be_destroyed.insert(Controls.m_data[i]->hWnd);
            result++;
        }

    for (auto it = windows_2be_destroyed.begin(); it != windows_2be_destroyed.end(); ++it)
        DestroyWindow((*it));

    return result;
}

int CControl_Base::EmptyControls()
{
    return EmptyControlsFromTo(0, 1000000);
}

void CControl_Base::Show()
{
    ShowWindow(hWnd, SW_SHOW);
}

void CControl_Base::Hide()
{
    ShowWindow(hWnd, SW_HIDE);
}

int CControl_Base::Refresh()
{
    InvalidateRect(hWnd, NULL, true);
    return 0;
}

void CControl_Base::PaintBackBitmap(bool refresh)
{
    static RECT Rect;
    static HDC hdcWindow;
    static PAINTSTRUCT ps;
    GetClientRect(hWnd, &Rect);
    hdcWindow = BeginPaint(hWnd, &ps);
    if (BackBitmap.Hdc)
        BitBlt(hdcWindow, 0, 0, Rect.right, Rect.bottom, BackBitmap.Hdc, 0, 0, SRCCOPY);
    EndPaint(hWnd, &ps);
    DeleteDC(hdcWindow);
    InvalidateRect(hWnd, 0, refresh);
}

void CControl_Base::SetRegion(HRGN hRgn)
{
    HRGN m_WinRgn = CreateRectRgn(0, 0, 1, 1);
    CombineRgn(m_WinRgn, hRgn, 0, RGN_COPY);
    SetWindowRgn(hWnd, m_WinRgn, true);
    DeleteObject(m_WinRgn);
    Refresh();
}

CControl_Base::CControl_Base(int Exstyle, const char* Classname, const char* caption, int style, int posx, int posy, int width, int height, CControl_Base* ParentObject, bool Floating, HINSTANCE HINST, On_Create On_CreateHandler, void* hmdiclientcreatestruct, int createchild, int a_EID)
{
    Create(Exstyle, Classname, caption, style, posx, posy, width, height, ParentObject, Floating, HINST, On_CreateHandler, hmdiclientcreatestruct, createchild, a_EID);
}

CControl_Base::CControl_Base()
{}

CControl_Base::~CControl_Base()
{}

void CControl_Base::GetClientCoordinates(RECT* windowrect)
{
    GetWindowRect(hWnd, windowrect);
    POINT tp;
    tp.x = windowrect->left;
    tp.y = windowrect->top;
    if (ParentWnd)
        ScreenToClient(ParentWnd, &tp);
    windowrect->left = tp.x;
    windowrect->top = tp.y;
    tp.x = windowrect->right;
    tp.y = windowrect->bottom;
    if (ParentWnd)
        ScreenToClient(ParentWnd, &tp);
    windowrect->right = tp.x;
    windowrect->bottom = tp.y;
}

void CControl_Base::GetScreenCoordinates(RECT* windowrect)
{
    GetWindowRect(hWnd, windowrect);
}

void CControl_Base::Create(int Exstyle, const char* Classname, const char* caption, int style, int posx, int posy, int width, int height, CControl_Base* ParentObject, bool Floating, HINSTANCE HINST, On_Create On_CreateHandler, void* hmdiclientcreatestruct, int createchild, int a_EID)
{
    IsMdiChild = false;
    EID = a_EID;
    MoveEngineIndx = 1;
    InitCControl_Base(HINST);
    NowStartedToSize = false;
    NowTracking = false;
    MouseIsOverControl = false;
    TrackingEnabled = false;
    SizingMargin = 0;
    DrawLevel = 0;
    MoveButton = 0;
    OldWndProc = 0;
    MoveCursor = -1;
    OverCursor = -1;
    dontdeleteondestroy = false;
    AlowMove = HORIZONTAL | VERTICAL;
    int Style = style;
    int ExStyle = Exstyle;
    char* ClassName;
    if (Classname)
        ClassName = CopyString(Classname);
    else
        ClassName = CopyString(CLASS_NAME);

    char* Caption = CopyString(caption);

    if (On_CreateHandler)
        OnCreate += On_CreateHandler;

    mousestate = 0;

    if (ParentObject)
    {
        ParentObj = ParentObject;
        ParentWnd = ParentObject->hWnd;
    }
    else
    {
        ParentObj = 0;
        ParentWnd = 0;
    }

    if (ParentObject)
    {
        ParentObject->Controls += this;
        ID = ParentObject->Controls.m_size;
    }
    else
        ID = 0;
    void* createstruct = (void*)this;
    if (hmdiclientcreatestruct)
        createstruct = hmdiclientcreatestruct;

    if (Floating)
    {
        if (!Style)
            Style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
        if (createchild)
        {
            IsMdiChild = true;
            hWnd = CreateMDIWindow ("MyMDIChild", Caption, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, posx, posy, width, height, ParentWnd, HINST, (LPARAM)createstruct);

            SetWindowLong(hWnd, GWL_USERDATA, (long)this);
            OldWndProc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG) (&CControl_Base::CControl_BaseWindowProc));
        }
        else
            hWnd = CreateWindowEx  (ExStyle, ClassName, Caption, Style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, posx, posy, width, height, 0, NULL, HINST, createstruct);

        if (strcmp(ClassName, CLASS_NAME) != 0)
        {
            SetWindowLong(hWnd, GWL_USERDATA, (long)this);
            OldWndProc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG) (&CControl_Base::CControl_BaseWindowProc));
        }
    }
    else
    {
        if (!Style)
            Style = 0x50010000;
        hWnd = CreateWindowEx(ExStyle, ClassName, Caption, Style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,  posx, posy,  width, height, ParentWnd, (HMENU)ID, HINST, createstruct);
        if (strcmp(ClassName, CLASS_NAME) != 0)
        {
            SetWindowLong(hWnd, GWL_USERDATA, (long)this);
            OldWndProc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG) (&CControl_Base::CControl_BaseWindowProc));
        }
    }
    ShowWindow (hWnd, SW_SHOW);
    CREATESTRUCT pls;
    pls.hInstance = HINST;
    Obj_OnCreate(0, (int)(&pls));

    if (Caption)
        delete[]Caption;
    if (ClassName)
        delete[]ClassName;
}

int CControl_Base::StartApp(char* a_file_name, char* a_params)
{
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);
    char lpstext[MAX_PATH * 2];
    strcpy(lpstext, a_file_name);
    if (a_params)
        strcat(lpstext, a_params);
    CreateProcess(NULL, (LPTSTR)lpstext, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
    return processInfo.dwProcessId;
}

void CControl_Base::SaveChildPositions(char* filename)
{
    CVector<RECT>lCoords;
    lCoords.Rebuild(Controls.m_size);
    for (unsigned int i = 0; i < Controls.m_size; i++)
    {
        Controls.m_data[i]->GetClientCoordinates(&lCoords.m_data[i]);
    }
}
void CControl_Base::LoadChildPositions(char* filename)
{
    CVector<RECT>lCoords;
    for (unsigned int i = 0; i < Controls.m_size; i++)
    {
        if (i >= lCoords.m_size)
            return;
        Controls.m_data[i]->Move(lCoords.m_data[i].left, lCoords.m_data[i].top, lCoords.m_data[i].right - lCoords.m_data[i].left, lCoords.m_data[i].bottom - lCoords.m_data[i].top);
    }
    for (unsigned int i = 0; i < Controls.m_size; i++)
    {
        if (i >= lCoords.m_size)
            return;
        Controls.m_data[i]->Refresh();
    }
}

void CControl_Base::SetChildsToEdit()
{
    for (unsigned int i = 0; i < Controls.m_size; i++)
    {
        Controls.m_data[i]->MoveButton = MK_LBUTTON;
        Controls.m_data[i]->MoveCursor = HAND;
        Controls.m_data[i]->OverCursor = SIZEALL;
        Controls.m_data[i]->SizingMargin = 6;
    }
}

void CopyString(char** dest, char* source)
{
    if (*dest)
        delete[](*dest);
    if (!source)
    {
        *dest = 0;
        return;
    }
    *dest = new char[strlen(source) + 1];
    strcpy(*dest, source);
}

char* CopyString(const char* source)
{
    if (!source)
        return 0;

    char* result = new char[strlen(source) + 1];
    strcpy(result, source);
    return result;
}

int RunApp()
{
    MSG Msg;
    while(GetMessage( &Msg, NULL, 0, 0 ))
    {
        if (!IsDialogMessage(GetActiveWindow(), &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
    return Msg.wParam;
}

int RunAppD()
{
    int hRet;
    MSG Msg;
    while((hRet = GetMessage( &Msg, NULL, 0, 0 )) != 0)
        /*if ((Msg.wParam == 9) && (Msg.message == 256))
        {
            if (!IsDialogMessage(GetActiveWindow(), &Msg))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
        else*/
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    SelectObject(memDC, oldbmp_memDC);
    DeleteDC(memDC);
    return Msg.wParam;
}

int RunAppDWhile(bool*releaseflag)
{
    int hRet;
    MSG Msg;
    while((hRet = GetMessage( &Msg, NULL, 0, 0 )) != 0)
    {
        if (releaseflag)
            if (*releaseflag)
            {
                *releaseflag = false;
                return 0;
            }
        if ((Msg.wParam == 9) && (Msg.message == 256))
        {
            if (!IsDialogMessage(GetActiveWindow(), &Msg))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
        else
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
    return Msg.wParam;
}

HRGN CreateRegion(const char* imageFile)
{
    BITMAP bmpInfo;
    HDC hdcMem = CreateCompatibleDC(NULL);

    HANDLE hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), imageFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hBitmap)
    {
        MessageBox(0, imageFile, "While creating a region: The specified file doesn't name a windows bitmap", MB_OK);
        return 0;
    }
    GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
    HGDIOBJ hGdiObj = SelectObject(hdcMem, hBitmap);
    HRGN hRgn = CreateRectRgn(0, 0, 0, 0);

    COLORREF crTransparent = RGB(255, 255, 255);

    int iX = 0;
    int iY = 0;
    int iRet = 0;
    for (iY = 0; iY < bmpInfo.bmHeight; iY++)
    {
        do
        {
            //skip over transparent pixels at start of lines.
            while (iX < bmpInfo.bmWidth && GetPixel(hdcMem, iX, iY) == crTransparent)
                iX++;
            //remember this pixel
            int iLeftX = iX;
            //now find first non transparent pixel
            while (iX < bmpInfo.bmWidth && GetPixel(hdcMem, iX, iY) != crTransparent)
                ++iX;
            //create a temp region on this info
            HRGN hRgnTemp = CreateRectRgn(iLeftX, iY, iX, iY + 1);
            //combine into main region.

            iRet = CombineRgn(hRgn, hRgn, hRgnTemp, RGN_OR);
            if (iRet == ERROR)
            {
                MessageBox(0, imageFile, "While creating a region: An error accured", MB_OK);
                return 0;
            }
            //delete the temp region for next pass
            DeleteObject(hRgnTemp);
        }
        while(iX < bmpInfo.bmWidth);
        iX = 0;
    }
    SelectObject(hdcMem, hGdiObj);
    DeleteDC(hdcMem);
    DeleteObject(hBitmap);
    return hRgn;
}

HRGN CreateRegion(Bitmap32* bitmap, int crTransparent)
{
    HRGN hRgn = CreateRectRgn(0, 0, 0, 0);

    int iX = 0;
    int iY = 0;
    int iRet = 0;
    for (iY = 0; iY < bitmap->Bmi.bmiHeader.biHeight; iY++)
    {
        do
        {
            //skip over transparent pixels at start of lines.
            while ((iX < bitmap->Bmi.bmiHeader.biWidth) && (bitmap->BmRGBQuads[iY][iX] == crTransparent))
                iX++;
            //remember this pixel
            int iLeftX = iX;
            //now find first non transparent pixel
            while (iX < bitmap->Bmi.bmiHeader.biWidth && bitmap->BmRGBQuads[iY][iX] != crTransparent)
                ++iX;
            //create a temp region on this info
            HRGN hRgnTemp = CreateRectRgn(iLeftX, iY, iX, iY + 1);
            //combine into main region.

            iRet = CombineRgn(hRgn, hRgn, hRgnTemp, RGN_OR);
            if (iRet == ERROR)
            {
                MessageBox(0, "Bitmap32", "While creating a region: An error accured", MB_OK);
                return 0;
            }
            //delete the temp region for next pass
            DeleteObject(hRgnTemp);
        }
        while(iX < bitmap->Bmi.bmiHeader.biWidth);
        iX = 0;
    }
    return hRgn;
}

BitmapLayer::BitmapLayer()
    : Bitmap32()
{
    ParentLayer = 0;
    Alpha = OPAQUEALPHA; /*255*/
    Stretch = false;
    TransparentColor = NOCOLOR;
    DstX = 0;
    DstY = 0;
    DstW = 0;
    DstH = 0;
    SrcX = 0;
    SrcY = 0;
    SrcW = MAXIMUMSIZE;
    SrcH = MAXIMUMSIZE;
}
int BitmapLayer::GetSerializedLen()
{
    int numofbytes = 0;
    numofbytes += Bitmap32::GetSerializedLen();
    numofbytes += ChildLayers.GetSerializedLen();
    numofbytes += Transform.GetSerializedLen();
    return numofbytes + 14 * 4;
}

unsigned char* BitmapLayer::Serialize (unsigned int* nrbytes)
{
    int numofbytes = 0;

    unsigned int numofBitmapBytes;
    unsigned char* BitmapData = Bitmap32::Serialize(&numofBitmapBytes);
    numofbytes += numofBitmapBytes;

    unsigned int numofChildLayersBytes;
    unsigned char* ChildLayersData = ChildLayers.Serialize(&numofChildLayersBytes);
    numofbytes += numofChildLayersBytes;

    unsigned int numofTransformBytes;
    unsigned char* TransformData = Transform.Serialize(&numofTransformBytes);
    numofbytes += numofTransformBytes;

    unsigned char* result = new unsigned char[numofbytes + 14 * 4];
    *((int*)result) = numofbytes + 14 * 4;
    *((int*)&result[4]) = Alpha;
    *((int*)&result[8]) = Stretch;
    *((int*)&result[12]) = 0; //RESERVED
    *((int*)&result[16]) = TransparentColor;
    *((int*)&result[20]) = 0; //RESERVED
    *((int*)&result[24]) = DstX;
    *((int*)&result[28]) = DstY;
    *((int*)&result[32]) = DstW;
    *((int*)&result[36]) = DstH;
    *((int*)&result[40]) = SrcX;
    *((int*)&result[44]) = SrcY;
    *((int*)&result[48]) = SrcW;
    *((int*)&result[52]) = SrcH;
    int shp = 14 * 4;

    memcpy(result + shp, BitmapData, numofBitmapBytes);
    shp += numofBitmapBytes;
    memcpy(result + shp, ChildLayersData, numofChildLayersBytes);
    shp += numofChildLayersBytes;
    memcpy(result + shp, TransformData, numofTransformBytes);
    shp += numofTransformBytes;

    delete[]BitmapData;
    delete[]ChildLayersData;
    delete[]TransformData;

    if (nrbytes)
        *nrbytes = numofbytes + 14 * 4;
    return result;
}

bool BitmapLayer::Deserialize(unsigned char* buffer, int* nrbytes)
{
    bool result = false;
    Alpha = *((int*)&buffer[4]);
    Stretch = *((int*)&buffer[8]);
    TransparentColor = *((int*)&buffer[16]);
    DstX =  *((int*)&buffer[24]);
    DstY =  *((int*)&buffer[28]);
    DstW =  *((int*)&buffer[32]);
    DstH =  *((int*)&buffer[36]);
    SrcX =  *((int*)&buffer[40]);
    SrcY =  *((int*)&buffer[44]);
    SrcW =  *((int*)&buffer[48]);
    SrcH =  *((int*)&buffer[52]);
    int  numofbytes = 14 * 4;

    int numofBitmapBytes;
    Bitmap32::Deserialize(buffer + numofbytes, &numofBitmapBytes);
    numofbytes += numofBitmapBytes;

    unsigned int numofChildLayersBytes;
    ChildLayers.Deserialize(buffer + numofbytes, &numofChildLayersBytes);
    for (unsigned int i = 0; i < ChildLayers.m_size; i++)
        ChildLayers.m_data[i]->ParentLayer = this;
    numofbytes += numofChildLayersBytes;

    int numofTransformBytes;
    Transform.Deserialize(buffer + numofbytes, &numofTransformBytes);

    numofbytes += numofTransformBytes;

    if (nrbytes)
        *nrbytes = numofbytes;
    return result;
}

void BitmapLayer::RedrawChildLayer(int childlayertoredraw)
{
    if (childlayertoredraw == NOREDRAW)
        return;
    if (childlayertoredraw == ALLLAYERS)
        for (unsigned int i = 0; i < ChildLayers.m_size; i++)
            ChildLayers.m_data[i]->Draw();
    if ((childlayertoredraw >= 0) && (childlayertoredraw < (int)ChildLayers.m_size))
        ChildLayers.m_data[childlayertoredraw]->Draw();
}

void BitmapLayer::Draw(int childlayertoredraw)
{
    RedrawChildLayer(childlayertoredraw);
    if (ParentLayer)
        if (ParentLayer->BmRGBQuads)
        {
            if (Stretch)
                Bitmap32::Stretch(ParentLayer, DstX, DstY, DstW, DstH, SrcX, SrcY, SrcW, SrcH, Alpha, TransparentColor);
            else
                Bitmap32::Blt(ParentLayer, DstX, DstY, DstW, DstH, SrcX, SrcY, Alpha, TransparentColor);
        }
}

bool  BitmapLayer::ApplyTransform ()
{
    return false;
}

void BitmapLayer::AddChildLayer()
{
    ChildLayers.RebuildPreserve(ChildLayers.m_size + 1);
    ChildLayers.m_data[ChildLayers.m_size - 1]->ParentLayer = this;
}
