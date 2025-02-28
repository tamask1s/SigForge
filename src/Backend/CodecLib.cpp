#include <windows.h>
#include "CodecFunctions.h"
#include "CodecLib.h"
#include <stdio.h>
#include <iostream>
using namespace std;

#define CODECSLENGTH 7  // Size of "Codecs\\" string
const char CodecsPath[] = "Codecs\\*.dll";

struct DCodecInfo
{
    int ID;               // Unique FileType identification code
    char Extension[64];   // File extension
    char Description[64]; // File type description
    DCodecInfo()
    {
        Extension[0] = 0;
        Description[0] = 0;
    }
};

int nDll = 0;
HMODULE* hDllArray;
DCodecInfo* FileTypeArray;
DGetCodecInfoProc DGetCodecInfo;

int LoadCodecTypes()
{
    int i, N = 0;
    char Path[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE Handle;
    HMODULE hDll;

    for (i = 0; i < MAX_PATH; Path[i++] = '\0')
        ;
    GetModuleFileName(NULL, Path, MAX_PATH);

    i = MAX_PATH;
    while ((i > 0) && (Path[--i] != '\\'))
        ;
    while ((i > 0) && (Path[--i] != '\\'))
        ;

    lstrcpy(&Path[++i], CodecsPath);

    if ((Handle = FindFirstFile(Path, &FindData)) != INVALID_HANDLE_VALUE)
    {
        lstrcpyn(&Path[i], CodecsPath, CODECSLENGTH + 1);
        lstrcpy(&Path[i + CODECSLENGTH], FindData.cFileName);
        if ((hDll = LoadLibrary(Path)))
        {
            if ((DGetCodecInfo = (DGetCodecInfoProc) GetProcAddress(hDll, "DGetCodecInfo")))
                N++;
        }
        FreeLibrary(hDll);

        while (FindNextFile(Handle, &FindData))
        {
            lstrcpyn(&Path[i], CodecsPath, CODECSLENGTH + 1);
            lstrcpy(&Path[i + CODECSLENGTH], FindData.cFileName);
            if ((hDll = LoadLibrary(Path)))
            {
                if ((DGetCodecInfo = (DGetCodecInfoProc) GetProcAddress(hDll, "DGetCodecInfo")))
                    N++;
            }
            FreeLibrary(hDll);
        }
        FindClose(Handle);
    }
    nDll = N;
    hDllArray = new HMODULE[nDll];
    FileTypeArray = new DCodecInfo[nDll];

    N = 0;
    lstrcpy(&Path[i], CodecsPath);
    if ((Handle = FindFirstFile(Path, &FindData)) != INVALID_HANDLE_VALUE)
    {
        lstrcpyn(&Path[i], CodecsPath, CODECSLENGTH + 1);
        lstrcpy(&Path[i + CODECSLENGTH], FindData.cFileName);
        if ((hDll = LoadLibrary(Path)))
        {
            if ((DGetCodecInfo = (DGetCodecInfoProc) GetProcAddress(hDll, "DGetCodecInfo")))
            {
                hDllArray[N] = hDll;
                DGetCodecInfo(FileTypeArray[N].Extension, FileTypeArray[N].Description, &FileTypeArray[N].ID);
                N++;
            }
        }
        while (FindNextFile(Handle, &FindData))
        {
            lstrcpyn(&Path[i], CodecsPath, CODECSLENGTH + 1);
            lstrcpy(&Path[i + CODECSLENGTH], FindData.cFileName);
            if ((hDll = LoadLibrary(Path)))
            {
                if ((DGetCodecInfo = (DGetCodecInfoProc) GetProcAddress(hDll, "DGetCodecInfo")))
                {
                    hDllArray[N] = hDll;
                    DGetCodecInfo(FileTypeArray[N].Extension, FileTypeArray[N].Description, &FileTypeArray[N].ID);
                    N++;
                }
            }
        }
        FindClose(Handle);
    }
    return nDll;
}

void FreeCodecTypes()
{
    for (int i = 0; i < nDll; FreeLibrary(hDllArray[i++]))
        ;
    delete[] hDllArray;
    delete[] FileTypeArray;
}

int QueryCodecCount()
{
    return nDll;
}

int QueryCodecIndex(int ID)
{
    for (int i = 0; i < nDll; i++)
        if (FileTypeArray[i].ID == ID)
            return i;
    return -1;
}

void QueryCodecExtensions(char* a_extensions)
{
    strcpy(a_extensions, "All supported formats (");
    for (int i = 0; i < nDll; i++)
    {
        strcat(a_extensions, FileTypeArray[i].Extension);
        strcat(a_extensions, ";");
    }
    a_extensions[strlen(a_extensions) - 1] = 0;
    strcat(a_extensions, ")\0");
    a_extensions += strlen(a_extensions) + 1;
    for (int i = 0; i < nDll; i++)
    {
        strcat(a_extensions, FileTypeArray[i].Extension);
        strcat(a_extensions, ";");
    }
    a_extensions[strlen(a_extensions) - 1] = 0;
    strcat(a_extensions, "\0");
    a_extensions += strlen(a_extensions) + 1;

    strcat(a_extensions, "All Files (*.*)\0");
    a_extensions += strlen(a_extensions) + 1;
    strcat(a_extensions, "*.*\0");
    a_extensions += strlen(a_extensions) + 1;

    for (int i = 0; i < nDll; i++)
    {
        strcpy(a_extensions, FileTypeArray[i].Description);
        strcat(a_extensions, " (");
        strcat(a_extensions, FileTypeArray[i].Extension);
        strcat(a_extensions, ")\0");
        a_extensions += strlen(a_extensions) + 1;
        strcpy(a_extensions, FileTypeArray[i].Extension);
        strcat(a_extensions, "\0");
        a_extensions += strlen(a_extensions) + 1;
    }

    strcpy(a_extensions, "\0");
}

int QueryCodecIdByExtension(char* aExtension)
{
    for (int i = 0; i < nDll; i++)
        if (strstr(FileTypeArray[i].Extension, aExtension))
            return FileTypeArray[i].ID;
    return -1;
}

TDataType* TDataType::CreateData(int ID)
{
    TDataType* Data = new TDataType;
    int codecIndex = QueryCodecIndex(ID);
    HMODULE hDll = 0;
    if (codecIndex == -1)
        goto Error;
    hDll = hDllArray[codecIndex];

    if (!(Data->DInitData = (DInitDataProc) GetProcAddress(hDll, "DInitData")))
        goto Error;
    if (!(Data->DOpenFile = (DOpenFileProc) GetProcAddress(hDll, "DOpenFile")))
        goto Error;
    if (!(Data->DCreateNewFile = (DCreateNewFileProc) GetProcAddress(hDll, "DCreateNewFile")))
        goto Error;
    if (!(Data->DCloseFile = (DCloseFileProc) GetProcAddress(hDll, "DCloseFile")))
        goto Error;
    if (!(Data->DGetDataRaw = (DGetDataRawProc) GetProcAddress(hDll, "DGetDataRaw")))
        goto Error;
    if (!(Data->DGetChannelRaw = (DGetChannelRawProc) GetProcAddress(hDll, "DGetChannelRaw")))
        goto Error;
    if (!(Data->DWriteHeader = (DWriteHeaderProc) GetProcAddress(hDll, "DWriteHeader")))
        goto Error;
    if (!(Data->DWriteBlock = (DWriteBlockProc) GetProcAddress(hDll, "DWriteBlock")))
        goto Error;
    if (!(Data->DAppendSamples = (DAppendSamplesProc) GetProcAddress(hDll, "DAppendSamples")))
        goto Error;
    if (!(Data->DDeleteData = (DDeleteDataProc) GetProcAddress(hDll, "DDeleteData")))
        goto Error;
    if (!(Data->DGetNrChannels = (DGetNrChannelsProc) GetProcAddress(hDll, "DGetNrChannels")))
        goto Error;
    if (!(Data->DGetTotalSamples = (DGetTotalSamplesProc) GetProcAddress(hDll, "DGetTotalSamples")))
        goto Error;
    if (!(Data->DGetSampleRates = (DGetSampleRatesProc) GetProcAddress(hDll, "DGetSampleRates")))
        goto Error;
    if (!(Data->DGetLabel = (DGetLabelProc) GetProcAddress(hDll, "DGetLabel")))
        goto Error;
    if (!(Data->DGetTransducer = (DGetTransducerProc) GetProcAddress(hDll, "DGetTransducer")))
        goto Error;
    if (!(Data->DGetVerticalUnit = (DGetVerticalUnitProc) GetProcAddress(hDll, "DGetVerticalUnit")))
        goto Error;
    if (!(Data->DGetHorizontalUnit = (DGetHorizontalUnitProc) GetProcAddress(hDll, "DGetHorizontalUnit")))
        goto Error;
    if (!(Data->DGetRecording = (DGetRecordingProc) GetProcAddress(hDll, "DGetRecording")))
        goto Error;
    if (!(Data->DGetPatient = (DGetPatientProc) GetProcAddress(hDll, "DGetPatient")))
        goto Error;
    if (!(Data->DGetDate = (DGetDateProc) GetProcAddress(hDll, "DGetDate")))
        goto Error;
    if (!(Data->DGetTime = (DGetTimeProc) GetProcAddress(hDll, "DGetTime")))
        goto Error;
    if (!(Data->DGetFileName = (DGetFileNameProc) GetProcAddress(hDll, "DGetFileName")))
        goto Error;
    if (!(Data->DSetSampleRates = (DSetSampleRatesProc) GetProcAddress(hDll, "DSetSampleRates")))
        goto Error;
    if (!(Data->DSetPhysicalMin = (DSetPhysicalMinProc) GetProcAddress(hDll, "DSetPhysicalMin")))
        goto Error;
    if (!(Data->DSetPhysicalMax = (DSetPhysicalMaxProc) GetProcAddress(hDll, "DSetPhysicalMax")))
        goto Error;
    if (!(Data->DSetRecordSamples = (DSetRecordSamplesProc) GetProcAddress(hDll, "DSetRecordSamples")))
        goto Error;
    if (!(Data->DSetRecordDuration = (DSetRecordDurationProc) GetProcAddress(hDll, "DSetRecordDuration")))
        goto Error;
    if (!(Data->DSetTotalSamples = (DSetTotalSamplesProc) GetProcAddress(hDll, "DSetTotalSamples")))
        goto Error;
    if (!(Data->DSetLabel = (DSetLabelProc) GetProcAddress(hDll, "DSetLabel")))
        goto Error;
    if (!(Data->DSetTransducer = (DSetTransducerProc) GetProcAddress(hDll, "DSetTransducer")))
        goto Error;
    if (!(Data->DSetVerticalUnit = (DSetVerticalUnitProc) GetProcAddress(hDll, "DSetVerticalUnit")))
        goto Error;
    if (!(Data->DSetHorizontalUnit = (DSetHorizontalUnitProc) GetProcAddress(hDll, "DSetHorizontalUnit")))
        goto Error;
    if (!(Data->DSetRecording = (DSetRecordingProc) GetProcAddress(hDll, "DSetRecording")))
        goto Error;
    if (!(Data->DSetPatient = (DSetRecordingProc) GetProcAddress(hDll, "DSetPatient")))
        goto Error;
    if (!(Data->DSetDate = (DSetDateProc) GetProcAddress(hDll, "DSetDate")))
        goto Error;
    if (!(Data->DSetTime = (DSetTimeProc) GetProcAddress(hDll, "DSetTime")))
        goto Error;

    Data->m_codec = Data->DInitData();
    return Data;
Error:
    delete Data;
    return NULL;
}

int TDataType::InitData()
{
    return DInitData();
}

bool TDataType::OpenFile(char* fullpath, const bool writeaccess)
{
    return DOpenFile(m_codec, fullpath, writeaccess);
}

bool TDataType::CreateNewFile(const char* fullpath, int nrchans, const bool rewrite)
{
    return DCreateNewFile(m_codec, fullpath, nrchans, rewrite);
}

bool TDataType::CloseFile()
{
    return DCloseFile(m_codec);
}

bool TDataType::GetDataRaw(double** buffer, unsigned int* start, unsigned int* stop)
{
    return DGetDataRaw(m_codec, buffer, start, stop);
}

bool TDataType::GetChannelRaw(double** buffer, int* enable, unsigned int* start, unsigned int* stop)
{
    return DGetChannelRaw(m_codec, buffer, enable, start, stop);
}

bool TDataType::WriteHeader()
{
    return DWriteHeader(m_codec);
}

bool TDataType::WriteBlock(double** buffer, unsigned int* start, unsigned int* stop)
{
    return DWriteBlock(m_codec, buffer, start, stop);
}

bool TDataType::AppendSamples(double** buffer, unsigned int nrsamples)
{
    return DAppendSamples(m_codec, buffer, nrsamples);
}

void TDataType::DeleteData()
{
    DDeleteData(m_codec);
}

int TDataType::GetNrChannels()
{
    return DGetNrChannels(m_codec);
}

bool TDataType::GetTotalSamples(unsigned int* a_total_samples)
{
    return DGetTotalSamples(m_codec, a_total_samples);
}

bool TDataType::GetSampleRates(double* a_sample_rates)
{
    return DGetSampleRates(m_codec, a_sample_rates);
}

bool TDataType::GetLabel(int aChannelIndx, char* aLabel)
{
    return DGetLabel(m_codec, aChannelIndx, aLabel);
}

bool TDataType::GetTransducer(int aChannelIndx, char* aTransducer)
{
    return DGetTransducer(m_codec, aChannelIndx, aTransducer);
}

bool TDataType::GetVerticalUnit(int aChannelIndx, char* aVerticalUnit)
{
    return DGetVerticalUnit(m_codec, aChannelIndx, aVerticalUnit);
}

bool TDataType::GetHorizontalUnit(char* aHorizontalUnit)
{
    return DGetHorizontalUnit(m_codec, aHorizontalUnit);
}

bool TDataType::GetRecording(char* a_recording)
{
    return DGetRecording(m_codec, a_recording);
}

bool TDataType::GetPatient(char* a_patient)
{
    return DGetPatient(m_codec, a_patient);
}

bool TDataType::GetDate(char* a_date)
{
    return DGetDate(m_codec, a_date);
}

bool TDataType::GetTime(char* a_time)
{
    return DGetTime(m_codec, a_time);
}

bool TDataType::GetFileName(char* a_file_name)
{
    return DGetFileName(m_codec, a_file_name);
}

bool TDataType::SetSampleRates(double* a_sample_rates)
{
    return DSetSampleRates(m_codec, a_sample_rates);
}

bool TDataType::SetPhysicalMin(double* a_physical_min)
{
    return DSetPhysicalMin(m_codec, a_physical_min);
}

bool TDataType::SetPhysicalMax(double* a_physical_max)
{
    return DSetPhysicalMax(m_codec, a_physical_max);
}

bool TDataType::SetRecordSamples(unsigned int* a_record_samples)
{
    return DSetRecordSamples(m_codec, a_record_samples);
}

bool TDataType::SetRecordDuration(double aRecordDuration)
{
    return DSetRecordDuration(m_codec, aRecordDuration);
}

bool TDataType::SetTotalSamples(unsigned int* a_total_samples)
{
    return DSetTotalSamples(m_codec, a_total_samples);
}

bool TDataType::SetLabel(int aChannelIndx, char* aLabel)
{
    return DSetLabel(m_codec, aChannelIndx, aLabel);
}

bool TDataType::SetTransducer(int aChannelIndx, char* aTransducer)
{
    return DSetTransducer(m_codec, aChannelIndx, aTransducer);
}

bool TDataType::SetVerticalUnit(int aChannelIndx, char* aVerticalUnit)
{
    return DSetVerticalUnit(m_codec, aChannelIndx, aVerticalUnit);
}

bool TDataType::SetHorizontalUnit(char* aHorizontalUnit)
{
    return DSetHorizontalUnit(m_codec, aHorizontalUnit);
}

bool TDataType::SetRecording(char* a_recording)
{
    return DSetRecording(m_codec, a_recording);
}

bool TDataType::SetPatient(char* a_patient)
{
    return DSetPatient(m_codec, a_patient);
}

bool TDataType::SetDate(char* a_date)
{
    return DSetDate(m_codec, a_date);
}

bool TDataType::SetTime(char* a_time)
{
    return DSetTime(m_codec, a_time);
}
