#include <iostream>
#include <map>
#include <vector>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "minvect.h"
#include "datastructures.h"

class IRuntimeProcessor
{
public:
    /** \brief * Processes 2 dimensional data array (a_data), in an in-placemaner.
    * @param a_data - The data to be processed.
    * @param a_nr_channels - The height/number of rows of the data array to be processed.
    * @param a_data_sizes - Array containing the widths/number of elements of the rows in a_data.
    * @param a_history_data - The function may use a processing history - a persistent 2 dimensional data array.
    * @param a_nr_hystory_layers - the number of time layers in a_history_data. a_history_data has a size of a_nr_hystory_layers(height) X a_nr_channels(width)
    */
    virtual void ProcessData(double** a_data, unsigned int a_nr_channels, unsigned int* a_data_sizes, double** a_hystory_data, unsigned int a_nr_hystory_layers) = 0;
};

class CIIRFilter: public IRuntimeProcessor
{
    double* m_denominator;
    double* m_numerator;
    unsigned int m_nr_coefs;

    /** \brief * Infinite impulse filter implemented for up to 7 coefficients. Filters data in an in-place manner.
    * @param x - The data to be filtered.
    * @param a_len -
    * @param n - Array containing the numerator coefficients.
    * @param d - Array containing the denominator coefficients.
    * @param a_nr_coefs -
    * @param a_history - a persistent data containing the last (a_nr_coefs - 1) values of the original input data, and output data, respectively.
    * The caller is responsible to allocate at least (a_nr_coefs - 1) * 2 elements for the a_history array.
    */
    void IIRFilter(double* x, const unsigned int a_len, const double* n, double* d, const unsigned int a_nr_coefs, double* a_history)
    {
        if (a_nr_coefs > 1)
        {
            double* y = new double[a_len];
            switch (a_nr_coefs)
            {
            case 2:
                y[0] = n[0]*x[0] + n[1]*a_history[0] - d[1]*a_history[1];
                for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                    y[i] = n[0]*x[i] + n[1]*x[i-1] - d[1]*y[i-1];
                a_history[0] = x[a_len-1];
                a_history[1] = y[a_len-1];
                break;
            case 3:
                y[0] = n[0]*x[0] + n[1]*a_history[0] + n[2]*a_history[2] - d[1]*a_history[1] - d[2]*a_history[3];
                y[1] = n[0]*x[1] + n[1]*x[0] + n[2]*a_history[0] - d[1]*y[0] - d[2]*a_history[1];
                for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                    y[i] = n[0]*x[i] + n[1]*x[i-1] + n[2]*x[i-2] - d[1]*y[i-1] - d[2]*y[i-2];
                a_history[0] = x[a_len-1];
                a_history[2] = x[a_len-2];
                a_history[1] = y[a_len-1];
                a_history[3] = y[a_len-2];
                break;
            case 4:
                y[0] = n[0]*x[0] + n[1]*a_history[0] + n[2]*a_history[2] + n[3]*a_history[4] - d[1]*a_history[1] - d[2]*a_history[3] - d[3]*a_history[5];
                y[1] = n[0]*x[1] + n[1]*x[0] + n[2]*a_history[0] + n[3]*a_history[2] - d[1]*y[0] - d[2]*a_history[1] - d[3]*a_history[3];
                y[2] = n[0]*x[2] + n[1]*x[1] + n[2]*x[0] + n[3]*a_history[0] - d[1]*y[1] - d[2]*y[0] - d[3]*a_history[1];
                for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                    y[i] = n[0]*x[i] + n[1]*x[i-1] + n[2]*x[i-2] + n[3]*x[i-3] - d[1]*y[i-1] - d[2]*y[i-2] - d[3]*y[i-3];
                a_history[0] = x[a_len-1];
                a_history[2] = x[a_len-2];
                a_history[4] = x[a_len-3];
                a_history[1] = y[a_len-1];
                a_history[3] = y[a_len-2];
                a_history[5] = y[a_len-3];
                break;
            case 5:
                y[0] = n[0]*x[0] + n[1]*a_history[0] + n[2]*a_history[2] + n[3]*a_history[4] + n[4]*a_history[6] - d[1]*a_history[1] - d[2]*a_history[3] - d[3]*a_history[5] - d[4]*a_history[7];
                y[1] = n[0]*x[1] + n[1]*x[0] + n[2]*a_history[0] + n[3]*a_history[2] + n[4]*a_history[4] - d[1]*y[0] - d[2]*a_history[1] - d[3]*a_history[3] - d[4]*a_history[5];
                y[2] = n[0]*x[2] + n[1]*x[1] + n[2]*x[0] + n[3]*a_history[0] + n[4]*a_history[2] - d[1]*y[1] - d[2]*y[0] - d[3]*a_history[1] - d[4]*a_history[3];
                y[3] = n[0]*x[3] + n[1]*x[2] + n[2]*x[1] + n[3]*x[0] + n[4]*a_history[0] - d[1]*y[2] - d[2]*y[1] - d[3]*y[0] - d[4]*a_history[1];
                for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                    y[i] = n[0]*x[i] + n[1]*x[i-1] + n[2]*x[i-2] + n[3]*x[i-3] + n[4]*x[i-4] - d[1]*y[i-1] - d[2]*y[i-2] - d[3]*y[i-3] - d[4]*y[i-4];
                a_history[0] = x[a_len-1];
                a_history[2] = x[a_len-2];
                a_history[4] = x[a_len-3];
                a_history[6] = x[a_len-4];
                a_history[1] = y[a_len-1];
                a_history[3] = y[a_len-2];
                a_history[5] = y[a_len-3];
                a_history[7] = y[a_len-4];
                break;
            case 6:
                y[0] = n[0]*x[0] + n[1]*a_history[0] + n[2]*a_history[2] + n[3]*a_history[4] + n[4]*a_history[6] + n[5]*a_history[8] - d[1]*a_history[1] - d[2]*a_history[3] - d[3]*a_history[5] - d[4]*a_history[7] - d[5]*a_history[9];
                y[1] = n[0]*x[1] + n[1]*x[0] + n[2]*a_history[0] + n[3]*a_history[2] + n[4]*a_history[4] + n[5]*a_history[6] - d[1]*y[0] - d[2]*a_history[1] - d[3]*a_history[3] - d[4]*a_history[5] - d[5]*a_history[7];
                y[2] = n[0]*x[2] + n[1]*x[1] + n[2]*x[0] + n[3]*a_history[0] + n[4]*a_history[2] + n[5]*a_history[4] - d[1]*y[1] - d[2]*y[0] - d[3]*a_history[1] - d[4]*a_history[3] - d[5]*a_history[5];
                y[3] = n[0]*x[3] + n[1]*x[2] + n[2]*x[1] + n[3]*x[0] + n[4]*a_history[0] + n[5]*a_history[2] - d[1]*y[2] - d[2]*y[1] - d[3]*y[0] - d[4]*a_history[1] - d[5]*a_history[3];
                y[4] = n[0]*x[4] + n[1]*x[3] + n[2]*x[2] + n[3]*x[1] + n[4]*x[0] + n[5]*a_history[0] - d[1]*y[3] - d[2]*y[2] - d[3]*y[1] - d[4]*y[0] - d[5]*a_history[1];
                for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                    y[i] = n[0]*x[i] + n[1]*x[i-1] + n[2]*x[i-2] + n[3]*x[i-3] + n[4]*x[i-4] + n[5]*x[i-5] - d[1]*y[i-1] - d[2]*y[i-2] - d[3]*y[i-3] - d[4]*y[i-4] - d[5]*y[i-5];
                a_history[0] = x[a_len-1];
                a_history[2] = x[a_len-2];
                a_history[4] = x[a_len-3];
                a_history[6] = x[a_len-4];
                a_history[8] = x[a_len-5];
                a_history[1] = y[a_len-1];
                a_history[3] = y[a_len-2];
                a_history[5] = y[a_len-3];
                a_history[7] = y[a_len-4];
                a_history[9] = y[a_len-5];
                break;
            case 7:
                y[0] = n[0]*x[0] + n[1]*a_history[0] + n[2]*a_history[2] + n[3]*a_history[4] + n[4]*a_history[6] + n[5]*a_history[8] + n[6]*a_history[10] - d[1]*a_history[1] - d[2]*a_history[3] - d[3]*a_history[5] - d[4]*a_history[7] - d[5]*a_history[9] - d[6]*a_history[11];
                y[1] = n[0]*x[1] + n[1]*x[0] + n[2]*a_history[0] + n[3]*a_history[2] + n[4]*a_history[4] + n[5]*a_history[6] + n[6]*a_history[8] - d[1]*y[0] - d[2]*a_history[1] - d[3]*a_history[3] - d[4]*a_history[5] - d[5]*a_history[7] - d[6]*a_history[9];
                y[2] = n[0]*x[2] + n[1]*x[1] + n[2]*x[0] + n[3]*a_history[0] + n[4]*a_history[2] + n[5]*a_history[4] + n[6]*a_history[6] - d[1]*y[1] - d[2]*y[0] - d[3]*a_history[1] - d[4]*a_history[3] - d[5]*a_history[5] - d[6]*a_history[7];
                y[3] = n[0]*x[3] + n[1]*x[2] + n[2]*x[1] + n[3]*x[0] + n[4]*a_history[0] + n[5]*a_history[2] + n[6]*a_history[4] - d[1]*y[2] - d[2]*y[1] - d[3]*y[0] - d[4]*a_history[1] - d[5]*a_history[3] - d[6]*a_history[5];
                y[4] = n[0]*x[4] + n[1]*x[3] + n[2]*x[2] + n[3]*x[1] + n[4]*x[0] + n[5]*a_history[0] + n[6]*a_history[2] - d[1]*y[3] - d[2]*y[2] - d[3]*y[1] - d[4]*y[0] - d[5]*a_history[1] - d[6]*a_history[3];
                y[5] = n[0]*x[5] + n[1]*x[4] + n[2]*x[3] + n[3]*x[2] + n[4]*x[1] + n[5]*x[0] + n[6]*a_history[0] - d[1]*y[4] - d[2]*y[3] - d[3]*y[2] - d[4]*y[1] - d[5]*y[0] - d[6]*a_history[1];
                for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                    y[i] = n[0]*x[i] + n[1]*x[i-1] + n[2]*x[i-2] + n[3]*x[i-3] + n[4]*x[i-4] + n[5]*x[i-5] + n[6]*x[i-6] - d[1]*y[i-1] - d[2]*y[i-2] - d[3]*y[i-3] - d[4]*y[i-4] - d[5]*y[i-5] - d[6]*y[i-6];
                a_history[0] = x[a_len-1];
                a_history[2] = x[a_len-2];
                a_history[4] = x[a_len-3];
                a_history[6] = x[a_len-4];
                a_history[8] = x[a_len-5];
                a_history[10] = x[a_len-6];
                a_history[1] = y[a_len-1];
                a_history[3] = y[a_len-2];
                a_history[5] = y[a_len-3];
                a_history[7] = y[a_len-4];
                a_history[9] = y[a_len-5];
                a_history[11] = y[a_len-6];
                break;
            }
            memcpy(x, y, a_len * sizeof(double));
            delete[] y;
        }
    }

public:
    CIIRFilter(double* a_denominator, double* a_numerator, unsigned int a_nr_coefs)
        :m_denominator(a_denominator),
         m_numerator(a_numerator),
         m_nr_coefs(a_nr_coefs)
    {}

