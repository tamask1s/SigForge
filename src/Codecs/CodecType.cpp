#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CodecType.h"

#define DllExport __declspec(dllexport)

extern "C"
{
    DllExport int DGetNrChannels(int aData)
    {
        pTDataType Data = (pTDataType)aData;
        return Data->NrChannels;
    }
    DllExport bool DGetTotalSamples(int aData, unsigned int* a_total_samples)
    {
        pTDataType Data = (pTDataType)aData;
        memcpy(a_total_samples, Data->m_total_samples, Data->NrChannels * sizeof(unsigned int));
        return true;
    }
    DllExport bool DGetSampleRates(int aData, double* a_sample_rates)
    {
        pTDataType Data = (pTDataType)aData;
        memcpy(a_sample_rates, Data->m_sample_rates, Data->NrChannels * sizeof(double));
        return true;
    }
    DllExport bool DGetLabel(int aData, int a_channel_index, char* aLabel)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(aLabel, Data->m_labels[a_channel_index]);
        return true;
    }
    DllExport bool DGetTransducer(int aData, int a_channel_index, char* aTransducer)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(aTransducer, Data->Transducer[a_channel_index]);
        return true;
    }
    DllExport bool DGetVerticalUnit(int aData, int a_channel_index, char* a_vertical_units)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(a_vertical_units, Data->m_vertical_units[a_channel_index]);
        return true;
    }
    DllExport bool DGetHorizontalUnit(int aData, char* a_horizontal_units)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(a_horizontal_units, Data->m_horizontal_units);
        return true;
    }
    DllExport bool DGetRecording(int aData, char* a_recording)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(a_recording, Data->m_recording);
        return true;
    }
    DllExport bool DGetPatient(int aData, char* a_patient)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(a_patient, Data->Patient);
        return true;
    }
    DllExport bool DGetDate(int aData, char* a_date)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(a_date, Data->Date);
        return true;
    }
    DllExport bool DGetTime(int aData, char* a_time)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(a_time, Data->Time);
        return true;
    }
    DllExport bool DGetFileName(int aData, char* a_file_name)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(a_file_name, Data->FileName);
        return true;
    }
    DllExport bool DSetSampleRates(int aData, double* a_sample_rates)
    {
        pTDataType Data = (pTDataType)aData;
        memcpy(Data->m_sample_rates, a_sample_rates, Data->NrChannels * sizeof(double));
        return true;
    }
    DllExport bool DSetPhysicalMin(int aData, double* a_physical_min)
    {
        pTDataType Data = (pTDataType)aData;
        memcpy(Data->PhysicalMin, a_physical_min, Data->NrChannels * sizeof(double));
        return true;
    }
    DllExport bool DSetPhysicalMax(int aData, double* a_physical_max)
    {
        pTDataType Data = (pTDataType)aData;
        memcpy(Data->PhysicalMax, a_physical_max, Data->NrChannels * sizeof(double));
        return true;
    }
    DllExport bool DSetRecordSamples(int aData, unsigned int* a_record_samples)
    {
        pTDataType Data = (pTDataType)aData;
        memcpy(Data->RecordSamples, a_record_samples, Data->NrChannels * sizeof(unsigned int));
        return true;
    }
    DllExport bool DSetRecordDuration(int aData, double aRecordDuration)
    {
        pTDataType Data = (pTDataType)aData;
        Data->RecordDuration = aRecordDuration;
        return true;
    }
    DllExport bool DSetTotalSamples(int aData, unsigned int* a_total_samples)
    {
        pTDataType Data = (pTDataType)aData;
        memcpy(Data->m_total_samples, a_total_samples, Data->NrChannels * sizeof(unsigned int));
        return true;
    }

    DllExport bool DSetLabel(int aData, int a_channel_index, char* aLabel)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->m_labels[a_channel_index], aLabel);
        return true;
    }
    DllExport bool DSetTransducer(int aData, int a_channel_index, char* aTransducer)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->Transducer[a_channel_index], aTransducer);
        return true;
    }
    DllExport bool DSetVerticalUnit(int aData, int a_channel_index, char* aVerticalUnit)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->m_vertical_units[a_channel_index], aVerticalUnit);
        return true;
    }
    DllExport bool DSetHorizontalUnit(int aData, char* aHorizontalUnit)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->m_horizontal_units, aHorizontalUnit);
        return true;
    }
    DllExport bool DSetRecording(int aData, char* a_recording)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->m_recording, a_recording);
        return true;
    }
    DllExport bool DSetPatient(int aData, char* a_patient)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->Patient, a_patient);
        return true;
    }
    DllExport bool DSetDate(int aData, char* a_date)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->Date, a_date);
        return true;
    }
    DllExport bool DSetTime(int aData, char* a_time)
    {
        pTDataType Data = (pTDataType)aData;
        strcpy(Data->Time, a_time);
        return true;
    }
    bool DGetChannelRaw(int a_handle, double** a_buffer, int* aEnable, unsigned int* a_start, unsigned int* a_stop);
    DllExport bool DGetSingleChannelRaw(int aData, double* Buffer, int Enable, unsigned int Start, unsigned int Stop)
    {
        pTDataType Data = (pTDataType)aData;
        for (int i = 0; i < Data->NrChannels; ++i)
        {
            Data->enableWrk[i] = 0;
            Data->startWrk[i] = Start;
            Data->stopWrk[i] = Stop;
        }
        Data->enableWrk[Enable] = 1;
        double* wrkData[1];
        wrkData[0] = Buffer;
        return DGetChannelRaw(aData, wrkData, Data->enableWrk, Data->startWrk, Data->stopWrk);
    }
    bool DWriteBlock(int aData, double** Buffer, unsigned int* Start, unsigned int* Stop);
    DllExport bool DAppendSamples(int aData, double** Buffer, unsigned int NrSamples)
    {
        pTDataType Data = (pTDataType)aData;
        unsigned int Start[1];
        unsigned int Stop[1];
        Start[0] = Data->m_total_samples[0];
        Stop[0] = Data->m_total_samples[0] + NrSamples;
        return DWriteBlock(aData, Buffer, Start, Stop);
    }
}

