#ifndef commonvisualcomponents_h_
#define commonvisualcomponents_h_

class CToolBar: public CControl_Base
{
    int onedgenow;
    int buttondownw;
    int buttondownh;
    int avgcontrolh;
    int*** block1;
    CVector <RECT> ControlRects;
    CVector <int> rowminsizes;
    CVector <int> winnerindxs;
    int bordw;
    int bordh;
    int findminindx(int** combsmx, int nrcombs, int nrrows, int* sizz);
    void RefreshWinnerRowMinSizes();

public:
    CToolBar(int posx, int posy, int width, int height, CControl_Base* ParentObject, bool floating = true, int style = 0);
    virtual ~CToolBar();
    virtual int Obj_OnEraseBkgnd (HDC hdc);
    int  Obj_OnSizing(RECT* newsizerect, int fwside);
    void Obj_OnMouseDown(int button, int x, int y, int client);
    virtual int Refresh();
};

class CBitButton: public CControl_Base
{
protected:
    HBRUSH m_white_brush;
    HPEN m_white_pen;
    HBRUSH m_blue_brush;
    int Transpareny[3];
    HBITMAP hBMP, hBMPdown, hBMPhl;
    HDC bkgrnd;
    int BtnState;
    HFONT ButtonFont;

public:
    int tristate;
    CBitButton(int posx, int posy, int width, int height, const char* caption, CControl_Base* ParentObject, const char* bmp = 0, const char* bmpdown = 0, const char* bmphl = 0, int transpar = 255, int a_EID = 0);
    virtual ~CBitButton();
    virtual void Obj_OnMouseDown(int button, int x, int y, int client);
    virtual void Obj_OnMouseUp(int button, int x, int y, int client);
    virtual void Obj_OnMouseMove(int button, int x, int y);
    virtual void Obj_OnResize(int x, int y, int width, int height);
    void RebuildBkg(HDC hdc, int px, int py);
    virtual int Obj_OnEraseBkgnd (HDC hdc);
};

class CBitBtn: public CBitButton
{
protected:
    HBITMAP hBMPchacked;

public:
    CBitBtn(int posx, int posy, int width, int height, const char* caption, CControl_Base* ParentObject, const char* bmp = 0, const char* bmpdown = 0, const char* bmphl = 0, const char* rbmp = 0, int transpar = 255, int a_EID = 0);
    void UnCheck();
    void Check();
    void ToggleCheck();
    virtual void Obj_OnClick(int button, int x, int y);
    virtual ~CBitBtn();
};

class CPlot1D: public CControl_Base
{
public:
    CPlot1D(int posx, int posy, int width, int height, CControl_Base* ParentObject, bool floating = true);
    virtual ~CPlot1D();
};

class CShellBrowser: public CControl_Base
{
    ITEMIDLIST* RootPIDL;
    char        Path[MAX_PATH];
    HWND        TreeV;
    ITEMIDLIST* ParsePath(const char *szPath);
    static int  AccessBrowserParent(CControl_Base** shb, bool set = true);
    static int  __stdcall BrowseCallbackProc(HWND  hwnd, UINT  uMsg, LPARAM  lParam, LPARAM  lpData);

public:
    typedef void (EObject::*On_SelectionChanged) (CControl_Base* a_sender, char* selection);
    CList <On_SelectionChanged> OnSelectionChanged;
    void   Obj_OnSelectionChanged(char* selection);
    void   GetPath(char* path);
    char*  GetPath();
    void   SetPath(char* path);
    int    Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam);
    int    Move(int x, int y, int width, int height, bool refresh);
    int    Refresh();
    CShellBrowser(int posx, int posy, int width, int height, CControl_Base* ParentObject, char* rootpath = 0, bool floating = false, int style = 0);
    virtual ~CShellBrowser();
};

class CScrollBar: public CControl_Base
{
    CControl_Base* ScrBar;
    SCROLLINFO ScrollInfo;
    bool   FloatngPoint;
    double Min;
    double Max;
    double LastPos;
    int    Orientation;
    bool   UpSideDown;
    double LineStep;
    double PageStep;
    double ScrollRealMax;
    int    TrackerSize;
    void   CheckBounds(double*pos);

public:
    double ActualPos;

    typedef void (EObject::*On_Change)    (CControl_Base* a_sender, double pos, int button);
    typedef void (EObject::*On_EndScroll) (CControl_Base* a_sender, double pos, double lastpos);
    CList <On_Change>    OnChange;
    void   Obj_OnChange       (double pos, int button);
    CList <On_EndScroll> OnEndScroll;
    void   Obj_OnEndScroll    (double pos);
    void   SetFloatingPointStyle(bool floatingp);
    void   SetRange(double topleft, double bottomright);
    double GetMinF ();
    double GetMaxF ();
    int    GetMin  ();
    int    GetMax  ();
    void   SetPos  (double pos, bool refresh = true);
    double GetPosF ();
    int    GetPos  ();
    void   SetTrackerSize (int trackersize);
    void   SetPageStep    (double page);
    void   SetLineStep    (double page);
    double GetPageStep    ();
    double GetLineStep    ();
    int Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam);
    int Move(int x, int y, int width, int height, bool refresh);
    CScrollBar(int posx, int posy, int width, int height, CControl_Base* ParentObject, int orientation = HORIZONTAL, int Exstyle = 0, int style = 0x50000000);
    virtual ~CScrollBar();
};

