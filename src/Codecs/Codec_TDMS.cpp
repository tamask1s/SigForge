#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "CodecType.h"

#define DllExport __declspec(dllexport)
#define HEADBUFFER 4096

/* Filetype information */
DFileType DllFileType = {5, "*.tdm;*.tdms;*.TDM;*.TDMS", "TDMS Data Format"};
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
        deAllocateData(Data);
        return true;
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
        Data->NrChannels = 5;
        /* Allocate memory for the channel specific arrays */
        allocateData(Data);
        /* Store the sample rates and initialize fields */
        for (int i = 0; i < Data->NrChannels; i++)
        {
            Data->RecordSamples[i] = 0;
            Data->m_sample_rates[i] = 1000.0;
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
        /* Determine the samples per channels */
        for (int i = 0; i < Data->NrChannels; Data->m_total_samples[i++] = 100000)
            ;
        /* Calculate the total duration in seconds */
        Data->TotalDuration = Data->m_total_samples[0] / Data->m_sample_rates[0];
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
//        pTDataType Data = (pTDataType)aData;
        return true;
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
        /* Test for a valid buffer and valid sequence */
        if ((Buffer == NULL) || (Start[0] >= Stop[0]))
            return false;

        unsigned int Channels = Data->NrChannels;
        /* Calculate the size of the necessary read buffer */
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * Channels * sizeof(double);
        double* FileBuffer = Data->FileBuffer;
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new double[FILEBUFFER];
            Data->FileBuffer = FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * Channels * sizeof(double) + Data->Offset;
//        DWORD NrBytes;

        FILEBUFFER /= sizeof(double);
        /* Transfer the doubles from the read buffer to the buffer */
        for (unsigned int i = 0; i < FILEBUFFER; i++)
            Buffer[i % Channels][i / Channels] = sin((double)i / 1000.0 * 2.0 * 3.14 * (double)(StartOffset / 1000000.0 + 1)) * 100.0;
        return true;
    }

    /** DGetChannelRaw - Reads the raw data by channels
      *
      *		Data   - address of the data structure
      *		Buffer - address of the output buffer
      *		Enable - channel enable index vector
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
        unsigned int Channels = Data->NrChannels;
        /* Calculate the number of enabled channels */
        unsigned int i, N = 0;
        for (i = 0; i < Channels; N += Enable[i++] & 1)
            ;
        /* Return if no channels found */
        if (!N)
            return false;
        /* Optimize, if all channels are enabled */
        if (N == Channels)
            return DGetDataRaw(aData, Buffer, Start, Stop);
        /* Calculate the size of the necessary read buffer */
        unsigned int FILEBUFFER = (Stop[0] - Start[0]) * Channels * sizeof(double);
        double* FileBuffer = (double*)Data->FileBuffer;
        /* Reallocate the buffer if the existing size differs from the new size */
        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new double[FILEBUFFER];
            Data->FileBuffer = FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }
        /* Calculate the start offset of reading from the file */
        unsigned int StartOffset = Start[0] * Channels * sizeof(double) + Data->Offset;
        /* Calculate the number of requested samples per channel */
        FILEBUFFER = Stop[0] - Start[0];
        /* Transfer the doubles from the read buffer to the buffer */
        FILEBUFFER *= N;
        for (i = 0; i < FILEBUFFER; i++)
            Buffer[i % N][i / N] = sin((double)i / 1000.0 * 2.0 * 3.14 * (double)(StartOffset / 1000000.0 + 1)) * 100.0;
        return true;
    }

    DllExport bool DWriteHeader(int aData)
    {
//        pTDataType Data = (pTDataType)aData;
        return true;
    }

    DllExport bool DWriteBlock(int aData, double** Buffer, unsigned int* Start, unsigned int* Stop)
    {
//        pTDataType Data = (pTDataType)aData;
        return true;
    }
}
