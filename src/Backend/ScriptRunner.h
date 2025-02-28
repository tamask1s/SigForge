#ifndef SCRIPTRUNNER_H_
#define SCRIPTRUNNER_H_

typedef int  (*GetProcedureList_Proc) (TFunctionLibrary* a_functionlibrary_reference);
typedef void (*InitLib_PROC)          (CSignalCodec_List* a_datalist, CVariable_List* a_variable_list, NewCVariable_Proc a_new_variable, NewChar_Proc a_new_char, Call_Proc a_inp_Call, FFT_PROC a_inp_FFT, RFFT_PROC a_inp_RFFT, FFTEXEC_PROC a_inp_FFTEXEC, FFTDESTROY_PROC a_inp_FFTDESTROY);

class CFunctionLibraryList: public vector<TFunctionLibrary>
{
    CSignalCodec_List* m_data_list_ref;
    CVariable_List*    m_variable_list_ref;

    static CVariable* NewCVariable();
    static char* NewChar(int a_size);

    int Fill(const char* a_path, WIN32_FIND_DATA* a_finddata);
    int LoadPlugins(const char* a_path);

public:
    int GetCopyrightInfo(CStringMx* a_info);
    int InitCFunctionLibraryList();
    void UnLoadPlugins();
    CFunctionLibraryList(CSignalCodec_List* a_data_list_reference, CVariable_List* a_variable_list_reference);
};

struct TInterval
{
    string m_interval_name;
    string m_statement_body;
    int m_interval;

    TInterval(const string& a_statement_body, const string& a_interval_name, const int a_interval)
        : m_interval_name(a_interval_name),
          m_statement_body(a_statement_body),
          m_interval(a_interval)
    {}
};

struct TDataEventCallback
{
    string m_statement_body;

    TDataEventCallback(const string& a_statement_body)
        : m_statement_body(a_statement_body)
    {}
};

class IScriptRunnerObserver
{
public:
    virtual int ScriptRunner_RequestDataDelete(char* a_data_codec) = 0;
    virtual void ScriptRunner_OnDataOut(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid) = 0;
    virtual char* ScriptRunner_OnRefreshDataWindow(const char* a_datam_varname, bool a_full_change) = 0;
    virtual char* ScriptRunner_OnFileOpen(const char* a_file_name, const char* a_datam_varname) = 0;
    virtual char* ScriptRunner_OnSystemExit() = 0;
    virtual char* ScriptRunner_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding) = 0;
};

class CScriptRunner
{
    static CScriptRunner*             m_actual_object;
    CVariable_List                    m_variable_list;
    CSignalCodec_List*                m_data_list_ref;
    vector<TInterval>                 m_intervals;
    map<string, TDataEventCallback>   m_data_changed_callbacks;
    map<string, CFunctionDescription> m_data_mousedown_callbacks;
    IScriptRunnerObserver*            m_script_runner_observer;
    static CFunctionLibraryList*      m_actualm_function_library_list;
    static bool                       m_stop_on_first_error;

    char* RequestDataDelete(char* a_data_name);
    char* DataOutput(char* a_outvarname, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid);
    char* NewDataAq(const char* a_dataAq_device_type, const char* a_dataAq_params, const char* a_sampling_rates, const char* a_gain, const char* a_physical_mapping, const char* a_milliSeconds_to_read, const char* a_fileCodec_type, const char* a_file_name, const char* a_signal_name);
    char* ExecuteFunction(CFunctionLibraryList* a_functionLibraryList, CFunctionDescription* a_function);
    char* ExecuteLastFunction(char* a_function, CFunctionLibraryList* a_functionLibraryList);
    char* RefreshDataWindow(const char* a_datam_varname, const char* a_params);
    char* FileOpen(const char* a_file_name, const char* a_datam_varname);
    char* SaveDataToFile(char* a_varname, char* a_filename, char* a_window_milliseconds, const char* a_file_codec_type = nullptr, bool save_variable = false);
    char* SaveVarToFile(char* a_varname, char* a_filename, char* a_window_milliseconds, const char* a_file_codec_type = nullptr);
    char* NewFileDataBasedOnData(char* a_dst_data_name, char* a_template_data_name, char* a_window_milliseconds, char* a_codec_type, char* a_filename, char* a_channels);
    char* NewMemoryData(char* a_dst_data_name, char* a_nr_channels, char* a_samplerates, char* a_nr_hor_units);
    static bool GetLastFunctionCoords(char* a_function, char** a_begin, char** a_end);
    char* SetStopOnFirstError(char* a_bool);

public:
    static char* functions_with_script_body_proc(int a_procindx, char* a_statement_body, char* a_param1, char* a_param2, char* a_param3, char* a_param4, char* a_param5, char* a_param6, char* a_param7, char* a_param8, char* a_param9, char* a_param10, char* a_param11, char* a_param12);
    char* OnDataEvent(ISignalCodec* a_codec, int a_event, double a_val_x, double a_val_y, int a_data_channel, double a_data_val, char* a_info);
    CScriptRunner(CSignalCodec_List* a_datalistreference, IScriptRunnerObserver* a_script_runner_observer);
    void SystemInterval();
    char* TriggerDataChanged(const char* a_data_varname);
    virtual~CScriptRunner();
    int RunScript(const char* a_script, bool reloadPlugins);
    static int Call(const char* a_script);
};

HMODULE InitFFTLib();

#endif /// SCRIPTRUNNER_H_
