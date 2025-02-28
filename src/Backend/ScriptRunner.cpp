#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <windows.h>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "datastructures.h"
#include "ScriptRunner.h"
#include "CodecLib.h"
#include "DataAq_Interface.h"
#include "FileBased_Codec.h"

FFT_PROC        FFT;
RFFT_PROC       RFFT;
FFTEXEC_PROC    FFTEXEC;
FFTDESTROY_PROC FFTDESTROY;


HMODULE InitFFTLib()
{
    HMODULE fftmodule = LoadLibrary("libfftw3-3.dll");
    if (fftmodule)
    {
        FFT = (FFT_PROC)GetProcAddress(fftmodule, TEXT("fftw_plan_dft_1d"));
        FFTEXEC = (FFTEXEC_PROC)GetProcAddress(fftmodule, TEXT("fftw_execute"));
        RFFT = (RFFT_PROC)GetProcAddress(fftmodule, TEXT("fftw_plan_dft_r2c_1d"));
        FFTDESTROY = (FFTDESTROY_PROC)GetProcAddress(fftmodule, TEXT("fftw_destroy_plan"));
    }
    return fftmodule;
}

CVariable* CFunctionLibraryList::NewCVariable()
{
    return new CVariable;
}

char* CFunctionLibraryList::NewChar(int a_size)
{
    return new char[a_size];
}

int CFunctionLibraryList::Fill(const char* Path, WIN32_FIND_DATA * a_find_data)
{
    int result = 0;
    char libpath [MAX_PATH];
    strcpy(libpath, Path);
    CutFileFormFullPath(libpath);
    strcat(libpath, a_find_data->cFileName);
    cout << "Load: " << libpath;
    if (HMODULE hDll = LoadLibrary(libpath))
    {
        if (GetProcedureList_Proc GetProcedureList = (GetProcedureList_Proc) GetProcAddress(hDll, "GetProcedureList"))
        {
            SPR_PROC SPR = (SPR_PROC)GetProcAddress(hDll, "Procedure");
            InitLib_PROC InitLib = (InitLib_PROC)GetProcAddress(hDll, "InitLib");
            if (SPR && InitLib)
            {
                cout << " Succes." << endl;
                resize(size() + 1);
                at(size() - 1).m_module_handle = (long long)hDll;
                strcpy(at(size() - 1).m_library_name, a_find_data->cFileName);
                result = GetProcedureList (&at(size() - 1));
                at(size() - 1).m_base_function_proc = SPR;
                InitLib(m_data_list_ref, m_variable_list_ref, NewCVariable, NewChar, CScriptRunner::Call, FFT, RFFT, FFTEXEC, FFTDESTROY);
                at(size() - 1).m_copyright_info_proc = (CopyrightInfo_Proc)GetProcAddress(hDll, "CopyrightInfo");
            }
        }
    }
    else
        cout << " Error: " << GetLastError() << endl;
    return result;
}

int CFunctionLibraryList::LoadPlugins(const char* a_path)
{
    HANDLE Handle;
    WIN32_FIND_DATA find_data;
    int result = 0;
    if ((Handle = FindFirstFile(a_path, &find_data)) != INVALID_HANDLE_VALUE)
    {
        result += Fill(a_path, &find_data);
        while (FindNextFile(Handle, &find_data))
        {
            result += Fill(a_path, &find_data);
        }
        FindClose(Handle);
    }
    return result;
}

int CFunctionLibraryList::InitCFunctionLibraryList()
{
    clear();
    char FunctionsDir [200];
    strcpy(FunctionsDir, "SignalProcessors");
    char fullpath [MAX_PATH];
    GetModuleFileName(0, fullpath, MAX_PATH);
    CutLastDir(fullpath);
    strcat(fullpath, FunctionsDir);
    strcat(fullpath, "\\*.dll");
    return LoadPlugins(fullpath);
}

void CFunctionLibraryList::UnLoadPlugins()
{
    for (auto it = begin(); it != end(); ++it)
        FreeLibrary((HMODULE)(*it).m_module_handle);
}

CFunctionLibraryList::CFunctionLibraryList(CSignalCodec_List* a_data_list_reference, CVariable_List* a_variable_list_reference)
{
    m_data_list_ref     = a_data_list_reference;
    m_variable_list_ref = a_variable_list_reference;
}

int CFunctionLibraryList::GetCopyrightInfo(CStringMx* a_info)
{
    for (auto it = begin(); it != end(); ++it)
    {
        if ((*it).m_copyright_info_proc)
        {
            (*it).m_copyright_info_proc(a_info);
        }
        else
        {
            a_info->RebuildPreserve(a_info->m_size + 1);
            a_info->m_data[a_info->m_size - 1]->AddElement("No copyright information available.");
        }
        a_info->m_data[a_info->m_size - 1]->AddElement("Library '");
        strcat(a_info->m_data[a_info->m_size - 1]->m_data[1].s, (*it).m_library_name);
        strcat(a_info->m_data[a_info->m_size - 1]->m_data[1].s, "' :");
    }
    return 0;
}

