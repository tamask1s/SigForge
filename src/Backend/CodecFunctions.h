#ifndef CODECFUNCTIONS_H_
#define CODECFUNCTIONS_H_

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

/* Method type descriptions for DLL bindings */
typedef void (*DGetCodecInfoProc)(char* a_extensions, char* a_description, int* a_id);
typedef int  (*DInitDataProc)();
typedef void (*DDeleteDataProc)(int a_handle);
typedef bool (*DOpenFileProc)(int a_handle, char* a_file_name, const bool a_write);
typedef bool (*DCreateNewFileProc)(int a_handle, const char* a_file_name, int a_nr_channels, const bool a_rewrite_file);
typedef bool (*DCloseFileProc)(int a_handle);
typedef bool (*DGetDataRawProc)(int a_handle, double** a_buffer, unsigned int* a_start, unsigned int* a_stop);
typedef bool (*DGetChannelRawProc)(int a_handle, double**, int*, unsigned int*, unsigned int*);
typedef bool (*DWriteHeaderProc)(int a_handle);
typedef bool (*DAppendSamplesProc)(int a_handle, double** a_buffer, unsigned int a_nr_samples);
typedef bool (*DWriteBlockProc)(int a_handle, double** a_buffer, unsigned int* a_start, unsigned int* a_stop);
typedef int  (*DGetNrChannelsProc)(int a_handle);
typedef bool (*DGetTotalSamplesProc)(int a_handle, unsigned int* a_total_samples);
typedef bool (*DGetSampleRatesProc)(int a_handle, double* a_sample_rates);
typedef bool (*DGetLabelProc)(int a_handle, int a_channel_index, char* aLabel);
typedef bool (*DGetTransducerProc)(int a_handle, int a_channel_index, char* aTransducer);
typedef bool (*DGetVerticalUnitProc)(int a_handle, int a_channel_index, char* a_vertical_units);
typedef bool (*DGetHorizontalUnitProc)(int a_handle, char* a_horizontal_units);
typedef bool (*DGetRecordingProc)(int a_handle, char* a_recording);
typedef bool (*DGetPatientProc)(int a_handle, char* a_patient);
typedef bool (*DGetDateProc)(int a_handle, char* a_date);
typedef bool (*DGetTimeProc)(int a_handle, char* a_time);
typedef bool (*DGetFileNameProc)(int a_handle, char* a_file_name);
typedef bool (*DSetSampleRatesProc)(int a_handle, double* a_sample_rates);
typedef bool (*DSetPhysicalMinProc)(int a_handle, double* a_physical_min);
typedef bool (*DSetPhysicalMaxProc)(int a_handle, double* a_physical_max);
typedef bool (*DSetRecordSamplesProc)(int a_handle, unsigned int* a_record_samples);
typedef bool (*DSetRecordDurationProc)(int a_handle, double aRecordDuration);
typedef bool (*DSetTotalSamplesProc)(int a_handle, unsigned int* a_total_samples);
typedef bool (*DSetLabelProc)(int a_handle, int aChannelIndx, char* aLabel);
typedef bool (*DSetTransducerProc)(int a_handle, int aChannelIndx, char* aTransducer);
typedef bool (*DSetVerticalUnitProc)(int a_handle, int aChannelIndx, char* aVerticalUnit);
typedef bool (*DSetHorizontalUnitProc)(int a_handle, char* aHorizontalUnit);
typedef bool (*DSetRecordingProc)(int a_handle, char* a_recording);
typedef bool (*DSetPatientProc)(int a_handle, char* a_patient);
typedef bool (*DSetDateProc)(int a_handle, char* a_date);
typedef bool (*DSetTimeProc)(int a_handle, char* a_time);

#endif /// CODECFUNCTIONS_H_
