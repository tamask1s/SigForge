#include <iostream>
#include <vector>
#include <map>
#include <shlobj.h>
#include <windows.h>
#include <stdio.h>
using namespace std;

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
#include "BackendObserver_dynamic_interface.h"
#include "Backend_dynamic_interface.h"
#include "scripteditor.h"
#include "constants.h"
#include "ShellObjects.h"
#include "ZaxJsonParser.h"

DGetDataRaw_PROC_TYPE DGetDataRaw_PROC;
DGetNrChannels_PROC_TYPE DGetNrChannels_PROC;
DGetTotalSamples_PROC_TYPE DGetTotalSamples_PROC;
DGetSampleRates_PROC_TYPE DGetSampleRates_PROC;
RunScript_PROC_TYPE RunScript_PROC;
OpenFile_PROC_TYPE OpenFile_PROC;
Interval_PROC_TYPE Interval_PROC;
InitializeBackend_PROC_TYPE InitializeBackend_PROC;
DestroyData_PROC_TYPE DestroyData_PROC;
GetCopyrightInfo_PROC_TYPE GetCopyrightInfo_PROC;
GetFunctionListRef_PROC_TYPE GetFunctionListRef_PROC;
SetDescription_PROC_TYPE SetDescription_PROC;
GetDescription_PROC_TYPE GetDescription_PROC;
GetFileFormatExtensions_PROC_TYPE GetFileFormatExtensions_PROC;
GetUnits_PROC_TYPE GetUnits_PROC;

class IBackendObserver
{
public:
    virtual int Backend_RequestDataDelete(char* a_data_codec) = 0;
    virtual int Backend_OnDataOut(const char* a_dataname, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid) = 0;
    virtual char* Backend_OnRefreshDataWindow(const char* a_dataname, bool a_full_change, bool a_first_change) = 0;
    virtual char* Backend_OnSystemExit() = 0;
    virtual char* Backend_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding) = 0;
};

IBackendObserver* g_backend_observer;

class CDataWrap: public ISignalCodec
{
public:
    CDataWrap(const char* a_name)
    {
        strcpy(m_varname, a_name);
        m_signalcodec_type = SignalCodecType_GVARCODEC;
        int nr_channels = DGetNrChannels_PROC(m_varname);
        unsigned int total_samples[nr_channels];
        DGetTotalSamples_PROC(m_varname, total_samples);
        m_total_samples.Rebuild(nr_channels);
        m_labels.Rebuild(nr_channels);
        m_transducers.Rebuild(nr_channels);
        m_vertical_units.Rebuild(nr_channels);
        for (unsigned int i = 0; i < m_total_samples.m_size; i++)
        {
            m_total_samples.m_data[i] = total_samples[i];
            m_labels.m_data[i].s[0] = 0;
            m_transducers.m_data[i].s[0] = 0;
            m_vertical_units.m_data[i].s[0] = 0;
        }

        //SetDescription_PROC(m_varname, R"({"recording_info": "Ez itt hetvenkilenc karakter lesz ha eszembee jut elég szó hozzá üöíé lássuk eztttttttttt"})");
        //SetDescription_PROC(m_varname, R"({"patient":"Evelin", "recording_info":"Klinika ketto", "date":"12.07.00", "time":"01.45.20", "horizontal_units":"sec", "surface2D_vert_units":"mV", "vertical_units":["mV", "mV", "mV", "mV"], "labels":["label 1", "label 2", "label 3", "label 4"], "transducer_type":["EKG 1", "EKG 2", "EKG 3", "EKG 4"]})");
        const char* descr = GetDescription_PROC(m_varname);
        bool success = false;
        ZaxJsonTopTokenizer document(descr, false, &success);
        if (success)
        {
            if (const char* patient = document.m_values["patient"])
                strcpy(m_patient, patient);
            if (const char* recording_info = document.m_values["recording_info"])
                strcpy(m_recording, recording_info);
            if (const char* date = document.m_values["date"])
                strcpy(m_date, date);
            if (const char* time = document.m_values["time"])
                strcpy(m_time, time);
            if (const char* vertical_units = document.m_values["vertical_units"])
            {
                ZaxJsonTopTokenizer vertical_units_document(vertical_units, false, &success);
                if (success)
                    for (unsigned int i = 0; i < vertical_units_document.m_list_values.size(); ++i)
                        strcpy(m_vertical_units.m_data[i].s, vertical_units_document.m_list_values[i]);
            }
            if (const char* vertical_units = document.m_values["labels"])
            {
                ZaxJsonTopTokenizer labels_document(vertical_units, false, &success);
                if (success)
                    for (unsigned int i = 0; i < labels_document.m_list_values.size(); ++i)
                        strcpy(m_labels.m_data[i].s, labels_document.m_list_values[i]);
            }
            if (const char* vertical_units = document.m_values["transducer_type"])
            {
                ZaxJsonTopTokenizer transducers_document(vertical_units, false, &success);
                if (success)
                    for (unsigned int i = 0; i < transducers_document.m_list_values.size(); ++i)
                        strcpy(m_transducers.m_data[i].s, transducers_document.m_list_values[i]);
            }
            if (const char* horizontal_units = document.m_values["horizontal_units"])
                strcpy(m_horizontal_units, horizontal_units);
            if (const char* surface2D_vert_units = document.m_values["surface2D_vert_units"])
                strcpy(m_surface2D_vert_units, surface2D_vert_units);
        }

        m_sample_rates.Rebuild(nr_channels);
        DGetSampleRates_PROC(m_varname, m_sample_rates.m_data);
    }