char* CScriptRunner::RequestDataDelete(char* a_data_name)
{
    char* result = 0;
    if (!m_script_runner_observer->ScriptRunner_RequestDataDelete(a_data_name))
    {
        result = new char[200];
        sprintf(result, "%s", "ERROR: RequestDataDelete: no data found!");
    }
    return result;
}

char* CScriptRunner::DataOutput(char* a_outvarname, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
{
    if (!a_outvarname)
    {
        char* result = new char[200];
        sprintf(result, "%s", "ERROR: DataDisplay: not enough arguments!");
        return result;
    }
    if (CVariable* var = m_variable_list.variablemap_find(a_outvarname))
    {
        m_script_runner_observer->ScriptRunner_OnDataOut(var, a_fit_width, a_display_type, a_align, a_grid);
        m_variable_list.erase(a_outvarname);
        return 0;
    }
    else
    {
        if (!strcmp(a_outvarname, "manage_windows") || !strcmp(a_outvarname, "scale_window") || !strcmp(a_outvarname, "setup_channels"))
        {
            m_script_runner_observer->ScriptRunner_OnDataOut(0, a_outvarname, a_fit_width, a_display_type, a_align);
            return 0;
        }
        else
        {
            char* result = new char[200];
            sprintf(result, "%s '%s'", "ERROR: DataDisplay: can't find variable:", a_outvarname);
            return result;
        }
    }
}

char* CScriptRunner::NewDataAq(const char* a_dataAq_device_type, const char* a_dataAq_params, const char* a_sampling_rates, const char* a_gain, const char* a_physical_mapping, const char* a_milliSeconds_to_read, const char* a_fileCodec_type, const char* a_file_name, const char* a_signal_name)
{
    int lDataAqDeviceType = atoi(a_dataAq_device_type);
    IntVec lSamplingRates;
    IntVec lGain;
    IntVec lPhysicalMapping;
    lSamplingRates.RebuildFrom((char*)a_sampling_rates);
    lGain.RebuildFrom((char*)a_gain);
    lPhysicalMapping.RebuildFrom((char*)a_physical_mapping);
    unsigned int lNrChannels = lSamplingRates.m_size;

    int lMilliSecondsToRead = atoi(a_milliSeconds_to_read);
    int lCFileCodectype = atoi(a_fileCodec_type);
    DataAqCodec* lDataAqCodec = new DataAqCodec(lDataAqDeviceType, a_dataAq_params, lNrChannels, lSamplingRates.m_data, lGain.m_data, lPhysicalMapping.m_data, lMilliSecondsToRead, lCFileCodectype, a_file_name);
    lDataAqCodec->startReading();
    if (a_signal_name)
        strcpy(lDataAqCodec->m_varname, a_signal_name);
    m_script_runner_observer->ScriptRunner_OnDataOut(lDataAqCodec, 0, 0, 0, 0);
    return 0;
}

char* CScriptRunner::RefreshDataWindow(const char* a_datam_varname, const char* a_params)
{
    return m_script_runner_observer->ScriptRunner_OnRefreshDataWindow(a_datam_varname, (a_params && !strcmp(a_params, "true")) ? true : false);
}

char* CScriptRunner::FileOpen(const char* a_file_name, const char* a_datam_varname)
{
    return m_script_runner_observer->ScriptRunner_OnFileOpen(a_file_name, a_datam_varname);
}

void CScriptRunner::SystemInterval()
{
    static unsigned int systemtickcount = 0;
    systemtickcount++;
    for (unsigned int i = 0; i < m_intervals.size(); ++i)
    {
        int divider = m_intervals[i].m_interval / 30;
        if (divider)
        {
            if (!(systemtickcount % divider))
                if (RunScript(m_intervals[i].m_statement_body.c_str(), false))
                {
                    cout << "Interval '" << m_intervals[i].m_interval_name << "' stopped due to handler error. " << endl;
                    m_intervals.erase(m_intervals.begin() + i);
                }
        }
        else
        {
            cout << "Interval '" << m_intervals[i].m_interval_name << "' stopped because interval is too small. " << endl;
            m_intervals.erase(m_intervals.begin() + i);
        }
    }
}

char* CScriptRunner::TriggerDataChanged(const char* a_data_varname)
{
    map<string, TDataEventCallback>::iterator lFound = m_data_changed_callbacks.find(a_data_varname);
    if (lFound != m_data_changed_callbacks.end())
        RunScript(lFound->second.m_statement_body.c_str(), false);
    return 0;
}

char* CScriptRunner::OnDataEvent(ISignalCodec* a_codec, int a_event, double a_val_x, double a_val_y, int a_data_channel, double a_data_val, char* a_info)
{
    map<string, CFunctionDescription>::iterator lFound = m_data_mousedown_callbacks.find(a_codec->m_varname);
    if (lFound != m_data_mousedown_callbacks.end())
    {
        vector<TFunctionParameter>& param_list = m_data_mousedown_callbacks[a_codec->m_varname].m_parameter_list;
        if (param_list.size() < 6 || param_list.size() > 8)
        {
            cout << "ERROR: OnDataEvent: event handler function's number of parameters must be between 6 and 8. Now it is: " << param_list.size() << endl;
            return 0;
        }
        char script_to_run[lFound->second.m_statement_body.size() + MAXSTRLEN];
        if (param_list.size() == 6)
            sprintf(script_to_run, "def %s(%s, %s, %s, %s, %s, %s) { %s }; %s(%s, %d, %f, %f, %d, %f);", lFound->second.m_function_name, param_list[0].ParameterName, param_list[1].ParameterName, param_list[2].ParameterName, param_list[3].ParameterName, param_list[4].ParameterName, param_list[5].ParameterName, lFound->second.m_statement_body.c_str(), lFound->second.m_function_name, a_codec->m_varname, a_event, a_val_x, a_val_y, a_data_channel, a_data_val);
        else if (param_list.size() == 7)
            sprintf(script_to_run, "def %s(%s, %s, %s, %s, %s, %s, %s) { %s }; %s(%s, %d, %f, %f, %d, %f, %s);", lFound->second.m_function_name, param_list[0].ParameterName, param_list[1].ParameterName, param_list[2].ParameterName, param_list[3].ParameterName, param_list[4].ParameterName, param_list[5].ParameterName, param_list[6].ParameterName, lFound->second.m_statement_body.c_str(), lFound->second.m_function_name, a_codec->m_varname, a_event, a_val_x, a_val_y, a_data_channel, a_data_val, param_list[6].Value);
        else if (param_list.size() == 8)
            sprintf(script_to_run, "def %s(%s, %s, %s, %s, %s, %s, %s, %s) { %s }; %s(%s, %d, %f, %f, %d, %f, %s, %s);", lFound->second.m_function_name, param_list[0].ParameterName, param_list[1].ParameterName, param_list[2].ParameterName, param_list[3].ParameterName, param_list[4].ParameterName, param_list[5].ParameterName, param_list[6].ParameterName, param_list[7].ParameterName, lFound->second.m_statement_body.c_str(), lFound->second.m_function_name, a_codec->m_varname, a_event, a_val_x, a_val_y, a_data_channel, a_data_val, param_list[6].Value, param_list[7].Value);
        RunScript(script_to_run, true);
    }
    return 0;
}

char* CScriptRunner::NewMemoryData(char* a_dst_data_name, char* a_nr_channels, char* a_samplerates, char* a_nr_hor_units)
{
    char *l_result = nullptr;
    if (!a_dst_data_name || !a_nr_channels || !a_samplerates || !a_nr_hor_units)
        l_result = MakeString(0, "ERROR: NewMemoryData: not enough arguments");
    else
    {
        DoubleVec l_samplerates;
        l_samplerates.RebuildFrom(a_samplerates);
        double l_nr_hor_units = atof(a_nr_hor_units);
        unsigned int l_nr_channels = atoi(a_nr_channels);
        ISignalCodec* l_mem_data = CreateMemoryData(a_dst_data_name, l_nr_channels, l_samplerates.m_data, l_nr_hor_units);
        m_script_runner_observer->ScriptRunner_OnDataOut(l_mem_data, 0, 0, 0, 0);
    }
    return l_result;
}

char* CScriptRunner::NewFileDataBasedOnData(char* a_dst_data_name, char* a_template_data_name, char* a_window_milliseconds, char* a_codec_type, char* a_filename, char* a_channels)
{
    char* result = 0;
    if (ISignalCodec* l_template_data = m_data_list_ref->datamap_find(a_template_data_name))
    {
        IntVec l_channels;
        if (a_channels)
            l_channels.RebuildFrom(a_channels);

        FileBasedCodec* FileBasedCodec1 = new FileBasedCodec;
        int m_milliSeconds_to_read = atoi(a_window_milliseconds);
        unsigned int m_nr_channels = l_channels.m_size;
        if (!m_nr_channels)
            m_nr_channels = l_template_data->m_total_samples.m_size;
        IntVec m_sampling_rates;
        m_sampling_rates.Rebuild(m_nr_channels);
        for (unsigned int i = 0; i < m_nr_channels; ++i)
            if (l_channels.m_size)
                m_sampling_rates.m_data[i] = l_template_data->m_sample_rates.m_data[l_channels.m_data[i] - 1];
            else
                m_sampling_rates.m_data[i] = l_template_data->m_sample_rates.m_data[i];

        if (FileBasedCodec1->CreateNewFile(a_filename, m_nr_channels, true, atoi(a_codec_type))) ///{ <------- API call ------- }
        {
            FileBasedCodec1->SetRecordDuration(m_milliSeconds_to_read / 1000.0);  ///{ <------- API call ------- }
            double DoubleBuf[m_nr_channels];
            unsigned int NrRecordSamples[m_nr_channels];
            ///{ Setup file writing }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
            {
                NrRecordSamples[i] = (m_sampling_rates.m_data[i] * m_milliSeconds_to_read) / 1000;
                if (!NrRecordSamples[i])
                    NrRecordSamples[i] = 1;
            }

            FileBasedCodec1->SetRecordSamples(NrRecordSamples);  ///{ <------- API call ------- }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
                DoubleBuf[i] = m_sampling_rates.m_data[i];
            FileBasedCodec1->SetSampleRates(DoubleBuf);  ///{ <------- API call ------- }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
                DoubleBuf[i] = -5000;//todonow
            FileBasedCodec1->SetPhysicalMin(DoubleBuf);  ///{ <------- API call ------- }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
                DoubleBuf[i] = 5000;
            FileBasedCodec1->SetPhysicalMax(DoubleBuf);  ///{ <------- API call ------- }
            FileBasedCodec1->WriteHeader();   ///{ <------- API call ------- }

            CMatrixVarLen <double> l_visible_buffer;
            l_visible_buffer.Rebuild(m_nr_channels, NrRecordSamples);

            FileBasedCodec1->WriteHeader();   ///{ <------- API call ------- }

            strcpy(FileBasedCodec1->m_varname, a_dst_data_name);
            m_script_runner_observer->ScriptRunner_OnDataOut(FileBasedCodec1, 0, 0, 0, 0);
        }
    }
    else
        result = MakeString(0, "ERROR: NewFileDataBasedOnData: non-existing template data was specified: ", a_template_data_name);
    return result;
}

char* CScriptRunner::SaveVarToFile(char* a_varname, char* a_filename, char* a_window_milliseconds, const char* a_file_codec_type)
{
    return SaveDataToFile(a_varname, a_filename, a_window_milliseconds, a_file_codec_type, true);
}

char* CScriptRunner::SaveDataToFile(char* a_varname, char* a_filename, char* a_window_milliseconds, const char* a_file_codec_type, bool save_variable)
{
    char* l_result = nullptr;
    if (!a_varname || !a_filename || !a_window_milliseconds)
        l_result = MakeString(0, "ERROR: SaveDataToFile: not enough arguments");
    int l_file_code_type = 4;
    if (a_file_codec_type)
    {
        switch (l_file_code_type = strtol(a_file_codec_type, nullptr, 10)) {
        case 1:                     /* EDF, file codec */
        case 3:                     /* DADiSP file codec */
        case 4:                     /* BDF file codec */
        case 8:                     /* BINMx file codec */
        case 10:                    /* MDE raw file codec */
            break;
        default:
            l_result = MakeString(0, "ERROR: SaveDataToFile: ", a_file_codec_type, " is not a valid file codec type");
        }
    }

    ISignalCodec* origISignalCodec;
    if (save_variable)
        origISignalCodec = m_variable_list.variablemap_find(a_varname);
    else
        origISignalCodec = m_data_list_ref->datamap_find(a_varname);

    if (!l_result && !origISignalCodec)
        l_result = MakeString(0, "ERROR: SaveDataToFile: while saving '", a_varname, "' to '", a_filename, "': can't find data in datalist: '", a_varname, "'");
    if (!l_result)
    {
        FileBasedCodec* FileBasedCodec1 = new FileBasedCodec;

        int m_milliSeconds_to_read = atoi(a_window_milliseconds);
        unsigned int m_nr_channels = origISignalCodec->m_total_samples.m_size;
        IntVec m_sampling_rates;
        m_sampling_rates.Rebuild(m_nr_channels);
        for (unsigned int i = 0; i < m_nr_channels; ++i)
            m_sampling_rates.m_data[i] = origISignalCodec->m_sample_rates.m_data[i];

        if (FileBasedCodec1->CreateNewFile(a_filename, m_nr_channels, true, l_file_code_type)) ///{ <------- API call ------- }
        {
            FileBasedCodec1->SetRecordDuration(m_milliSeconds_to_read / 1000.0);  ///{ <------- API call ------- }
            double DoubleBuf[m_nr_channels];
            unsigned int NrRecordSamples[m_nr_channels];
            unsigned int starts[m_nr_channels];
            ///{ Setup file writing }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
            {
                NrRecordSamples[i] = (m_sampling_rates.m_data[i] * m_milliSeconds_to_read) / 1000;
                if (!NrRecordSamples[i])
                    NrRecordSamples[i] = 1;
            }

            FileBasedCodec1->SetRecordSamples(NrRecordSamples);  ///{ <------- API call ------- }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
                DoubleBuf[i] = m_sampling_rates.m_data[i];

            FileBasedCodec1->SetSampleRates(DoubleBuf);  ///{ <------- API call ------- }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
                DoubleBuf[i] = -5000;//todonow

            FileBasedCodec1->SetPhysicalMin(DoubleBuf);  ///{ <------- API call ------- }
            for (unsigned int i = 0; i < m_nr_channels; ++i)
                DoubleBuf[i] = 5000;

            FileBasedCodec1->SetPhysicalMax(DoubleBuf);  ///{ <------- API call ------- }
            FileBasedCodec1->WriteHeader();   ///{ <------- API call ------- }

            CMatrixVarLen <double> l_visible_buffer;
            l_visible_buffer.Rebuild(m_nr_channels, NrRecordSamples);

            float maxsamplerate = 0;
            for (unsigned int i = 0; i < m_nr_channels; i++)
                if (maxsamplerate < origISignalCodec->m_total_samples.m_data[i] / origISignalCodec->m_sample_rates.m_data[i])
                    maxsamplerate = origISignalCodec->m_total_samples.m_data[i] / origISignalCodec->m_sample_rates.m_data[i];

            for (unsigned int i = 0; i < origISignalCodec->m_total_samples.m_data[0] / NrRecordSamples[0]; ++i)
            {
                for (unsigned int j = 0; j < m_nr_channels; ++j)
                    starts[j] = i * NrRecordSamples[j];
                origISignalCodec->GetDataBlock(l_visible_buffer.m_data, starts, NrRecordSamples);
                FileBasedCodec1->AppendSamples(l_visible_buffer.m_data, NrRecordSamples[0]); /// { <------- API call ------- }
            }
            FileBasedCodec1->WriteHeader();   ///{ <------- API call ------- }
            delete FileBasedCodec1;
        }
        else
        {
            l_result = MakeString(0, "ERROR:  SaveDataToFile: DataAqCodec: Can't create file", a_filename);
        }
    }

    return l_result;
}

char* CScriptRunner::ExecuteFunction(CFunctionLibraryList* a_functionLibraryList, CFunctionDescription* a_function)
{
    char* result = 0;
    char* parameters[12];
    for (unsigned int i = 0; i < 12; ++i)
        if (i < a_function->m_parameter_list.size())
            parameters[i] = a_function->m_parameter_list[i].ParameterName;
        else
            parameters[i] = 0;
    if (!strcmp(a_function->m_function_name, "DisplayData"))
        return DataOutput(parameters[0], parameters[1], parameters[2], parameters[3], parameters[4]);
    if (!strcmp(a_function->m_function_name, "DataDelete"))
        return RequestDataDelete(parameters[0]);
    if (!strcmp(a_function->m_function_name, "DataAq"))
        return NewDataAq(parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5], parameters[6], parameters[7], parameters[8]);
    if (!strcmp(a_function->m_function_name, "RefreshDataWindow"))
        return RefreshDataWindow(parameters[0], parameters[1]);
    if (!strcmp(a_function->m_function_name, "FileOpen"))
        return FileOpen(parameters[0], parameters[1]);
    if (!strcmp(a_function->m_function_name, "SaveDataToFile"))
        return SaveDataToFile(parameters[0], parameters[1], parameters[2], parameters[3]);
    if (!strcmp(a_function->m_function_name, "SaveVarToFile"))
        return SaveVarToFile(parameters[0], parameters[1], parameters[2], parameters[3]);
    if (!strcmp(a_function->m_function_name, "NewFileDataBasedOnData"))
        return NewFileDataBasedOnData(parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5]);
    if (!strcmp(a_function->m_function_name, "NewMemoryData"))
        return NewMemoryData(parameters[0], parameters[1], parameters[2], parameters[3]);
    if (!strcmp(a_function->m_function_name, "TriggerDataChanged"))
        return TriggerDataChanged(parameters[0]);
    if (!strcmp(a_function->m_function_name, "SystemInterval"))
    {
        if (!a_function->m_statement_body.length())
        {
            for (unsigned int i = 0; i < m_intervals.size(); ++i)
                if (!strcmp(m_intervals[i].m_interval_name.c_str(), parameters[0]))
                {
                    m_intervals.erase(m_intervals.begin() + i);
                    break;
                }
        }
        else
            m_intervals.push_back(TInterval(a_function->m_statement_body, parameters[0], atoi(parameters[1])));
        return 0;
    }
    if (!strcmp(a_function->m_function_name, "OnDataChange"))
    {
        m_data_changed_callbacks.insert(pair<string, TDataEventCallback>(parameters[0], TDataEventCallback(a_function->m_statement_body)));
        return 0;
    }
    if (!strcmp(a_function->m_function_name, "SystemExit"))
        return m_script_runner_observer->ScriptRunner_OnSystemExit();
    if (!strcmp(a_function->m_function_name, "BindScrolling"))
        return m_script_runner_observer->ScriptRunner_OnBindScrolling(parameters[0], parameters[1], parameters[2]);
    if (!strcmp(a_function->m_function_name, "SetStopOnFirstError"))
        return SetStopOnFirstError(parameters[0]);
    if (!strcmp(a_function->m_function_name, "OnDataEvent"))
    {
        if (!strcmp(parameters[1], "mousedown"))
            for (unsigned int i = 0; i < (*a_functionLibraryList)[a_functionLibraryList->size() - 1].m_function_description_list.size(); ++i)
                if (!strcmp((*a_functionLibraryList)[a_functionLibraryList->size() - 1].m_function_description_list[i].m_function_name, parameters[2]))
                {
                    m_data_mousedown_callbacks[parameters[0]] = (*a_functionLibraryList)[a_functionLibraryList->size() - 1].m_function_description_list[i];
                    if (parameters[3])
                        strcpy(m_data_mousedown_callbacks[parameters[0]].m_parameter_list[6].Value, parameters[3]);
                    if (parameters[3] && parameters[4])
                        strcpy(m_data_mousedown_callbacks[parameters[0]].m_parameter_list[7].Value, parameters[4]);
                }
        return 0;
    }
    for (unsigned int s = 0; s < a_functionLibraryList->size(); ++s)
        for (unsigned int i = 0; i < (*a_functionLibraryList)[s].m_function_description_list.size(); ++i)
            if (!strcmp((*a_functionLibraryList)[s].m_function_description_list[i].m_function_name, a_function->m_function_name))
                return (*a_functionLibraryList)[s].m_base_function_proc(i, (char*)a_function->m_statement_body.c_str(), parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5], parameters[6], parameters[7], parameters[8], parameters[9], parameters[10], parameters[11]);
    result = new char [200];
    sprintf(result, "%s '%s'", "ERROR: Script: No such function:", a_function->m_function_name);
    return result;
}

