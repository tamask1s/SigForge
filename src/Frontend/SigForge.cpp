#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <shlobj.h>
#include <windows.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <Gdiplus.h>
using namespace std;
using namespace Gdiplus;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "bitmap.h"
#include "Defines.h"
#include "Control_Base.h"
#include "datastructures.h"
#include "commonvisualcomponents.h"
#include "VisualComponents.h"
#include "CodecLib.h"
#include "Backend.h"
#include "scripteditor.h"
#include "constants.h"
#include "ShellObjects.h"
#include "ZaxJsonParser.h"

class CAboutBox: public CControl_Base
{
public:
    CAboutBox(const char* a_copyright_info)
        : CControl_Base(0, 0, "About...", 0, 700, 250, 540, 150, 0, true)
    {
        ZaxJsonTopTokenizer l_info(a_copyright_info, false);
        if (l_info.m_values["general"])
            new CControl_Base(0, "static", l_info.m_values["general"], 0x50010141, 10, 10, 500, 80, this, false);
        if (l_info.m_values["libraries"])
        {
            ZaxJsonTopTokenizer l_libs(l_info.m_values["libraries"], false);
            for (unsigned int i = 0; i < l_libs.m_list_values.size(); ++i)
            {
                new CControl_Base(0, "static", l_libs.m_list_values[i], 0x50010141, 10, 140 + i * 100, 490, 40, this, false);
                //new CControl_Base(0, "static", a_copyright_info.m_data[i]->m_data[0].s, 0x50010141, 0, 70 + i * 100 + 40, 490, 40, this, false);
            }
        }
    }
};

struct TConfig_scrip_windows
{
    string file;
    int size = 0;
    ZAX_JSON_SERIALIZABLE(TConfig_scrip_windows, JSON_PROPERTY(file), JSON_PROPERTY(size))
};

struct TConfig
{
    vector<TConfig_scrip_windows> script_windows;
    ZAX_JSON_SERIALIZABLE(TConfig, JSON_PROPERTY(script_windows))
};

inline string read_file(const string& path)
{
    ostringstream buf;
    buf << ifstream(path.c_str()).rdbuf();
    return buf.str();
}

class CMainForm: public CControl_Base, public IBackendObserver
{
    IBackend*                    m_backend;
    CBitButton*                  m_record_button;
    CControl_Base*               m_mdi_client;
    CSplitter*                   m_hor_splitter;
    CSplitter*                   m_vert_splitter;
    CEdit*                       m_edit;
    CEdit*                       m_dataval_display = 0;
    map<string, CSignalDisplay*> m_codec_to_display_map;
    int                          m_splitterlastpos;
    TConfig                      m_config;
    map<CSignalDisplay*, set<CSignalDisplay*>> m_scroll_binding_map;

public:
    CMainForm(HINSTANCE hThisInstance, bool show_gui = true)
        : CControl_Base(0, 0, "Data Browser B31 24X uV", 0, 150, 20, 1601, 950, 0, true, hThisInstance)
    {
        m_config = read_file("SigForge.config");
        m_backend = IBackend::New(this);
        SetDirectories();
        if (show_gui)
            InitializeComponents();
        SetTimer(hWnd, 10011, 20, 0);
    }

    virtual ~CMainForm()
    {
        IBackend::Delete(m_backend);
        PostQuitMessage(0);
    };

    static inline void step_dir_up(std::string& path)
    {
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos)
            path = path.substr(0, pos);
    }

    void ExecuteScriptFile(const char* a_fiename)
    {
        int size;
        unsigned char* buff = LoadBuffer(a_fiename, 0, &size);
        if (buff)
        {
            char Script[size + 1];
            memcpy(Script, buff, size);
            Script[size] = 0;
            delete[] buff;
            string script_dir = a_fiename;
            step_dir_up(script_dir);
            SetCurrentDirectory(script_dir.c_str());
            m_backend->RunScript(Script, true);
        }
    }

