#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CodecType.h"
#include "Codec_BDF.h"

#define DllExport __declspec(dllexport)
#define HEADBUFFER 256

/* Filetype information */
DFileType DllFileType = {4, "*.bdf", "Biosemi Data Format"};
int DllVersion = 10; // Version 1.0
/* Temporary string for numerical conversions */
char TempString[16];
static int const BYTESPERSAMPLE = 3;

/** strtrn - String truncation
  *
  *		dest - address of destination string
  *		src  - address of source string
  *     num  - maximum size of source string
  *
  * Copies the source string to the destination buffer without the preceding
  * and trailing spaces and tabs.
  */
void strtrn(char* dest, char* src, int num)
{
    char* ptr = src;
    int l = 0;
    while ((*(ptr) != '\0') && (l < num))
    {
        ptr++;
        l++;
    }
    if (ptr > src)
        while (((*(ptr - 1) == ' ') || (*(ptr - 1) == '\t')) && (l > 0))
        {
            ptr--;
            l--;
        }
    int i = 0;
    if (l > 0)
    {
        ptr = src;
        while (((*(ptr) == ' ') || (*(ptr) == '\t') || (*(ptr) == '\0')) && (i < num))
        {
            ptr++;
            i++;
        }
    }
    lstrcpyn(dest, ptr, (l - i) + 1);
}

/** strapd - String append
  *
  *		dest - address of destination string
  *		src  - address of source string
  *     num  - maximum size of source string
  *
  * Copies the source string with appending whitespaces to the end, until the
  * length of the string will reach num. The terminating null character will not
  * be copied.
  */
