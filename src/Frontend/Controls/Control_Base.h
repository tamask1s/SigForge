#ifndef _CControl_Base_H
#define _CControl_Base_H

#ifndef WINVER
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#endif

#define PLATFORM_WINDOWS

void InitCBLib();

class Debugger
{
public:
    template <typename dvt1, typename dvt2, typename dvt3, typename dvt4, typename dvt5>
    void Print(dvt1 indv1, dvt2 indv2, dvt3 indv3, dvt4 indv4, dvt5 indv5)
    {
        DPrint (indv1, 0);
        DPrint (indv2, 1);
        DPrint (indv3, 2);
        DPrint (indv4, 3);
        DPrint (indv5, 4);
        CDPrint();
    }
    template <typename dvt1, typename dvt2, typename dvt3, typename dvt4>
    void Print(dvt1 indv1, dvt2 indv2, dvt3 indv3, dvt4 indv4)
    {
        Print (indv1, indv2, indv3, indv4, "N/A");
    }
    template <typename dvt1, typename dvt2, typename dvt3>
    void Print(dvt1 indv1, dvt2 indv2, dvt3 indv3)
    {
        Print (indv1, indv2, indv3, "N/A", "N/A");
    }
    template <typename dvt1, typename dvt2>
    void Print(dvt1 indv1, dvt2 indv2)
    {
        Print (indv1, indv2, "N/A", "N/A", "N/A");
    }
    template <typename dv1>
    void Print(dv1 indv1)
    {
        Print (indv1, "N/A", "N/A", "N/A", "N/A");
    }
    void Pause();
    void Watch(int* indv, int datalen, int tableindx, bool jumptoend = false);
    void Watch(unsigned int* indv, int datalen, int tableindx, bool jumptoend = false);
    void Watch(float* indv, int datalen, int tableindx, bool jumptoend = false);
    void Watch(byte* indv, int datalen, int tableindx, bool jumptoend = false);
    void Watch(char** indv, int datalen, int tableindx, bool jumptoend = false);
    void Watch(double* indv, int datalen, int tableindx, bool jumptoend = false);
private:
    static char dpd[5][1000];            /*STATIC DEBUGGER BUFFER*/
    void DPrint(int indv, int indx);
    void DPrint(double& indv, int indx);
    void DPrint(float& indv, int indx);
    void DPrint(const char*& indv, int indx);
    void DPrint(char* indv, int indx);
    void CDPrint();
};

class BitmapLayer: public Bitmap32
{
public:
    BitmapLayer*            ParentLayer;
    CListRS <BitmapLayer> ChildLayers;
    /*--------------------------------DRAWING STYLE DATA--------------------------*/
    int   Alpha;
    int   Stretch;
    int   TransparentColor;
    WTransform Transform;
    /*--------------------------------EO DRAWING STYLE DATA-----------------------*/
    int   DstX, DstY, DstW, DstH, SrcX, SrcY, SrcW, SrcH;
    BitmapLayer            ();
    void  RedrawChildLayer (int childlayertoredraw = ALLLAYERS);
    void  Draw             (int childlayertoredraw = NOREDRAW);
    void  AddChildLayer    ();
    int   GetSerializedLen ();
    byte* Serialize        (unsigned int* nrbytes = 0);
    bool  Deserialize      (byte* buffer, int* nrbytes = 0);
    bool  ApplyTransform   ();
};

#define EObject CControl_Base

class CControl_Base
{
    bool  NowTracking;
    void InitCControl_Base(HINSTANCE HINST);
    static int __stdcall CControl_BaseWindowProc(HWND a_hwnd, int a_message, int a_wparam, int a_lparam);
    WNDPROC OldWndProc;
    int mousestate;
    int NowStartedToSize;
    RECT ButtonDownRect;

protected:
    int   ID;

public:
    int   EID;
    typedef void (EObject::*On_Create)     (CControl_Base* a_sender, int wParam, int lParam);
    typedef bool (EObject::*On_Close)      (CControl_Base* a_sender);
    typedef void (EObject::*On_Destroy)    (CControl_Base* a_sender);
    typedef void (EObject::*On_NcDestroy)  (CControl_Base* a_sender);
    typedef void (EObject::*On_Click)      (CControl_Base* a_sender, int button, int x, int y);
    typedef void (EObject::*On_DoubleClick)(CControl_Base* a_sender, int button, int x, int y);
    typedef void (EObject::*On_MouseDown)  (CControl_Base* a_sender, int button, int x, int y, int client);
    typedef void (EObject::*On_MouseUp)    (CControl_Base* a_sender, int button, int x, int y, int client);
    typedef void (EObject::*On_MouseEnter) (CControl_Base* a_sender);
    typedef void (EObject::*On_MouseLeave) (CControl_Base* a_sender);
    typedef void (EObject::*On_MouseWheel) (CControl_Base* a_sender, int button, int x, int y, int z);
    typedef void (EObject::*On_MouseMove)  (CControl_Base* a_sender, int button, int x, int y);
    typedef int  (EObject::*On_Move)       (CControl_Base* a_sender, int x, int y, int width, int height);
    typedef void (EObject::*On_Moved)      (CControl_Base* a_sender, int x, int y, int width, int height);
    typedef void (EObject::*On_Resize)     (CControl_Base* a_sender, int x, int y, int width, int height);
    typedef int  (EObject::*On_Sizing)     (CControl_Base* a_sender, RECT* newsizerect, int fwside);
    typedef int  (EObject::*On_SetCursor)  (CControl_Base* a_sender, int wParam, int lParam);
    typedef int  (EObject::*On_Message)    (CControl_Base* a_sender, int message, int wParam, int lParam);
    typedef int  (EObject::*On_EraseBkgnd) (CControl_Base* a_sender, HDC hdc);
    typedef int  (EObject::*On_Paint)      (CControl_Base* a_sender, HDC hdc);