bool CScriptRunner::GetLastFunctionCoords(char* a_function, char** a_begin, char** a_end)
{
    char* pos1 = strstrfromend(a_function, "(");

    char* braceletpos1 = strstr(a_function, "{");
    if (braceletpos1 && braceletpos1 < pos1) /// handling f() { f() };
    {
        pos1 = 0;
        for (char* i = braceletpos1; i > a_function; --i)
            if (*i == '(')
            {
                pos1 = i;
                break;
            }
    }

    if (!pos1)
        return false;
    *a_end = strstr(pos1, ")");

    if (!*a_end)
        return false;

    *a_begin = a_function;
    bool wassomethingelsethanspace = false;
    for (char* i = pos1 - 2; i >= a_function; --i)
    {
        if (*i == 32 || *i == 40 || *i == 44 || *i == 59 || *i == '{') /// " (,;{"
            if ((*i != 32) || wassomethingelsethanspace)
            {
                *a_begin = i + 1;
                goto found;
            }
        if (*i != 32)
            wassomethingelsethanspace = true;
    }
found:
    return true;
}

char* CScriptRunner::SetStopOnFirstError(char* a_bool)
{
    if (!a_bool)
        return MakeString(0, "ERROR: SetStopOnFirstError: not enough arguments!");
    if (!strcmp(a_bool, "true"))
        m_stop_on_first_error = true;
    else if (!strcmp(a_bool, "false"))
        m_stop_on_first_error = false;
    else
        return MakeString(0, "ERROR: SetStopOnFirstError: not a valid argument '", a_bool, "'");
    return 0;
}

