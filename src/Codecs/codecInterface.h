/** DGetCodecInfo - Function to get information on the supported file type
  *     
  *     aExtensions - the list of supported file extensions, for example "*.txt;*.pdf"
  *     aDescription - description of codec and the supported file types
  *     aID - unique ID. This number identifies the codec.
  *
  * The ID should be globally unique within an application. 
  * Two codecs shouldn't have the same ID.
  */
DllExport void DGetCodecInfo(char* aExtensions, char* aDescription, int* aID);

/** DInitData - Initializes a codec object.
  *
  * Returns codec handle. (It is the address of the data structure.)
  * Fills the data structure with the codec provided default values.
  * You must call this function in order to have a codec handle.
  * You should have a codec handle before using any other functionality.
  *
  */
DllExport int DInitData()

/** DCloseFile - Close the previously opened file
  *
  *     aHandle - codec handle
  *
  * Closes the previously opened file and performs the aditional memory
  * cleanups.
  */
DllExport bool DCloseFile(int aHandle);

/** DDeleteData - Deinitializes the codec object
  *
  *     aHandle - codec handle
  *
  * Deletes the whole data codec structure. It also closes the file if it is open.
  * The handle is freed up and is not usable any further.
  */
DllExport void DDeleteData(int aHandle);

/** DOpenFile - Open a file
  *
  *     aHandle   - codec handle
  *     aFileName - file name with full path
  *     aWrite    - true: Read/Write access, but no sharing
  *                 false: Read only access, but with sharing
  *
  * Opens a file, and fills up the data structure with the header of the file.
  * If Write is true then the file is opened with R/W access, but no sharing is
  * possible. If Write is false, the file is opened with RO access, read only
  * sharing is possible, but the DSaveHeader function is disabled.
  */
DllExport bool DOpenFile(int aHandle, char* aFileName, const bool aWrite);

/** DCreateNewFile - Create a file
  *
  *     aHandle   - codec handle
  *     aFileName - file name with full path
  *     aChannels = number of channels
  *     aRewrite  - true: rewrites existing file
  *                 false: check for existing file
  *
  * Creates a new file. If Rewrite is true then the file will be rewritten.
  * If Rewrite is false, the file will be checked, and function will fail
  * if the file exists.  If the number of channels is above zero, then the
  * necessary memory alocations will be performed.
  */
DllExport bool DCreateNewFile(int aHandle, char* aFileName, int aChannels, const bool aRewrite);

/** DGetDataRaw - Reads the raw data
  *
  *     aHandle - codec handle
  *     aBuffer  - address of the output buffer
  *     aStart   - start sample index vector
  *     aStop    - stop sample index vector
  *
  * Fills the buffer with the raw data from start to stop indexes for each
  * channel.
  */
DllExport bool DGetDataRaw(int aHandle, double** aBuffer, unsigned int* aStart, unsigned int* aStop);

/** DGetChannelRaw - Reads the raw data by channels
  *
  *     aData   - address of the data structure
  *     aBuffer - address of the output buffer
  *     aEnable - channel enable index vector
  *     aStart  - start sample index vector
  *     aStop   - stop sample index vector
  *
  * Fills the buffer with the raw data from start to stop indexes for the
  * specified channels. Enable is an array with boolean values to specify
  * the active channels.
  */
DllExport bool DGetChannelRaw(int aHandle, double** aBuffer, int* aEnable, unsigned int* aStart, unsigned int* aStop);

/** DSingleGetChannelRaw - Reads the raw data of a single channel
  *
  *     aData   - address of the data structure
  *     aBuffer - address of the output buffer
  *     aEnable - index of the enabled channel
  *     aStart  - start sample index
  *     aStop   - stop sample index
  *
  * Fills the buffer with the raw data from start to stop indexes for the
  * specified channel. Enable is the index of the channel.
  */
DllExport bool DGetSingleChannelRaw(int aData, double* Buffer, int Enable, unsigned int Start, unsigned int Stop)

/** DWriteHeader - Writes file header
  *
  *     aData   - address of the data structure
  */
DllExport bool DWriteHeader(int aHandle);