    int MoveCursor;
    int OverCursor;
    int MoveButton;
    int AlowMove;
    int SizingMargin;
    int MoveEngineIndx;
    bool TrackingEnabled;
    bool MouseIsOverControl;

    HWND hWnd;
    HWND ParentWnd;
    CControl_Base* ParentObj;

    bool dontdeleteondestroy;
    CList <CControl_Base*> Controls;

    static int StartApp(char* a_file_name, char* a_params = 0);
    void SaveChildPositions(char* filename);
    void LoadChildPositions(char* filename);
    void SetChildsToEdit();
    void DefaultDragHandler(CControl_Base* a_sender, int x, int y, int width, int height);
    void TogglePopupStyle();
    int EmptyControls();
    int EmptyControlsFromTo(int eidstart, int eidstorp);
    int ChackMargins(RECT& rect, POINT& movepoint);
    int IsPopup();
    int IsChild();
    int IsCursorOnWindow();
    bool HasControl(CControl_Base* control);
    virtual void Hide();
    virtual void Show();
    virtual int  Refresh();
    virtual void PaintBackBitmap(bool refresh = false);
    void SetRegion(HRGN hRgn);
    virtual int Move(int x, int y, int width = NOCHANGE, int height = NOCHANGE, bool refresh = true);
    virtual int Resize(int width, int height, bool refresh = true);

    virtual void CBSetCursor(int CursorIndx);
    void GetClientCoordinates(RECT* WindowRect);
    void GetScreenCoordinates(RECT* WindowRect);

    CList <On_SetCursor> OnSetCursor;
    virtual int Obj_OnSetCursor (HWND hwnd, int wParam, int lParam);

    CList <On_Message> OnMessage;
    virtual int Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam);

    CList <On_EraseBkgnd> OnEraseBkgnd;
    virtual int Obj_OnEraseBkgnd (HDC hdc);

    CList <On_Paint> OnPaint;
    virtual int Obj_OnPaint (HDC hdc);

    CList <On_Click> OnClick;
    virtual void Obj_OnClick(int button, int x, int y);

    CList <On_Create> OnCreate;
    virtual void Obj_OnCreate(int wParam, int lParam);

    CList <On_Destroy> OnDestroy;
    virtual void Obj_OnDestroy();

    CList <On_Close> OnClose;
    virtual bool Obj_OnClose();

    CList <On_MouseMove> OnMouseMove;
    virtual void Obj_OnMouseMove(int button, int x, int y);

    CList <On_Move> OnMove;
    virtual int Obj_OnMove(int x, int y, int width, int height);

    CList <On_Moved> OnMoved;
    virtual void Obj_OnMoved(int x, int y, int width, int height);

    CList <On_Resize> OnResize;
    virtual void Obj_OnResize(int x, int y, int width, int height);

    CList <On_Sizing> OnSizing;
    virtual int  Obj_OnSizing(RECT* newsizerect, int fwside);

    CList <On_MouseDown> OnMouseDown;
    virtual void Obj_OnMouseDown(int button, int x, int y, int client);

    CList <On_MouseUp> OnMouseUp;
    virtual void Obj_OnMouseUp(int button, int x, int y, int client);

    CList <On_MouseEnter> OnMouseEnter;
    virtual void Obj_OnMouseEnter();

    CList <On_MouseLeave> OnMouseLeave;
    virtual void Obj_OnMouseLeave();

    void Create (int Exstyle, const char* Classname, const char* caption, int style, int posx, int posy, int width, int height, CControl_Base* ParentObject, bool Floating = false, HINSTANCE HINST = 0, On_Create On_CreateHandler = 0, void* hmdiclientcreatestruct = 0, int createchild = false, int a_EID = 0);
    CControl_Base(int Exstyle, const char* Classname, const char* caption, int style, int posx, int posy, int width, int height, CControl_Base* ParentObject, bool Floating = false, HINSTANCE HINST = 0, On_Create On_CreateHandler = 0, void* hmdiclientcreatestruct = 0, int createchild = false, int a_EID = 0);
    CControl_Base();
    virtual~CControl_Base();
    BitmapLayer BackBitmap;
    int   DrawLevel;
    bool  IsMdiChild;
};

int RunApp();
int RunAppD();
int RunAppDWhile(bool*releaseflag = 0);
void CopyString(char** dest, char* source);
char* CopyString(const char* source);

void DefaultDragHandler(CControl_Base* a_sender, int x, int y, int width, int height);
HRGN CreateRegion(const char* imageFile);
HRGN CreateRegion(Bitmap32* bitmap, int crTransparent);

#endif