char* CScriptRunner::functions_with_script_body_proc(int a_procindx, char* a_statement_body, char* a_param1, char* a_param2, char* a_param3, char* a_param4, char* a_param5, char* a_param6, char* a_param7, char* a_param8, char* a_param9, char* a_param10, char* a_param11, char* a_param12)
{
    TFunctionLibrary& functions_with_script_body_lib = (*m_actualm_function_library_list)[m_actualm_function_library_list->size() - 1];
    char* result = 0;
    if (!functions_with_script_body_lib.m_function_description_list[a_procindx].m_statement_body.length())
    {
        result = new char[200];
        sprintf(result, "%s %s", "ERROR: functions_with_script_body_proc: function body not found! ", functions_with_script_body_lib.m_function_description_list[a_procindx].m_function_name);
    }
    else
    {
        char* function_body = new char[functions_with_script_body_lib.m_function_description_list[a_procindx].m_statement_body.length() * 10]; ///TODO: this is not safe - calculate final size, or use std::string replace instead of ReplaceStringWithString bellow
        strcpy(function_body, functions_with_script_body_lib.m_function_description_list[a_procindx].m_statement_body.c_str());
        vector<TFunctionParameter>& parameter_list_ref = functions_with_script_body_lib.m_function_description_list[a_procindx].m_parameter_list;
        char* parameters[12] = {a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7, a_param8, a_param9, a_param10, a_param11, a_param12};
        for (unsigned int i = 0; i < 12; ++i)
            if (parameter_list_ref.size() > i)
                ReplaceStringWithString(function_body, parameter_list_ref[i].ParameterName, parameters[i]);
            else
                break;
        if (int sciptres = CScriptRunner::Call(function_body) != 0)
        {
            result = new char[200];
            sprintf(result, "%s%s. Error code: %d", "ERROR: functions_with_script_body_proc: error executing function: ", functions_with_script_body_lib.m_function_description_list[a_procindx].m_function_name, sciptres);
        }
        delete[] function_body;
    }
    return result;
}