    virtual bool GetDataBlock(double** buffer, unsigned int* start, unsigned int* nrelements, int* enable = 0)
    {
        return DGetDataRaw_PROC(m_varname, buffer, start, nrelements, enable);
    }

    virtual bool WriteDataBlock(double** sbuffer, unsigned int* dstart, unsigned int* dnrelements, int* dactchans = 0)
    {
        return true;
    }

    virtual bool AppendSamples(double** buffer, unsigned int nrsamples)
    {
        return true;
    }
};

int Backend_RequestDataDelete(char* a_dataname)
{
    return g_backend_observer->Backend_RequestDataDelete(a_dataname);
}

bool DBackend_OnDataOut(char* a_dataname, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
{
    return g_backend_observer->Backend_OnDataOut(a_dataname, a_fit_width, a_display_type, a_align, a_grid);
}

char* DBackend_OnRefreshDataWindow(char* a_dataname, bool a_full_change, bool a_first_change)
{
    return g_backend_observer->Backend_OnRefreshDataWindow(a_dataname, a_full_change, a_first_change);
}

char* DBackend_OnSystemExit()
{
    return g_backend_observer->Backend_OnSystemExit();
}

char* DBackend_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding)
{
    return g_backend_observer->Backend_OnBindScrolling(a_src_data_name, a_dst_data_name, a_remove_binding);
}

void InitBackend()
{
    HMODULE backend_module = LoadLibrary("Backend.dll");
    if (backend_module)
    {
        DGetDataRaw_PROC = (DGetDataRaw_PROC_TYPE)GetProcAddress(backend_module, TEXT("DGetDataRaw"));
        DGetNrChannels_PROC = (DGetNrChannels_PROC_TYPE)GetProcAddress(backend_module, TEXT("DGetNrChannels"));
        DGetTotalSamples_PROC = (DGetTotalSamples_PROC_TYPE)GetProcAddress(backend_module, TEXT("DGetTotalSamples"));
        DGetSampleRates_PROC = (DGetSampleRates_PROC_TYPE)GetProcAddress(backend_module, TEXT("DGetSampleRates"));
        RunScript_PROC = (RunScript_PROC_TYPE)GetProcAddress(backend_module, TEXT("RunScript"));
        OpenFile_PROC = (OpenFile_PROC_TYPE)GetProcAddress(backend_module, TEXT("DOpenFile"));
        Interval_PROC = (Interval_PROC_TYPE)GetProcAddress(backend_module, TEXT("Interval"));
        GetCopyrightInfo_PROC = (GetCopyrightInfo_PROC_TYPE)GetProcAddress(backend_module, TEXT("GetCopyrightInfo"));
        GetFunctionListRef_PROC = (GetFunctionListRef_PROC_TYPE)GetProcAddress(backend_module, TEXT("GetFunctionListRef"));
        DestroyData_PROC = (DestroyData_PROC_TYPE)GetProcAddress(backend_module, TEXT("DestroyData"));
        SetDescription_PROC = (SetDescription_PROC_TYPE)GetProcAddress(backend_module, TEXT("SetDescription"));
        GetDescription_PROC = (GetDescription_PROC_TYPE)GetProcAddress(backend_module, TEXT("GetDescription"));
        GetFileFormatExtensions_PROC = (GetFileFormatExtensions_PROC_TYPE)GetProcAddress(backend_module, TEXT("GetFileFormatExtensions"));
        GetUnits_PROC = (GetUnits_PROC_TYPE)GetProcAddress(backend_module, TEXT("GetUnits"));
        InitializeBackend_PROC = (InitializeBackend_PROC_TYPE)GetProcAddress(backend_module, TEXT("InitializeBackend"));
        InitializeBackend_PROC(Backend_RequestDataDelete, DBackend_OnDataOut, DBackend_OnRefreshDataWindow, DBackend_OnSystemExit, DBackend_OnBindScrolling);
    }
}