/** DWriteBlock - writes raw data
  *
  *     aHandle  - codec handle
  *     aBuffer  - address of the buffer to write
  *     aStart   - start sample index vector
  *     aStop    - stop sample index vector
  *
  * Writes the buffer with the raw data to the file 
  * from start to stop indexes for each channel.
  * TotalSamples are not incremented.
  */
DllExport bool DWriteBlock(int aHandle, double** Buffer, unsigned int* Start, unsigned int* Stop);

/** DAppendSamples - appends raw data
  *
  *     aHandle   - codec handle
  *     aBuffer   - address of the buffer to write
  *     NrSamples - number of samples to write
  *
  * Appends the buffer with the raw data to the file
  * TotalSamples are incremented with NrSamples.
  */
DllExport bool DAppendSamples(int aData, double** Buffer, unsigned int NrSamples);

/** DGetNrChannels - Gets the number of channels
  *
  *     aHandle - codec handle
  *
  * Returns the number of channels.
  */
dllexport int DGetNrChannels(int aHandle);

/** DGetTotalSamples - Gets the total samples per channels
  *
  *     aHandle - codec handle
  *     aTotalSamples - vector of sample numbers
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetTotalSamples(int aHandle, unsigned int* aTotalSamples);

/** DGetSampleRates - Gets the sample rates per channels
  *
  *     aHandle - codec handle
  *     aSampleRates - vector of sample rates
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetSampleRates(int aHandle, double* aSampleRates);

/** DGetLabel - Gets the label of one channel
  *
  *     aHandle - codec handle
  *     aChannelIndex - index of channel
  *     aLabel - label string
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetLabel(int aHandle, int aChannelIndex, char* aLabel);

/** DGetVerticalUnit - Gets the vertical unit of one channel
  *
  *     aHandle - codec handle
  *     aChannelIndex - index of channel
  *     aVerticalUnit - vertical unit
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetVerticalUnit(int aHandle, int aChannelIndex, char* aVerticalUnit);

/** DGetHorizontalUnit - Gets the horiyontal unit
  *
  *     aHandle - codec handle
  *     aHorizontalUnit - horiyontal unit
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetHorizontalUnit(int aHandle, char* aHorizontalUnit);

/** DGetRecording - Gets the recording description
  *
  *     aHandle - codec handle
  *     aRecording - recording description
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetRecording(int aHandle, char* aRecording);

/** DGetDate - Gets the date of recording
  *
  *     aHandle - codec handle
  *     aDate - date of recording
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetDate(int aHandle, char* aDate);

/** DGetTime - Gets the time of recording
  *
  *     aHandle - codec handle
  *     aTime - time of recording
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetTime(int aHandle, char* aTime);

/** DGetFileName - Gets the current file name
  *
  *     aHandle - codec handle
  *     aFileName - file name
  *
  * Fills the aTotalSamples with the sample numbers for each channel.
  */
dllexport bool DGetFileName(int aHandle, char* aFileName);

/** Setter functions for corresponding data
  *
  *     aHandle - codec handle
  *
  */
dllexport bool DSetSampleRates(int aHandle, double* aSampleRates);
dllexport bool DSetPhysicalMin(int aHandle, double* aPhysicalMin);
dllexport bool DSetPhysicalMax(int aHandle, double* aPhysicalMax);
dllexport bool DSetRecordSamples(int aHandle, unsigned int* aRecordSamples);
dllexport bool DSetRecordDuration(int aHandle, double aRecordDuration);
dllexport bool DSetTotalSamples(int aHandle, unsigned int* aTotalSamples);
dllexport bool DSetLabel(int aHandle, int aChannelIndex, char* aLabel);
dllexport bool DSetVerticalUnit(int aHandle, int aChannelIndex, char* aVerticalUnit);
dllexport bool DSetHorizontalUnit(int aHandle, char* aHorizontalUnit);
dllexport bool DSetRecording(int aHandle, char* aRecording);
dllexport bool DSetDate(int aHandle, char* aDate);
dllexport bool DSetTime(int aHandle, char* aTime);
