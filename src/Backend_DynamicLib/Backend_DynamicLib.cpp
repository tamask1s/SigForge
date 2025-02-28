#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "minvect.h"
#include "datastructures.h"
#include "CodecLib.h"
#include "DataAq_Interface.h"
#include "FileBased_Codec.h"
#include "ScriptRunner.h"
#include "Backend.h"
#include "ZaxJsonParser.h"
#include "BackendObserver_dynamic_interface.h"

class CBackendUser: public IBackendObserver
{
    IBackend* m_backend;
    map<string, ISignalCodec*> m_codec_to_display_map;
    DBackend_RequestDataDelete_PROC_TYPE m_datadelete_proc;
    DBackend_OnDataOut_PROC_TYPE m_dataout_proc;
    DBackend_OnRefreshDataWindow_PROC_TYPE m_refresh_proc;
    DBackend_OnSystemExit_PROC_TYPE m_exit_proc;
    DBackend_OnBindScrolling_PROC_TYPE m_BindScrolling_proc;
    string m_copyright_info;

public:
    CBackendUser()
        :m_dataout_proc(0),
        m_refresh_proc(0),
        m_exit_proc(0)
    {
        m_backend = IBackend::New(this);
        //m_backend->RunScript("CreateVector(data, 1 2 345.678 -9); WriteAscii(data, test001.dbascii); SystemExit(0);", true);
    }

    virtual ~CBackendUser()
    {
        IBackend::Delete(m_backend);
    };

    void InitializeBackend(DBackend_RequestDataDelete_PROC_TYPE a_datadelete_proc, DBackend_OnDataOut_PROC_TYPE a_dataout_proc, DBackend_OnRefreshDataWindow_PROC_TYPE a_refresh_proc, DBackend_OnSystemExit_PROC_TYPE a_exit_proc, DBackend_OnBindScrolling_PROC_TYPE a_BindScrolling_proc)
    {
        m_datadelete_proc = a_datadelete_proc;
        m_dataout_proc = a_dataout_proc;
        m_refresh_proc = a_refresh_proc;
        m_exit_proc = a_exit_proc;
        m_BindScrolling_proc = a_BindScrolling_proc;
    }

    static CBackendUser& BackendUser()
    {
        static CBackendUser backend_user;
        return backend_user;
    }

    char* OpenFile(const char* a_file_name, const char* a_data_name)
    {
        return m_backend->Open_File(a_file_name, a_data_name);
    }

    bool GetDataRaw(const char* a_data_name, double** a_buffer, unsigned int* a_starts, unsigned int* a_nr_elements, int* enable)
    {
        if (m_codec_to_display_map.find(a_data_name) != m_codec_to_display_map.end())
            return m_codec_to_display_map[a_data_name]->GetDataBlock(a_buffer, a_starts, a_nr_elements, enable);
        else
            return false;
    }

    virtual int RunScript(const char* a_script, bool reloadPlugins)
    {
        return m_backend->RunScript(a_script, reloadPlugins);
    }

    int GetNrChannels(const char* a_data_name)
    {
        return m_codec_to_display_map[a_data_name]->m_total_samples.m_size;
    }

    bool Get_total_samples(const char* a_data_name, unsigned int* a_total_samples)
    {
        memcpy(a_total_samples, m_codec_to_display_map[a_data_name]->m_total_samples.m_data, m_codec_to_display_map[a_data_name]->m_total_samples.m_size * sizeof(unsigned int));
        return true;
    }

    bool Get_sample_rates(const char* a_data_name, double* a_sample_rates)
    {
        memcpy(a_sample_rates, m_codec_to_display_map[a_data_name]->m_sample_rates.m_data, m_codec_to_display_map[a_data_name]->m_sample_rates.m_size * sizeof(double));
        return true;
    }

    void Interval()
    {
        m_backend->Interval();
    }

    void DestroyData(const char* a_data_name)
    {
        ISignalCodec* l_data_codec = m_codec_to_display_map[a_data_name];
        m_codec_to_display_map.erase(a_data_name);
        m_backend->DestroyCodec(l_data_codec);
    }

    const char* GetCopyrightInfo()
    {
        return m_backend->GetCopyrightTextRef();
    }

    void DataEvent(ISignalCodec* a_codec, int a_event, double a_val_x, double a_val_y, int a_data_channel, double a_data_val, char* a_info)
    {
        m_backend->DataEvent(a_codec, a_event, a_val_x, a_val_y, a_data_channel, a_data_val, a_info);
    }

    const char* GetFunctionListRef()
    {
        return m_backend->GetFunctionListRef();
    }