    virtual void ProcessData(double** a_data, unsigned int a_nr_channels, unsigned int* a_data_sizes, double** a_hystory_data, unsigned int a_nr_hystory_layers)
    {
        double l_history[(m_nr_coefs - 1) * 2];
        for (unsigned int i = 0; i < a_nr_channels; ++i)
        {
            for (unsigned int j = 0; j < m_nr_coefs - 1; ++j)
            {
                l_history[j*2] = a_hystory_data[j*2 + 1][i];
                l_history[j*2+1] = a_hystory_data[j*2 + 2][i];
            }
            IIRFilter(a_data[i], a_data_sizes[i], m_numerator, m_denominator, m_nr_coefs, l_history);
            for (unsigned int j = 0; j < m_nr_coefs - 1; ++j)
            {
                a_hystory_data[j*2 + 1][i] = l_history[j*2];
                a_hystory_data[j*2 + 2][i] = l_history[j*2+1];
            }
        }
    }
};

class CBasicProcsC: public CSignalProcessorBase
{
public:
    static CBasicProcsC& BasicProcsC()
    {
        static CBasicProcsC singleton_instance;
        return singleton_instance;
    }

    /** \brief Relays a changing source data (src) to a destination data (dst), potentially passing the actual increment through a processing unit.
    * @param a_src_dataname
    * @param a_history_dataname - The name of the data where the history of processing is stored.
    * The history data contains at least one row (channel).
    * The size of each row in the history data is equal to the number of channels in the source data.
    * Relaying history is stored in the 0th channel. Every value stored is the sample index where the processing/relaying was left (the last sample index in the source data which was already relayed).
    * Further channels in history data if present, are used by IRuntimeProcessor objects.
    * @param a_dst_dataname
    * @param a_dst_window_milliseconds
    * @param a_dst_codec_type
    * @param a_dst_filename
    * @param {integer list string} a_channels_to_relay - Optional. The channel indexes to relay from src to dst.
    * If set, dst will have as many channels as the number of integers in the list. Otherwise dst channel number will match the src channel number.
    * @param {IRuntimeProcessor} a_processor - Optional. If set, he data relayed will be piped through the a_processor.
    * @returns {string} Returns a new string according to standard signal processor interface.
    */
    char* DirectRelay_RT(char* a_src_dataname, char* a_history_dataname, char* a_dst_dataname, char* a_dst_window_milliseconds, char* a_dst_codec_type, char* a_dst_filename, char* a_channels_to_relay, IRuntimeProcessor* a_processor = 0)
    {
        char* res = 0;
        if (!a_src_dataname || !a_dst_dataname || !a_history_dataname)
            res = MakeString(m_newchar_proc, "ERROR: DirectRelay_RT: not enough arguments!");
        else
        {
            if (ISignalCodec* l_src_data = m_data_list_ref->datamap_find(a_src_dataname))
            {
                /** Search for destination data. Create it if not found. */
                ISignalCodec* l_dst_data = m_data_list_ref->datamap_find(a_dst_dataname);
                if (!l_dst_data)
                {
                    if (!a_channels_to_relay)
                        CallScript("NewFileDataBasedOnData(%s, %s, %s, %s, %s);", a_dst_dataname, a_src_dataname, a_dst_window_milliseconds, a_dst_codec_type, a_dst_filename);
                    else
                        CallScript("NewFileDataBasedOnData(%s, %s, %s, %s, %s, %s);", a_dst_dataname, a_src_dataname, a_dst_window_milliseconds, a_dst_codec_type, a_dst_filename, a_channels_to_relay);
                    l_dst_data = m_data_list_ref->datamap_find(a_dst_dataname);
                }
                unsigned int nr_src_channels = l_src_data->m_total_samples.m_size;

                /** Search for history data. Create it if not found. */
                CStringVec l_history_dataname;
                l_history_dataname.RebuildFrom(a_history_dataname);
                ISignalCodec* l_hist_data = m_data_list_ref->datamap_find(l_history_dataname.m_data[0].s);
                int l_nr_history_channels = 1;
                if (l_history_dataname.m_size == 2)
                    l_nr_history_channels = atoi(l_history_dataname.m_data[1].s);
                if (!l_hist_data)
                {
                    char l_sample_rates[l_nr_history_channels * 2 + 1];
                    l_sample_rates[0] = 0;
                    for (int i = 0; i < l_nr_history_channels; ++i)
                        strcat(l_sample_rates, "1 ");
                    CallScript("NewMemoryData(%s, %d, %s, %d);", l_history_dataname.m_data[0].s, l_nr_history_channels, (char*)l_sample_rates, nr_src_channels);
                    l_hist_data = m_data_list_ref->datamap_find(l_history_dataname.m_data[0].s);
                }

                /** Read history data, and get the data buffer with the last data indexes relayed. */
                CMatrix<double> l_hist_data_buff(l_nr_history_channels, nr_src_channels);
                unsigned int hist_start[l_nr_history_channels];
                unsigned int hist_nrelements[l_nr_history_channels];
                for (int i = 0; i < l_nr_history_channels; ++i)
                {
                    hist_start[i] = 0;
                    hist_nrelements[i] = nr_src_channels;
                }
                l_hist_data->GetDataBlock(l_hist_data_buff.m_data, hist_start, hist_nrelements);
                /** Convert double to unsigned int. */
                UIntVec l_hist_data_int_buff(nr_src_channels);
                for (unsigned int i = 0; i < nr_src_channels; ++i)
                    l_hist_data_int_buff.m_data[i] = l_hist_data_buff.m_data[0][i];

                /** Calculate source and destination chunk buffer sizes (widths) (the difference between the old and current src sizes). */
                unsigned int nr_dst_channels = nr_src_channels;
                UIntVec l_channels;
                if (a_channels_to_relay)
                {
                    l_channels.RebuildFrom(a_channels_to_relay);
                    nr_dst_channels = l_channels.m_size;
                }
                unsigned int buff_sizes[nr_src_channels];
                unsigned int dst_buff_sizes[nr_dst_channels];
                for (unsigned int i = 0; i < nr_src_channels; ++i)
                    buff_sizes[i] = l_src_data->m_total_samples.m_data[i] - l_hist_data_int_buff.m_data[i];
                unsigned int j = 0;
                for (unsigned int i = 0; i < nr_dst_channels; ++i)
                    if (l_channels.m_size)
                        dst_buff_sizes[j++] = buff_sizes[l_channels.m_data[i] - 1];
                    else
                        dst_buff_sizes[i] = buff_sizes[i];
                if (buff_sizes[0])
                {
                    int* enable = 0;
                    if (l_channels.m_size)
                    {
                        enable = new int[nr_src_channels];
                        memset(enable, 0, sizeof(int) * nr_src_channels);
                        for (unsigned int i = 0; i < nr_dst_channels; ++i)
                            enable[l_channels.m_data[i] - 1] = 1;
                    }
                    CMatrixVarLen<double> data_buffer(nr_dst_channels, dst_buff_sizes);
                    if (l_src_data->GetDataBlock(data_buffer.m_data, l_hist_data_int_buff.m_data, buff_sizes, enable))
                    {
                        /** process data */
                        if (a_processor)
                            a_processor->ProcessData(data_buffer.m_data, nr_dst_channels, buff_sizes, l_hist_data_buff.m_data, l_nr_history_channels);
                        /** update dst data */
                        l_dst_data->AppendSamples(data_buffer.m_data, buff_sizes[0]);
                        /** update src position data */
                        for (unsigned int i = 0; i < nr_src_channels; ++i)
                            l_hist_data_buff.m_data[0][i] += buff_sizes[i];
                        l_hist_data->WriteDataBlock(l_hist_data_buff.m_data, hist_start, hist_nrelements);
                        /** display data - todo: remove this after implementation ready */
                        CallScript("RefreshDataWindow(%s, true); RefreshDataWindow(%s, true);", l_dst_data->m_varname, l_hist_data->m_varname);
                    }
                    if (enable)
                        delete[] enable;
                }
            }
            else
                res = MakeString(m_newchar_proc, "ERROR: DirectRelay_RT: no source data found!");
        }
        return res;
    }