char* CScriptRunner::ExecuteLastFunction(char* a_function, CFunctionLibraryList* a_function_library_list)
{
    CFunctionDescription functionDescription;
    char* lastfuncstart;
    char* lastfuncstop;
    if (GetLastFunctionCoords(a_function, &lastfuncstart, &lastfuncstop))
    {
        bool is_definition = false;
        if (*((int*)a_function) == *((int*)"def "))
            is_definition = true;

        char nextfunc[lastfuncstop - lastfuncstart + 2];
        memcpy(nextfunc, lastfuncstart, lastfuncstop - lastfuncstart + 1);
        nextfunc[lastfuncstop - lastfuncstart + 1] = 0;
        char* firstBraceletOpenAfterFuncStop = strstr(lastfuncstop, "{");
        if (firstBraceletOpenAfterFuncStop)
        {
            char* matchingbracelet = findMatchingBracelet(firstBraceletOpenAfterFuncStop);
            if (matchingbracelet)
            {
                functionDescription.m_statement_body = firstBraceletOpenAfterFuncStop + 1;
                ((char*)functionDescription.m_statement_body.c_str())[functionDescription.m_statement_body.length() - 1] = 0;
                ReplaceInplaceCharWithChar((char*)functionDescription.m_statement_body.c_str(), '`', ';');
            }
        }
        bool succes = functionDescription.ParseDescription(nextfunc);
        if (succes)
        {
            if (!is_definition)
            {
                char* result = ExecuteFunction(a_function_library_list, &functionDescription);
                if (result)
                    if (strstr(result, "RESULT:"))
                    {
                        ReplaceStringWithString(result, "RESULT:", "");
                        ReplaceStringWithString(a_function, nextfunc, result);
                        delete[] result;
                        return 0;
                    }
                a_function[0] = 0;
                return result;
            }
            else
            {
                TFunctionLibrary* functions_with_script_body_lib = &(*a_function_library_list)[a_function_library_list->size() - 1];
                if (strcmp(functions_with_script_body_lib->m_library_name, "functions_with_script_body"))
                {
                    a_function_library_list->resize(a_function_library_list->size() + 1);
                    functions_with_script_body_lib = &(*a_function_library_list)[a_function_library_list->size() - 1];
                    strcpy(functions_with_script_body_lib->m_library_name, "functions_with_script_body");
                    functions_with_script_body_lib->m_base_function_proc = functions_with_script_body_proc;
                }

                functions_with_script_body_lib->m_function_description_list.resize(functions_with_script_body_lib->m_function_description_list.size() + 1);
                CFunctionDescription& function_descr = functions_with_script_body_lib->m_function_description_list[functions_with_script_body_lib->m_function_description_list.size() - 1];
                function_descr.ParseDescription(nextfunc);
                function_descr.m_statement_body = functionDescription.m_statement_body;
                a_function[0] = 0;
                return 0;
            }
        }
        else
        {
            char* error = new char[300];
            sprintf(error, "%s %s", "ERROR: Parser: Can't parse:", nextfunc);
            a_function[0] = 0;
            return error;
        }
    }
    else
    {
        char* error = new char[300];
        sprintf(error, "%s %s", "ERROR: Parser: Can't find valid function in: ", a_function);
        a_function[0] = 0;
        return error;
    }
}