    template<typename T>
    void appendList(string& a_JSON, const char* aPropName, T& a_list)
    {
        addInBracelets(a_JSON, aPropName);
        a_JSON += " : [ ";
        if (a_list.m_size)
        {
            addInBracelets(a_JSON, a_list.m_data[0].s);
            for (unsigned int i = 1; i < a_list.m_size; ++i)
            {
                a_JSON += ", ";
                addInBracelets(a_JSON, a_list.m_data[i].s);
            }
        }
        a_JSON += " ]";
    }

    const char* GetDescription(const char* a_data_name)
    {
        static string description;
        description = "";
        ISignalCodec* l_data_codec = m_codec_to_display_map[a_data_name];
        if (l_data_codec)
        {
            description += "{ ";
            appendProperty(description, "patient", l_data_codec->m_patient);
            description += ", ";
            appendProperty(description, "recording_info", l_data_codec->m_recording);
            description += ", ";
            appendProperty(description, "date", l_data_codec->m_date);
            description += ", ";
            appendProperty(description, "time", l_data_codec->m_time);
            description += ", ";
            appendProperty(description, "horizontal_units", l_data_codec->m_horizontal_units);
            description += ", ";
            //appendProperty(description, "surface2D_vert_units", l_data_codec->m_surface2D_vert_units);
            //description += ", ";
            appendList(description, "vertical_units", l_data_codec->m_vertical_units);
            description += ", ";
            appendList(description, "labels", l_data_codec->m_labels);
            description += ", ";
            appendList(description, "transducer_type", l_data_codec->m_transducers);
            description += " }";
        }
        return description.c_str();
    }

    void SetDescription(const char* a_data_name, char* a_description)
    {
        ZaxJsonTopTokenizer document(a_description, false);
        ISignalCodec* l_data_codec = m_codec_to_display_map[a_data_name];
        if (l_data_codec)
        {
            if (const char* patient = document.m_values["patient"])
                strcpy(l_data_codec->m_patient, patient);
            if (const char* recording_info = document.m_values["recording_info"])
                strcpy(l_data_codec->m_recording, recording_info);
            if (const char* date = document.m_values["date"])
                strcpy(l_data_codec->m_date, date);
            if (const char* time = document.m_values["time"])
                strcpy(l_data_codec->m_time, time);
            if (const char* vertical_units = document.m_values["vertical_units"])
            {
                ZaxJsonTopTokenizer vertical_units_document(vertical_units, false);
                for (unsigned int i = 0; i < vertical_units_document.m_list_values.size(); ++i)
                    strcpy(l_data_codec->m_vertical_units.m_data[i].s, vertical_units_document.m_list_values[i]);
            }
            if (const char* vertical_units = document.m_values["labels"])
            {
                ZaxJsonTopTokenizer labels_document(vertical_units, false);
                for (unsigned int i = 0; i < labels_document.m_list_values.size(); ++i)
                    strcpy(l_data_codec->m_labels.m_data[i].s, labels_document.m_list_values[i]);
            }
            if (const char* vertical_units = document.m_values["transducer_type"])
            {
                ZaxJsonTopTokenizer transducers_document(vertical_units, false);
                for (unsigned int i = 0; i < transducers_document.m_list_values.size(); ++i)
                    strcpy(l_data_codec->m_transducers.m_data[i].s, transducers_document.m_list_values[i]);
            }
            if (const char* horizontal_units = document.m_values["horizontal_units"])
                strcpy(l_data_codec->m_horizontal_units, horizontal_units);
            if (const char* surface2D_vert_units = document.m_values["surface2D_vert_units"])
                strcpy(l_data_codec->m_surface2D_vert_units, surface2D_vert_units);
            l_data_codec->RefreshSource();
        }

    }

    const char* GetFileFormatExtensions()
    {
        static char lExtensions[10000];
        memset(lExtensions, 0, sizeof(lExtensions));
        QueryCodecExtensions(lExtensions);
        return lExtensions;
    }

    void appendProperty(string& aJSON, const char* aPropName, const char* aPropVal)
    {
        aJSON += "\"";
        aJSON += aPropName;
        aJSON += "\" : \"";
        aJSON += aPropVal;
        aJSON += "\"";
    }

    void addInBracelets(string& aJSON, const char* aElement)
    {
        aJSON += "\"";
        aJSON += aElement;
        aJSON += "\"";
    }

    const char* GetUnits(const char* a_data_name)
    {
        static string unitsString;
        unitsString = "";
        ISignalCodec* l_data_codec = m_codec_to_display_map[a_data_name];
        if (l_data_codec)
        {
            unitsString += "{ ";
            appendProperty(unitsString, "horizontal_units", l_data_codec->m_horizontal_units);
            unitsString += ", ";
            appendProperty(unitsString, "surface2D_vert_units", l_data_codec->m_surface2D_vert_units);
            unitsString += ", ";
            appendList(unitsString, "vertical_units", l_data_codec->m_vertical_units);
            unitsString += " }";
        }
        return unitsString.c_str();
    }

private:
    int Backend_RequestDataDelete(char* a_data_codec)
    {
        return m_datadelete_proc(a_data_codec);
    }