    /** \brief * Same parameters and return value as DirectRelay_RT except:
    * @param a_filter_dataname - The name of the filter data to be used as a signal processor when calling DirectRelay_RT.
    */
    char* IIRFilter_RT(char* a_src_dataname, char* a_history_dataname, char* a_dst_dataname, char* a_dst_window_milliseconds, char* a_dst_codec_type, char* a_dst_filename, char* a_channels_to_relay, char* a_filter_dataname)
    {
        char* res = 0;
        if (!a_src_dataname || !a_dst_dataname || !a_history_dataname || !a_filter_dataname)
            res = MakeString(m_newchar_proc, "ERROR: IIRFilter_RT: not enough arguments!");
        else
        {
            CVariable* l_filter_data = dynamic_cast<CVariable*>(m_data_list_ref->datamap_find(a_filter_dataname));
            if (l_filter_data)
            {
                CIIRFilter l_filter(l_filter_data->m_data[0], l_filter_data->m_data[1], l_filter_data->m_total_samples.m_data[0]);
                res = DirectRelay_RT(a_src_dataname, a_history_dataname, a_dst_dataname, a_dst_window_milliseconds, a_dst_codec_type, a_dst_filename, a_channels_to_relay, &l_filter);
            }
            else
                res = MakeString(m_newchar_proc, "ERROR: IIRFilter_RT: filter not found!");
        }
        return res;
    }
};

