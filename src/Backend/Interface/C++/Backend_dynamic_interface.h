#ifndef BACKEND_DYNAMIC_INTERFACE_H
#define BACKEND_DYNAMIC_INTERFACE_H

typedef bool (*DGetDataRaw_PROC_TYPE) (const char* a_data_name, double** a_buffer, unsigned int* a_starts, unsigned int* a_nr_elements, int* enable);
typedef int (*DGetNrChannels_PROC_TYPE) (const char* a_data_name);
typedef bool (*DGetTotalSamples_PROC_TYPE) (const char* a_data_name, unsigned int* a_total_samples);
typedef bool (*DGetSampleRates_PROC_TYPE) (const char* a_data_name, double* a_sample_rates);
typedef int (*RunScript_PROC_TYPE) (const char* a_script, bool a_reload_lugins);
typedef char* (*OpenFile_PROC_TYPE) (const char* a_file_name, const char* a_data_name);
typedef void (*Interval_PROC_TYPE) ();
typedef void (*InitializeBackend_PROC_TYPE) (DBackend_RequestDataDelete_PROC_TYPE a_datadelete_proc, DBackend_OnDataOut_PROC_TYPE a_dataout_proc, DBackend_OnRefreshDataWindow_PROC_TYPE a_refresh_proc, DBackend_OnSystemExit_PROC_TYPE a_exit_proc, DBackend_OnBindScrolling_PROC_TYPE a_BindScrolling_proc);
typedef void (*DestroyData_PROC_TYPE) (const char* a_data_name);
typedef const char* (*GetCopyrightInfo_PROC_TYPE) ();
typedef const char* (*GetFunctionListRef_PROC_TYPE) ();
typedef void (*SetDescription_PROC_TYPE) (const char* a_data_name, char* a_description);
typedef const char* (*GetDescription_PROC_TYPE) (const char* a_data_name);
typedef const char* (*GetFileFormatExtensions_PROC_TYPE) ();
typedef const char* (*GetUnits_PROC_TYPE) (const char* a_data_name);

#endif /// BACKEND_DYNAMIC_INTERFACE_H
