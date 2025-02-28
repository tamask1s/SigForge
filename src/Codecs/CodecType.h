#ifndef _CODECTYPE_H_
#define _CODECTYPE_H_

/**# DInitData - Initialize the data structure
  *
  *		Data - address of the data structure
  *
  * Fills the data structure with the codec provided default values.
  *
  *# DOpenFile - Open a file
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
  *
  *# DCreateNewFile - Create a file
  *
  *		Data	 - address of the data structure
  *		FileName - file name with full path
  *		Channels = number of channels
  *		Rewrite	 - true: rewrites existing file
  *				   false: check for existing file
  *
  * Creates a new file. If Rewrite is true then the file will be rewritten.
  * If Rewrite is false, the file will be checked, and function will fail
  * if the file exists. If the number of channels is above zero, then the
  * necessary memory alocations will be performed.
  *
  *# DCloseFile - Close the previously opened file
  *
  *		Data - address of the data structure
  *
  * Closes the previously opened file and performs the aditional memory
  * cleanups.
  *
  *# DGetDataRaw - Reads the raw data
  *
  *		Data   - address of the data structure
  *		Buffer - address of the output buffer
  *		Start  - start sample index vector
  *		Stop   - stop sample index vector
  *
  * Fills the buffer with the raw data from start to stop indexes for each
  * channel.
  *
  *# DGetChannelRaw - Reads the raw data by channels
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

/* FileType structure */
typedef struct DFileType
{
    int ID;						// Unique FileType identification code
    char Extension[32];			// File extension
    char Description[64];		// File type description
}
DFileType, *pDFileType;

/* Complementary string types for DataTyp structure */
typedef char UnitsType[12];		// String type for Units
typedef char m_labelsType[20];	// String type for m_labels
typedef char StringType[84];	// String type for longer text

/* DataType structure */
typedef struct TDataTypeX
{
    /* File specific informations */
    int Version;				// Used DLL version
    DFileType FileType;			// File type information
    char FilePath[MAX_PATH];	// Filename with path
    char* FileName;				// Just the filename
    HANDLE FileHandle;			// Handle of the opened file
    /* Patient specific informations */
    StringType Patient;			// Patient info
    StringType m_recording;		// m_recording info
    char Date[12];				// Start date of registration
    char Time[12];				// Start time of registration
    unsigned int NrRecords;		// Number of data records
    double TotalDuration;		// Total duration in m_horizontal_units (usually seconds)
    double RecordDuration;		// Duration of a record in m_horizontal_units (usually seconds)
    int NrChannels;				// Number of channels
    StringType Comment;			// Comment
    /* Channel specific information */
    unsigned int* m_total_samples;	// Total samples per channels
    unsigned int* RecordSamples;// Samples per record per channel
    double* m_sample_rates;			// Sample rates per channels
    UnitsType m_horizontal_units;	// Horizontal unit (usually Sec.)
    UnitsType* m_vertical_units;	// Vertical units (physical dimensions)
    m_labelsType* m_labels;			// Channel labels
    StringType* Prefiltering;	// Prefiltering information
    StringType* Transducer;		// Used transducer type
    double* PhysicalMin;		// Physical minimum
    double* PhysicalMax;		// Physical maximum
    int* DigitalMin;			// Digital minimum
    int* DigitalMax;			// Digital maximum

    /* Internally used variables */
    unsigned int RecordSize;	// Size of a data record (in samples)
    unsigned int Offset;		// Data Offset in the file
    double* FileBuffer;			// Pointer to the alocated memory buffer
    unsigned int FILEBUFFER;	// Size of the allocated memory buffer
    unsigned int* JumpTable;	// Used for interlaced access
    double* Ratio;				// Physical / Digital domain ratio

    int* enableWrk;             // Working buffer for GetChannelData
    unsigned int* startWrk;     // Working buffer for GetChannelData
    unsigned int* stopWrk;      // Working buffer for GetChannelData
}
*pTDataType;

pTDataType createData();
void allocateData(pTDataType Data);
void deAllocateData(pTDataType Data);
bool CreateNewFile_i(int aData, const char* FileName, int Channels, const bool Rewrite, int DigitalMin, int DigitalMax);

#endif // _CODECTYPE_H_