int CScriptRunner::Call(const char* a_script)
{
    char* script = new char [strlen(a_script) + 1];
    strcpy(script, a_script);

    char* scriptIter = script;
    while (char* bracelet = strstr(scriptIter, "{"))
    {
        char* matchingbracelet = findMatchingBracelet(bracelet);
        if (matchingbracelet && (nextNonSpace(matchingbracelet) != ')') && (nextNonSpace(matchingbracelet) != ','))
        {
            ReplaceInplaceCharWithCharLen(bracelet, matchingbracelet - bracelet, ';', '`');
            if (*(matchingbracelet + 1) == ' ' || *(matchingbracelet + 1) == 10 || *(matchingbracelet + 1) == 13)
                *(matchingbracelet + 1) = ';';
            else if (*(matchingbracelet + 1) != ';' && *(matchingbracelet + 1) != '`')
            {
                if (strlen(matchingbracelet) > 24)
                    strcpy(matchingbracelet + 20, " ...");
                cout << "Error on position " << matchingbracelet - script << ": " << matchingbracelet << endl;
                cout << "Script error: Block closing bracelet must be followed by characters ';', ' ', or new line. Execution terminated." << endl;
                return 7;
            }
        }
        else if (nextNonSpace(matchingbracelet) != ')' && (nextNonSpace(matchingbracelet) != ','))
        {
            if (strlen(bracelet) > 24)
                strcpy(bracelet + 20, " ...");
            cout << "Error on position " << bracelet - script << ": " << bracelet << endl;
            cout << "Script error: Bracelet matching error. Execution terminated." << endl;
            return 8;
        }
        scriptIter = matchingbracelet + 1;
    }

    RemoveEnters(script);
    CStringVec* strvec = SplitString(script, ";");
    strvec->RebuildPreserve(strvec->m_size - 1);
    RemoveStringsFromEndAndBegin(strvec, " ");
    int linecount = 1;
    int res = 0;
    for (unsigned int i = 0; i < strvec->m_size; i++)
    {
        while(strlen(strvec->m_data[i].s))
        {
            char* result = m_actual_object->ExecuteLastFunction(strvec->m_data[i].s, m_actualm_function_library_list);
            if (result)
            {
                res = 1;
                cout << result << endl;
                delete[]result;
                if (m_stop_on_first_error)
                {
                    cout << "Execution terminated on first error." << endl;
                    goto scriptend;
                }
                else
                {
                    cout << "An error occurred, but the execution continue" << endl;
                }
            }
            linecount++;
        }
    }
scriptend:
    delete strvec;
    delete[] script;
    return res;
}

