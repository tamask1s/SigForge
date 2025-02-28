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
#include "datastructures.h"
#include "CodecLib.h"
#include "DataAq_Interface.h"
#include "FileBased_Codec.h"
#include "Backend.h"

class CBackendTest: public IBackendObserver
{
    IBackend* m_backend;

public:
    CBackendTest()
    {
        m_backend = IBackend::New(this);
        m_backend->RunScript("CreateVector(data, 1 2 345.678 -9); WriteAscii(data, test001.dbascii); SystemExit(0);", true);
    }

    virtual ~CBackendTest()
    {
        IBackend::Delete(m_backend);
    };

private:
    int Backend_RequestDataDelete(char* a_data_codec)
    {
        return 0;
    }

    int Backend_OnDataOut(ISignalCodec* a_data_codec, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
    {
        return 0;
    }

    char* Backend_OnRefreshDataWindow(ISignalCodec* a_data, bool a_full_change, bool a_first_change)
    {
        return 0;
    }

    char* Backend_OnSystemExit()
    {
        return 0;
    }

    char* Backend_OnBindScrolling(const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding)
    {
        return 0;
    }
};

int main()
{
    CBackendTest* backend_test = new CBackendTest();
    delete backend_test;
    return 0;
}
