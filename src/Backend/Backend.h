#ifndef BACKEND_H
#define BACKEND_H

class IBackend
{
public:
    virtual int RunScript(const char* a_script, bool reloadPlugins) = 0;
    virtual char* Open_File(const char* a_file_name, const char* a_DataVarName) = 0;
    virtual const char* GetFunctionListRef() = 0;
    virtual void Interval() = 0;
    virtual void DestroyCodec(ISignalCodec* a_codec) = 0;
    virtual void DataEvent(ISignalCodec* a_codec, int a_event, double a_val_x, double a_val_y, int a_data_channel, double a_data_val, char* a_info) = 0;
    virtual const char* GetCopyrightTextRef() = 0;
    static IBackend* New(class IBackendObserver* a_backend_observer);
    static void Delete(IBackend* a_instance);
};

class IBackendObserver
{
public:
    virtual int Backend_RequestDataDelete(char* a_data_codec) = 0;
    virtual int Backend_OnDataOut(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid) = 0;
    virtual char* Backend_OnRefreshDataWindow(ISignalCodec* a_data, bool a_full_change, bool a_first_change) = 0;
    virtual char* Backend_OnSystemExit() = 0;
    virtual char* Backend_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding) = 0;
};

#endif /// BACKEND_H
