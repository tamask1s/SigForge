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
#include "BackendObserver_dynamic_interface.h"

class CBackend: public IBackend, public IScriptRunnerObserver
{
    CStringMx m_copyright_info;
    IBackendObserver* m_backend_observer;
    CScriptRunner* m_script_runner = 0;
    CSignalCodec_List m_signal_codec_list; /** DataList */
    CFunctionLibraryList* m_function_library_list_owned = 0;
    set<ISignalCodec*> m_data_indx_already_refreshed;

    virtual int ScriptRunner_RequestDataDelete(char* a_data_codec)
    {
        return m_backend_observer->Backend_RequestDataDelete(a_data_codec);
    }

    virtual void DestroyCodec(ISignalCodec* a_codec)
    {
        if (a_codec)
        {
            size_t erased = m_signal_codec_list.erase(a_codec->m_varname);
            if (erased)
            {
                m_data_indx_already_refreshed.erase(a_codec);
                delete a_codec;
            }
        }
    }

    virtual void ScriptRunner_OnDataOut(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
    {
        m_backend_observer->Backend_OnDataOut(a_data_codec, a_fit_width, a_display_type, a_align, a_grid);

        if (a_data_codec)
            m_signal_codec_list[a_data_codec->m_varname] = a_data_codec;
    }

    virtual char* ScriptRunner_OnRefreshDataWindow(const char* a_datam_varname, bool a_full_change)
    {
        map<string, ISignalCodec*>::iterator lFound = m_signal_codec_list.find(a_datam_varname);
        if (lFound != m_signal_codec_list.end())
            RefreshObserver(lFound->second, a_full_change);
        return 0;
    }

    virtual char* ScriptRunner_OnFileOpen(const char* a_file_name, const char* a_datam_varname)
    {
        char* result = 0;
        if (!Open_File(a_file_name, a_datam_varname))
        {
            result = new char[200];
            sprintf(result, "Cannot open file %s", a_file_name);
        }
        return result;
    }

    virtual char* ScriptRunner_OnSystemExit()
    {
        return m_backend_observer->Backend_OnSystemExit();
    }

    virtual char* ScriptRunner_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding)
    {
        return m_backend_observer->Backend_OnBindScrolling(a_src_data_name, a_dst_data_name, a_remove_binding);
    }

    virtual int RunScript(const char* a_script, bool reloadPlugins)
    {
        return m_script_runner->RunScript(a_script, reloadPlugins);
    }

    virtual char* Open_File(const char* a_file_name, const char* a_DataVarName)
    {
        char* res = 0;
        FileBasedCodec* FileBasedCodec1 = new FileBasedCodec;
        int success = FileBasedCodec1->OpenFile((char*)a_file_name, false);
        if (a_DataVarName)
            strcpy(FileBasedCodec1->m_varname, a_DataVarName);
        if (!success)
        {
            delete FileBasedCodec1;
            FileBasedCodec1 = 0;
        }
        else
        {
            ScriptRunner_OnDataOut(FileBasedCodec1, 0, 0, 0, 0);
            res = FileBasedCodec1->m_varname;
        }
        return res;
    }

    virtual void DataEvent(ISignalCodec* a_codec, int a_event, double a_val_x, double a_val_y, int a_data_channel, double a_data_val, char* a_info)
    {
        m_script_runner->OnDataEvent(a_codec, a_event, a_val_x, a_val_y, a_data_channel, a_data_val, a_info);
    }

    virtual const char* GetCopyrightTextRef()
    {
        static string copyright_text;
        copyright_text = "{ \"general\" : \"This application belongs to IT-MEDicine ltd. All rights reserved. You may use the Software freely for research\\, evaluation\\, development\\, educational or personal and individual use excluding use or distribution for direct or indirect commercial (including strategic) gain or advantage your own use. Pleas see the license attached 'License.rtf'.\",\"libraries\" : [ ";
        for (unsigned int i = 0; i < m_copyright_info.m_size; ++i)
        {
            copyright_text += "{ \"lib\" : \"";
            copyright_text += m_copyright_info.m_data[i]->m_data[1].s;
            copyright_text += "\",\"license\" : \"";
            copyright_text += m_copyright_info.m_data[i]->m_data[0].s;
            copyright_text += "\"}";
            if (i != m_copyright_info.m_size - 1)
                copyright_text += ",";
        }
        copyright_text += " ] }";
        return copyright_text.c_str();
    }

    virtual const char* GetFunctionListRef()
    {
        static string function_list;
        function_list = "{ \"libraries\" : [ ";
        for (unsigned int s = 0; s < m_function_library_list_owned->size(); s++)
        {
            if (s==1)
                function_list += "{ \"name\" : \"default\", \"functions\" : [\"def\", \"main\", \"DisplayData\", \"RefreshDataWindow\", \"SaveDataToFile\", \"SaveVarToFile\", \"OnDataEvent\"] } , ";
            if (s==3)
                function_list += "{ \"name\" : \"default\", \"functions\" : [\"#define\"] } , ";
            function_list += "{ \"name\" : \"";
            function_list += (*m_function_library_list_owned)[s].m_library_name;
            function_list += "\",\"functions\" : [ ";
            for (unsigned int i = 0; i < (*m_function_library_list_owned)[s].m_function_description_list.size(); ++i)
            {
                function_list += "\"";
                function_list += (*m_function_library_list_owned)[s].m_function_description_list[i].m_function_name;
                function_list += "\"";
                if (i != (*m_function_library_list_owned)[s].m_function_description_list.size() - 1)
                    function_list += ",";
            }
            function_list += " ] }";
            if (s != m_function_library_list_owned->size() - 1)
                function_list += ",";
        }
        function_list += " ] }";
        return function_list.c_str();
    }

    void RefreshObserver(ISignalCodec* a_data_codec, bool a_full_change)
    {
        set<ISignalCodec*>::iterator found = m_data_indx_already_refreshed.find(a_data_codec);
        bool l_first_change = false;
        if (found == m_data_indx_already_refreshed.end())
        {
            m_data_indx_already_refreshed.insert(a_data_codec);
            l_first_change = true;
        }
        m_backend_observer->Backend_OnRefreshDataWindow(a_data_codec, a_full_change, l_first_change);
    }

    virtual void Interval()
    {
        std::for_each(m_signal_codec_list.begin(), m_signal_codec_list.end(), [&](std::pair<string, ISignalCodec*>  l_codec)
        {
            if (l_codec.second->RefreshDataFromSource())
            {
                RefreshObserver(l_codec.second, true);
                m_script_runner->TriggerDataChanged(l_codec.second->m_varname);
            }
        });
        m_script_runner->SystemInterval();
    }

public:
    CBackend(IBackendObserver* a_backend_observer)
    {
        InitFFTLib();
        LoadCodecTypes();
        IDataAq::LoadDataAqModules();

        if (false)
            cout << IDataAq::GetModuleDescriptions() << endl;

        m_backend_observer = a_backend_observer;
        m_script_runner = new CScriptRunner(&m_signal_codec_list, this);

        m_function_library_list_owned = new CFunctionLibraryList(0, 0);
        m_function_library_list_owned->InitCFunctionLibraryList();
        m_function_library_list_owned->GetCopyrightInfo(&m_copyright_info);
        m_function_library_list_owned->UnLoadPlugins();
    }

    virtual ~CBackend()
    {
        delete m_script_runner;
        delete m_function_library_list_owned;
    }
};

IBackend* IBackend::New(IBackendObserver* a_backend_observer)
{
    return new CBackend(a_backend_observer);
}

void IBackend::Delete(IBackend* a_instance)
{
    delete (CBackend*)a_instance;
}
