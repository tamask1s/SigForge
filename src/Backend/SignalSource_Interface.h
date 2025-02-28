/*
This software is licensed under MIT License.

Copyright (c) 2019 Tamas Levente Kis - tamkis@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _SIGNALSOURCE_INTERFACE_H_
#define _SIGNALSOURCE_INTERFACE_H_

/**
 * \brief Interface for signal input devices.
 */

/**
 * \brief Initializes the library. It must be called before calling
 * \brief any other functions
 * \param[in] aInitDescriptor Descriptor passed to the library. It is library specific,
 * \param[in] aInitDescriptor and its structure must be specified by every implementation.
 * \param[in] aInitDescriptor The implementation should handle NULL value.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) initLib(const char* aInitDescriptor);

/**
 * \brief Provides module information
 * \param[out] aID This number identifies the module. The ID should be globally unique within an application.
 * \param[out] aID Two modules shouldn't have the same ID.
 * \param[out] aGeneralDescription JSON descriptor containing at least these fields:
 * \param[out] aGeneralDescription {"Resolution":"10", "MaxNrChannels":"8", "MaxSPSTotal":"2000", "MaxSPSTotalMpx":"1000"}.
 * \param[out] aGeneralDescription Resolution: the resolution in bits;
 * \param[out] aGeneralDescription MaxNrChannels the maximum number of channels;
 * \param[out] aGeneralDescription MaxSPSTotal: maximum number of samples per second. The sum of samples recorded by all channels.
 * \param[out] aGeneralDescription MaxSPSTotalMpx: maximum number of samples per second in case of multiplexed recording.
 * \param[out] aCapabilityDescriptor Reserved.
 * \param[in] aNrMaxCharacters The number of characters available in the preallocated output buffers.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) getModuleInfo(int* aID, char* aGeneralDescription, char* aCapabilityDescriptor, int aNrMaxCharacters);

/**
 * \brief Connects the data source device.
 * \brief "initLib" must be called before calling this function.
 * \param[in] aParameter Descriptor passed to the library. It is library specific,
 * \param[in] aParameter and its structure must be specified by every implementation.
 * \param[in] aParameter The implementation should handle NULL value.
 * \return Device handle value on success, value of zero on error.
 */
int __declspec (dllimport) connectDevice(const char* aParameter);

/**
 * \brief Sets up the recording.
 * \brief "initLib" and "connectDevice" must be called before calling this function.
 * \param[in] aHandle The value returned by the "connectDevice" function.
 * \param[in] aNrChannelsToUse Number of channels to use. Must correlate with the "aGeneralDescription" parameter returned by "getModuleInfo".
 * \param[in] aSampleRates Array of integers. The sample rate values
 * \param[in] aSampleRates must correlate with the "aGeneralDescription" parameter returned by "getModuleInfo".
 * \param[in] aMillisecondsToRead The recording takes place continuously, but the data can be get in fixed sized segments.
 * \param[in] aMillisecondsToRead After registering an amount of data, the segment is available, and can be get by calling the "analogRead" function.
 * \param[in] aMillisecondsToRead The amount of data in a segment is determined by the three para,eters above.
 * \param[in] aMillisecondsToRead The number of total samples can be get at once = SUM(aSampleRates[S/s]) * aMillisecondsToRead[ms] * 1000 Samples.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) setupReading(int aHandle, int aNrChannelsToUse, int* aSampleRates, int aMillisecondsToRead);

/**
 * \brief Starts the recording.
 * \brief "initLib", "connectDevice" and "setupReading" must be called before calling this function.
 * \param[in] aHandle The value returned by the "connectDevice" function.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) startReading(int aHandle);

/**
 * \brief Stops the recording.
 * \brief "initLib", "connectDevice" and "setupReading" must be called before calling this function.
 * \param[in] aHandle The value returned by the "connectDevice" function.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) stopReading(int aHandle);

/**
 * \brief Not supported.
 * \param[in] aHandle The value returned by the "connectDevice" function.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) digitalRead(int aHandle, int* aData, int aSamplesPerChannelToRead, double aTimeout);

/**
 * \brief Not supported.
 * \param[in] aHandle The value returned by the "connectDevice" function.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) digitalRange(int aHandle, int* aDigitalMin, int* aDigitalMax);

/**
 * \brief Reads analogue data.
 * \param[in] aHandle The value returned by the "connectDevice" function.
 * \param[out] aData The buffer allocated by the caller. It must have room for SUM(aSampleRates[S/s]) * aMillisecondsToRead[ms] * 1000 Samples.
 * \param[out] aData The data from the first channel is readed first: aSampleRates[0] * aMillisecondsToRead[ms] * 1000 Samples.
 * \param[out] aData The data from the second channel is readed afterwards if used: aSampleRates[1] * aMillisecondsToRead[ms] * 1000 Samples, and so on.
 * \param[out] aData Please refer to the "setupReading" function description also.
 * \return Value of zero on success, and non-zero value on error.
 */
int __declspec (dllimport) analogRead(int aHandle, double* aData);

#endif ///_SIGNALSOURCE_INTERFACE_H_
