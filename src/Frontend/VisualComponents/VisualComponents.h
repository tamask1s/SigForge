#ifndef COMMON_VISUAL_COMPONENTS_
#define COMMON_VISUAL_COMPONENTS_

#define HOVER_NO_VALUE 10000000

struct TVisualization
{
    bool                  m_seeking_allowed;            // READDATA has the right to read from the disk (the flag is set after the data is painted)
    float                 m_horizontal_units_to_display;
    int                   m_auto_offset_adjust;
    int                   m_show_grid;                  // 0 none; 1-horizontal big;2 horizontal small; 32vertical big; 64 vertical small
    int                   m_line_surface_table;         // 0:lines; 1:surface; 2:table
    int                   m_view_style;
    CVector<int>          m_visible_channels;
    CVector<float>        m_magnifiers;                 // channel magnifiers
    CVector<int>          m_flips;                      // flip channels
    float                 m_global_magnifier;           // global magnifier
    vector<POINT>         m_points_ref;
    CMatrixVarLen<double> m_visible_buffer;             // visible data buffer
    CVector<TMarker*>     m_marker_refs;                // visible markers
    int                   m_hover_x = 0;
    int                   m_hover_ch = 0;
    int                   m_last_hover_x = HOVER_NO_VALUE;
    int                   m_last_hover_y = HOVER_NO_VALUE;
    int                   m_last_cross_datapoints[60];
    int                   m_line_style = 1; //0: antialiased 1: normal line
    int                   m_line_width = 1;

    TVisualization();
    ~TVisualization();
    int GetActiveChNr();
};

class PlotPanel: public CControl_Base
{
    HFONT  m_font;
    HBRUSH m_white_brush;
    HPEN   m_white_pen;
    HPEN   m_black_pen;
    HBRUSH m_blue_brush;
    double m_max_stop_phys_x_3_div_stop_phys_x_2;
    int    m_marker_indx;
    int    m_start_stop;
    int    m_mouse_down_x;

    void DrawCoord(HDC hdc, RECT* rect, double a_min_phy_s_x, double a_max_phy_s_x, int a_data_size);
    void DrawVCoord(HDC hdc, RECT* rect, double a_min_phy_s_x, double a_max_phy_s_x);
    void DrawVCoord(RECT* Rect, double offset, int magnifindx = 0);
    static void CalculateCoordParameters(int scale_area_len, double a_min_phy_s_x, double a_max_phy_s_x, vector<double>&, double& physxxwidth_div_intlen, double& hossz1_div_ints, double& rate1, double& physw);
    void PaintVCoord(vector<double>& values, HDC hdc, RECT* rect, int* border, int scale_area_len, double a_min_phy_s_x, double physxxwidth_div_intlen, double hossz1_div_ints, double rate1, double physw);
    void SetCursors(int x);

public:
    CMatrix<double> dmx1;
    TVisualization* m_vis;
    ISignalCodec*   m_codec;
    double          m_startphysx;
    double          m_stopphysx;

    PlotPanel(const char* caption, int posx, int posy, int width, int height, CControl_Base* parent);
    int Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam);
    void Obj_OnResize(int x, int y, int width, int height);
    void Paint(bool a_paint_hover_only = false);
    virtual ~PlotPanel();
    void Obj_OnMouseMove(int button, int x, int y);
    void Obj_OnMouseDown(int button, int x, int y, int client);
    void Obj_OnMouseUp(int button, int x, int y, int client);
    bool TMarkersHasPoint(int x, int* markerindx = 0, int* startstop = 0, char* marker_label = 0);
};

typedef void (EObject::*On_Changes) (CControl_Base* a_sender, int chanibndx);
class SetupChannels: public CControl_Base
{
    int                 m_checkboxes_start;
    CBitButton*         m_button_selectNone;
    CBitButton*         m_button_selectAll;
    TVisualization*     m_vis;

public:
    CList<On_Changes> OnChanges;

    SetupChannels(const char* caption, int posx, int posy, int width, int height, CControl_Base* parent, TVisualization* vis, ISignalCodec* signalcodec = 0);
    ~SetupChannels();
    void Obj_OnChanges(int chanibndx);
    void CheckBoxes_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void Button_SelectNone_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void Button_SelectAll_OnClick(CControl_Base* a_sender, int button, int x, int y);
    int Obj_OnMessage(HWND a_hwnd, int a_message, int a_wparam, int a_lparam);
    virtual bool Obj_OnClose();
};

typedef void (EObject::*On_Changes) (CControl_Base* a_sender, int chanibndx);
class MagnifControlPanel: public CControl_Base
{
    unsigned int m_check_btn_start_indx;
    int          m_selector_btns_start;
    int          m_magnif_scrolls_start;
    int          m_labels_start;

public:
    CList<On_Changes> OnChanges;
    TVisualization*   m_vis;
    ISignalCodec*     m_codec;