void ProcessScriptIncludes(string& a_script)
{
    size_t incl_pos = a_script.find("#include ");
    while (incl_pos != string::npos)
    {
        size_t nl_pos = a_script.find("\r\n", incl_pos);
        string file = a_script.substr(incl_pos + strlen("#include "), nl_pos - incl_pos - strlen("#include "));
        a_script.erase(incl_pos, nl_pos - incl_pos + 2);
        int fsize;
        if (byte* buff = LoadBuffer(file.c_str(), 0, &fsize))
        {
            string included((char*)buff, fsize);
            replace_all_between(included, "main ", ";");
            included += "\r\n";
            PreprocessComments(included);
            a_script.insert(incl_pos, included);
        }
        incl_pos = a_script.find("#include ");
    }
}

void ProcessScriptDefines(string& a_script)
{
    size_t incl_pos = a_script.find("#define ");
    while (incl_pos != string::npos)
    {
        size_t trp_pos = a_script.find_first_not_of(' ', incl_pos + strlen("#define "));

        size_t nl_pos = a_script.find("\r\n", incl_pos);
        string to_replace = a_script.substr(trp_pos, a_script.find(" ", trp_pos) - trp_pos);
        string replace_with = a_script.substr(trp_pos + to_replace.length(), nl_pos - trp_pos - to_replace.length());

        a_script.erase(incl_pos, nl_pos - incl_pos);
        replace_all(a_script, to_replace, replace_with);
        incl_pos = a_script.find("#define ");
    }
}