pTDataType createData()
{
    pTDataType Data = new TDataTypeX;
    /* File specific informations */
    FillMemory(Data->FilePath, MAX_PATH, 0);
    Data->FileName = NULL;
    Data->FileHandle = INVALID_HANDLE_VALUE;
    /* Patient specific informations */
    FillMemory(Data->Patient, sizeof(StringType), 0);
    FillMemory(Data->m_recording, sizeof(StringType), 0);
    strcpy(Data->Date, "02.07.21");
    strcpy(Data->Time, "00.45.00");
    Data->NrRecords = 0;
    Data->TotalDuration = 0.0;
    Data->RecordDuration = 0.0;
    Data->NrChannels = 0;
    FillMemory(Data->Comment, sizeof(StringType), 0);
    /* Channel specific information */
    Data->m_total_samples = NULL;
    Data->RecordSamples = NULL;
    Data->m_sample_rates = NULL;
    FillMemory(Data->m_horizontal_units, sizeof(UnitsType), 0);
    Data->m_vertical_units = NULL;
    Data->m_labels = NULL;
    Data->Prefiltering = NULL;
    Data->Transducer = NULL;
    Data->PhysicalMin = NULL;
    Data->PhysicalMax = NULL;
    Data->DigitalMin = NULL;
    Data->DigitalMax = NULL;
    /* Internally used variables */
    Data->RecordSize = 0;
    Data->Offset = 0;
    Data->FileBuffer = NULL;
    Data->FILEBUFFER = 0;
    Data->JumpTable = NULL;
    Data->Ratio = NULL;

    return Data;
}