    MagnifControlPanel(const char* caption, int posx, int posy, int width, int height, CControl_Base* parent);
    void Obj_OnChanges (int chanibndx);
    void RebuildMagnifControls();
    void SelectionBtns_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void Obj_OnResize(int x, int y, int width, int height);
    int Obj_OnMessage(HWND a_hwnd, int a_message, int a_wparam, int a_lparam);
    virtual bool Obj_OnClose();
    virtual ~MagnifControlPanel();
//    CBitButton* LoadButton;
//    CBitButton* SaveButton;
};

enum EFitWidthType
{
    EFitWidthType_normal,
    EFitWidthType_fitWidth,
    EFitWidthType_end
};

enum EDisplayType
{
    EDisplayType_1D_waveforms,
    EDisplayType_2D_map,
    EDisplayType_2D_map_surface,
    EDisplayType_value_list,
    EDisplayType_xy_plot,
    EDisplayType_end
};

class CSignalDisplay: public CControl_Base
{
    SetupChannels*      m_setup_channels;
    TVisualization      m_vis;
    CScrollBar*         m_scrollbar;
    PlotPanel*          m_plot_panel;
    CBitBtn*            m_button_setup_visible_channels;
    MagnifControlPanel* m_magnif_control_panel;
    int                 m_selector_btns_start;
    int                 m_magnif_scrolls_start;
    bool                m_now_scrolling;
    EDisplayType        m_display_type;
    EFitWidthType       m_fit_width_type;
    int                 m_splitterlastpos;

    void CreateComponent(const char* caption, CControl_Base* mdiclient, CControl_Base* eparent);
    void SetupChannels_OnChanges(CControl_Base* a_sender, int indx);
    void Obj_OnResize(int x, int y, int width, int height);
    int Obj_OnSizing(RECT* newsizerect, int fwside);
    void SetupChannels_OnDestroy(CControl_Base* a_sender);
    void SetupVisibleChannels_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void CheckHorizontalUnitsToDisplay();
    void MagnifControlPanel_On_Changes(CControl_Base* a_sender, int chanibndx);
    void Obj_OnMouseDown(int button, int x, int y, int client);
    void Scrollbar_OnChange(CControl_Base* a_sender, double curpos, int button);
    void Scrollbar_OnEndScroll(CControl_Base* a_sender, double pos, double lastpos);
    void RebuildVisibleBuffer();
    void ApproxGlobalMagnif();
    void PlotPanel_OnDataHover(CControl_Base* a_sender, int button, int x, int y);
    void PlotPanel_OnMouseDown(CControl_Base* a_sender, int button, int x, int y, int client);
    void DataHover(int x, int y, int& a_data_channel, double& a_val_x, double& a_val_y, double& a_data_val, char* a_info);

public:
    CSplitter*       m_v_splitter;
    CControl_Base*   m_button_zoom_in;
    CControl_Base*   m_button_zoom_out;
    CControl_Base*   m_button_magnif;
    CControl_Base*   m_button_shrink;
    CControl_Base*   m_button_hor_grid;
    CControl_Base*   m_button_ver_grid;
    CControl_Base*   m_button_line_surf_table;
    CBitBtn*         m_button_auto_offset_adjust;
    ISignalCodec*    m_codec;

    typedef void (EObject::*On_DataHover) (CControl_Base* a_sender, const char* a_text);
    CList<On_DataHover> OnDataHover;
    typedef void (EObject::*On_DataEvent) (CControl_Base* a_sender, int a_event, double a_val_x, double a_val_y, int a_data_channel, double a_data_val, char* a_info);
    CList<On_DataEvent> OnDataEvent;
    typedef void (EObject::*On_DataScrolled) (CControl_Base* a_sender, double pos_percentage);
    CList<On_DataScrolled> OnDataScrolled;

    void SnapButton1_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void ZoomButtons_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void ButtonAutoOffsetAdjust_OnClick(CBitBtn* a_sender, int button, int x, int y);
    void ButtonLineSurfTable_OnClick(CBitBtn* a_sender, int button, int x, int y);
    void ButtonMagnif_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void ButtonGrid_OnClick(CControl_Base* a_sender, int button, int x, int y);
    void ResetCodec(ISignalCodec* a_codec);
    CSignalDisplay(const char* caption, CControl_Base* mdiclient, CControl_Base* eparent);
    CSignalDisplay(const char* caption, ISignalCodec* a_data_codec, int fitWidth, int displayType, char* a_align, char* a_grid, CControl_Base* mdiclient, CControl_Base* eparent);
    void Replot(bool a_rebuild_visual_buffer = false);
    void RebuildDataWindow(bool a_jump_to_end = false);
    void SetScrollPosPercentage(double pos_percentage);
    void SetupVisibleChannels(unsigned int* a_channels);
    ~CSignalDisplay();
    void SetDefaults();
    void ResetHorizontal();
    virtual bool Obj_OnClose();
};

#endif // COMMON_VISUAL_COMPONENTS_