    int Backend_OnDataOut(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
    {
        m_codec_to_display_map[a_data_codec->m_varname] = a_data_codec;
        m_dataout_proc(a_data_codec->m_varname, a_fit_width, a_display_type, a_align, a_grid);
        return 1;
    }

    char* Backend_OnRefreshDataWindow(ISignalCodec* a_data, bool a_full_change, bool a_first_change)
    {
        m_refresh_proc(a_data->m_varname, a_full_change, a_first_change);
        return 0;
    }

    char* Backend_OnSystemExit()
    {
        m_exit_proc();
        return 0;
    }

    char* Backend_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding)
    {
        return m_BindScrolling_proc(a_src_data_name, a_dst_data_name, a_remove_binding);
    }
};

extern "C"
{
    /** DGetDataRaw - Reads the raw data
      *
      *		a_data_name   - address of the data structure
      *		a_buffer      - address of the output buffer
      *		a_starts      - start sample index vector
      *		a_nr_elements - nr of samples vector
      *
      * Fills the buffer with the raw data from start to stop indexes for each
      * channel.
      */
    bool __declspec(dllexport) DGetDataRaw(const char* a_data_name, double** a_buffer, unsigned int* a_starts, unsigned int* a_nr_elements, int* enable)
    {
        return CBackendUser::BackendUser().GetDataRaw(a_data_name, a_buffer, a_starts, a_nr_elements, enable);
    }

    char* __declspec(dllexport) DOpenFile(const char* a_file_name, const char* a_data_name)
    {
        return CBackendUser::BackendUser().OpenFile(a_file_name, a_data_name);
    }

    int __declspec(dllexport) DGetNrChannels(const char* a_data_name)
    {
        return CBackendUser::BackendUser().GetNrChannels(a_data_name);
    }

    bool __declspec(dllexport) DGetTotalSamples(const char* a_data_name, unsigned int* a_total_samples)
    {
        return CBackendUser::BackendUser().Get_total_samples(a_data_name, a_total_samples);
    }

    bool __declspec(dllexport) DGetSampleRates(const char* a_data_name, double* a_sample_rates)
    {
        return CBackendUser::BackendUser().Get_sample_rates(a_data_name, a_sample_rates);
    }

    int __declspec(dllexport) RunScript(const char* a_script, bool reloadPlugins)
    {
        return CBackendUser::BackendUser().RunScript(a_script, reloadPlugins);
    }

    void __declspec(dllexport) Interval()
    {
        return CBackendUser::BackendUser().Interval();
    }

    void __declspec(dllexport) InitializeBackend(DBackend_RequestDataDelete_PROC_TYPE a_datadelete_proc, DBackend_OnDataOut_PROC_TYPE a_dataout_proc, DBackend_OnRefreshDataWindow_PROC_TYPE a_refresh_proc, DBackend_OnSystemExit_PROC_TYPE a_exit_proc, DBackend_OnBindScrolling_PROC_TYPE a_BindScrolling_proc)
    {
        return CBackendUser::BackendUser().InitializeBackend(a_datadelete_proc, a_dataout_proc, a_refresh_proc, a_exit_proc, a_BindScrolling_proc);
    }

    void __declspec(dllexport) DestroyData(const char* a_data_name)
    {
        return CBackendUser::BackendUser().DestroyData(a_data_name);
    }

    const char* __declspec(dllexport) GetCopyrightInfo()
    {
        return CBackendUser::BackendUser().GetCopyrightInfo();
    }

    const char*  __declspec(dllexport) GetFunctionListRef()
    {
        return CBackendUser::BackendUser().GetFunctionListRef();
    }

    void __declspec(dllexport) SetDescription(const char* a_data_name, char* a_description)
    {
        CBackendUser::BackendUser().SetDescription(a_data_name, a_description);
    }

    const char* __declspec(dllexport) GetDescription(const char* a_data_name)
    {
        return CBackendUser::BackendUser().GetDescription(a_data_name);
    }

    const char* __declspec(dllexport) GetFileFormatExtensions()
    {
        return CBackendUser::BackendUser().GetFileFormatExtensions();
    }

    const char* __declspec(dllexport) GetUnits(const char* a_data_name)
    {
        return CBackendUser::BackendUser().GetUnits(a_data_name);
    }

    BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpm_reserved)
    {
        switch (fdwReason)
        {
        case DLL_PROCESS_ATTACH:
            cout << "Backend DLL loaded." << endl;
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
        }
        return TRUE;
    }
}
