#include <windows.h>
#include <stdio.h>
#include <iostream>
using namespace std;

#include "DataAq_Types.h"
#include "DataAq_Interface.h"

class CDataAq_Singleton
{
    struct TDataAqModuleInfo
    {
        int mID; /// Unique identification code
        char mGeneralDescription[512];
        char mCapabilityDescriptor[512];
    };

    int mNrDlls;
    HMODULE* mHDllList;
    TDataAqModuleInfo* mModuleInfoList;
    const char DataAqModulesPath[19] = "InputModules\\*.dll";
    static const unsigned int MODULEPATHSLENGTH = 13;  /// Size of "InputModules\\" string

public:
    static CDataAq_Singleton& DataAq_Singleton()
    {
        static CDataAq_Singleton instance;
        return instance;
    }

    HMODULE getHModuleByModuleIndx(unsigned int aModuleIndx)
    {
        return mHDllList[aModuleIndx];
    }

    int QueryDataAqModuleIndex(int ID)
    {
        for (int i = 0; i < mNrDlls; i++)
            if (mModuleInfoList[i].mID == ID)
                return i;
        return -1;
    }

    int LoadDataAqModules()
    {
        mNrDlls = 0;
        int i, N = 0;
        char Path[MAX_PATH];
        WIN32_FIND_DATA FindData;
        HANDLE Handle;
        HMODULE hDll;

        for (i = 0; i < MAX_PATH; Path[i++] = '\0')
            ;
        //HMODULE hm = GetModuleHandleA("Backend.dll");
        GetModuleFileName(NULL, Path, MAX_PATH);

        i = MAX_PATH;
        while ((i > 0) && (Path[--i] != '\\'))
            ;
        while ((i > 0) && (Path[--i] != '\\'))
            ;

        lstrcpy(&Path[++i], DataAqModulesPath);

        if ((Handle = FindFirstFile(Path, &FindData)) != INVALID_HANDLE_VALUE)
        {
            lstrcpyn(&Path[i], DataAqModulesPath, MODULEPATHSLENGTH + 1);
            lstrcpy(&Path[i + MODULEPATHSLENGTH], FindData.cFileName);
            if ((hDll = LoadLibrary(Path)))
            {
                if (GetProcAddress(hDll, "getModuleInfo"))
                    N++;
            }
            FreeLibrary(hDll);

            while (FindNextFile(Handle, &FindData))
            {
                lstrcpyn(&Path[i], DataAqModulesPath, MODULEPATHSLENGTH + 1);
                lstrcpy(&Path[i + MODULEPATHSLENGTH], FindData.cFileName);
                if ((hDll = LoadLibrary(Path)))
                {
                    if (GetProcAddress(hDll, "getModuleInfo"))
                        N++;
                }
                FreeLibrary(hDll);
            }
            FindClose(Handle);
        }
        mNrDlls = N;
        mHDllList = new HMODULE[mNrDlls];
        mModuleInfoList = new TDataAqModuleInfo[mNrDlls];

        N = 0;
        lstrcpy(&Path[i], DataAqModulesPath);
        if ((Handle = FindFirstFile(Path, &FindData)) != INVALID_HANDLE_VALUE)
        {
            lstrcpyn(&Path[i], DataAqModulesPath, MODULEPATHSLENGTH + 1);
            lstrcpy(&Path[i + MODULEPATHSLENGTH], FindData.cFileName);
            if ((hDll = LoadLibrary(Path)))
            {
                if (funcRef_getModuleInfo getModuleInfo = (funcRef_getModuleInfo) GetProcAddress(hDll, "getModuleInfo"))
                {
                    mHDllList[N] = hDll;
                    getModuleInfo(&mModuleInfoList[N].mID, mModuleInfoList[N].mGeneralDescription, mModuleInfoList[N].mCapabilityDescriptor, 512);
                    N++;
                }
            }
            while (FindNextFile(Handle, &FindData))
            {
                lstrcpyn(&Path[i], DataAqModulesPath, MODULEPATHSLENGTH + 1);
                lstrcpy(&Path[i + MODULEPATHSLENGTH], FindData.cFileName);
                if ((hDll = LoadLibrary(Path)))
                {
                    if (funcRef_getModuleInfo getModuleInfo = (funcRef_getModuleInfo) GetProcAddress(hDll, "getModuleInfo"))
                    {
                        mHDllList[N] = hDll;
                        getModuleInfo(&mModuleInfoList[N].mID, mModuleInfoList[N].mGeneralDescription, mModuleInfoList[N].mCapabilityDescriptor, 512);
                        N++;
                    }
                }
            }
            FindClose(Handle);
        }
        return mNrDlls;
    }