class CSplitter: public CControl_Base
{
    CControl_Base* Window1;
    CControl_Base* Window2;
    WINDOWRECT    SpRect;
    int  Orientation;
    int  SpPos;
    int  Ratio;
    void MoveWindows(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
    int  MoveObjects(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

public:
    int  SpWeight;
    int  SpSpace;
    int  MinSizes;
    int  Refresh   ();
    int  GetPos    ();
    int  SetPos    (int pos);
    int  Move      (int x, int y, int width, int height, bool refresh = true);
    int  Obj_OnMove(int x, int y, int width, int height);
    void SetWindows(CControl_Base* window1, CControl_Base* window2);
    void GetRect   (WINDOWRECT* rect);
    CControl_Base*  GetWindow(int windowindx);
    CSplitter       (int posx, int posy, int width, int height, CControl_Base* ParentObject, int orientation = VERTICAL, const char* caption = 0, const char* clasname = "button", int Exstyle = 0, int style = 0x50000002 | BS_PUSHLIKE);
    virtual ~CSplitter();
};

class CBButton: public CControl_Base
{
protected:
    HBRUSH m_white_brush;
    HPEN m_white_pen;
    HBRUSH m_blue_brush;
    int Transpareny[3];
    HDC bkgrnd;
    int BtnState;
    HFONT ButtonFont;

public:
    int tristate;
    int BtnResReferenceIndex;
    CVectorRS <Bitmap32> BtnResource;
    CVectorRS <Bitmap32> * BtnResReference;
    CBButton(int posx, int posy, int width, int height, char* caption, CControl_Base* ParentObject, char* btnresfilename);
    CBButton(int posx, int posy, int width, int height, char* caption, CControl_Base* ParentObject, CVectorRS <Bitmap32> * btnresreference, int btnresreferenceindex);
    virtual ~CBButton();
    virtual void Obj_OnMouseDown(int button, int x, int y, int client);
    virtual void Obj_OnMouseUp(int button, int x, int y, int client);
    virtual void Obj_OnMouseMove(int button, int x, int y);
    void RebuildBkg(HDC hdc, int px, int py);
    virtual int Obj_OnEraseBkgnd (HDC hdc);
};

class BBtn: public CBButton
{
protected:
    HBITMAP hBMPchacked;

public:
    BBtn(int posx, int posy, int width, int height, char* caption, CControl_Base* ParentObject, char* btnresfilename, int regionpicureindex = -1);
    bool IsChecked();
    void UnCheck();
    void Check();
    void ToggleCheck();
    virtual void Obj_OnClick(int button, int x, int y);
    virtual ~BBtn();
};

class CheckBox: public CControl_Base
{
public:
    HFONT ButtonFont;
    CheckBox(CControl_Base* parent, char* caption, int posx, int posy, int width, int height, bool state = false);
    virtual void Obj_OnClick(int button, int x, int y);
    void SetState(bool state);
    bool GetState();
    virtual~CheckBox();
};

class CStringMX: public CMatrix<char[1000]> {};

class CEdit: public CControl_Base
{
public:
    CEdit(CControl_Base* parent, const char* caption, int x, int y, int w, int h);
    void GetText(char* string, int maxlen = 1000);
    void SetText(const char* string);
};

class CListView: public CControl_Base
{
    int lastheight;
    CStringMX* m_data;
    bool flipped;
    void ConstructMe(int start);
    void AddRow(int StrListindx, int listheight);
    void AddColumn(int StrListindx, int listheight, int indx);

public:
    CListView(CControl_Base* parent, int x, int y, int w, int h);
    void Redisplay();
    void Redisplay(CStringMX*  data);
    int GetSel();
    void SelectItem(int indx);
    void UnselectItem(int indx);
};

class CStringMXViewer: public CControl_Base
{
protected:
    CListView*     CListView1;
    CList<CEdit*>  RecordEditors;
    CBitButton*    AddButton;
    CBitButton*    RemoveButton;
    CBitButton*    UpdateButton;
    CStringMX*     StringMXRef;
    virtual int Obj_OnSizing(RECT* newsizerect, int fwside);
    virtual void Obj_OnChange();
    virtual void InitializeComponents();
    void CListView1_OnMouseUp (CControl_Base* a_sender, int button, int x, int y, int client);
    virtual void UpdateButton_OnClick(CControl_Base* a_sender, int button, int x, int y);
    virtual void RemoveButton_OnClick(CControl_Base* a_sender, int button, int x, int y);
    virtual void AddButton_OnClick(CControl_Base* a_sender, int button, int x, int y);

public:
    typedef void (EObject::*On_Change)    (CControl_Base* a_sender);
    CList <On_Change> OnChange;
    CStringMXViewer(CControl_Base* parent, int posx, int posy, char* caption, CStringMX*stringMXRef, bool floating = true);
    int Refresh();
};

class CFileChooser: public CControl_Base
{
protected:
    char* iFilter;
    bool iSaveOrLoad;
    virtual void Obj_OnFileChanged(char* aFile);
    virtual void ChooseButton_OnClick(CControl_Base* a_sender, int button, int x, int y);
    virtual void Obj_OnResize(int x, int y, int width, int height);
    BOOL GetFileName(HWND hwnd, BOOL bSave, const char pszFileName2[], const char* filterr = "All Files (*.*)\0*.*\0\0");

public:
    typedef void (EObject::*On_FileChanged) (CFileChooser* aSender, char* aFile);
    CList <On_FileChanged> OnFileChanged;
    CEdit*  iEditBox;
    CBitButton*     iChooseButton;
    CFileChooser(int aPosX, int aPosY, int aWidth, int aHeight, CControl_Base* aParentObject, int aStyle = 0, int aExtStyle = 0, bool aSaveOrLoad = false, const char* a_Filter = "All Files (*.*)\0*.*\0\0", const char* a_DefaultFile = 0);
    virtual~CFileChooser();
};

#endif // commonvisualcomponents_h_
