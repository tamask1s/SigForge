#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include <inttypes.h>
#include "CodecType.h"

using namespace std;

#define DllExport __declspec(dllexport)
#define HEADBUFFER 256

DFileType DllFileType = {10, "*.raw", "MDE raw Binary Data Format"};
int DllVersion = 10; // Version 1.0
int BYTESPERSAMPLE = 4;
#define data_type int

struct mde_header
{
    char cardiologic_[12] = "CardioLogic";
    uint32_t flags_ = 1;
    uint32_t nr_samples_ = 0;
    uint32_t sampling_rate_ = 1000;
    uint32_t scaling_factor_ = 104;
    uint32_t compression_id_ = 13155;
};

extern "C"
{
    BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpm_reserved)
    {
        switch( fdwReason )
        {
        case DLL_PROCESS_ATTACH:
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

    DllExport void DGetCodecInfo(char* a_extensions, char* a_description, int* a_id)
    {
        strcpy(a_extensions, DllFileType.Extension);
        strcpy(a_description, DllFileType.Description);
        *a_id = DllFileType.ID;
    }

    DllExport int DInitData()
    {
        pTDataType Data = createData();
        /* File specific informations */
        Data->Version = DllVersion;
        Data->FileType = DllFileType;
        return (int)Data;
    }

    DllExport bool DCloseFile(int aData)
    {
        pTDataType Data = (pTDataType)aData;
        bool Success = false;
        if (Data->FileHandle != INVALID_HANDLE_VALUE)
        {
            Success = CloseHandle(Data->FileHandle);
            deAllocateData(Data);
        }
        Data->FileHandle = INVALID_HANDLE_VALUE;
        return Success;
    }

    DllExport void DDeleteData(int aData)
    {
        DCloseFile(aData);
        pTDataType Data = (pTDataType)aData;
        delete Data;
    }

    /** DOpenFile - Open a file
      *
      *		Data	 - address of the data structure
      *		FileName - file name with full path
      *		Write	 - true: Read/Write access, but no sharing
      *				   false: Read only access, but with sharing
      *
      * Opens a file, and fills up the data structure with the header of the file.
      * If Write is true then the file is opened with R/W access, but no sharing is
      * possible. If Write is false, the file is opened with RO access, read only
      * sharing is possible, but the DSaveHeader function is disabled.
      */
    DllExport bool DOpenFile(int aData, char* FileName, const bool Write = true)
    {
        pTDataType Data = (pTDataType)aData;
        HANDLE hFile;
        int Channels;
        unsigned char Buffer[HEADBUFFER];
        DWORD NrBytes = 0;
        double Rates = 0;

        /* Exit if a file is already open */
        if (Data->FileHandle != INVALID_HANDLE_VALUE)
            return false;
        /* Enable also write access if needed */
        if (Write)
            NrBytes = GENERIC_WRITE;

        hFile = CreateFile(FileName, GENERIC_READ | NrBytes, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
        /* Exit on file open error */
        if (hFile == INVALID_HANDLE_VALUE)
            return false;
        /* Read the Main Header or exit on error */
        if (!ReadFile(hFile, Buffer, HEADBUFFER, &NrBytes, NULL))
            return false;

        mde_header* header = (mde_header*)Buffer;

        unsigned int Size = GetFileSize(hFile, &NrBytes);

        if (header->compression_id_ == 13155)
            BYTESPERSAMPLE = 4;
        else if (header->compression_id_ == 29539)
            BYTESPERSAMPLE = 2;
        else
        {
            BYTESPERSAMPLE = 4;
            cout << "MDE codec: resolution cannot be defined.";
            //goto Error;
        }

        /* Offset of the data in the file */
        Data->Offset = 32;
        if (1)
        {
            Channels = (Size - Data->Offset) / header->nr_samples_ / BYTESPERSAMPLE;
        }
        else
        {
            Channels = 16;
            header->nr_samples_ = (Size - Data->Offset) / (Channels * BYTESPERSAMPLE);
            header->sampling_rate_ = 1000;
            header->scaling_factor_ = 10000;
        }
        cout << "Channels: " << Channels << endl;
        /* Exit if no channels are specified (possible corrupt file) */
        if (!Channels)
            goto Error;

        /* Import the Sample Rate */
        Rates = header->sampling_rate_;
        /* Exit if sequence not found (possible corrupt file) */
        if (Rates == 0.0)
        {
Error:		/* Close the opened file and exit */
            CloseHandle(hFile);
            return false;
        }

        /* Store the file handle */
        Data->FileHandle = hFile;
        /* Store the full path with filename */
        lstrcpy(Data->FilePath, FileName);
        /* Retrieve the filename form the full path */
        Data->FileName = Data->FilePath;
        /* Store the number of channels */
        Data->NrChannels = Channels;
        strcpy(Data->m_horizontal_units, "s");

        /* Allocate memory for the channel specific arrays */
        allocateData(Data);
        /* Store the sample rates and initialize fields */
        for (int i = 0; i < Channels; i++)
        {
            Data->RecordSamples[i] = 0;
            Data->m_sample_rates[i] = Rates;
            FillMemory(Data->m_vertical_units[i], sizeof(UnitsType), 0);
            FillMemory(Data->m_labels[i], sizeof(m_labelsType), 0);
            FillMemory(Data->Prefiltering[i], sizeof(StringType), 0);
            FillMemory(Data->Transducer[i], sizeof(StringType), 0);
            Data->PhysicalMin[i] = -2500;
            Data->PhysicalMax[i] = 2500;
            //Data->DigitalMin[i] = -8388608; /// 8388608 = 2^23, 1073741824 = 2^30, 2^29 = 536870912
            //Data->DigitalMax[i] = 8388608;
            Data->DigitalMin[i] = Data->PhysicalMin[i] * ((double)header->scaling_factor_ * 10.0);
            Data->DigitalMax[i] = Data->PhysicalMax[i] * ((double)header->scaling_factor_ * 10.0);
            Data->Ratio[i] = (Data->PhysicalMax[i] - Data->PhysicalMin[i]) / (Data->DigitalMax[i] - Data->DigitalMin[i]);
        }
        int i = 0;
        while (i < Channels)
            strcpy(Data->m_vertical_units[i++], "uV");
        /* Determine the samples per channels */
        Size = GetFileSize(hFile, &NrBytes);
        Size = (Size - Data->Offset) / (Channels * BYTESPERSAMPLE);
        for (int i = 0; i < Channels; Data->m_total_samples[i++] = Size)
            ;
        /* Calculate the total duration in seconds */
        Data->TotalDuration = Size / Rates;
        return true;
    }

    DllExport bool DWriteHeader(int aData)
    {
        pTDataType Data = (pTDataType)aData;
        mde_header header;
        header.nr_samples_ = Data->m_total_samples[0];
        header.sampling_rate_ = Data->m_sample_rates[0];
        for (int i = 0; i < Data->NrChannels; i++)
        {
            Data->DigitalMin[i] = Data->PhysicalMin[i] * ((double)header.scaling_factor_ * 10.0);
            Data->DigitalMax[i] = Data->PhysicalMax[i] * ((double)header.scaling_factor_ * 10.0);
            Data->Ratio[i] = (Data->PhysicalMax[i] - Data->PhysicalMin[i]) / (Data->DigitalMax[i] - Data->DigitalMin[i]);
        }
        if (SetFilePointer(Data->FileHandle, 0, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        DWORD NrBytes;
        if (!WriteFile(Data->FileHandle, &header, sizeof(mde_header), &NrBytes, NULL))
            return false;
        if (sizeof(mde_header) != (int)NrBytes)
            return false;

        Data->Offset = sizeof(mde_header);

        return true;
    }

    /** DCreateNewFile - Create a file
      *
      *		Data	 - address of the data structure
      *		FileName - file name with full path
      *		Channels = number of channels
      *		Rewrite	 - true: rewrites existing file
      *				   false: check for existing file
      *
      * Creates a new file. If Rewrite is true then the file will be rewritten.
      * If Rewrite is false, the file will be checked, and function will fail
      * if the file exists.  If the number of channels is above zero, then the
      * necessary memory alocations will be performed.
      */
    DllExport bool DCreateNewFile(int aData, const char* FileName, int Channels, const bool Rewrite = false)
    {
        return CreateNewFile_i(aData, FileName, Channels, Rewrite, 0, 0);
    }

    /** DGetDataRaw - Reads the raw data
      *
      *		Data   - address of the data structure
      *		Buffer - address of the output buffer
      *		Start  - start sample index vector
      *		Stop   - stop sample index vector
      *
      * Fills the buffer with the raw data from start to stop indexes for each
      * channel.
      */
    DllExport bool DGetDataRaw(int aData, double** Buffer, unsigned int* Start, unsigned int* Stop)
    {
        pTDataType Data = (pTDataType)aData;
        /* Exit if no file is open */
        if (Data->FileHandle == INVALID_HANDLE_VALUE)
            return false;
        /* Test for a valid buffer and valid sequence */
        if ((Buffer == NULL) || (Start[0] >= Stop[0]))
            return false;

        int Channels = Data->NrChannels;
        /* Calculate the size of the necessary read buffer */
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * (Channels * BYTESPERSAMPLE);
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            Data->FileBuffer = (double*) new double[FILEBUFFER];
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * (Channels * BYTESPERSAMPLE) + Data->Offset;
        DWORD NrBytes;

        /* Seek the file pointer to the reading offset */
        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        /* Read the data */
        if (!ReadFile(Data->FileHandle, Data->FileBuffer, FILEBUFFER, &NrBytes, NULL))
            return false;

        /* Transfer data from the read buffer to the buffer */
        int nrsamples = NrBytes / (Channels * BYTESPERSAMPLE);
        unsigned char* data = (unsigned char*)Data->FileBuffer;
        for (int i = 0; i < nrsamples; ++i)
        {
            for (int j = 0; j < Channels; ++j)
            {
                int tmp = 0;
                memcpy(&tmp, data, BYTESPERSAMPLE);
                Buffer[j][i] = Data->Ratio[j] * tmp; ///Ratio[j] * (tmp - Data->DigitalMin[j]) + PhysicalMin[j];
                data += BYTESPERSAMPLE;
            }
        }

        return true;
    }

    /** DGetChannelRaw - Reads the raw data by channels
      *
      *		Data   - address of the data structure
      *		Buffer - address of the output buffer
      *		Enable - channel enable index vector - only channels marked with a value of "1" will be present in the destination buffer
      *		Start  - start sample index vector
      *		Stop   - stop sample index vector
      *
      * Fills the buffer with the raw data from start to stop indexes for the
      * specified channels. Enable is an array with boolean values to specify
      * the active channels.
      */
    DllExport bool DGetChannelRaw(int aData, double** Buffer, int* Enable, unsigned int* Start, unsigned int* Stop)
    {
        pTDataType Data = (pTDataType)aData;
        /* Exit if no file is open */
        if (Data->FileHandle == INVALID_HANDLE_VALUE)
            return false;
        /* Test for a valid buffer and valid sequence */
        if ((Buffer == NULL) || (Start[0] == Stop[0]))
            return false;

        int i, N = 0;
        int Channels = Data->NrChannels;
        /* Calculate the number of enabled channels */
        for (i = 0; i < Channels; N += Enable[i++] & 1)
            ;
        /* Return if no channels found */
        if (!N)
            return false;
        /* Optimize, if all channels are enabled */
        if (N == Channels)
            return DGetDataRaw(aData, Buffer, Start, Stop);
        /* Calculate the size of the necessary read buffer */
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * (Channels * BYTESPERSAMPLE);
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            Data->FileBuffer = (double*) new double[FILEBUFFER];
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * (Channels * BYTESPERSAMPLE) + Data->Offset;
        DWORD NrBytes;

        /* Seek the file pointer to the reading offset */
        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        /* Read the data */
        if (!ReadFile(Data->FileHandle, Data->FileBuffer, FILEBUFFER, &NrBytes, NULL))
            return false;
        /* Calculate the number of requested samples per channel */
        int nrsamples = NrBytes / (Channels * BYTESPERSAMPLE);
        unsigned char* data = (unsigned char*)Data->FileBuffer;
        for (int i = 0; i < nrsamples; ++i)
        {
            int chindx = 0;
            for (int j = 0; j < Channels; ++j)
            {
                if (Enable[j])
                {
                    int tmp = 0;
                    memcpy(&tmp, data, BYTESPERSAMPLE);
                    Buffer[chindx++][i] = Data->Ratio[j] * tmp; ///Ratio[j] * (tmp - Data->DigitalMin[j]) + PhysicalMin[j];
                }
                data += BYTESPERSAMPLE;
            }
        }
        return true;
    }

    DllExport bool DWriteBlock(int aData, double** Buffer, unsigned int* Start, unsigned int* Stop)
    {
        pTDataType Data = (pTDataType)aData;
        /* Exit if no file is open */
        if (Data->FileHandle == INVALID_HANDLE_VALUE)
            return false;
        if ((Buffer == NULL) || (Start[0] == Stop[0]))
            return false;

        int Channels = Data->NrChannels;
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * Channels * BYTESPERSAMPLE;
        data_type* FileBuffer = (data_type*)Data->FileBuffer;

        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new data_type[FILEBUFFER];
            Data->FileBuffer = (double*)FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }
        unsigned int N = FILEBUFFER / BYTESPERSAMPLE;
        for (unsigned int i = 0; i < N; i++)
            FileBuffer[i] =  std::ceil(Buffer[i % Channels][i / Channels] / Data->Ratio[i % Channels]);

        unsigned int StartOffset = Start[0] * Channels * BYTESPERSAMPLE + Data->Offset;
        DWORD NrBytes;
        if (GetFileSize(Data->FileHandle, &NrBytes) < StartOffset)
            return false;
        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        if (!WriteFile(Data->FileHandle, FileBuffer, FILEBUFFER, &NrBytes, NULL))
            return false;
        if (FILEBUFFER != NrBytes)
            return false;

        Data->NrRecords++;
        for (int i = 0; i < Channels; ++i)
            Data->m_total_samples[i] = Data->RecordSamples[i] * Data->NrRecords;

        return true;
    }
}