private:
    int Backend_RequestDataDelete(char* a_data_codec)
    {
        if (!a_data_codec)
        {
            CloseAllWindows();
            return true;
        }
        else
        {
            CSignalDisplay* display = m_codec_to_display_map[a_data_codec];
            if (display)
                SendMessage(display->hWnd, WM_CLOSE, 0, 0);
            return display ? true : false;
        }
    }

    int Backend_OnDataOut(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
    {
        if (!a_data_codec)
        {
            if (!strcmp(a_fit_width, "manage_windows"))
            {
                if (!strcmp(a_display_type, "tile_horizontally"))
                    PostMessage(m_mdi_client->hWnd, WM_MDITILE, MDITILE_HORIZONTAL, 0);
                else if (!strcmp(a_display_type, "tile_vertically"))
                    PostMessage(m_mdi_client->hWnd, WM_MDITILE, MDITILE_VERTICAL, 0);
                else if (!strcmp(a_display_type, "cascade"))
                    PostMessage(m_mdi_client->hWnd, WM_MDICASCADE, 0, 0);
                else if (!strcmp(a_display_type, "maximize"))
                {
                    CSignalDisplay* display = m_codec_to_display_map[a_align];
                    if (display)
                        PostMessage(m_mdi_client->hWnd, WM_MDIMAXIMIZE, (WPARAM)display->hWnd, 0);
                }
            }
            else if (!strcmp(a_fit_width, "scale_window"))
            {
                CSignalDisplay* display = 0;
                if (a_align)
                    display = m_codec_to_display_map[a_align];
                else
                    display = m_codec_to_display_map.begin()->second;
                if (display)
                {
                    if (!strcmp(a_display_type, "horzontally_out"))
                        display->ZoomButtons_OnClick(display->m_button_zoom_out, 0, 0, 0); // unaddressable access here while closing a recording window before 20sec
                    else if (!strcmp(a_display_type, "horzontally_in"))
                        display->ZoomButtons_OnClick(display->m_button_zoom_in, 0, 0, 0);
                    else if (!strcmp(a_display_type, "vertically_out"))
                        display->ButtonMagnif_OnClick(display->m_button_shrink, 0, 0, 0);
                    else if (!strcmp(a_display_type, "vertically_in"))
                        display->ButtonMagnif_OnClick(display->m_button_magnif, 0, 0, 0);
                    else if (!strcmp(a_display_type, "reset_horizontal"))
                        display->ResetHorizontal();
                }
            }
            else if (!strcmp(a_fit_width, "setup_channels"))
            {
                CSignalDisplay* display = 0;
                if (a_align)
                    display = m_codec_to_display_map[a_align];
                else
                    display = m_codec_to_display_map.begin()->second;
                if (display)
                {
                    UIntVec visibe_channels;
                    visibe_channels.RebuildFrom(a_display_type);
                    display->SetupVisibleChannels(visibe_channels.m_data);
                }
            }
            return 0;
        }
        else
            return EditData(a_data_codec, a_fit_width, a_display_type, a_align, a_grid);
    }

    char* Backend_OnRefreshDataWindow(ISignalCodec* a_data, bool a_full_change, bool a_first_change)
    {
        map<string, CSignalDisplay*>::iterator lFound = m_codec_to_display_map.find(a_data->m_varname);
        if (lFound != m_codec_to_display_map.end())
        {
            if (a_first_change)
                lFound->second->SetDefaults();
            lFound->second->RebuildDataWindow(a_full_change);
        }
        return 0;
    }

    char* Backend_OnSystemExit()
    {
        PostMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;
    }

    void ScriptEditor1_On_RunScript(CScriptEditor* a_sender, const char* a_script)
    {
        char l_app_path[2048];
        DWORD result = GetModuleFileName(NULL, l_app_path, MAX_PATH);
        if (result != 0)
        {
            string app_path = l_app_path;
            step_dir_up(app_path);
            step_dir_up(app_path);
            app_path = app_path + "\\Scripts";
            SetCurrentDirectory(app_path.c_str());
        }
        m_backend->RunScript(a_script, true);
    }

    void SetDirectories()
    {}

    void InitializeComponents()
    {
        HMENU hMenu = LoadMenu(0, "PMAIN");
        SetMenu(hWnd, hMenu);
        CLIENTCREATESTRUCT ccs;
        ccs.hWindowMenu = GetSubMenu(GetMenu(hWnd), 2);
        ccs.idFirstChild = ID_MDI_FIRSTCHILD;

        m_mdi_client = new CControl_Base(0, "mdiclient", 0, WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | 4, 100, 100, 950, 600, this, false, 0, 0, (void*)&ccs);
        m_record_button = new CBitButton (10, 255, 134, 33, "Start/Restart", this);
        m_record_button->OnClick += &CMainForm::RecordButton_OnClick;
        m_dataval_display = new CEdit(this, "", 0, 0, 100, 250);
        m_edit = new CEdit(this, "_Recording_FE", 0, 0, 100, 250);
        CScriptEditor* scriptEditor = new CScriptEditor(m_backend->GetFunctionListRef(), m_config.script_windows.size() ? m_config.script_windows[0].file.c_str() : (char*)0, 0, 0, 100, 100, this, false);
        scriptEditor->m_on_run_script += &CMainForm::ScriptEditor1_On_RunScript;

        m_vert_splitter = new CSplitter(0, 22, 1600, 950, this, VERTICAL);
        m_vert_splitter->MinSizes = -1;
        m_vert_splitter->SpWeight = 10;
        m_vert_splitter->SpSpace = 0;
        m_vert_splitter->SetWindows(scriptEditor, m_mdi_client);
        m_splitterlastpos = 1100;
        m_vert_splitter->SetPos(m_splitterlastpos);
        SetWindowText(m_vert_splitter->hWnd, "<");
        m_vert_splitter->OnClick += &CMainForm::SnapButton_OnClick;

        CSplitter* last_splitter = m_vert_splitter;
        if (int nr_scripts = m_config.script_windows.size())
            for (int i = 1; i < nr_scripts; ++i)
            {
                int split_size = (nr_scripts - i) * ((950 - 50) / nr_scripts);
                if (m_config.script_windows[i].size)
                {
                    split_size += ((950 - 50) / nr_scripts);
                    split_size -= m_config.script_windows[i].size;
                }
                last_splitter = add_editor_to_splitter(last_splitter, m_config.script_windows[i].file.c_str(), split_size);
            }

        if (true)
            ShowWindow(hWnd, SW_MAXIMIZE);
        else
            Resize(1600, 950, true);
    }

    void SnapButton_OnClick(CControl_Base* a_sender, int button, int x, int y)
    {
        if (m_splitterlastpos == m_vert_splitter->GetPos())
        {
            if (m_vert_splitter->GetPos())
            {
                m_vert_splitter->SetPos(0);
                SetWindowText(m_vert_splitter->hWnd, ">");
            }
        }
        else
        {
            if (!m_vert_splitter->GetPos())
                m_vert_splitter->SetPos(m_splitterlastpos);
            else
                m_splitterlastpos = m_vert_splitter->GetPos();
            SetWindowText(m_vert_splitter->hWnd, "<");
        }
        SendMessage(m_vert_splitter->hWnd, WM_KILLFOCUS, 0, 0);
    }

    CSplitter* add_editor_to_splitter(CSplitter* parent_splitter, const char* script_file, int pos)
    {
        CScriptEditor* scriptEditor = new CScriptEditor(m_backend->GetFunctionListRef(), script_file, 0, 0, 100, 100, this, false);
        scriptEditor->m_on_run_script += &CMainForm::ScriptEditor1_On_RunScript;
        CSplitter* splitter = new CSplitter(0, 0, 400, 700, this, HORIZONTAL);
        CControl_Base* oldwindow1 = parent_splitter->GetWindow(1);
        CControl_Base* oldwindow2 = parent_splitter->GetWindow(2);
        parent_splitter->SetWindows(splitter, oldwindow2);
        splitter->SetWindows(scriptEditor, oldwindow1);
        splitter->SpWeight = 5;
        splitter->SpSpace  = 0;
        splitter->SetPos(pos);
        return splitter;
    }

    void Obj_OnResize(int x, int y, int width, int height)
    {
        if (HasControl(m_vert_splitter))
        {
            m_vert_splitter->Resize(width, height - 22);
            m_dataval_display->Move(width - 480, 0, 480, 22);
            m_record_button->Move(0, 0, 134, 22);
            m_edit->Move(134, 0, width - 134 - 480, 22);
        }
        return CControl_Base::Obj_OnResize(x, y, width, height);
    }

    void Children_On_MouseDown(CControl_Base* a_sender, int button, int x, int y, int client)
    {}

    void Children_On_DataEvent(CControl_Base* a_sender, int a_event, double a_val_x, double a_val_y, int a_data_channel, double a_data_val, char* a_info)
    {
        m_backend->DataEvent(((CSignalDisplay*)a_sender)->m_codec, a_event, a_val_x, a_val_y, a_data_channel, a_data_val, a_info);
    }

    void Children_On_DataHover(CControl_Base* a_sender, const char* a_text)
    {
        if (m_dataval_display)
            m_dataval_display->SetText(a_text);
    }

    void Children_On_DataScrolled(CControl_Base* a_sender, double pos_percentage)
    {
        auto set_bound = m_scroll_binding_map[(CSignalDisplay*)a_sender];
        for(auto d = set_bound.begin(); d != set_bound.end(); ++d)
            if (*d != a_sender)
                (*d)->SetScrollPosPercentage(pos_percentage);
    }

    bool scroll_binding_crc_check(CSignalDisplay* orig_src, CSignalDisplay* current_src/*, CSignalDisplay* current_target*/)
    {
        bool result = true;
        if (m_scroll_binding_map[current_src].find(orig_src) != m_scroll_binding_map[current_src].end())
            result = false;
        else
        {
            auto set_bound = m_scroll_binding_map[current_src];
            for(auto d = set_bound.begin(); d != set_bound.end(); ++d)
                if ((result = scroll_binding_crc_check(orig_src, *d)) != true)
                    break;
        }
        return result;
    }

    char* Backend_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding)
    {
        char* result = 0;
        CSignalDisplay* src = m_codec_to_display_map[string(a_src_data_name)];
        CSignalDisplay* dst = m_codec_to_display_map[string(a_dst_data_name)];
        if (src && dst)
        {
            if (scroll_binding_crc_check(src, dst))
            {
                if (!a_remove_binding || strcmp(a_remove_binding, "false"))
                {
                    m_scroll_binding_map[src].insert(dst);
                    src->RebuildDataWindow(false);
                }
                else
                    m_scroll_binding_map[src].erase(dst);
            }
            else
            {
                result = new char [200];
                sprintf(result, "%s '%s' or '%s'.", "ERROR: BindScrolling: circular redundancy error: ", a_src_data_name, a_dst_data_name);
            }
        }
        else
        {
            result = new char [200];
            sprintf(result, "%s '%s' or '%s'.", "ERROR: BindScrolling: source or target data not found: ", a_src_data_name, a_dst_data_name);
        }
        return result;
    }

    void CloseAllWindows()
    {
        while (m_codec_to_display_map.size())
            SendMessage(m_codec_to_display_map.begin()->second->hWnd, WM_CLOSE, 0, 0);
    }

    void RecordButton_OnClick(CControl_Base* a_sender, int button, int x, int y)
    {
        CloseAllWindows();
        char tetx[10000], script[10000];
        m_edit->GetText(tetx);
        snprintf(script, 10000, "DataAq(1, 192.168.4.1:80, 2000 2000 2000 2000, 24 24 24 24, 0 1 2 3, 100, 4, DateAndTime(,_%s.bdf));\
                 SystemInterval(int1, 20000){\
                 DisplayData(manage_windows, tile_horizontally); \
                 DisplayData(scale_window, horzontally_out); \
                 DisplayData(scale_window, horzontally_out); \
                 DisplayData(scale_window, horzontally_out); \
                 DisplayData(scale_window, horzontally_out); \
                 DisplayData(scale_window, horzontally_out); \
                 DisplayData(scale_window, horzontally_out); \
                 DisplayData(scale_window, vertically_in); \
                 DisplayData(scale_window, vertically_in); \
                 DisplayData(scale_window, vertically_in); \
                 DisplayData(scale_window, vertically_in); \
                 DisplayData(setup_channels, 1 1 1 0); \
                 SystemInterval(int1);\
                 };", tetx);
        m_backend->RunScript(script, false);
    }

    void ChildActivated()
    {
        HMENU hMenu, hFileMenu;
        BOOL EnableFlag;
        hMenu = GetMenu(hWnd);
        EnableFlag = TRUE;
        EnableMenuItem(hMenu, 1, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hMenu, 2, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hMenu, 3, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hMenu, 4, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));

        hFileMenu = GetSubMenu(hMenu, 0);
        EnableMenuItem(hFileMenu, CM_FILE_SAVE,   MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(hFileMenu, CM_FILE_SAVEAS, MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));

        DrawMenuBar(hWnd);
    }

    void CSignalDisplay_OnDestroy(CSignalDisplay* a_sender)
    {
        m_codec_to_display_map.erase(a_sender->m_codec->m_varname);
        m_scroll_binding_map.erase(a_sender);
        for(auto d = m_scroll_binding_map.begin(); d != m_scroll_binding_map.end(); ++d)
            d->second.erase(a_sender);

        m_backend->DestroyCodec(a_sender->m_codec);
    }

    int EditData(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid, bool aDestroyDataOnClose = true) //todonow: parameter/new func for new or same window in case of an existing varname
    {
        int result = 0;
        CSignalDisplay* child2 = m_codec_to_display_map[a_data_codec->m_varname];
        if (child2)
        {
            m_backend->DestroyCodec(child2->m_codec);
            child2->ResetCodec(a_data_codec);
            child2->RebuildDataWindow(false);
        }
        else if (a_data_codec)
        {
            EDisplayType lDisplayType = EDisplayType::EDisplayType_1D_waveforms;
            if (a_display_type)
            {
                if (!strcmp(a_display_type, "2D_map"))
                    lDisplayType = EDisplayType::EDisplayType_2D_map;
                else if (!strcmp(a_display_type, "2D_map_surface"))
                    lDisplayType = EDisplayType::EDisplayType_2D_map_surface;
                else if (!strcmp(a_display_type, "value_list"))
                    lDisplayType = EDisplayType::EDisplayType_value_list;
                else if (!strcmp(a_display_type, "xy_plot"))
                    lDisplayType = EDisplayType::EDisplayType_xy_plot;
            }
            EFitWidthType lfitWidth = EFitWidthType::EFitWidthType_normal;
            if (a_fit_width)
                if (!strcmp(a_fit_width, "fit_width") || !strcmp(a_fit_width, "true"))
                    lfitWidth = EFitWidthType::EFitWidthType_fitWidth;
            child2 = new CSignalDisplay("NO DATA", a_data_codec, lfitWidth, lDisplayType, a_align, a_grid, m_mdi_client, this);
            child2->OnMouseDown += &CMainForm::Children_On_MouseDown;
            child2->OnDataHover += &CMainForm::Children_On_DataHover;
            child2->OnDataEvent += &CMainForm::Children_On_DataEvent;
            child2->OnDataScrolled += &CMainForm::Children_On_DataScrolled;
            child2->EID = 1000 + m_codec_to_display_map.size();
            if (aDestroyDataOnClose)
                child2->OnDestroy += &CMainForm::CSignalDisplay_OnDestroy;
            ChildActivated();
            m_codec_to_display_map[a_data_codec->m_varname] = child2;
            result = 1;
        }
        return result;
    }

    int Obj_OnMessage(HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
    {
        if (a_message == WM_TIMER)
            m_backend->Interval();

        if (a_message == WM_COMMAND)
        {
            switch(LOWORD(a_wparam))
            {
            case CM_FILE_SAVE:
            {}
            break;
            case CM_FILE_OPEN:
            {
                char lExtensions[10000];
                memset(lExtensions, 0, sizeof(lExtensions));
                QueryCodecExtensions(lExtensions);

                char FileName[MAX_PATH];
                if (!GetFileNameT(a_hwnd, FileName, FALSE, 1, lExtensions))
                    break;
                for (unsigned int i = 0; i < 1; ++i)
                    m_backend->Open_File(FileName, 0);
            }
            break;
            case CM_HELP_ABOUT:
            {
                new CAboutBox(m_backend->GetCopyrightTextRef());
            }
            break;
            case CM_OPTIONS_SETUPVCHANS:
            {}
            break;
            case CM_TOOLS_FILTERS:
            {}
            break;
            case CM_FILE_EXIT:
                PostMessage(hWnd, WM_CLOSE, 0, 0);
                break;
            case CM_FILE_ACQUIRE:
                MessageBox(NULL, "No devices found.", "USB device scanner:", MB_OK);
                break;
            case CM_FILE_NEW:
            {
                CScriptEditor* mScriptEditor = new CScriptEditor(m_backend->GetFunctionListRef(), "Script editor", 300, 150, 700, 500, this, true);
                mScriptEditor->m_on_run_script += &CMainForm::ScriptEditor1_On_RunScript;
            }
            break;
            case CM_WINDOW_TILEHORZ:
                PostMessage(m_mdi_client->hWnd, WM_MDITILE, MDITILE_HORIZONTAL, 0);
                break;
            case CM_WINDOW_TILEVERT:
                PostMessage(m_mdi_client->hWnd, WM_MDITILE, MDITILE_VERTICAL, 0);
                break;
            case CM_WINDOW_CASCADE:
                PostMessage(m_mdi_client->hWnd, WM_MDICASCADE, 0, 0);
                break;
            case CM_WINDOW_CLOSEALL:
                CloseAllWindows();
                break;
            case CM_WINDOW_ARRANGE:
                PostMessage(m_mdi_client->hWnd, WM_MDIICONARRANGE, 0, 0);
                break;
            default:
                return DefFrameProc(a_hwnd, m_mdi_client->hWnd, a_message, a_wparam, a_lparam);
            }
        }
        return CControl_Base::Obj_OnMessage(a_hwnd, a_message, a_wparam, a_lparam);
    }
};

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    HINSTANCE hinstLib = LoadLibrary(TEXT("RICHED20.DLL"));
    InitCBLib();
    InitBitmapLib();
    CMainForm* mainForm = new CMainForm(hThisInstance, (lpszArgument && strlen(lpszArgument)) ? false : true);
    if (lpszArgument && strlen(lpszArgument))
        mainForm->ExecuteScriptFile(lpszArgument);
    int res = RunAppD();
    FreeLibrary(hinstLib);
    GdiplusShutdown(gdiplusToken);
    return res;
}