int CScriptRunner::RunScript(const char* a_script, bool reloadPlugins)
{
    string script_copy(a_script);
    script_copy += "\r\n";
    PreprocessComments(script_copy);
    ProcessScriptIncludes(script_copy);
    ProcessScriptDefines(script_copy);

    int res = 0;
    if (!m_actualm_function_library_list)
    {
        m_actualm_function_library_list = new CFunctionLibraryList(m_data_list_ref, &m_variable_list);
        bool libinited = m_actualm_function_library_list->InitCFunctionLibraryList();
        if (!libinited)
        {
            res = 2;
            cout << "Can't load processing plugins." << endl;
            delete m_actualm_function_library_list;
            m_actualm_function_library_list = 0;
        }
    }
    m_actual_object = this;
    if (m_actualm_function_library_list)
    {
        try
        {
            res = Call(script_copy.c_str());
        }
        catch (const std::exception& e)
        {
            std::cout << "exception: " << e.what() << '\n';
            res = 3;
        }
        if (reloadPlugins) /// "leak switch": Win8 and higher Win API calls are leaking DLL loads and releases with ~100kB / reload. Therefore plugin reload is disabled for intervals and data change callbacks."
        {
            m_actualm_function_library_list->UnLoadPlugins();
            delete m_actualm_function_library_list;
            m_actualm_function_library_list = 0;
        }
    }
    for (std::map<string, CVariable*>::iterator it = m_variable_list.begin(); it != m_variable_list.end(); it++)
        delete it->second;
    m_variable_list.clear();
    return res;
}

CScriptRunner::CScriptRunner(CSignalCodec_List* a_data_list_reference, IScriptRunnerObserver* a_script_runner_observer)
{
    m_script_runner_observer = a_script_runner_observer;
    m_data_list_ref = a_data_list_reference;
}

CScriptRunner::~CScriptRunner()
{
    for (std::map<string, CVariable*>::iterator it = m_variable_list.begin(); it != m_variable_list.end(); it++)
        delete it->second;
    if (m_actualm_function_library_list)
    {
        m_actualm_function_library_list->UnLoadPlugins();
        delete m_actualm_function_library_list;
    }
}

CScriptRunner* CScriptRunner::m_actual_object = 0;
CFunctionLibraryList* CScriptRunner::m_actualm_function_library_list = 0;
bool CScriptRunner::m_stop_on_first_error = true;