void allocateData(pTDataType Data)
{
    unsigned int Channels = Data->NrChannels;
    Data->m_total_samples = new unsigned int[Channels];
    Data->RecordSamples = new unsigned int[Channels];
    Data->m_sample_rates = new double[Channels];
    Data->m_vertical_units = new UnitsType[Channels];
    Data->m_labels = new m_labelsType[Channels];
    memset(Data->m_labels, 0, sizeof(m_labelsType) * Channels);
    Data->Prefiltering = new StringType[Channels];
    Data->Transducer = new StringType[Channels];
    memset(Data->Transducer, 0, sizeof(StringType) * Channels);
    Data->PhysicalMin = new double[Channels];
    Data->PhysicalMax = new double[Channels];
    Data->DigitalMin = new int[Channels];
    Data->DigitalMax = new int[Channels];
    Data->JumpTable = new unsigned int[Channels];
    Data->Ratio = new double[Channels];
    Data->enableWrk = new int[Channels];
    Data->startWrk = new unsigned int[Channels];
    Data->stopWrk = new unsigned int[Channels];
}

void deAllocateData(pTDataType Data)
{
    delete[] Data->m_total_samples;
    delete[] Data->RecordSamples;
    delete[] Data->m_sample_rates;
    delete[] Data->m_vertical_units;
    delete[] Data->m_labels;
    delete[] Data->Prefiltering;
    delete[] Data->Transducer;
    delete[] Data->PhysicalMin;
    delete[] Data->PhysicalMax;
    delete[] Data->DigitalMin;
    delete[] Data->DigitalMax;
    delete[] Data->FileBuffer;
    delete[] Data->JumpTable;
    delete[] Data->Ratio;
    delete[] Data->enableWrk;
    delete[] Data->startWrk;
    delete[] Data->stopWrk;

    Data->m_total_samples = 0;
    Data->RecordSamples = 0;
    Data->m_sample_rates = 0;
    Data->m_vertical_units = 0;
    Data->m_labels = 0;
    Data->Prefiltering = 0;
    Data->Transducer = 0;
    Data->PhysicalMin = 0;
    Data->PhysicalMax = 0;
    Data->DigitalMin = 0;
    Data->DigitalMax = 0;
    Data->FileBuffer = 0;
    Data->JumpTable = 0;
    Data->Ratio = 0;
    Data->enableWrk = 0;
    Data->startWrk = 0;
    Data->stopWrk = 0;
}

bool CreateNewFile_i(int aData, const char* FileName, int Channels, const bool Rewrite, int DigitalMin, int DigitalMax)
{
    pTDataType Data = (pTDataType)aData;
    HANDLE hFile;
    char* ptr;
    DWORD Creation = CREATE_NEW;

    /* Exit if a file is already open */
    if (Data->FileHandle != INVALID_HANDLE_VALUE)
        return false;
    /* Enable rewrite of the file if needed */
    if (Rewrite)
        Creation = CREATE_ALWAYS;
    hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, Creation, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    /* Exit on file creation error */
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    /* Store the file handle */
    Data->FileHandle = hFile;
    /* Store the full path with filename */
    lstrcpy(Data->FilePath, FileName);
    /* Retrieve the filename form the full path */
    ptr = strrchr(Data->FilePath, '\\');
    if (ptr == NULL)
        Data->FileName = Data->FilePath;
    else
        Data->FileName = ptr + 1;
    if (Channels)
    {
        Data->NrChannels = Channels;
        /* Allocate memory for the channel specific arrays */
        allocateData(Data);
        /* Initialize fields */
        for (int i = 0; i < Channels; i++)
        {
            Data->m_total_samples[i] = 0;
            Data->RecordSamples[i] = 0;
            Data->m_sample_rates[i] = 0.0;
            FillMemory(Data->m_vertical_units[i], sizeof(UnitsType), 0);
            FillMemory(Data->m_labels[i], sizeof(m_labelsType), 0);
            FillMemory(Data->Prefiltering[i], sizeof(StringType), 0);
            FillMemory(Data->Transducer[i], sizeof(StringType), 0);
            Data->PhysicalMin[i] = -2500.0;
            Data->PhysicalMax[i] = 2500.0;
            Data->DigitalMin[i] = DigitalMin;
            Data->DigitalMax[i] = DigitalMax;
            Data->JumpTable[i] = 0;
            Data->Ratio[i] = 0.0;
        }
    }
    return true;
}
