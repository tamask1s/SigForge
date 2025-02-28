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

#ifndef _DATAAQ_TYPES_H_
#define _DATAAQ_TYPES_H_

/**
 * \brief Type definitions for signal input device interface functions.
 * \brief For more information pls see the description in the "SignalSource_Interface.h" file.
 */

typedef int (*funcRef_initLib) (const char* a_init_descriptor);
typedef int (*funcRef_getModuleInfo) (int* a_id, char* a_general_description, char* a_capability_descriptor, int a_nr_max_characters);
typedef int (*funcRef_connectDevice) (const char* a_connection_parameter);
typedef int (*funcRef_disconnectDevice) (int a_handle);
typedef int (*funcRef_setupReading) (int a_handle, int a_nr_channels_to_use, const int* a_sample_rates, const int* a_gain, const int* a_physical_mapping, int a_milliseconds_to_read);
typedef int (*funcRef_startReading) (int a_handle);
typedef int (*funcRef_stopReading) (int a_handle);
typedef int (*funcRef_digitalRead) (int a_handle, int* aData, int a_samples_per_channel_to_read, double a_timeout);
typedef int (*funcRef_digitalRange) (int a_handle, int* a_digital_min, int* a_digital_max);
typedef int (*funcRef_analogRead) (int a_handle, double* aData);

#endif /// _DATAAQ_TYPES_H_