void strapd(char* dest, const char* src, int num)
{
    int i = 0;
    while ((i < num) && (src[i] != '\0'))
    {
        dest[i] = src[i];
        ++i;
    }
    while (i < num)
        dest[i++] = ' ';
}

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
        double Duration;
        unsigned int Channels;
        char Buffer[HEADBUFFER + 1];
        DWORD NrBytes = 0;
        char* ptr;
        bool Success = true;

        /* Exit if a file is already open */
        if (Data->FileHandle != INVALID_HANDLE_VALUE)
            return false;
        /* Enable also write access if needed */
        //if (Write)
        //     = GENERIC_WRITE;
        hFile = CreateFile(FileName, GENERIC_READ | (Write ? GENERIC_WRITE : 0), FILE_SHARE_READ, \
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | \
                           FILE_FLAG_RANDOM_ACCESS, NULL);
        /* Exit on file open error */
        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        memset(Buffer, 0, sizeof(Buffer));
        /* Read the Main Header or exit on error */
        if (!ReadFile(hFile, Buffer, HEADBUFFER, &NrBytes, NULL))
            return false;

        if (HEADBUFFER != NrBytes)
            return false;

        /* Import the number of Channels */
        strtrn(TempString, Buffer + NRCHANNELS, sizeof(BDFMainHeader.NrChannels));
        Channels = atoi(TempString);
        /* Exit if no channels are specified (possible corrupt file) */
        if (!Channels)
            goto Error;
        /* Import the duration of a record */
        strtrn(TempString, Buffer + DURATION, sizeof(BDFMainHeader.Duration));
        Duration = atof(TempString);
        /* Exit if no duration is specified (possible corrupt file) */
        if (Duration == 0.0)
            goto Error;
        /* Calculate the effective header size */
        strtrn(TempString, Buffer + NRBYTES, sizeof(BDFMainHeader.NrBytes));
        NrBytes = atoi(TempString);
        /* Check for a valid BDF file by comparing the calculated and saved sizes */
        if (NrBytes != (Channels + 1) * HEADBUFFER)
        {
Error:		/* Close the opened file and exit */
            CloseHandle(hFile);
            return false;
        }
        /* Assuming the BDF file is a valid and correct file */

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
        /* Import Patient identification */
        strtrn(Data->Patient, Buffer + PATIENT, sizeof(BDFMainHeader.Patient));
        /* Import m_recording identification */
        strtrn(Data->m_recording, Buffer + RECORDING, sizeof(BDFMainHeader.m_recording));
        /* Import Start date */
        strtrn(Data->Date, Buffer + STARTDATE, sizeof(BDFMainHeader.StartDate));
        /* Import Start time */
        strtrn(Data->Time, Buffer + STARTTIME, sizeof(BDFMainHeader.StartTime));
        /* Store the duration of a data record */
        Data->RecordDuration = Duration;
        /* Store the number of channels */
        Data->NrChannels = Channels;
        /* Setting the default value for the horizontal unit */
        strtrn(Data->m_horizontal_units, Buffer + RESERVED + 6, sizeof(UnitsType));
        for (unsigned int i = 0; i < sizeof(UnitsType); ++i)
            if (Data->m_horizontal_units[i] == ' ')
                Data->m_horizontal_units[i] = 0;
        if (!strlen(Data->m_horizontal_units))
            lstrcpy(Data->m_horizontal_units, "N/A");
        /* Allocate memory for the channel specific arrays */
        allocateData(Data);
        /* Allocate temporary buffer for the signal headers */
        NrBytes = Channels * HEADBUFFER;
        char ReadBuffer[NrBytes];
        /* Read the whole signal header or exit on error */
        if (!ReadFile(hFile, ReadBuffer, NrBytes, &NrBytes, NULL))
        {
Cleanup:
            /* Cleanup */
            DCloseFile(aData);
            return false;
        }
        /* Import the channel m_labels */
        unsigned int Size = sizeof(BDFChannelHeader.Label);
        char* Offset = ReadBuffer + LABEL * Channels;
        char tmp[sizeof(BDFChannelHeader.Label)];
        for (unsigned int i = 0; i < Channels; i++)
        {
            memcpy(tmp, Offset, sizeof(BDFChannelHeader.Label));
            memcpy(Data->m_labels[i], tmp, Size);
            Data->m_labels[i][Size - 1] = 0;
            Offset += Size;
        }
        /* Import the channel Transducers */
        Size = sizeof(BDFChannelHeader.Transducer);
        Offset = ReadBuffer + TRANSDUCER * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(Data->Transducer[i], Offset, Size);
            Offset += Size;
        }
        /* Import the physical Dimensions */
        Size = sizeof(BDFChannelHeader.Dimension);
        Offset = ReadBuffer + DIMENSION * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(Data->m_vertical_units[i], Offset, Size);
            if (!strlen(Data->m_vertical_units[i]))
                strcpy(Data->m_vertical_units[i], "uV");
            Offset += Size;
        }
        /* Import the Physical Minimum */
        Size = sizeof(BDFChannelHeader.PhysicalMin);
        Offset = ReadBuffer + PHYSICALMIN * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(TempString, Offset, Size);
            Data->PhysicalMin[i] = atof(TempString);
            Offset += Size;
        }
        /* Import the Physical Maximum */
        Size = sizeof(BDFChannelHeader.PhysicalMax);
        Offset = ReadBuffer + PHYSICALMAX * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(TempString, Offset, Size);
            /* Clear the success flag if MAX < MIN */
            if ((Data->PhysicalMax[i] = atof(TempString)) < Data->PhysicalMin[i])
                Success = false;
            Offset += Size;
        }
        /* Import the Digital Minimum */
        Size = sizeof(BDFChannelHeader.DigitalMin);
        Offset = ReadBuffer + DIGITALMIN * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(TempString, Offset, Size);
            Data->DigitalMin[i] = atoi(TempString);
            Offset += Size;
        }
        /* Import the Digital Maximum */
        Size = sizeof(BDFChannelHeader.DigitalMax);
        Offset = ReadBuffer + DIGITALMAX * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(TempString, Offset, Size);
            /* Clear the success flag if MAX < MIN */
            if ((Data->DigitalMax[i] = atoi(TempString)) < Data->DigitalMin[i])
                Success = false;
            /* Calculate the Ratio for BDF transformation */
            Data->Ratio[i] = 0.0;
            if (Data->DigitalMax[i] - Data->DigitalMin[i] > 0)
                Data->Ratio[i] = (Data->PhysicalMax[i] - Data->PhysicalMin[i]) / \
                                 (Data->DigitalMax[i] - Data->DigitalMin[i]);
            Offset += Size;
        }
        /* Import the channel Prefiltering */
        Size = sizeof(BDFChannelHeader.Prefiltering);
        Offset = ReadBuffer + PREFILTERING * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(Data->Prefiltering[i], Offset, Size);
            Offset += Size;
        }
        /* Used in calculation of the size of a record */
        unsigned int Sum = 0;
        /* Import the number of samples per record */
        Size = sizeof(BDFChannelHeader.NrSamples);
        Offset = ReadBuffer + NRSAMPLES * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strtrn(TempString, Offset, Size);
            /* Clear the success flag if the value is 0 */
            unsigned int Samples = atoi(TempString);
            if (!Samples)
                Success = false;
            Data->m_sample_rates[i] = Samples / Duration;
            Data->RecordSamples[i] = Samples;
            Sum += Samples;
            Offset += Size;
        }
        /* Exit with error on corrupt data */
        if (!Success)
            goto Cleanup;
        Data->RecordSize = Sum;
        /* Set the file data offset */
        Data->Offset = (Channels + 1) * HEADBUFFER;
        Size = GetFileSize(hFile, &NrBytes) - Data->Offset;
        Size /= Sum * BYTESPERSAMPLE; // 3bytes
        Data->NrRecords = Size;
        /* Calculate the total samples per channels */
        for (unsigned int i = 0; i < Channels; i++)
            Data->m_total_samples[i] = Data->RecordSamples[i] * Size;
        /* Calculate the total duration in seconds */
        Data->TotalDuration = Duration * Size;
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
        hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, 0, \
                           NULL, Creation, FILE_ATTRIBUTE_NORMAL | \
                           FILE_FLAG_RANDOM_ACCESS, NULL);
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
                strcpy(Data->m_vertical_units[i], "uV");
                FillMemory(Data->m_labels[i], sizeof(m_labelsType), 0);
                FillMemory(Data->Prefiltering[i], sizeof(StringType), 0);
                FillMemory(Data->Transducer[i], sizeof(StringType), 0);
                Data->PhysicalMin[i] = 0.0;
                Data->PhysicalMax[i] = 0.0;
                Data->DigitalMin[i] = -8388608;
                Data->DigitalMax[i] = 8388607;
                Data->JumpTable[i] = 0;
                Data->Ratio[i] = 0.0;
            }
        }
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
        /* Exit if no file is open */
        if (Data->FileHandle == INVALID_HANDLE_VALUE)
            return false;
        if (Buffer == NULL)
            return false;

        unsigned int Channels = Data->NrChannels;
        unsigned int FILEBUFFER = Data->RecordSize;
        unsigned char* FileBuffer = (unsigned char*) Data->FileBuffer;

        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new unsigned char[FILEBUFFER * BYTESPERSAMPLE];
            Data->FileBuffer = (double *)FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }

        unsigned int* Jump = Data->JumpTable;
        unsigned int TempBlock;
        unsigned int StartBlock = 0;
        unsigned int NrBlock = 0;
        //printf("NrBlock: %u\n", NrBlock);
        Jump[0] = 0;
        for (unsigned int i = 0; i < Channels; i++)
        {
            TempBlock = Start[i] / Data->RecordSamples[i];
            if (StartBlock < TempBlock)
                StartBlock = TempBlock;
            TempBlock = Stop[i] / Data->RecordSamples[i];
            if (NrBlock < TempBlock)
                NrBlock = TempBlock;
            //printf("%u, NrBlock: %u\n", i, NrBlock);
            Jump[i] = 0;
        }
        NrBlock -= StartBlock;
        unsigned int StartOffset = StartBlock * Data->RecordSize * BYTESPERSAMPLE + Data->Offset;
        DWORD NrBytes;

        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;

        unsigned int m, p, q, r;
        double* PhysicalMin = Data->PhysicalMin;
        double* Ratio = Data->Ratio;
        //printf("Ratio: %f\n", Ratio[0]);
        for (unsigned int i = 0; i <= NrBlock; i++)
        {
            //printf("i=%u ", i);
            TempBlock = 0;
            if (!ReadFile(Data->FileHandle, FileBuffer, FILEBUFFER * BYTESPERSAMPLE, &NrBytes, NULL))
                return false;
            if (FILEBUFFER * BYTESPERSAMPLE != NrBytes)
                return false; /// TODONOW: check why "i <= NrBlock" is needed. sometimes the buffer is not filled entirely without it, but monst of the time we just end up here in this line, where we shouldnt. Check also why was returned "true" previously.
            //for (unsigned int ix = 0; ix < 10; printf("%i,", FileBuffer[ix++]));
            for (unsigned int j = 0; j < Channels; j++)
            {
                p = 0;
                r = Data->RecordSamples[j];
                if ((Start[j] / r) == (StartBlock + i))
                    p = Start[j] % r;
                if ((Start[j] / r) > (StartBlock + i))
                    p = r;
                q = r;
                if ((Stop[j] / r) == (StartBlock + i))
                    q = Stop[j] % r;
                if ((Stop[j] / r) < (StartBlock + i))
                    q = 0;
                //printf("%u, %u;\n", p, q);
                if (p < q)
                {
                    m = Jump[j];
                    for (unsigned int k = p; k < q; k++)
                    {
                        int tmp = 0;
                        memcpy(&tmp, FileBuffer + (TempBlock + k) * BYTESPERSAMPLE, BYTESPERSAMPLE);
                        if (tmp & 0x00800000)
                            tmp |= 0xFF000000;
                        Buffer[j][m + k - p] = Ratio[j] * (tmp - Data->DigitalMin[j]) + PhysicalMin[j];
                    }
                    Jump[j] += q - p;
                }
                TempBlock += r;
            }
        }
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
        /* Exit if no file is open */
        if (Data->FileHandle == INVALID_HANDLE_VALUE)
            return false;
        if (Buffer == NULL)
            return false;

        unsigned int i, N = 0;
        unsigned int Channels = Data->NrChannels;
        /* Calculate the number of enabled channels */
        for (i = 0; i < Channels; N += Enable[i++] & 1)
            ;
        /* Return if no channels found */
        if (!N)
            return false;
        /* Optimize, if all channels are enabled */
        //cout << aData << " Start " << Start[0] << " Stop:" << Stop[0] << "Data->m_total_samples[0]: " << Data->m_total_samples[i] << endl;

        if (N == Channels)
            return DGetDataRaw(aData, Buffer, Start, Stop);

        unsigned int FILEBUFFER = Data->RecordSize;
        unsigned char* FileBuffer = (unsigned char*) Data->FileBuffer;

        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new unsigned char[FILEBUFFER * BYTESPERSAMPLE];
            Data->FileBuffer = (double *)FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }

        unsigned int* Jump = Data->JumpTable;
        unsigned int TempBlock;
        unsigned int StartBlock = 0;
        int NrBlock = 0;
        //printf("NrBlock: %u\n", NrBlock);
        for (unsigned int i = 0; i < Channels; i++)
        {
            if (Enable[i])
            {
                TempBlock = Start[i] / Data->RecordSamples[i];
                if (StartBlock < TempBlock)
                    StartBlock = TempBlock;
                TempBlock = Stop[i] / Data->RecordSamples[i];
                if (NrBlock < (int)TempBlock)
                    NrBlock = TempBlock;
                //printf("%u, NrBlock: %u\n", i, NrBlock);
                Jump[i] = 0;
            }
        }
        NrBlock -= StartBlock + 1;
        if (NrBlock < 0)
            NrBlock = 0;
        unsigned int StartOffset = StartBlock * Data->RecordSize * BYTESPERSAMPLE + Data->Offset;
        DWORD NrBytes;

        if (SetFilePointer(Data->FileHandle, StartOffset, 0, FILE_BEGIN) == 0xffffffff)
            return false;

        unsigned int l, m, p, q, r;
        double* PhysicalMin = Data->PhysicalMin;
        double* Ratio = Data->Ratio;
        //printf("Ratio: %f\n", Ratio[0]);
        for (int i = 0; i <= NrBlock; i++)
        {
            //printf("i=%u ", i);
            TempBlock = 0;
            l = 0;
            if (!ReadFile(Data->FileHandle, FileBuffer, FILEBUFFER * BYTESPERSAMPLE, &NrBytes, NULL))
                return false;
            if (FILEBUFFER * BYTESPERSAMPLE != NrBytes)
                return false; /// RT FUNCTIONS!!! - TODO: check
            //cout << "return false2" << FILEBUFFER * BYTESPERSAMPLE << " / " << NrBytes << " i: " << i << " NrBlock: " << NrBlock << endl;

            for (unsigned int j = 0; j < Channels; j++)
            {
                r = Data->RecordSamples[j];
                if (Enable[j])
                {
                    p = 0;
                    if ((Start[j] / r) == (StartBlock + i))
                        p = Start[j] % r;
                    if ((Start[j] / r) > (StartBlock + i))
                        p = r;
                    q = r;
                    if ((Stop[j] / r) == (StartBlock + i))
                        q = Stop[j] % r;
                    if ((Stop[j] / r) < (StartBlock + i))
                        q = 0;
                    //printf("%u, %u;\n", p, q);
                    if (p < q)
                    {
                        m = Jump[j];
                        for (unsigned int k = p; k < q; k++)
                        {
                            int tmp = 0;
                            memcpy(&tmp, FileBuffer + (TempBlock + k) * BYTESPERSAMPLE, BYTESPERSAMPLE);
                            if (tmp & 0x00800000)
                                tmp |= 0xFF000000;
                            Buffer[l][m + k - p] = Ratio[j] * (tmp - Data->DigitalMin[j]) + PhysicalMin[j];
                        }
                        Jump[j] += q - p;
                    }
                    l++;
                }
                TempBlock += r;
            }
        }
        return true;
    }

    DllExport bool DWriteHeader(int aData)
    {
        pTDataType Data = (pTDataType)aData;
        unsigned int Channels = Data->NrChannels;
        char Buffer[Channels * HEADBUFFER];
        unsigned int Length = Channels * HEADBUFFER;
        DWORD NrBytes;

        /* Exit if no file is open */
        if (Data->FileHandle == INVALID_HANDLE_VALUE)
            return false;

        BDFMainHeader.Version[0] = 0xff;
        BDFMainHeader.Version[1] = 0x42;
        BDFMainHeader.Version[2] = 0x49;
        BDFMainHeader.Version[3] = 0x4f;
        BDFMainHeader.Version[4] = 0x53;
        BDFMainHeader.Version[5] = 0x45;
        BDFMainHeader.Version[6] = 0x4d;
        BDFMainHeader.Version[7] = 0x49;
        strapd(Buffer + VERSION, BDFMainHeader.Version, sizeof(BDFMainHeader.Version));
        strapd(Buffer + PATIENT, Data->Patient, sizeof(BDFMainHeader.Patient));
        strapd(Buffer + RECORDING, Data->m_recording, sizeof(BDFMainHeader.m_recording));
        strapd(Buffer + STARTDATE, Data->Date, sizeof(BDFMainHeader.StartDate));
        strapd(Buffer + STARTTIME, Data->Time, sizeof(BDFMainHeader.StartTime));
        sprintf(TempString, "%u", Length + HEADBUFFER);
        strapd(Buffer + NRBYTES, TempString, sizeof(BDFMainHeader.NrBytes));
        FillMemory(Buffer + RESERVED, sizeof(BDFMainHeader.m_reserved), ' ');
        strncpy((char*)(Buffer + RESERVED), "24BIT", 6);
        strcpy((char*)(Buffer + RESERVED + 6), Data->m_horizontal_units);
        sprintf(TempString, "%u", Data->NrRecords);
        strapd(Buffer + NRRECORDS, TempString, sizeof(BDFMainHeader.NrRecords));
        sprintf(TempString, "%f", Data->RecordDuration);
        strapd(Buffer + DURATION, TempString, sizeof(BDFMainHeader.Duration));
        sprintf(TempString, "%u", Channels);
        /* Write the number of Channels */
        strapd(Buffer + NRCHANNELS, TempString, sizeof(BDFMainHeader.NrChannels));

        if (SetFilePointer(Data->FileHandle, 0, 0, FILE_BEGIN) == 0xffffffff)
            return false;
        if (!WriteFile(Data->FileHandle, Buffer, HEADBUFFER, &NrBytes, NULL))
            return false;
        if (HEADBUFFER != NrBytes)
            return false;

        unsigned int Size = sizeof(BDFChannelHeader.Label);
        char* Offset = Buffer + LABEL * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strapd(Offset, Data->m_labels[i], Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.Transducer);
        Offset = Buffer + TRANSDUCER * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strapd(Offset, Data->Transducer[i], Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.Dimension);
        Offset = Buffer + DIMENSION * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strapd(Offset, Data->m_vertical_units[i], Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.PhysicalMin);
        Offset = Buffer + PHYSICALMIN * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            sprintf(TempString, "%f", Data->PhysicalMin[i]);
            strapd(Offset, TempString, Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.PhysicalMax);
        Offset = Buffer + PHYSICALMAX * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            sprintf(TempString, "%f", Data->PhysicalMax[i]);
            strapd(Offset, TempString, Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.DigitalMin);
        Offset = Buffer + DIGITALMIN * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            sprintf(TempString, "%i", Data->DigitalMin[i]);
            strapd(Offset, TempString, Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.DigitalMax);
        Offset = Buffer + DIGITALMAX * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            sprintf(TempString, "%i", Data->DigitalMax[i]);
            strapd(Offset, TempString, Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.Prefiltering);
        Offset = Buffer + PREFILTERING * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            strapd(Offset, Data->Prefiltering[i], Size);
            Offset += Size;
        }
        Data->RecordSize = 0;
        Size = sizeof(BDFChannelHeader.NrSamples);
        Offset = Buffer + NRSAMPLES * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            Data->RecordSize += Data->RecordSamples[i];
            sprintf(TempString, "%u", Data->RecordSamples[i]);
            strapd(Offset, TempString, Size);
            Offset += Size;
        }
        Size = sizeof(BDFChannelHeader.m_reserved);
        Offset = Buffer + CRESERVED * Channels;
        for (unsigned int i = 0; i < Channels; i++)
        {
            FillMemory(Offset, Size, ' ');
            Offset += Size;
        }

        if (!WriteFile(Data->FileHandle, Buffer, Length, &NrBytes, NULL))
            return false;
        if (Length != NrBytes)
            return false;

        Data->Offset = Length + HEADBUFFER;
        for (unsigned int i = 0; i < Channels; i++)
        {
            Data->Ratio[i] = 0.0;
            if (Data->DigitalMax[i] - Data->DigitalMin[i] > 0)
                Data->Ratio[i] = (Data->PhysicalMax[i] - Data->PhysicalMin[i]) / \
                                 (Data->DigitalMax[i] - Data->DigitalMin[i]);
        }
        return true;
    }

    DllExport bool DWriteBlock(int aData, double** Buffer, unsigned int* Start, unsigned int* Stop)
    {
        pTDataType Data = (pTDataType)aData;
        /* Exit if no file is open */
        if (Data->FileHandle == INVALID_HANDLE_VALUE)
            return false;
        if (Buffer == NULL)
            return false;

        unsigned int Channels = Data->NrChannels;
        unsigned int FILEBUFFER = Data->RecordSize;
        unsigned char* FileBuffer = (unsigned char*) Data->FileBuffer;

        if (FILEBUFFER != Data->FILEBUFFER)
        {
            delete[] Data->FileBuffer;
            FileBuffer = new unsigned char[FILEBUFFER * BYTESPERSAMPLE];
            Data->FileBuffer = (double*)FileBuffer;
            Data->FILEBUFFER = FILEBUFFER;
        }
        /*unsigned int StartOffset = Start[0] * Channels * sizeof(double) + Data->Offset;*/
        DWORD NrBytes;
        /*if (GetFileSize(Data->FileHandle, &NrBytes) < StartOffset)
            return false;*/
        if (SetFilePointer(Data->FileHandle, 0, 0, FILE_END) == 0xffffffff)
            return false;

        int* DigitalMin = Data->DigitalMin;
        double* Ratio = Data->Ratio;
        unsigned int NrBlock = (Stop[0] - Start[0]) / Data->RecordSamples[0];
        unsigned int Index;
        for (unsigned int i = 0; i < NrBlock; i++)
        {
            Index = 0;
            for (unsigned int j = 0; j < Channels; j++)
            {
                for (unsigned int k = 0; k < Data->RecordSamples[j]; k++)
                {
                    int digitalVal = (int)((Buffer[j][i * Data->RecordSamples[j] + k] - Data->PhysicalMin[j]) / Ratio [j]) + DigitalMin[j];
                    memcpy(FileBuffer + (Index + k) * BYTESPERSAMPLE, &digitalVal, BYTESPERSAMPLE);
                }
                Index += Data->RecordSamples[j];
            }
            Data->NrRecords++;
            for (unsigned int i = 0; i < Channels; ++i)
                Data->m_total_samples[i] = Data->RecordSamples[i] * Data->NrRecords;
            if (!WriteFile(Data->FileHandle, FileBuffer, FILEBUFFER * BYTESPERSAMPLE, &NrBytes, NULL))
                return false;
            if (FILEBUFFER * BYTESPERSAMPLE != NrBytes)
                return false;
        }
        return true;
    }
}
