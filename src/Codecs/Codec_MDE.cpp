#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CodecType.h"

#define DllExport __declspec(dllexport)
#define HEADBUFFER 256

/* Filetype information */
DFileType DllFileType = {9, "*.mdebin", "MDE Binary Data Format"};
int DllVersion = 10; // Version 1.0
static int const BYTESPERSAMPLE = 3;

extern "C"
{
    BOOL WINAPI DllMain(
        HINSTANCE hinstDLL,  // handle to DLL module
        DWORD fdwReason,     // reason for calling function
        LPVOID lpm_reserved )  // reserved
    {
        // Perform actions based on the reason for calling.
        switch( fdwReason )
        {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
            // Perform any necessary cleanup.
            break;
        }
        return TRUE;  // Successful DLL_PROCESS_ATTACH.
    }

    /** DGetFileType - Get the supported file type
      *
      *		FileType - address of the return structure
      *
      * Returns a structure with the file type information of the supported file
      * type.
      */
    DllExport void DGetCodecInfo(char* a_extensions, char* a_description, int* a_id)
    {
        strcpy(a_extensions, DllFileType.Extension);
        strcpy(a_description, DllFileType.Description);
        *a_id = DllFileType.ID;
    }

    /** DInitData - Initialize the data structure
      *
      *		Data - address of the data structure
      *
      * Fills the data structure with the codec provided default values.
      */
    DllExport int DInitData()
    {
        pTDataType Data = createData();
        /* File specific informations */
        Data->Version = DllVersion;
        Data->FileType = DllFileType;
        return (int)Data;
    }

    /** DCloseFile - Close the previously opened file
      *
      *		Data - address of the data structure
      *
      * Closes the previously opened file and performs the aditional memory
      * cleanups.
      */
    DllExport bool DCloseFile(int aData)
    {
        pTDataType Data = (pTDataType)aData;
        bool Success = false;
        /* Exit if no file is open */
        if (Data->FileHandle != INVALID_HANDLE_VALUE)
        {
            /* Close the file */
            Success = CloseHandle(Data->FileHandle);
            /* Memory cleanup */
            deAllocateData(Data);
            /* Set the data fields to their default values */
        }
        Data->FileHandle = INVALID_HANDLE_VALUE;
        return Success;
    }

    /** DDeleteData - Deinitializes the data structure
      *
      *		aData - address of the data structure
      *
      * Deletes the data codec structure. It also closes the file if it is open.
      */
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
        char* ptr;
        double Rates = 0;

        /* Exit if a file is already open */
        if (Data->FileHandle != INVALID_HANDLE_VALUE)
            return false;
        /* Enable also write access if needed */
        if (Write)
            NrBytes = GENERIC_WRITE;

        hFile = CreateFile(FileName, GENERIC_READ | NrBytes, FILE_SHARE_READ, \
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | \
                           FILE_FLAG_RANDOM_ACCESS, NULL);
        /* Exit on file open error */
        if (hFile == INVALID_HANDLE_VALUE)
            return false;
        /* Read the Main Header or exit on error */
        if (!ReadFile(hFile, Buffer, HEADBUFFER, &NrBytes, NULL))
            return false;

        Channels = 16;
        /* Exit if no channels are specified (possible corrupt file) */
        if (!Channels)
            goto Error;
        /* Offset of the data in the file */

        Data->Offset = 0;
        for (unsigned int i = 0; i < NrBytes - 1; ++i)
            if (Buffer[i] == 0x44)
                if (Buffer[i + 1] == 0x54)
                {
                    Data->Offset = i;
                    break;
                }

        /* Import the Sample Rate */
        Rates = 1000.0;
//        /* Exit if sequence not found (possible corrupt file) */
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
        ptr = strrchr(Data->FilePath, '\\');
        if (ptr == NULL)
            Data->FileName = Data->FilePath;
        else
            Data->FileName = ptr + 1;
        /* Store the number of channels */
        Data->NrChannels = Channels;
        //        /* TODO: Import Labels */
        //        ptr = strstr(Buffer, "COMMENT");
        //        if (ptr)
        //        {
        //            ptr += 7;
        //            int i = strcspn (ptr, "\r\n");
        //            strtrn(Data->Comment, ptr, i);
        //        }
        /* Set the horizontal unit */
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
            Data->PhysicalMin[i] = 0.0;
            Data->PhysicalMax[i] = 0.0;
            Data->DigitalMin[i] = 0;
            Data->DigitalMax[i] = 0;
            Data->Ratio[i] = 0.0;
        }
        int i = 0;
        while (i < Channels)
            strcpy(Data->m_vertical_units[i++], "uV");
        /* Determine the samples per channels */
        unsigned int Size = GetFileSize(hFile, &NrBytes);
        Size = (Size - Data->Offset) / (4 + Channels * BYTESPERSAMPLE);
        for (int i = 0; i < Channels; Data->m_total_samples[i++] = Size)
            ;
        /* Calculate the total duration in seconds */
        Data->TotalDuration = Size / Rates;
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
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * (4 + Channels * BYTESPERSAMPLE);
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            Data->FileBuffer = (double*) new double[FILEBUFFER];
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * (4 + Channels * BYTESPERSAMPLE) + Data->Offset;
        DWORD NrBytes;

        /* Seek the file pointer to the reading offset */
        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        /* Read the data */
        if (!ReadFile(Data->FileHandle, Data->FileBuffer, FILEBUFFER, &NrBytes, NULL))
            return false;

        /* Transfer data from the read buffer to the buffer */
        int nrsamples = NrBytes / (4 + Channels * BYTESPERSAMPLE);
        unsigned char* data = (unsigned char*)Data->FileBuffer;
        for (int i = 0; i < nrsamples; ++i)
        {
            data += 3;
            for (int j = 0; j < Channels; ++j)
            {
                int tmp = 0;
                memcpy(&tmp, data, BYTESPERSAMPLE);
                if (tmp & 0x00800000)
                    tmp |= 0xFF000000;
                Buffer[j][i] = tmp; ///Ratio[j] * (tmp - Data->DigitalMin[j]) + PhysicalMin[j];
                data += BYTESPERSAMPLE;
            }
            data += 1;
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
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * (4 + Channels * BYTESPERSAMPLE);
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            Data->FileBuffer = (double*) new double[FILEBUFFER];
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * (4 + Channels * BYTESPERSAMPLE) + Data->Offset;
        DWORD NrBytes;

        /* Seek the file pointer to the reading offset */
        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        /* Read the data */
        if (!ReadFile(Data->FileHandle, Data->FileBuffer, FILEBUFFER, &NrBytes, NULL))
            return false;
        /* Calculate the number of requested samples per channel */
        int nrsamples = NrBytes / (4 + Channels * BYTESPERSAMPLE);
        unsigned char* data = (unsigned char*)Data->FileBuffer;
        for (int i = 0; i < nrsamples; ++i)
        {
            data += 3;
            int chindx = 0;
            for (int j = 0; j < Channels; ++j)
            {
                if (Enable[j])
                {
                    int tmp = 0;
                    memcpy(&tmp, data, BYTESPERSAMPLE);
                    if (tmp & 0x00800000)
                        tmp |= 0xFF000000;
                    Buffer[chindx++][i] = tmp; ///Ratio[j] * (tmp - Data->DigitalMin[j]) + PhysicalMin[j];
                }
                data += BYTESPERSAMPLE;
            }
            data += 1;
        }
        return true;
    }

    DllExport bool DWriteHeader(int aData)
    {
//        pTDataType Data = (pTDataType)aData;
//        char Buffer[HEADBUFFER];
//        memset(Buffer, 0, HEADBUFFER);
//        long long int Channels = Data->NrChannels;
//        double nr_cols = Data->m_total_samples[0];
//        int Length = 24;
//        double sampling_rate = Data->m_sample_rates[0];
//        memcpy(Buffer, &sampling_rate, sizeof(double));
//        memcpy(Buffer + sizeof(double), &Channels, sizeof(long long int));
//        memcpy(Buffer + sizeof(double) + sizeof(long long int), &nr_cols, sizeof(double));
//
//        if (SetFilePointer(Data->FileHandle, 0, 0, FILE_BEGIN) == 0xffffffff)
//            return false;
//        DWORD NrBytes;
//        if (!WriteFile(Data->FileHandle, Buffer, Length, &NrBytes, NULL))
//            return false;
//        if (Length != (int)NrBytes)
//            return false;
//
//        Data->Offset = Length;

        return true;
    }

    DllExport bool DWriteBlock(int aData, double** Buffer, unsigned int* Start, unsigned int* Stop)
    {
//        pTDataType Data = (pTDataType)aData;
//        /* Exit if no file is open */
//        if (Data->FileHandle == INVALID_HANDLE_VALUE)
//            return false;
//        if ((Buffer == NULL) || (Start[0] == Stop[0]))
//            return false;
//
//        int Channels = Data->NrChannels;
//        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * Channels * sizeof(double);
//        double* FileBuffer = (double*)Data->FileBuffer;
//
//        if (FILEBUFFER != Data->FILEBUFFER)
//        {
//            delete[] Data->FileBuffer;
//            FileBuffer = new double[FILEBUFFER];
//            Data->FileBuffer = (double*)FileBuffer;
//            Data->FILEBUFFER = FILEBUFFER;
//        }
//        unsigned int N = FILEBUFFER / sizeof(double);
//        for (unsigned int i = 0; i < N; i++)
//            FileBuffer[i] = Buffer[i % Channels][i / Channels];
//
//        unsigned int StartOffset = Start[0] * Channels * sizeof(double) + Data->Offset;
//        DWORD NrBytes;
//        if (GetFileSize(Data->FileHandle, &NrBytes) < StartOffset)
//            return false;
//        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
//            return false;
//        if (!WriteFile(Data->FileHandle, FileBuffer, FILEBUFFER, &NrBytes, NULL))
//            return false;
//        if (FILEBUFFER != NrBytes)
//            return false;
//
//        Data->NrRecords++;
//        for (int i = 0; i < Channels; ++i)
//            Data->m_total_samples[i] = Data->RecordSamples[i] * Data->NrRecords;

        return true;
    }
}