class CAboutBox: public CControl_Base
{
public:
    CAboutBox(const char* a_copyright_info)
        :CControl_Base(0, 0, "About...", 0, 250, 250, 500, 800, 0, true)
    {
        ZaxJsonTopTokenizer l_info(a_copyright_info, false);
        if (l_info.m_values["general"])
            new CControl_Base(0, "static", l_info.m_values["general"], 0x50010141, 0, 0, 500, 50, this, false);
        if (l_info.m_values["libraries"])
        {
            ZaxJsonTopTokenizer l_libs(l_info.m_values["libraries"], false);
            for (unsigned int i = 0; i < l_libs.m_list_values.size(); ++i)
            {
                new CControl_Base(0, "static", l_libs.m_list_values[i], 0x50010141, 0, 70 + i * 100, 490, 40, this, false);
                //new CControl_Base(0, "static", a_copyright_info.m_data[i]->m_data[0].s, 0x50010141, 0, 70 + i * 100 + 40, 490, 40, this, false);
            }
        }
    }
};

class CMainForm: public CControl_Base, public IBackendObserver
{
    CBitButton*                  m_record_button;
    CControl_Base*               m_mdi_client;
    CSplitter*                   m_hor_splitter;
    CSplitter*                   m_vert_splitter;
    CEdit*                       m_edit;
    map<string, CSignalDisplay*> m_codec_to_display_map;

public:
    CMainForm(HINSTANCE hThisInstance)
        :CControl_Base(0, 0, "Data Browser B15 12X", 0, 750, 150, 1024, 800, 0, true, hThisInstance)
    {
        SetDirectories();
        InitializeComponents();
        InitBackend();

        Resize(1000, 800, true);
        if (false)
            ShowWindow(hWnd, SW_MAXIMIZE);
    }

    virtual ~CMainForm()
    {
        ///2doIBackend::Delete(m_backend);
        PostQuitMessage(0);
    };

