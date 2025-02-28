#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CodecType.h"

#define DllExport __declspec(dllexport)
#define HEADBUFFER 409600

/* Filetype information */
DFileType DllFileType = {7, "*.vhdr", "BrainVision Data Format"};
int DllVersion = 10; // Version 1.0

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
        char Buffer[HEADBUFFER];
        DWORD NrBytes = 0;
        char* ptr;
        char* eeg_file;
        char* eeg_extension;

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
        /* Ensure the end of string character */
        Buffer[NrBytes] = 0;
        /* Check for valid Data format*/
        ptr = strstr(Buffer, "IEEE_FLOAT_32");
        if (!ptr)
        {
            printf("BrainVision format error: IEEE_FLOAT_32 format is supported only\n");
            goto Error;
        }
        /* Check for Data file*/
        ptr = strstr(Buffer, "DataFile=");
        /* Exit if sequence not found (possible corrupt file) */
        if (!ptr)
            goto Error;
        ptr += 9;
        eeg_file = ptr;
        eeg_extension = strstr(Buffer, ".eeg");
        if (!eeg_extension)
            goto Error;
        ptr = eeg_extension + 5;
        eeg_extension[4] = 0;
        /* Import the number of Channels */
        ptr = strstr(ptr, "Number of channels:");
        /* Exit if sequence not found (possible corrupt file) */
        if (!ptr)
            goto Error;
        ptr += 19;
        Channels = atoi(ptr);
        /* Exit if no channels are specified (possible corrupt file) */
        if (!Channels)
            goto Error;
        /* Offset of the data in the file */
        Data->Offset = 0;
        /* Import the Sample Rate */
        ptr = strstr(ptr, "Sampling Rate [Hz]:");
        /* Exit if sequence not found (possible corrupt file) */
        if (!ptr)
        {
Error:		/* Close the opened file and exit */
            CloseHandle(hFile);
            return false;
        }
        ptr += 19;
        double Rates = atof(ptr);

        CloseHandle(hFile);
        ptr = strrchr(FileName, '\\');
        if (ptr)
        {
            ptr[1] = 0;
            strcat(FileName, eeg_file);
        }
        else
            FileName = eeg_file;
        if (Write)
            NrBytes = GENERIC_WRITE;
        else
            NrBytes = 0;
        hFile = CreateFile(FileName, GENERIC_READ | NrBytes, FILE_SHARE_READ, \
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | \
                           FILE_FLAG_RANDOM_ACCESS, NULL);
        /* Assuming the BrainVision file is a valid and correct file */
        /* Exit on file open error */
        if (hFile == INVALID_HANDLE_VALUE)
        {
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
        /* TODO: Import Labels */
        /* TODO: Import the Vertical Units */
        if (true)
        {
            int i = Channels;
            while (i < Channels)
                strcpy(Data->m_vertical_units[i++], "µV");
        }
        /* Determine the samples per channels */
        unsigned int Size = GetFileSize(hFile, &NrBytes);
        Size = (Size - Data->Offset) / Channels;
        Size /= sizeof(float);
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
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * Channels * sizeof(float);
        float* FileBuffer = (float*) Data->FileBuffer;
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new float[FILEBUFFER];
            Data->FileBuffer = (double*) FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * Channels * sizeof(float) + Data->Offset;
        DWORD NrBytes;

        /* Seek the file pointer to the reading offset */
        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        /* Read the data */
        if (!ReadFile(Data->FileHandle, FileBuffer, FILEBUFFER, &NrBytes, NULL))
            return false;
        FILEBUFFER /= sizeof(float);
        /* Transfer the floats from the read buffer to the buffer */
        for (unsigned int i = 0; i < FILEBUFFER; i++)
            Buffer[i % Channels][i / Channels] = FileBuffer[i];
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
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * Channels * sizeof(float);
        float* FileBuffer = (float*)Data->FileBuffer;
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new float[FILEBUFFER];
            Data->FileBuffer = (double*)FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * Channels * sizeof(float) + Data->Offset;
        DWORD NrBytes;

        /* Seek the file pointer to the reading offset */
        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        /* Read the data */
        if (!ReadFile(Data->FileHandle, FileBuffer, FILEBUFFER, &NrBytes, NULL))
            return false;
        /* Calculate the number of requested samples per channel */
        FILEBUFFER = Stop[0] - Start[0];
        unsigned int j = N - 1;
        unsigned int k = 1;
        unsigned int* Jump = Data->JumpTable;
        /* Reset the jump table values */
        for (i = 0; i < Channels; Jump[i++] = 0)
            ;
        /* Build the jump table for skipping the unnecessary values */
        for (i = 0; i < Channels; i++)
            if (!Enable[i])
                k++;
            else
            {
                Jump[j++] = k;
                j %= N;
                k = 1;
            }
        Jump[N - 1] += k - 1;
        /* Calculate the starting index value of the read buffer */
        k = 0;
        while (!Enable[k++])
            ;
        k--;
        /* Transfer the floats from the read buffer to the buffer */
        for (j = 0; j < FILEBUFFER; j++)
            for (i = 0; i < N; i++)
            {
                Buffer[i][j] = FileBuffer[k];
                k += Jump[i];
            }
        return true;
    }

    DllExport bool DWriteHeader(int aData)
    {
        // TODO
        return true;
    }

    DllExport bool DWriteBlock(int aData, double** Buffer, unsigned int* Start, unsigned int* Stop)
    {
        //  TODO
        return true;
    }
}