    string GetModuleDescriptions()
    {
        string res = "{ \n";
        char tmp[100];
        for (int i = 0; i < mNrDlls; i++)
        {
            sprintf(tmp, "  \"%d\": ", mModuleInfoList[i].mID);
            res += tmp;
            res += mModuleInfoList[i].mGeneralDescription;
            if (i != mNrDlls - 1)
                res += ",\n";
        }
        res += "\n}";
        return res;
    }

    void FreeDataAqModules()
    {
        for (int i = 0; i < mNrDlls; FreeLibrary(mHDllList[i++]));
        ;
        delete[] mHDllList;
        delete[] mModuleInfoList;
    }
};

int IDataAq::LoadDataAqModules()
{
    return CDataAq_Singleton::DataAq_Singleton().LoadDataAqModules();
}

string IDataAq::GetModuleDescriptions()
{
    return CDataAq_Singleton::DataAq_Singleton().GetModuleDescriptions();
}

void IDataAq::FreeDataAqModules()
{
    return CDataAq_Singleton::DataAq_Singleton().FreeDataAqModules();
}

class CDataAq: public IDataAq
{
    int mDataAqID;
    int mHandle;
    int (*func_connectDevice) (const char* a_connection_parameter);
    int (*func_disconnectDevice) (int a_handle);
    int (*func_setupReading) (int a_handle, int a_nr_channels_to_use, const int* a_sample_rates, const int* a_gain, const int* a_physical_mapping, int a_milliseconds_to_read);
    int (*func_startReading) (int a_handle);
    int (*func_stopReading) (int a_handle);
    int (*func_digitalRead) (int a_handle, int* aData, int a_samples_per_channel_to_read, double a_timeout);
    int (*func_digitalRange) (int a_handle, int* a_digital_min, int* a_digital_max);
    int (*func_analogRead) (int a_handle, double* aData);

public:
    virtual ~CDataAq()
    {}

    CDataAq(unsigned int a_dataAq_id)
        :mDataAqID(a_dataAq_id)
    {
        mHandle = 0;
        int ModuleIndex = CDataAq_Singleton::DataAq_Singleton().QueryDataAqModuleIndex(a_dataAq_id);
        HMODULE hDll = 0;
        bool success = true;
        if (ModuleIndex != -1)
        {
            hDll = CDataAq_Singleton::DataAq_Singleton().getHModuleByModuleIndx(ModuleIndex);
            if (!(func_connectDevice = (funcRef_connectDevice) GetProcAddress(hDll, "connectDevice")))
                success = false;
            if (!(func_disconnectDevice = (funcRef_disconnectDevice) GetProcAddress(hDll, "disconnectDevice")))
                success = false;
            if (!(func_setupReading = (funcRef_setupReading) GetProcAddress(hDll, "setupReading")))
                success = false;
            if (!(func_startReading = (funcRef_startReading) GetProcAddress(hDll, "startReading")))
                success = false;
            if (!(func_stopReading = (funcRef_stopReading) GetProcAddress(hDll, "stopReading")))
                success = false;
            if (!(func_digitalRead = (funcRef_digitalRead) GetProcAddress(hDll, "digitalRead")))
                success = false;
            if (!(func_digitalRange = (funcRef_digitalRange) GetProcAddress(hDll, "digitalRange")))
                success = false;
            if (!(func_analogRead = (funcRef_analogRead) GetProcAddress(hDll, "analogRead")))
                success = false;
        }
        cout << "Data acquisition dynamic load: " << std::boolalpha << success << endl;
    }

    int connectDevice(const char* a_connection_parameter)
    {
        ///cout << "connecting device" << endl;
        mHandle = func_connectDevice(a_connection_parameter);
        return mHandle;
    }

    int disconnectDevice()
    {
        ///cout << "connecting device" << endl;
        return func_disconnectDevice(mHandle);
    }

    int setupReading(int a_nr_channels_to_use, const int* a_sample_rates, const int* a_gain, const int* a_physical_mapping, int a_milliseconds_to_read)
    {
        return func_setupReading(mHandle, a_nr_channels_to_use, a_sample_rates, a_gain, a_physical_mapping, a_milliseconds_to_read);
    }

    int startReading()
    {
        return func_startReading(mHandle);
    }

    int stopReading()
    {
        return func_stopReading(mHandle);
    }

    int digitalRead(int* a_data, int a_samples_per_channel_to_read, double a_timeout)
    {
        return func_digitalRead(mHandle, a_data, a_samples_per_channel_to_read, a_timeout);
    }

    int digitalRange(int* a_digital_min, int* a_digital_max)
    {
        return func_digitalRange(mHandle, a_digital_min, a_digital_max);
    }

    int analogRead(double* aData)
    {
        return func_analogRead(mHandle, aData);
    }
};

IDataAq* IDataAq::New(unsigned int a_dataAq_id)
{
    return new CDataAq(a_dataAq_id);
}

void IDataAq::Delete(IDataAq* a_instance)
{
    delete ((CDataAq*) a_instance);
}