    void ExecuteScriptFile(const char* a_fiename)
    {
        int size;
        byte* buff = LoadBuffer(a_fiename, 0, &size);
        if (buff)
        {
            char Script[size + 1];
            memcpy(Script, buff, size);
            Script[size] = 0;
            delete[] buff;
            RunScript_PROC(Script, true);
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

    int Backend_OnDataOut(const char* a_dataname, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
    {
        CDataWrap* data_codec = new CDataWrap(a_dataname);
        return EditData(data_codec, a_fit_width, a_display_type, a_align, a_grid);
    }

    char* Backend_OnRefreshDataWindow(const char* a_dataname, bool a_full_change, bool a_first_change)
    {
        if (CSignalDisplay* l_display = m_codec_to_display_map[a_dataname])
        {
            DGetTotalSamples_PROC(a_dataname, l_display->m_codec->m_total_samples.m_data);
            if (a_first_change)
                l_display->SetDefaults();
            l_display->RebuildDataWindow(a_full_change);
        }
        return 0;
    }

    char* Backend_OnSystemExit()
    {
        PostMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;
    }

    char* Backend_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding)
    {
        return 0;
    }

    void ScriptEditor1_On_RunScript(CScriptEditor* a_sender, const char* a_script)
    {
        RunScript_PROC(a_script, true);
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

        m_mdi_client = new CControl_Base(0, "mdiclient", 0, WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | 4, 100, 100, 900, 600, this, false, 0, 0, (void*)&ccs);
        m_record_button = new CBitButton (10, 255, 134, 33, "Start/Restart", this);
        m_edit = new CEdit(this, "Recording_FE_", 0, 0, 100, 250);
        m_vert_splitter = new CSplitter(0, 0, 500, 500, this, VERTICAL);

        m_record_button->OnClick += &CMainForm::RecordButton_OnClick;

        m_vert_splitter->SpWeight = 5;
        m_vert_splitter->SpSpace = 0;
        m_vert_splitter->SetWindows(m_record_button, m_edit);
        m_vert_splitter->SetPos(150);

        m_hor_splitter = new CSplitter(0, 0, 400, 400, this, HORIZONTAL);
        m_hor_splitter->SpWeight = 5;
        m_hor_splitter->SpSpace  = 0;
        m_hor_splitter->SetWindows(m_vert_splitter, m_mdi_client);
        m_hor_splitter->SetPos(22);
        SetTimer(hWnd, 10011, 20, 0);
    }

    void Obj_OnResize(int x, int y, int width, int height)
    {
        if (HasControl(m_hor_splitter))
            m_hor_splitter->Resize(width, height);
        return CControl_Base::Obj_OnResize(x, y, width, height);
    }

    void Children_On_MouseDown(CControl_Base* a_sender, int button, int x, int y, int client)
    {}

    void CloseAllWindows()
    {
        while (m_codec_to_display_map.size())
            SendMessage(m_codec_to_display_map.begin()->second->hWnd, WM_CLOSE, 0, 0);
    }

    void RecordButton_OnClick(CControl_Base* a_sender, int button, int x, int y)
    {
        CloseAllWindows();
        char tetx[1000], script[1000];
        m_edit->GetText(tetx);
        snprintf(script, 1000, "DataAq(1, 192.168.4.1:80, 2000 2000 2000 2000, 24 24 24 24, 0 1 2 3, 100, 4, DateAndTime(%s,.bdf));", tetx);
        RunScript_PROC(script, false);
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
        DestroyData_PROC(a_sender->m_codec->m_varname);
        m_codec_to_display_map.erase(a_sender->m_codec->m_varname);
        delete a_sender->m_codec;
    }

    int EditData(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid, bool aDestroyDataOnClose = true) //todonow: parameter/new func for new or same window in case of an existing varname
    {
        int result = 0;
        CSignalDisplay* child2 = m_codec_to_display_map[a_data_codec->m_varname];
        if (child2)
        {
            if (a_data_codec != child2->m_codec)
            {
                DestroyData_PROC(child2->m_codec->m_varname);
                delete child2->m_codec;
                child2->ResetCodec(a_data_codec);
                child2->RebuildDataWindow(false);
            }
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
            }
            EFitWidthType lfitWidth = EFitWidthType::EFitWidthType_normal;
            if (a_fit_width)
                if (!strcmp(a_fit_width, "fit_width") || !strcmp(a_fit_width, "true"))
                    lfitWidth = EFitWidthType::EFitWidthType_fitWidth;
            child2 = new CSignalDisplay("NO DATA", a_data_codec, lfitWidth, lDisplayType, a_align, a_grid, m_mdi_client, this);
            child2->OnMouseDown += &CMainForm::Children_On_MouseDown;
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
            Interval_PROC();

        if (a_message == WM_COMMAND)
        {
            switch(LOWORD(a_wparam))
            {
            case CM_FILE_SAVE:
            {}
            break;
            case CM_FILE_OPEN:
            {
                const char* lExtensions = GetFileFormatExtensions_PROC();
                char FileName[MAX_PATH];
                if (!GetFileNameT(a_hwnd, FileName, FALSE, 1, lExtensions))
                    break;
                for (unsigned int i = 0; i < 1; ++i)
                    OpenFile_PROC(FileName, 0);
            }
            break;
            case CM_HELP_ABOUT:
            {
                new CAboutBox(GetCopyrightInfo_PROC());
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
                CScriptEditor* mScriptEditor = new CScriptEditor(GetFunctionListRef_PROC(), "Script editor", 300, 150, 700, 500, this, true);
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
    LoadLibrary(TEXT("RICHED20.DLL"));
    InitCBLib();
    InitBitmapLib();
    CMainForm* mainForm = new CMainForm(hThisInstance);
    g_backend_observer = mainForm;
    if (lpszArgument && strlen(lpszArgument))
        mainForm->ExecuteScriptFile(lpszArgument);
    return RunAppD();
}