/** Dynamically loaded data processor module interface functions. */
extern "C"
{
    char* __declspec (dllexport) Procedure(int a_procindx, char* a_statement_body, char* a_param1, char* a_param2, char* a_param3, char* a_param4, char* a_param5, char* a_param6, char* a_param7, char* a_param8, char* a_param9, char* a_param10, char* a_param11, char* a_param12)
    {
        switch (a_procindx)
        {
        case 0:
            return CBasicProcsC::BasicProcsC().DirectRelay_RT(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7);
        case 1:
            return CBasicProcsC::BasicProcsC().IIRFilter_RT(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7, a_param8);
        }
        return 0;
    }

    void __declspec (dllexport) InitLib(CSignalCodec_List* a_datalist, CVariable_List* a_variablelist, NewCVariable_Proc a_newCVariable, NewChar_Proc a_newChar, Call_Proc a_inpCall, FFT_PROC a_inpFFT, RFFT_PROC a_inpRFFT, FFTEXEC_PROC a_inpFFTEXEC, FFTDESTROY_PROC a_inpFFTDESTROY)
    {
        CBasicProcsC::BasicProcsC().initialize(a_datalist, a_variablelist, a_newCVariable, a_newChar, a_inpCall, a_inpFFT, a_inpRFFT, a_inpFFTEXEC, a_inpFFTDESTROY);
    }

    int __declspec (dllexport) GetProcedureList(TFunctionLibrary* a_functionlibrary_reference)
    {
        CStringVec FunctionList;
        FunctionList.AddElement("DirectRelay_RT(a_src_dataname'the input data', a_history_dataname'last positions where the processing was left', a_dst_dataname'the output', window_milliseconds'the output file timewindow',codec_type'the output codec type', filename'the output filename', a_channels_to_relay'channel indexes to relay')");
        FunctionList.AddElement("IIRFilter_RT(a_src_dataname'the input data', a_history_dataname'last positions where the processing was left', a_dst_dataname'the output', window_milliseconds'the output file timewindow',codec_type'the output codec type', filename'the output filename', a_channels_to_relay'channel indexes to relay', a_filter_dataname)");
        a_functionlibrary_reference->ParseFunctionList(&FunctionList);
        return FunctionList.m_size;
    }

    bool __declspec (dllexport) CopyrightInfo(CStringMx* a_copyrightinfo)
    {
        a_copyrightinfo->RebuildPreserve(a_copyrightinfo->m_size + 1);
        a_copyrightinfo->m_data[a_copyrightinfo->m_size - 1]->AddElement("This library is a part of the DB's basic signal processing libraries.");
        return 0;
    }

    bool __stdcall DllMain(int hInst, int reason, int reserved)
    {
        return true;
    }
}
