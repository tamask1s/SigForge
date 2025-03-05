//#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <locale>
#include <chrono>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "minvect.h"
#include "datastructures.h"
#include "math.h"
#include "ZaxJsonParser.h"

CVariable_List*    m_variable_list_ref = 0; /// reference for the application's variable list.
CSignalCodec_List* m_data_list_ref = 0;     /// reference for the application's data list.
NewChar_Proc       NewChar;                 /// function call for application's "new char[n]" constructor. Strings that are going to be deleted by other parts of the system, needs to be allocated with this.
NewCVariable_Proc  NewCVariable;            /// function call for application's "new CVariable" constructor. Variables that are going to be deleted by other parts of the system (added to m_variable_list_ref), needs to be allocated with this.
Call_Proc          Call;                    /// function implemented by the application. Executes a script in the current running script's context / sandbox.

/** functions loaded by the application and passed to signal processor modules. */
FFT_PROC        FFT;
RFFT_PROC       RFFT;
FFTEXEC_PROC    FFTEXEC;
FFTDESTROY_PROC FFTDESTROY;

#define HANNING_FACTOR 2.00501

struct GenericVarChannelJSON
{
    string label;
    string unit_v;
    string sampling_rate;
    vector<double> data;
    ZAX_JSON_SERIALIZABLE(GenericVarChannelJSON, JSON_PROPERTY(label), JSON_PROPERTY(sampling_rate), JSON_PROPERTY(unit_v), JSON_PROPERTY(data))
};

struct GenericVarJSON
{
    string unit_h;
    vector<GenericVarChannelJSON> channels;
    ZAX_JSON_SERIALIZABLE(GenericVarJSON, JSON_PROPERTY(unit_h), JSON_PROPERTY(channels))
};

char* DataInput(char* a_in_data_name, char* a_var_name_to_store, char* a_active_channels = 0)
{
    if (!a_in_data_name || !a_var_name_to_store)
        return MakeString(NewChar, "ERROR: DataInput: while importing '", a_in_data_name, "' to '", a_var_name_to_store, "': not enough arguments!");

    if (ISignalCodec* found_data = m_data_list_ref->datamap_find(a_in_data_name))
    {
        if (a_active_channels)
        {
            unsigned int max_channel = found_data->m_total_samples.m_size;
            char copy_of_a_active_channels[strlen(a_active_channels)];
            sprintf(copy_of_a_active_channels, a_active_channels);
            char *token = strtok(copy_of_a_active_channels, " ");
            while (token != nullptr)
            {
                unsigned int channel_number = strtoul(token, NULL, 10);
                if ((channel_number > max_channel) || (channel_number == 0))
                    return MakeString(NewChar, "ERROR: DataInput: Invalid channel number: ", token);

                token = strtok(nullptr, " ");
            }
        }
        CVariable* var_to_store = NewCVariable();
        var_to_store->CopyFrom(found_data, a_var_name_to_store, a_active_channels);
        if (var_to_store)
            m_variable_list_ref->Insert(var_to_store->m_varname, var_to_store);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: DataInput: while importing '", a_in_data_name, "' to '", a_var_name_to_store, "': can't find data in datalist: '", a_in_data_name, "'");
}

bool is_equal_tolerance(const double lhs, const double rhs, const double tolerance)
{
    if (lhs - rhs <= tolerance && rhs - lhs <= tolerance)
        return true;
    else
        return false;
}

char* IsEqual(char* a_indataname, char* a_indataname2, char* a_tolerance)
{
    if (!a_indataname || !a_indataname2)
        return MakeString(NewChar, "ERROR: IsEqual: not enough arguments!");
    ISignalCodec* data = m_data_list_ref->datamap_find(a_indataname);
    ISignalCodec* data2 = m_data_list_ref->datamap_find(a_indataname2);
    if (!data && !data2)
    {
        char* result = NewChar(16);
        sprintf(result, "RESULT:%s", strcmp(a_indataname, a_indataname2) ? "0" : "1");
        return result;
    }
    else if (data && data2)
    {
        double tolerance = 0;
        if (a_tolerance && strlen(a_tolerance))
            tolerance = atof(a_tolerance);
        bool isequal = true;
        unsigned int nrchannels = data->m_total_samples.m_size;
        unsigned int nrchannels2 = data2->m_total_samples.m_size;
        if (nrchannels != nrchannels2)
            isequal = false;
        else
        {
            for (unsigned int i = 0; i < nrchannels; ++i)
                if (data->m_total_samples.m_data[i] != data2->m_total_samples.m_data[i])
                {
                    isequal = false;
                    break;
                }
            if (isequal)
            {
                unsigned int starts[nrchannels];
                for (unsigned int i = 0; i < nrchannels; ++i)
                    starts[i] = 0;

                CMatrixVarLen<double>datt, datt2;
                datt.Rebuild(nrchannels, data->m_total_samples.m_data);
                data->GetDataBlock(datt.m_data, starts, data->m_total_samples.m_data);

                datt2.Rebuild(nrchannels, data->m_total_samples.m_data);
                data2->GetDataBlock(datt2.m_data, starts, data->m_total_samples.m_data);

                for (unsigned int i = 0; i < nrchannels; ++i)
                {
                    if (!tolerance)
                    {
                        for (unsigned int j = 0; j < data->m_total_samples.m_data[i]; ++j)
                            if (datt.m_data[i][j] != datt2.m_data[i][j])
                            {
                                isequal = false;
                                break;
                            }
                    }
                    else
                    {
                        for (unsigned int j = 0; j < data->m_total_samples.m_data[i]; ++j)
                            if (!is_equal_tolerance(datt.m_data[i][j],datt2.m_data[i][j], tolerance))
                            {
                                isequal = false;
                                break;
                            }
                    }
                }
            }
        }

        char* result = NewChar(16);
        sprintf(result, "RESULT:%s", isequal ? "1" : "0");
        return result;
    }
    else
        return MakeString(NewChar, "ERROR: IsEqual: can't find data in datalist.");
}

char* AlterSignal(char* a_dataname, char* a_value)
{
    if (!a_dataname || !a_value)
        return MakeString(NewChar, "ERROR: AlterSignal: not enough arguments!");

    double l_value = atof(a_value);
    if (ISignalCodec* data = m_data_list_ref->datamap_find(a_dataname))
    {
        unsigned int nrchannels = data->m_total_samples.m_size;
        unsigned int starts    [nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
            starts[i] = 0;

        CMatrixVarLen<double>datt;
        datt.Rebuild(nrchannels, data->m_total_samples.m_data);
        data->GetDataBlock(datt.m_data, starts, data->m_total_samples.m_data);

        for (unsigned int i = 0; i < nrchannels; ++i)
            for (unsigned int j = 0; j < data->m_total_samples.m_data[i]; ++j)
                datt.m_data[i][j] += l_value;

        data->WriteDataBlock(datt.m_data, starts, data->m_total_samples.m_data);
    }
    else
        return MakeString(NewChar, "ERROR: AlterSignal: Can't find data in datalist: '", a_dataname, "'");
    return 0;
}

char* CreateVector(char* a_outdataname, char* a_list_of_elements)
{
    if (!a_outdataname || !a_list_of_elements)
        return MakeString(NewChar, "ERROR: CreateVector: not enough arguments!");
    DoubleVec dv;
    dv.RebuildFrom(a_list_of_elements);
    UIntVec Nrsamples;

    Nrsamples.Rebuild(1);
    Nrsamples.m_data[0] = dv.m_size;

    CVariable* output2 = NewCVariable();
    output2->Rebuild(Nrsamples.m_size, Nrsamples.m_data);

    output2->m_sample_rates.m_data[0] = 1;
    for (unsigned int i = 0; i < output2->m_widths.m_data[0]; ++i)
        output2->m_data[0][i] = dv.m_data[i];

    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(output2->m_varname, output2);
    return 0;
}

void CreateVariable_(char* a_outdataname, unsigned int a_nr_channels, unsigned int* a_nrsamples, double* a_samplerates)
{
    CVariable* output2 = NewCVariable();
    output2->Rebuild(a_nr_channels, a_nrsamples);

    for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
    {
        output2->m_sample_rates.m_data[i] = a_samplerates[i];
        for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
            output2->m_data[i][j] = 0;
    }
    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(output2->m_varname, output2);
}

char* CreateVariable(char* a_outdataname, char* a_nrsamples, char* a_samplerates)
{
    char* l_result = nullptr;
    if (!a_outdataname || !a_nrsamples || !a_samplerates)
        l_result = MakeString(NewChar, "ERROR: CreateVariable: not enough arguments");
    if (!l_result)
    {
        UIntVec Nrsamples;
        DoubleVec Samplerates;

        Nrsamples.RebuildFrom(a_nrsamples);
        Samplerates.RebuildFrom(a_samplerates);

        CreateVariable_(a_outdataname, Nrsamples.m_size, Nrsamples.m_data, Samplerates.m_data);
    }
    return l_result;
}

char* AppendSine(char* a_data_name, char* a_nr_samples, char* a_frequencies)
{
    int nr_samples = atoi(a_nr_samples);
    if (ISignalCodec* data = m_data_list_ref->datamap_find(a_data_name))
    {
        unsigned int nr_channels = data->m_total_samples.m_size;
        UIntVec Channels; // What is the purpose of this variable?
        DoubleVec Frequencies;
        Channels.Rebuild(nr_channels);
        Frequencies.RebuildFrom(a_frequencies);
        CMatrixVarLen<double> data_to_append;
        unsigned int new_widths[nr_channels];
        for (unsigned int i = 0; i < nr_channels; ++i)
            new_widths[i] = (nr_samples * data->m_total_samples.m_data[i]) / data->m_total_samples.m_data[0];
        data_to_append.Rebuild(nr_channels, new_widths);
        for (unsigned int i = 0; i < nr_channels; ++i)
        {
            for (unsigned int j = 0; j < new_widths[i]; ++j)
            {
                data_to_append.m_data[i][j] = sin((double)j / data->m_sample_rates.m_data[i] * 2.0 * M_PI * Frequencies.m_data[0]) * 100.0;
                for (unsigned int k = 1; k < Frequencies.m_size; ++k)
                    data_to_append.m_data[i][j] += sin((double)j / data->m_sample_rates.m_data[i] * 2.0 * M_PI * Frequencies.m_data[k]) * 100.0;
            }
        }
        data->AppendSamples(data_to_append.m_data, nr_samples);
    }
    else
        return MakeString(NewChar, "ERROR: AppendSine: Can't find data in datalist: '", a_data_name, "'");
    return 0;
}

char* InputFirstData(char* a_output_var_name, char* a_active_channels = 0)
{
    char* l_error = nullptr;
    if (!a_output_var_name)
        l_error = MakeString(NewChar, "ERROR: InputFirstData: while importing first data to '", a_output_var_name, "': not enough arguments!");
    if (!m_data_list_ref->size())
        l_error = MakeString(NewChar, "ERROR: InputFirstData: while importing first data to '", a_output_var_name, "': no data in datalist");
    else
    {
        if (a_active_channels)
        {
            unsigned l_channel_number = (unsigned)atoi(a_active_channels);
            if (1 > l_channel_number || l_channel_number > m_data_list_ref->begin()->second->m_total_samples.m_size)
                l_error = MakeString(NewChar, "ERROR: InputFirstData: not a valid channel number: ", a_active_channels);
        }
    }
    if (!l_error)
    {
        CVariable* l_datatostore = NewCVariable();
        l_datatostore->CopyFrom(m_data_list_ref->begin()->second, a_output_var_name, a_active_channels);
        if (l_datatostore)
            m_variable_list_ref->Insert(l_datatostore->m_varname, l_datatostore);
    }
    return l_error;
}

char* IfStatement(char* a_statement_body, char* a_param)
{
    char* res = 0;
    if (!a_param || !strlen(a_param) || !strcmp(a_param, "0") || !strcmp(a_param, "false"))
        return 0;
    if (!a_statement_body)
        res = MakeString(NewChar, "ERROR: IfStatement: no script to execute!");
    else if (atoi(a_param) || !strcmp(a_param, "true"))
        Call(a_statement_body);
    return res;
}

char* ForStatement(char* a_statement_body, char* a_iterator_var_name, char* a_from, char* a_to, char* a_step)
{
    if (!a_iterator_var_name || !a_from || !a_to)
        return MakeString(NewChar, "ERROR: ForStatement: Not enough arguments");

    CVariable* iter = NewCVariable();
    unsigned int tmp[1] = {1};
    iter->Rebuild(1, tmp);
    strcpy(iter->m_varname, a_iterator_var_name);
    m_variable_list_ref->Insert(iter->m_varname, iter);

    int to = atoi(a_to);
    double step = atof(a_step);
    if (!step)
        step = 1;

    for (iter->m_data[0][0] = atoi(a_from); iter->m_data[0][0] <= to; iter->m_data[0][0] += step)
        Call(a_statement_body);

    m_variable_list_ref->erase(iter->m_varname);
    delete iter;

    return 0;
}

char* Iterator(char* a_iterator_varname)
{
    if (!a_iterator_varname)
        return MakeString(NewChar, "ERROR: Iterator: Not enough arguments");

    if (CVariable* l_iter = m_variable_list_ref->variablemap_find(a_iterator_varname))
    {
        char tmp[32] = "RESULT: ";
        sprintf(tmp + 8, "%f", l_iter->m_data[0][0]);
        return MakeString(NewChar, tmp);
    }
    else
        return MakeString(NewChar, "ERROR: Iterator: iterator vatiable not found");
}

char* CatStrings(char* a_1, char* a_2, char* a_3, char* a_4, char* a_5, char* a_6, char* a_7, char* a_8)
{
    if (!a_1 || !a_2)
        return MakeString(NewChar, "ERROR: FormatString: Not enough arguments");

    char tmp[4096] = "RESULT: ";
    strcat(tmp, a_1);
    strcat(tmp, a_2);
    if (a_3)
        strcat(tmp, a_3);
    if (a_4)
        strcat(tmp, a_4);
    if (a_5)
        strcat(tmp, a_5);
    if (a_6)
        strcat(tmp, a_6);
    if (a_7)
        strcat(tmp, a_7);
    if (a_8)
        strcat(tmp, a_8);
    return MakeString(NewChar, tmp);
}

char* Filterx(char* a_src, char* a_dst)
{
    if (!a_src || !a_dst)
        return MakeString(NewChar, "ERROR: Filterx: not enugh arguments!");
    if (/*ISignalCodec* data = */m_data_list_ref->datamap_find(a_dst))
        if (/*ISignalCodec* data2 = */m_data_list_ref->datamap_find(a_src))
        {
            CVariable* datatostore = NewCVariable();
            if (datatostore)
                m_variable_list_ref->Insert(datatostore->m_varname, datatostore);
            return 0;
        }
    return 0;
}

char* GetDataName(char* a_indx)
{
    return MakeString(NewChar, "DATASERIES0");
//    IntVec Indx;
//    Indx.RebuildFrom(a_indx);
//    if (!Indx.m_size)
//        return MakeString(NewChar, "ERROR: GetFirstDataName: no valid indx specified");
//    if (m_data_list_ref->m_size > (unsigned int)Indx.m_data[0])
//        return MakeString(NewChar, "RESULT:", m_data_list_ref->m_data[Indx.m_data[0]]->m_varname);
//    else
//        return MakeString(NewChar, "ERROR: GetFirstDataName: indx is too big.");
}

char* CleanupCodec(char* a_in_data_name)
{
    if (!a_in_data_name)
        return MakeString(NewChar, "ERROR: CleanupCodec: not enough argument");
    if (ISignalCodec* data = m_data_list_ref->datamap_find(a_in_data_name))
    {
        unsigned int starts    [data->m_total_samples.m_size];
        unsigned int nrelements[data->m_total_samples.m_size];
        int achs[data->m_total_samples.m_size];
        for (unsigned int i = 0; i < data->m_total_samples.m_size; ++i)
        {
            starts[i] = 0;
            if (data->m_total_samples.m_data[i])
                nrelements[i] = 1;
            else
                nrelements[i] = 0;
            achs[i] = 1;
        }
        CMatrix<double>datt;
        datt.Rebuild(data->m_total_samples.m_size, 2);
        data->GetDataBlock(datt.m_data, starts, nrelements, achs);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: CleanupCodec: can't find data in the datalist: '", a_in_data_name, "'");
}

char* CleanupFirstCodec(char* a_dummy_argument)
{
    if (m_data_list_ref->size())
    {
        ISignalCodec* data = m_data_list_ref->begin()->second;
        unsigned int starts    [data->m_total_samples.m_size];
        unsigned int nrelements[data->m_total_samples.m_size];
        int achs[data->m_total_samples.m_size];
        for (unsigned int i = 0; i < data->m_total_samples.m_size; ++i)
        {
            starts[i] = 0;
            if (data->m_total_samples.m_data[i])
                nrelements[i] = 1;
            else
                nrelements[i] = 0;
            achs[i] = 1;
        }
        CMatrix<double>datt;
        datt.Rebuild(data->m_total_samples.m_size, 2);
        data->GetDataBlock(datt.m_data, starts, nrelements, achs);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: CleanupCodec: m_data_list_ref empty");
}

char* DeleteData(char* a_invarname)
{
    if (!a_invarname)
        return MakeString(NewChar, "WARNING: DeleteData: while deleting '", a_invarname, "': not enough arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        m_variable_list_ref->erase(var->m_varname);
        delete var;
        return 0;
    }
    else
        return MakeString(NewChar, "WARNING: DeleteData: while deleting '", a_invarname, "': can't find data in datalist: '", a_invarname, "'");
}

/**
The output will not be a power spectrum density! The units of the output will match with the units of the fft's input,
ie if the units were mVolts, the spectrum's units will be mVolts too.
The output is normalized such as an 1mV sine wave in the input will result in an 1mV peak in the spectrum.
a_cx: a complex double vector; size=N+2 double numbers, (ie N/2+1 complex numbers), (but only N doubles are really used: indxs from 1 to N).;
a_spectrum: a real double vector; size=N/2+1 double numbers.
a_cx: the 0th element is the DC component's real part, the 1st element is it's imag part (not used).;
a_cx: the Nth element is the Nyquist component's real part, the N+1th is it's imag part (not used, that's why only N+1 doubles are used in a_cx).;
a_spectrum: the 0th element is the DC component.
a_spectrum: the Nth element is the Nyquist component.
*/

/** beálitási lehetoségek. Kell ennyi vajon? Kell még?
- psd
- psdn
- amp
- ampn
- dbi */
static inline void GetSpectrum(double* a_cx, double* a_spectrum, int N, double a_window_ratio /** example: hanning window = 2.0 */, char* a_spectrum_representation, const char* a_orig_vert_units = 0, const char* a_orig_hor_units = 0, char* a_taget_vert_units = 0)
{
    static int const AMPN = 1852861793; /// "ampn"
    static int const PSD = 6583152;     /// "psd"
    static int const PSDN = 1852076912; /// "psdn"
    static int const DBI = 6906468;     /// "dbi"
    static int const AMP = 7368033;     /// "amp"
    if (!a_spectrum_representation)
        a_spectrum_representation = (char*)"ampn";
    int n = N >> 1;
    switch (*((int*)a_spectrum_representation))
    {
    case AMPN: /** normalizált amplitudó. mértékegység azonos, de kell a 2.0? eleve n = N / 2. matlab mond valamit? */
    {
        double RNP2 = 2.0 / n * a_window_ratio;
        a_spectrum[0] = fabs(a_cx[0]) * RNP2;
        for (int i = 1; i < n ; ++i)
            a_spectrum[i] = sqrt(a_cx[i << 1] * a_cx[i << 1] + a_cx[(i << 1) + 1] * a_cx[(i << 1) + 1]) * RNP2;
        a_spectrum[n] = fabs(a_cx[N]) * RNP2;
        if (a_orig_vert_units && a_taget_vert_units)
            strcpy(a_taget_vert_units, a_orig_vert_units);
    }
    break;
    case AMP: /** kell ez vajon egyáltalán? mondjuk picit gyorsabb, mert nincs szorzás. */
        a_spectrum[0] = fabs(a_cx[0]);
        for (int i = 1; i < n ; ++i)
            a_spectrum[i] = sqrt(a_cx[i << 1] * a_cx[i << 1] + a_cx[(i << 1) + 1] * a_cx[(i << 1) + 1]);
        a_spectrum[n] = fabs(a_cx[N]);
        if (a_orig_vert_units && a_taget_vert_units)
            strcpy(a_taget_vert_units, a_orig_vert_units);
    break;
    case PSDN: /** power sprectrum density normalized. Van itt egyáltalán értelme normalizálásnak? Matlab mond valamit? Vagy te? :) */
    {
        double RNP2 = 2.0 / n * a_window_ratio; /** csak agyatlanul átmásoltam az ampn-bol, de ennek gondolom nincs sok értelme. vagy? */
        a_spectrum[0] = a_cx[0] * a_cx[0] * RNP2;
        for (int i = 1; i < n ; ++i)
            a_spectrum[i] = a_cx[i << 1] * a_cx[i << 1] + a_cx[(i << 1) + 1] * a_cx[(i << 1) + 1] * RNP2;
        a_spectrum[n] = a_cx[N] * a_cx[N] * RNP2;
        if (a_orig_vert_units && a_orig_hor_units && a_taget_vert_units)
            sprintf(a_taget_vert_units, "(%s)^2*%s", a_orig_vert_units, a_orig_hor_units);
    }
    break;
    case PSD: /** power sprectrum density. Az ok, hogy ilyen nagyok a számok? Mértékegység? (mondjuk mértékegységet meg tudom nézni) */
        a_spectrum[0] = a_cx[0] * a_cx[0];
        for (int i = 1; i < n ; ++i)
            a_spectrum[i] = a_cx[i << 1] * a_cx[i << 1] + a_cx[(i << 1) + 1] * a_cx[(i << 1) + 1];
        a_spectrum[n] = a_cx[N] * a_cx[N];
        if (a_orig_vert_units && a_orig_hor_units && a_taget_vert_units)
            sprintf(a_taget_vert_units, "(%s)^2*%s", a_orig_vert_units, a_orig_hor_units);
    break;
    case DBI: /** Ez jó igy? Matlab? Te? :) Mértékegység? */ /** ref: https://stackoverflow.com/questions/32276728/plotting-frequency-spectrum-with-c */
        a_spectrum[0] = 20.0 * log(fabs(a_cx[0])) / N;
        for (int i = 1; i < n; ++i)
            a_spectrum[i] = (20.0 * log(sqrt(a_cx[i << 1] * a_cx[i << 1] + a_cx[(i << 1) + 1] * a_cx[(i << 1) + 1]))) / N;
        a_spectrum[n] = 20.0 * log(fabs(a_cx[N])) / N;
        if (a_orig_vert_units && a_taget_vert_units)
            sprintf(a_taget_vert_units, "log10(%s)", a_orig_vert_units);
    break;
    }
}

int GetSmallestPowOfTwo(int a_val)
{
    int l_val = 1;
    while(l_val < a_val)
        l_val <<= 1;
    return l_val;
}

double* NewHanning(unsigned int n)
{
    double nm1 = 2.0 * M_PI / (n - 1);
    double* hanning = new double [n];
    for (unsigned int k = 0; k < n + 0; ++k)
        hanning[k - 0] = (0.5 - 0.5 * cos((double)k * nm1));
    return hanning;
}

char* Hanning(char* a_varnametostore, char* a_samples)
{
    UIntVec nrsamples;
    nrsamples.RebuildFrom(a_samples);
    if (nrsamples.m_size && a_varnametostore)
    {
        CVariable* output2 = NewCVariable();
        output2->Rebuild(nrsamples.m_size, nrsamples.m_data);
        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            double nm1 = 2.0 * M_PI / (nrsamples.m_data[i] - 1);
            for (unsigned int j = 0; j < output2->m_widths.m_data[i] + 0; ++j)
                output2->m_data[i][j - 0] = (0.5 - 0.5 * cos((double)j * nm1)); //*1.027;//!!!1.027-nemtommerkell
        }
        strcpy(output2->m_varname, a_varnametostore);
        m_variable_list_ref->Insert(output2->m_varname, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Hanning: not enough arguments");
}

char* Spectrum_RT(char* a_srcdataname, char* a_nrhunits, char* a_outvarname, char* a_spectrum_representation)
{
    if (ISignalCodec* input = m_data_list_ref->datamap_find(a_srcdataname))
    {
        unsigned int nrchannels = input->m_total_samples.m_size;
        unsigned int interval[nrchannels]; /// number of samples corresponding to ininterval seconds
        int fftinterval[nrchannels];
        DoubleVec ininterval;
        ininterval.RebuildFrom(a_nrhunits);

        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            interval[i] = (int)(input->m_sample_rates.m_data[i] * ininterval.m_data[i]);
            fftinterval[i] = interval[i];
            fftinterval[i] = GetSmallestPowOfTwo((int)fftinterval[i]);
        }
        unsigned int outputwidths[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
            outputwidths[i] = (int)(fftinterval[i] / 2) + 1;
        CVariable* output = NewCVariable();
        output->Rebuild(nrchannels, outputwidths);

        unsigned int starts[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
            starts[i] = input->m_total_samples.m_data[i] - interval[i];
        CMatrixVarLen<double>inbuff;
        inbuff.Rebuild(nrchannels, input->m_total_samples.m_data);
        bool res = input->GetDataBlock(inbuff.m_data, starts, interval);

        for (unsigned int i = 0; i < nrchannels; ++i)
            if (res && interval[i])
            {
                double* fft_in = new double [fftinterval[i] * 2];
                memset(fft_in, 0, fftinterval[i] * 2 * sizeof(double));
                double* fft_out = new double [fftinterval[i] + 2];
                memset(fft_out, 0, (fftinterval[i] + 2)*sizeof(double));
                if (!RFFT)
                {
                    printf("FFT Library 'libfftw3-3.dll' must be located near 'SigForge.exe'\n");
                    return 0;
                }
                int* FFTplan2 = RFFT(fftinterval[i], fft_in, fft_out, FFTW_ESTIMATE ); // FFTW_MEASURE FFTW_PATIENT FFTW_ESTIMATE
                double* hanning = NewHanning(interval[i]);

                for (unsigned int k = 0; k < interval[i]; ++k)
                    fft_in[k] = (inbuff.m_data[i][k]) * hanning[k];

                FFTEXEC(FFTplan2);
                double factor = (double)fftinterval[i] / (double)interval[i] * HANNING_FACTOR;
                GetSpectrum(fft_out, output->m_data[i], fftinterval[i], factor, a_spectrum_representation);

                delete[] hanning;
                FFTDESTROY(FFTplan2);
                delete[]fft_in;
                delete[]fft_out;
            }

        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            output->m_sample_rates.m_data[i] = (((double)fftinterval[i] / 2.0) + 0.0) / ((input->m_sample_rates.m_data[i] / 2.0 + 0.0));
            strcpy(output->m_vertical_units.m_data[i].s, input->m_vertical_units.m_data[i].s);
            strcpy(output->m_labels.m_data[i].s, input->m_labels.m_data[i].s);
            strcpy(output->m_transducers.m_data[i].s, input->m_transducers.m_data[i].s);
        }
        sprintf(output->m_horizontal_units, "1/");
        strcat(output->m_horizontal_units, input->m_horizontal_units);
        strcpy(output->m_varname, a_outvarname);
        m_variable_list_ref->Insert(output->m_varname, output);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Spectrum_RT: Can't find data in datalist: '", a_srcdataname, "'");
    return 0;
}

char* SpectrumTimeline_RT(char* a_srcdataname, char* a_nrhunits, char* a_dstdataname, char* a_spectrumbounds, char* a_spectrum_representation)
{
    DoubleVec spectrumbounds;
    spectrumbounds.RebuildFrom(a_spectrumbounds);

    if (ISignalCodec* input = m_data_list_ref->datamap_find(a_srcdataname))
    {
        unsigned int nrchannels = input->m_total_samples.m_size;

        unsigned int interval[nrchannels]; /// number of samples corresponding to ininterval seconds
        int fftinterval[nrchannels];
        DoubleVec ininterval;
        ininterval.RebuildFrom(a_nrhunits);

        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            interval[i] = (int)(input->m_sample_rates.m_data[i] * ininterval.m_data[i]);
            fftinterval[i] = interval[i];
            fftinterval[i] = GetSmallestPowOfTwo((int)fftinterval[i]);
        }
        unsigned int outputwidths[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
            outputwidths[i] = (int)(fftinterval[i] / 2) + 1;

        ISignalCodec* output[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            output[i] = 0;
            char l_outdatname[strlen(a_dstdataname) + 20];
            strcpy(l_outdatname, a_dstdataname);
            if (nrchannels > 1)
                sprintf(l_outdatname + strlen(l_outdatname), "%d", i);

            if (ISignalCodec* l_output = m_data_list_ref->datamap_find(l_outdatname))
            {
                output[i] = l_output;
            }
            else
            {
                CVariable* output_ = NewCVariable();
                unsigned int specturm_size = (fftinterval[i] / 2) + 1;
                unsigned int output_sizes[specturm_size];
                for (unsigned int x = 0; x < specturm_size; x++)
                    output_sizes[x] = 1;
                int spectrum_freq_start = (int)spectrumbounds.m_data[0];
                int spectrum_freq_stop = (int)spectrumbounds.m_data[1];
                output_->Rebuild(spectrum_freq_stop - spectrum_freq_start, output_sizes);

                for (unsigned int x = 0; x < output_->m_total_samples.m_size; x++)
                {
                    output_->m_sample_rates.m_data[x] = ((double)input->m_sample_rates.m_data[i]) / (input->m_sample_rates.m_data[i] * ininterval.m_data[i]);
                    strcpy(output_->m_vertical_units.m_data[x].s, "1/");
                    strcat(output_->m_vertical_units.m_data[x].s, input->m_horizontal_units);
                }

                strcpy(output_->m_surface2D_vert_units, "1/");
                strcat(output_->m_surface2D_vert_units, input->m_horizontal_units);
                output_->m_surface2D_vert_axis_max = (double)spectrum_freq_start;// * 2.0 * (double)interval[i] / (double)fftinterval[i] - 1;
                output_->m_surface2D_vert_axis_min = (double)spectrum_freq_stop;// * 2.0 * (double)interval[i] / (double)fftinterval[i];
                cout << "spectrum_freq_start: " << spectrum_freq_start << endl;
                cout << "spectrum_freq_stop: " << spectrum_freq_stop << endl;

                cout << "fftinterval[i]: " << fftinterval[i] << endl;
                cout << "interval[i]: " << interval[i] << endl;

                double max_frequency = input->m_sample_rates.m_data[i] / 2.0;
                //output_->m_surface2D_vert_axis_min = (double) max_frequency * (double)interval[i] / ((double)fftinterval[i] - 1);
                //output_->m_surface2D_vert_axis_max = (double)0;

                output_->m_surface2D_vert_axis_min = (double)spectrum_freq_stop * (double)interval[i] / ((double)fftinterval[i] - 1);
                output_->m_surface2D_vert_axis_max = (double)0;

                cout << "max_frequency: " << max_frequency << endl;
                cout << "m_surface2D_vert_axis_min: " << output_->m_surface2D_vert_axis_min << endl;
                cout << "m_surface2D_vert_axis_max: " << output_->m_surface2D_vert_axis_max << endl;

                strcpy(output_->m_horizontal_units, input->m_horizontal_units);
                strcpy(output_->m_varname, l_outdatname);
                m_variable_list_ref->Insert(output_->m_varname, output_);

                char l_display_script[200];
                sprintf(l_display_script, "DisplayData(%s, fit_width, 2D_map);", l_outdatname);
                Call(l_display_script);
                output[i] = m_data_list_ref->datamap_find(l_outdatname);
            }
            if (!output[i])
            {
                cout << "no output! " << endl;
                return 0;
            }
        }

        unsigned int starts[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
            starts[i] = input->m_total_samples.m_data[i] - interval[i];
        CMatrixVarLen<double>inbuff;
        inbuff.Rebuild(nrchannels, input->m_total_samples.m_data);
        bool res = input->GetDataBlock(inbuff.m_data, starts, interval);

        for (unsigned int i = 0; i < nrchannels; ++i)
            if (res && interval[i])
            {
                double* fft_in = new double [fftinterval[i] * 2];
                memset(fft_in, 0, fftinterval[i] * 2 * sizeof(double));
                double* fft_out = new double [fftinterval[i] + 2];
                memset(fft_out, 0, (fftinterval[i] + 2)*sizeof(double));
                if (!RFFT)
                {
                    printf("FFT Library 'libfftw3-3.dll' must be located near 'SigForge.exe'\n");
                    return 0;
                }
                int* FFTplan2 = RFFT(fftinterval[i], fft_in, fft_out, FFTW_ESTIMATE); // FFTW_MEASURE FFTW_PATIENT FFTW_ESTIMATE
                double* hanning = NewHanning(interval[i]);

                for (unsigned int k = 0; k < interval[i]; ++k)
                    fft_in[k] = (inbuff.m_data[i][k]) * hanning[k];

                FFTEXEC(FFTplan2);
                double spectrm_out[outputwidths[i]];
                double factor = (double)fftinterval[i] / (double)interval[i] * HANNING_FACTOR;
                GetSpectrum(fft_out, spectrm_out, fftinterval[i], factor, a_spectrum_representation);

                CMatrix<double> data_to_append;
                data_to_append.Rebuild(outputwidths[i], 1);
                int spectrum_freq_start = (int)spectrumbounds.m_data[0];
                int spectrum_freq_stop = (int)spectrumbounds.m_data[1];
                for (int k = spectrum_freq_start; k < spectrum_freq_stop; ++k)
                    data_to_append.m_data[k - spectrum_freq_start][0] = spectrm_out[k];

                output[i]->AppendSamples(data_to_append.m_data, 1);

                char l_display_script[200];
                sprintf(l_display_script, "RefreshDataWindow (%s, true);", output[i]->m_varname);
                Call(l_display_script);

                //cout << "AppendSamples" << endl;

                delete[] hanning;
                FFTDESTROY(FFTplan2);
                delete[]fft_in;
                delete[]fft_out;
            }
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: SpectrumTimeline_RT: Can't find data in datalist: '", a_srcdataname, "'");
    return 0;
}

CVariable* MSpectrum(CVariable* a_input, char* a_outdatname, double* a_ininterval, double* a_infftinterval, int* a_step, int* a_activechannels, char* a_spectrum_representation)
{
    unsigned int nrchannels = a_input->m_total_samples.m_size;
    int interval[nrchannels]; /// number of samples corresponding to ininterval seconds
    int fftinterval[nrchannels];

    for (unsigned int i = 0; i < nrchannels; ++i)
    {
        interval[i] = (int)(a_input->m_sample_rates.m_data[i] * a_ininterval[i]);
        fftinterval[i] = (int)(a_input->m_sample_rates.m_data[i] * a_infftinterval[i]);
        fftinterval[i] = GetSmallestPowOfTwo((int)fftinterval[i]);
    }
    unsigned int outputwidths[nrchannels];
    for (unsigned int i = 0; i < nrchannels; ++i)
    {
        outputwidths[i] = ((int)(a_input->m_total_samples.m_data[i] / interval[i])) * ((int)(fftinterval[i] / 2) + 1);
    }
    CVariable* output = new CVariable();
    output->Rebuild(nrchannels, outputwidths);
    for (unsigned int i = 0; i < nrchannels; ++i)
        if (interval[i])
        {
            double* fft_in = new double [fftinterval[i] * 2];
            memset(fft_in, 0, fftinterval[i] * 2 * sizeof(double));
            double* fft_out = new double [fftinterval[i] + 2];
            memset(fft_out, 0, (fftinterval[i] + 2)*sizeof(double));
            if (!RFFT)
            {
                printf("FFT Library 'libfftw3-3.dll' must be located near 'SigForge.exe'\n");
                return 0;
            }
            int* FFTplan2 = RFFT(fftinterval[i], fft_in, fft_out, FFTW_ESTIMATE ); // FFTW_MEASURE FFTW_PATIENT FFTW_ESTIMATE
            double* hanning = NewHanning(interval[i]);

            for (int j = 0; j < ((int)(a_input->m_total_samples.m_data[i] / interval[i])); ++j)
            {
                for (int k = 0; k < interval[i]; ++k)
                    fft_in[k] = ((a_input->m_data[i] + j * interval[i])[k]) /** hanning[k]*/;

                FFTEXEC(FFTplan2);
                double factor = (double)fftinterval[i] / (double)interval[i] * HANNING_FACTOR;
                GetSpectrum(fft_out, output->m_data[i] + j * ((int)(fftinterval[i] / 2) + 1), fftinterval[i], factor, a_spectrum_representation, a_input->m_vertical_units.m_data[i].s, a_input->m_horizontal_units, output->m_vertical_units.m_data[i].s);
            }
            delete[] hanning;
            FFTDESTROY(FFTplan2);
            delete[]fft_in;
            delete[]fft_out;
        }

    for (unsigned int i = 0; i < output->m_total_samples.m_size; ++i)
    {
        output->m_sample_rates.m_data[i] = ((int)(fftinterval[i] / 2) + 1);
        strcpy(output->m_labels.m_data[i].s, a_input->m_labels.m_data[i].s);
        strcpy(output->m_transducers.m_data[i].s, a_input->m_transducers.m_data[i].s);
    }
    /*----------------------------MEAN------------------------------*/
    CVariable* output2 = NewCVariable();
    unsigned int output2sizes[output->m_total_samples.m_size];
    for (unsigned int i = 0; i < output->m_total_samples.m_size; ++i)
        output2sizes[i] = ((int)(fftinterval[i] / 2) + 1);
    output2->Rebuild(nrchannels, output2sizes);
    for (unsigned int i = 0; i < nrchannels; ++i)
        if (fftinterval[i])
        {
            for (int j = 0; j < ((int)(a_input->m_total_samples.m_data[i] / interval[i])); ++j)
            {
                for (int k = 0; k < ((int)(fftinterval[i] / 2) + 1); ++k)
                    output2->m_data[i][k] += output->m_data[i][j * ((int)(fftinterval[i] / 2) + 1) + k];
            }
            for (int k = 0; k < ((int)(fftinterval[i] / 2) + 1); ++k)
                output2->m_data[i][k] = output2->m_data[i][k] / ((double)((int)(a_input->m_total_samples.m_data[i] / interval[i])));
        }
    for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
    {
        output2->m_sample_rates.m_data[i] = (((double)fftinterval[i] / 2.0) + 0.0) / ((a_input->m_sample_rates.m_data[i] / 2.0 + 0.0));
        cout << "fftinterval[i]: " << fftinterval[i] << " a_input->m_sample_rates.m_data[i]:" << a_input->m_sample_rates.m_data[i] << endl;
        strcpy(output2->m_vertical_units.m_data[i].s, output->m_vertical_units.m_data[i].s);
        strcpy(output2->m_labels.m_data[i].s, a_input->m_labels.m_data[i].s);
        strcpy(output2->m_transducers.m_data[i].s, a_input->m_transducers.m_data[i].s);
    }
    sprintf(output2->m_horizontal_units, "1/");
    strcat(output2->m_horizontal_units, a_input->m_horizontal_units);
    strcpy(output2->m_varname, a_outdatname);
    delete output;
    return output2;
}

CVariable* FullSpectrum(CVariable* a_input, char* a_outdatname, double* a_ininterval, double* a_spectrumbounds, int* a_step, int* a_activechannels, char* a_spectrum_representation)
{
    unsigned int nrchannels = a_input->m_total_samples.m_size;
    int interval[nrchannels]; /// number of samples corresponding to ininterval seconds
    int fftinterval[nrchannels];

    for (unsigned int i = 0; i < nrchannels; ++i)
    {
        interval[i] = (int)(a_input->m_sample_rates.m_data[i] * a_ininterval[i]);
        fftinterval[i] = interval[i];
        fftinterval[i] = GetSmallestPowOfTwo((int)fftinterval[i]);
    }
    unsigned int outputwidths[nrchannels];
    for (unsigned int i = 0; i < nrchannels; ++i)
        outputwidths[i] = ((int)(a_input->m_total_samples.m_data[i] / interval[i])) * ((int)(fftinterval[i] / 2) + 1);

    CVariable* output = new CVariable();
    output->Rebuild(nrchannels, outputwidths);

    for (unsigned int i = 0; i < nrchannels; ++i)
        if (interval[i])
        {
            double* fft_in = new double [fftinterval[i] * 2];
            memset(fft_in, 0, fftinterval[i] * 2 * sizeof(double));
            double* fft_out = new double [fftinterval[i] + 2];
            memset(fft_out, 0, (fftinterval[i] + 2)*sizeof(double));
            if (!RFFT)
            {
                printf("FFT Library 'libfftw3-3.dll' must be located near 'SigForge.exe'\n");
                return 0;
            }
            int* FFTplan2 = RFFT(fftinterval[i], fft_in, fft_out, FFTW_ESTIMATE); // FFTW_MEASURE FFTW_PATIENT FFTW_ESTIMATE
            double* hanning = NewHanning(interval[i]);

            int nr_spectrums = a_input->m_total_samples.m_data[i] / interval[i];
            for (int j = 0; j < nr_spectrums; ++j)
            {
                for (int k = 0; k < interval[i]; ++k)
                    fft_in[k] = ((a_input->m_data[i] + j * interval[i])[k]) * hanning[k];

                FFTEXEC(FFTplan2);
                double factor = (double)fftinterval[i] / (double)interval[i] * HANNING_FACTOR;
                GetSpectrum(fft_out, &output->m_data[i][j * ((int)(fftinterval[i] / 2) + 1)], fftinterval[i], factor, a_spectrum_representation);
            }
            delete[] hanning;
            FFTDESTROY(FFTplan2);
            delete[]fft_in;
            delete[]fft_out;
        }

    for (unsigned int i = 0; i < nrchannels; ++i)
    {
        output->m_sample_rates.m_data[i] = ((int)(fftinterval[i] / 2) + 1);
        strcpy(output->m_vertical_units.m_data[i].s, a_input->m_vertical_units.m_data[i].s);
        strcpy(output->m_labels.m_data[i].s, a_input->m_labels.m_data[i].s);
        strcpy(output->m_transducers.m_data[i].s, a_input->m_transducers.m_data[i].s);
    }

    /*----------------------------Creating output wavelet------------------------------*/

    for (unsigned int i = 0; i < nrchannels; ++i)
    {
        CVariable* output_ = NewCVariable();
        unsigned int specturm_size = (fftinterval[i] / 2) + 1;
        unsigned int output_sizes[specturm_size];
        int nr_spectrums = a_input->m_total_samples.m_data[i] / interval[i];
        for (unsigned int x = 0; x < specturm_size; x++)
            output_sizes[x] = nr_spectrums;

        if (a_spectrumbounds)
        {
            a_spectrumbounds[0] *= a_ininterval[i];
            a_spectrumbounds[1] *= a_ininterval[i];
        }

        int spectrum_freq_start = a_spectrumbounds ? (int)a_spectrumbounds[0] : 0;
        int spectrum_freq_stop = a_spectrumbounds ? (int)a_spectrumbounds[1] : specturm_size;
        output_->Rebuild(spectrum_freq_stop - spectrum_freq_start, output_sizes);

        if (fftinterval[i])
            for (int j = 0; j < nr_spectrums; ++j)
            {
                int j_x_specturm_size = j * specturm_size;
                for (int k = spectrum_freq_start; k < spectrum_freq_stop; ++k)
                    output_->m_data[k - spectrum_freq_start][j] = output->m_data[i][j_x_specturm_size + k];
            }

        for (unsigned int x = 0; x < output_->m_total_samples.m_size; x++)
        {
            output_->m_sample_rates.m_data[x] = ((double)a_input->m_sample_rates.m_data[i]) / (a_input->m_sample_rates.m_data[i] * a_ininterval[i]);
            strcpy(output_->m_vertical_units.m_data[x].s, "1/");
            strcat(output_->m_vertical_units.m_data[x].s, a_input->m_vertical_units.m_data[i].s);

            strcpy(output_->m_vertical_units.m_data[x].s, "1/");
            strcat(output_->m_vertical_units.m_data[x].s, a_input->m_horizontal_units);

            //if (a_input->m_labels.m_data[i].s)
            //    printf("%s\n", a_input->m_labels.m_data[i].s);
            // strcpy(output_->m_labels.m_data[i].s,a_input->m_labels.m_data[i].s);
        }

        strcpy(output_->m_surface2D_vert_units, "1/");
        strcat(output_->m_surface2D_vert_units, a_input->m_horizontal_units);
        //output_->Surface2DVertAxisChannelIndexRatio = (spectrum_freq_stop - spectrum_freq_start) / a_input->m_sample_rates.m_data[i];
        //output_->Surface2DVertAxisChannelIndexRatio = (a_input->m_sample_rates.m_data[i] / 2) * ((spectrum_freq_stop - spectrum_freq_start) / a_input->m_sample_rates.m_data[i]);
        //output_->m_surface2D_vert_axis_max = (double)spectrum_freq_start * 2.0 * (double)interval[i] / (double)fftinterval[i] - 1;
        //output_->m_surface2D_vert_axis_min = (double)spectrum_freq_stop * 2.0 * (double)interval[i] / (double)fftinterval[i];

        output_->m_surface2D_vert_axis_max = (double)spectrum_freq_start * (double)interval[i] / ((double)fftinterval[i] - 0.0) * (1.0 / a_ininterval[i]);
        output_->m_surface2D_vert_axis_min = (double)spectrum_freq_stop *  (double)interval[i] / ((double)fftinterval[i] - 0.0) * (1.0 / a_ininterval[i]);

        strcpy(output_->m_horizontal_units, a_input->m_horizontal_units);
        char outdatname_[strlen(a_outdatname) + 20];
        strcpy(outdatname_, a_outdatname);
        if (nrchannels > 1)
            sprintf(outdatname_ + strlen(outdatname_), "%d", i);
        strcpy(output_->m_varname, outdatname_);
        m_variable_list_ref->Insert(output_->m_varname, output_);
    }
    delete output;
    return 0;
}

char* Spectrogram(char* a_input, char* a_channel, char* a_output, char* a_block_len, char* a_spectrumbounds, char* a_overlap, char* a_spectrum_presentation)
{
    if (!a_input || !a_channel || !a_output || !a_block_len)
        return MakeString(NewChar, "ERROR: Spectrogram: while making spectrum from: '", a_input, "' to '", a_output, "': not enough arguments!");

    double block_len = atof(a_block_len);
    unsigned int channel = atoi(a_channel);
    double overlap = 0;
    if (a_overlap && strlen(a_overlap))
        overlap = atof(a_overlap);

    if (CVariable* input = m_variable_list_ref->variablemap_find(a_input))
    {
        if (channel >= input->m_total_samples.m_size)
            return MakeString(NewChar, "ERROR: Spectrogram: while making spectrum from: '", a_input, "' channel index overflow '", a_channel, ".");

        unsigned int orig_block_samples = input->m_sample_rates.m_data[channel] * block_len;
        unsigned int block_samples = GetSmallestPowOfTwo(orig_block_samples);
        if (((int)(block_samples * (100 - overlap))) % 100)
            return MakeString(NewChar, "ERROR: Spectrogram: while making spectrum from: '", a_input, "' invalid overlapping value '", a_overlap, "'.");
        unsigned int overlap_samples_increment = ((int)(block_samples * (100 - overlap))) / 100;
        unsigned int nr_spectrums = input->m_total_samples.m_data[channel] / overlap_samples_increment;
        unsigned int specturm_size = (block_samples / 2) + 1;
        unsigned int outputwidths[1];
        outputwidths[0] = nr_spectrums * specturm_size;

        CVariable* tempvar = new CVariable();
        tempvar->Rebuild(1, outputwidths);

        {
            double* fft_in = new double [block_samples * 2];
            memset(fft_in, 0, block_samples * 2 * sizeof(double));
            double* fft_out = new double [block_samples + 2];
            memset(fft_out, 0, (block_samples + 2)*sizeof(double));
            if (!RFFT)
            {
                printf("FFT Library 'libfftw3-3.dll' must be located near 'SigForge.exe'\n");
                return 0;
            }
            int* FFTplan2 = RFFT(block_samples, fft_in, fft_out, FFTW_ESTIMATE); /// FFTW_MEASURE FFTW_PATIENT FFTW_ESTIMATE
            double* hanning = NewHanning(block_samples);

            for (unsigned int j = 0; j < nr_spectrums - block_samples / overlap_samples_increment + 1; ++j)
            {
                for (unsigned int k = 0; k < block_samples; ++k)
                    fft_in[k] = ((input->m_data[channel] + j * overlap_samples_increment)[k]) * hanning[k];
                FFTEXEC(FFTplan2);
                GetSpectrum(fft_out, &tempvar->m_data[0][j * specturm_size], block_samples, HANNING_FACTOR, a_spectrum_presentation);
            }

            delete[] hanning;
            FFTDESTROY(FFTplan2);
            delete[]fft_in;
            delete[]fft_out;
        }

        {
            tempvar->m_sample_rates.m_data[0] = specturm_size;
            strcpy(tempvar->m_vertical_units.m_data[0].s, input->m_vertical_units.m_data[channel].s);
            strcpy(tempvar->m_labels.m_data[0].s, input->m_labels.m_data[channel].s);
            strcpy(tempvar->m_transducers.m_data[0].s, input->m_transducers.m_data[channel].s);
        }

        /**---------------------------- Creating output ------------------------------*/
        {
            CVariable* output = NewCVariable();
            unsigned int output_sizes[specturm_size];
            for (unsigned int x = 0; x < specturm_size; x++)
                output_sizes[x] = nr_spectrums;

            int spectrum_freq_start = 0;
            int spectrum_freq_stop = specturm_size;

            if (a_spectrumbounds)
            {
                DoubleVec spectrumbounds;
                spectrumbounds.RebuildFrom(a_spectrumbounds);

                spectrum_freq_start = spectrumbounds.m_data[0] * block_len;
                spectrum_freq_stop = spectrumbounds.m_data[1] * block_len;
            }

            output->Rebuild(spectrum_freq_stop - spectrum_freq_start, output_sizes);
            for (unsigned int j = 0; j < nr_spectrums - block_samples / overlap_samples_increment + 1; ++j)
            {
                int j_x_specturm_size = j * specturm_size;
                for (int k = spectrum_freq_start; k < spectrum_freq_stop; ++k)
                    output->m_data[k - spectrum_freq_start][j] = tempvar->m_data[0][j_x_specturm_size + k];
            }

            for (unsigned int x = 0; x < output->m_total_samples.m_size; x++)
            {
                output->m_sample_rates.m_data[x] = ((double)input->m_sample_rates.m_data[channel]) / (input->m_sample_rates.m_data[channel] * block_len) * (double)block_samples / (double)overlap_samples_increment;
                strcpy(output->m_vertical_units.m_data[x].s, "1/");
                strcat(output->m_vertical_units.m_data[x].s, input->m_vertical_units.m_data[channel].s);

                strcpy(output->m_vertical_units.m_data[x].s, "1/");
                strcat(output->m_vertical_units.m_data[x].s, input->m_horizontal_units);
            }

            strcpy(output->m_surface2D_vert_units, "1/");
            strcat(output->m_surface2D_vert_units, input->m_horizontal_units);

            output->m_surface2D_vert_axis_max = (double)spectrum_freq_start * (double)orig_block_samples / (double)block_samples / block_len;
            output->m_surface2D_vert_axis_min = (double)spectrum_freq_stop *  (double)orig_block_samples / (double)block_samples / block_len;

            strcpy(output->m_horizontal_units, input->m_horizontal_units);
            strcpy(output->m_varname, a_output);
            m_variable_list_ref->Insert(output->m_varname, output);
        }
        delete tempvar;
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Spectrogram: variable not found: '", a_input);
}

char* CopyForAllChannels(char* a_indataname, char* a_nrhunits)
{
    if (!a_indataname || !a_nrhunits)
        return MakeString(NewChar, "ERROR: CopyForAllChannels: not enough arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        DoubleVec dnrhunits;
        dnrhunits.RebuildFrom(a_nrhunits);
        if (dnrhunits.m_size == 1)
        {
            DoubleVec samplenrs;
            samplenrs.Rebuild(var->m_total_samples.m_size);
            for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
                samplenrs.m_data[i] = dnrhunits.m_data[0];
            char* temres = samplenrs.ToString(" ");
            char* result = NewChar(strlen(temres) + 10);
            sprintf(result, "%s%s", "RESULT:", temres);
            delete[]temres;
            return result;
        }
        else
            return MakeString(NewChar, "ERROR: CopyForAllChannels: '", a_indataname, "': invalid argument: '", a_nrhunits, "'");
    }
    else
        return MakeString(NewChar, "ERROR: CopyForAllChannels: can't find variable in variablelist: '", a_indataname, "'");
}

char* DateAndTime(char* a_prefix, char* a_suffix)
{
    char* l_result = nullptr;
    time_t l_time_now = time(nullptr);
    char l_time[20]; /// 20 is calculated from "%Y-%m-%d_%H-%M-%S" time format
    if (strftime(l_time, sizeof(l_time), "%Y-%m-%d_%H-%M-%S", localtime(&l_time_now)))
    {
        string l_time_fmt = l_time;
        if (a_prefix)
            l_time_fmt = a_prefix + l_time_fmt;

        if (a_suffix)
            l_time_fmt = l_time_fmt + a_suffix;

        l_time_fmt = "RESULT:" + l_time_fmt;
        l_result = MakeString(NewChar, l_time_fmt.c_str());
    }
    else
        MakeString(NewChar, "ERROR: DateAndTime: can't get date and time from the system");
    return l_result;
}

char* TimeElapsed(char* a_last_time)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_last_time))
    {
        long long now = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
        var->m_data[0][1] = now - var->m_data[0][0];
        var->m_data[0][0] = now;
    }
    else
    {
        CVariable* output = NewCVariable();
        unsigned int output_sizes[1];
        output_sizes[0] = 2;
        output->Rebuild(1, output_sizes);
        output->m_data[0][0] = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
        output->m_data[0][1] = 0;
        strcpy(output->m_varname, a_last_time);
        m_variable_list_ref->Insert(output->m_varname, output);
    }
    return 0;
}

char* Print(char* a_input_var_name, char* y, char* x)
{
    if (!a_input_var_name)
        return MakeString(NewChar, "ERROR: Print: Not enough argument");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_input_var_name))
    {
        if (!y && !x)
        {
            ZaxJsonParser::set_indent(4);
            GenericVarJSON jsonobj;
            jsonobj.channels.resize(var->m_total_samples.m_size);
            for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            {
                jsonobj.channels[i].data.resize(var->m_total_samples.m_data[i]);
                jsonobj.channels[i].label = var->m_labels.m_data[i].s;
                for (unsigned int j = 0; j < var->m_total_samples.m_data[i]; ++j)
                    jsonobj.channels[i].data[j] = var->m_data[i][j];
            }
            cout << a_input_var_name << ": " << jsonobj << endl;
        }
        else if (y && !x)
        {
            ZaxJsonParser::set_indent(4);
            GenericVarChannelJSON channel;
            unsigned int i = atoi(y);
            channel.data.resize(var->m_total_samples.m_data[i]);
            channel.label = var->m_labels.m_data[i].s;
            for (unsigned int j = 0; j < var->m_total_samples.m_data[i]; ++j)
                channel.data[j] = var->m_data[i][j];
            cout << a_input_var_name << ".channels[" << i << "]: " << channel << endl;
        }
        else if (y && x)
            cout << a_input_var_name << "[" << atoi(y) << "][" << atoi(x) << "]: " << var->m_data[atoi(y)][atoi(x)] << endl;
        ZaxJsonParser::set_indent(0);
    }
    else
        return MakeString(NewChar, "ERROR: Print: can't find data in variablelist: '", a_input_var_name, "'");
    return 0;
}

char* MakeMSprctrum(char* a_indataname, char* a_outdataname, char* a_nrhunits, char* a_nrffthunits, bool a_mean, char* a_spectrum_representation)
{
    if (!a_indataname || !a_outdataname || !a_nrhunits)
        return MakeString(NewChar, "ERROR: MeanSpectrum: while making spectrum from: '", a_indataname, "' to '", a_outdataname, "': not enough arguments!");

    DoubleVec dnrhunits;
    dnrhunits.RebuildFrom(a_nrhunits);
    DoubleVec dnrffthunits;
    if (a_nrffthunits)
        dnrffthunits.RebuildFrom(a_nrffthunits);
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        if (dnrhunits.m_size < var->m_total_samples.m_size)
            return MakeString(NewChar, "ERROR: MeanSpectrum: while making spectrum from: '", a_indataname, "' to '", a_outdataname, "': not enough nrhunits given in argument 3:", a_nrhunits);
        if (a_nrffthunits)
            if (dnrffthunits.m_size < var->m_total_samples.m_size)
                return MakeString(NewChar, "ERROR: MeanSpectrum: while making spectrum from: '", a_indataname, "' to '", a_outdataname, "': not enough nrffthunits given in argument 4:", a_nrffthunits);

        CVariable* spectrum;
        if (a_mean)
            spectrum = MSpectrum(var, a_outdataname, dnrhunits.m_data, dnrffthunits.m_data, 0, 0, a_spectrum_representation);
        else
            spectrum = FullSpectrum(var, a_outdataname, dnrhunits.m_data, a_nrffthunits ? dnrffthunits.m_data : 0, 0, 0, a_spectrum_representation);
        if (spectrum)
            m_variable_list_ref->Insert(spectrum->m_varname, spectrum);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: MeanSpectrum: while making spectrum from: '", a_indataname, "' to '", a_outdataname, "': can't find data in variablelist: '", a_indataname, "'");
}

char* Padding(char* a_varname, char* a_prepad, char* a_prepad_val, char* a_postpad, char* a_postpad_val)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_varname))
    {
        double prepad_hu = 0;
        if (a_prepad && strlen(a_prepad))
            prepad_hu = atof(a_prepad);
        double prepad_val = 10000000000;
        if (a_prepad_val && strlen(a_prepad_val))
            prepad_val = atof(a_prepad_val);
        double postpad_hu = 0;
        if (a_postpad && strlen(a_postpad))
            postpad_hu = atof(a_postpad);
        double postpad_val = 10000000000;
        if (a_postpad_val && strlen(a_postpad_val))
            postpad_val = atof(a_postpad_val);

        unsigned int nrchannels = var->m_total_samples.m_size;
        unsigned int new_sizes[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            int prepad_samples = prepad_hu * var->m_sample_rates.m_data[i];
            int postpad_samples = postpad_hu * var->m_sample_rates.m_data[i];
            new_sizes[i] = prepad_samples + postpad_samples + var->m_total_samples.m_data[i];
        }

        CVariable* output = NewCVariable();
        output->Rebuild(nrchannels, new_sizes);
        var->SCopyTo(output);
        strcpy(output->m_varname, var->m_varname);

        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            int prepad_samples = prepad_hu * var->m_sample_rates.m_data[i];
            int postpad_samples = postpad_hu * var->m_sample_rates.m_data[i];
            double current_prepad_val = prepad_val;
            double current_postpad_val = postpad_val;
            if (current_prepad_val == 10000000000)
                current_prepad_val = var->m_data[i][0];
            if (current_postpad_val == 10000000000)
                current_postpad_val = var->m_data[i][var->m_total_samples.m_data[i] - 2];
            for (int j = 0; j < prepad_samples; ++j)
                output->m_data[i][j] = current_prepad_val;
            for (unsigned int j = 0; j < var->m_total_samples.m_data[i]; ++j)
                output->m_data[i][j + prepad_samples] = var->m_data[i][j];
            for (int j = 0; j < postpad_samples; ++j)
                output->m_data[i][j + prepad_samples + var->m_total_samples.m_data[i]] = current_postpad_val;
        }

        delete var;
        (*m_variable_list_ref)[output->m_varname] = output;

        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Padding: variable not found: '", a_varname, "'.");
}

char* Truncate(char* a_varname, char* a_pretruncate, char* a_posttruncate)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_varname))
    {
        double pretruncate_hu = 0;
        if (a_pretruncate && strlen(a_pretruncate))
            pretruncate_hu = atof(a_pretruncate);
        double posttruncate_hu = 0;
        if (a_posttruncate && strlen(a_posttruncate))
            posttruncate_hu = atof(a_posttruncate);

        unsigned int nrchannels = var->m_total_samples.m_size;
        unsigned int new_sizes[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            int pretruncate_samples = pretruncate_hu * var->m_sample_rates.m_data[i];
            int posttruncate_samples = posttruncate_hu * var->m_sample_rates.m_data[i];
            new_sizes[i] = var->m_total_samples.m_data[i] - pretruncate_samples - posttruncate_samples;
        }

        CVariable* output = NewCVariable();
        output->Rebuild(nrchannels, new_sizes);
        var->SCopyTo(output);
        strcpy(output->m_varname, var->m_varname);

        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            int pretruncate_samples = pretruncate_hu * var->m_sample_rates.m_data[i];
            int posttruncate_samples = posttruncate_hu * var->m_sample_rates.m_data[i];
            for (unsigned int j = 0; j < var->m_total_samples.m_data[i] - pretruncate_samples - posttruncate_samples; ++j)
                output->m_data[i][j] = var->m_data[i][j + pretruncate_samples];
        }

        delete var;
        (*m_variable_list_ref)[output->m_varname] = output;

        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Truncate: variable not found: '", a_varname, "'.");
}

char* SaveJSON(char* a_input_var_name, char* a_file_name)
{
    if (!a_input_var_name || !a_file_name)
        return MakeString(NewChar, "ERROR: SaveJSON: Not enough argument");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_input_var_name))
    {
        //ZaxJsonParser::set_indent(4);
        GenericVarJSON jsonobj;
        jsonobj.channels.resize(var->m_total_samples.m_size);
        jsonobj.unit_h = var->m_horizontal_units;
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
        {
            jsonobj.channels[i].data.resize(var->m_total_samples.m_data[i]);
            jsonobj.channels[i].label = var->m_labels.m_data[i].s;
            jsonobj.channels[i].unit_v = var->m_vertical_units.m_data[i].s;
            char buf[32];
            sprintf(buf, "%.2f", (float)var->m_sample_rates.m_data[i]);
            jsonobj.channels[i].sampling_rate = buf;
            for (unsigned int j = 0; j < var->m_total_samples.m_data[i]; ++j)
                jsonobj.channels[i].data[j] = var->m_data[i][j];
        }

        string jsonstr = jsonobj;
        char* str1 = (char*)jsonstr.c_str();
        if (str1)
        {
            SaveBuffer(a_file_name, (unsigned char*)str1, strlen(str1), 0, true);
            delete[] str1;
            return 0;
        }
        else
            return MakeString(NewChar, "ERROR: SaveJSON: can't create string from data to save file");
    }
    else
        return MakeString(NewChar, "ERROR: SaveJSON: can't find data in variablelist: '", a_input_var_name, "'");
}

char* LoadJSON(char* a_out_var_name, char* asciifilename)
{
    int len;
    char* buff = (char*)LoadBuffer(asciifilename, 0, &len);
    if (buff)
    {
        bool is_multichannel = false;
        char chan_str[] = "chan";
        for (int i = 0; i < (len < 100 ? len : 100); ++i)
            if (*((int*)(buff + i)) == *((int*)(chan_str)))
            {
                is_multichannel = true;
                break;
            }

        CVariable* output = NewCVariable();
        if (is_multichannel)
        {
            GenericVarJSON tmp;
            tmp.zax_from_json(buff);
            unsigned int output_sizes[tmp.channels.size()];
            for (unsigned int i = 0; i < tmp.channels.size(); ++i)
                output_sizes[i] = tmp.channels[i].data.size();
            output->Rebuild(tmp.channels.size(), output_sizes);
            for (unsigned int i = 0; i < output->m_total_samples.m_size; ++i)
            {
                strcpy(output->m_labels.m_data[i].s, tmp.channels[i].label.c_str());
                for (unsigned int j = 0; j < output->m_total_samples.m_data[i]; ++j)
                    output->m_data[i][j] = tmp.channels[i].data[j];
            }
        }
        else
        {
            GenericVarChannelJSON tmp;
            tmp.zax_from_json(buff);
            unsigned int output_sizes[1];
            output_sizes[0] = tmp.data.size();
            output->Rebuild(1, output_sizes);
            strcpy(output->m_labels.m_data[0].s, tmp.label.c_str());
            for (unsigned int j = 0; j < output->m_total_samples.m_data[0]; ++j)
                output->m_data[0][j] = tmp.data[j];
        }
        strcpy(output->m_varname, a_out_var_name);
        m_variable_list_ref->Insert(output->m_varname, output);
        delete[] buff;
    }
    return 0;
}

extern "C"
{
    char* __declspec (dllexport) Procedure(int a_procindx, char* a_statement_body, char* a_param1, char* a_param2, char* a_param3, char* a_param4, char* a_param5, char* a_param6, char* a_param7, char* a_param8, char* a_param9, char* a_param10, char* a_param11, char* a_param12)
    {
        switch (a_procindx)
        {
        case 0:
            return DataInput(a_param1, a_param2, a_param3);
        case 1:
            return MakeMSprctrum(a_param1, a_param2, a_param3, a_param4, true, a_param5);
        case 2:
            return CopyForAllChannels(a_param1, a_param2);
        case 3:
            return DeleteData(a_param1);
        case 4:
            return CleanupCodec(a_param1);
        case 5:
            return CleanupFirstCodec(a_param1);
        case 6:
            return InputFirstData(a_param1, a_param2);
        case 7:
            return GetDataName(a_param1);
        case 8:
            return Hanning(a_param1, a_param2);
        case 9:
            return IfStatement(a_statement_body, a_param1);
        case 10:
            return Filterx(a_param1, a_param2);
        case 11:
            return CreateVariable(a_param1, a_param2, a_param3);
        case 12:
            return MakeMSprctrum(a_param1, a_param2, a_param3, a_param4, false, a_param5);
        case 13:
            return AppendSine(a_param1, a_param2, a_param3);
        case 14:
            return CreateVector(a_param1, a_param2);
        case 15:
            return AlterSignal(a_param1, a_param2);
        case 16:
            return IsEqual(a_param1, a_param2, a_param3);
        case 17:
            return Spectrum_RT(a_param1, a_param2, a_param3, a_param4);
        case 18:
            return SpectrumTimeline_RT(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 19:
            return DateAndTime(a_param1, a_param2);
        case 20:
            return ForStatement(a_statement_body, a_param1, a_param2, a_param3, a_param4);
        case 21:
            return Iterator(a_param1);
        case 22:
            return CatStrings(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7, a_param8);
        case 23:
            return Spectrogram(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7);
        case 24:
            return Padding(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 25:
            return Truncate(a_param1, a_param2, a_param3);
        case 26:
            return LoadJSON(a_param1, a_param2);
        case 27:
            return SaveJSON(a_param1, a_param2);
        case 28:
            return TimeElapsed(a_param1);
        case 29:
            return Print(a_param1, a_param2, a_param3);
        }
        return 0;
    }

    void __declspec (dllexport) InitLib(CSignalCodec_List* a_datalist, CVariable_List* a_variablelist, NewCVariable_Proc a_newCVariable, NewChar_Proc a_newChar, Call_Proc a_inpCall, FFT_PROC a_inpFFT, RFFT_PROC a_inpRFFT, FFTEXEC_PROC a_inpFFTEXEC, FFTDESTROY_PROC a_inpFFTDESTROY)
    {
        m_variable_list_ref = a_variablelist;
        m_data_list_ref     = a_datalist;
        NewChar             = a_newChar;
        NewCVariable        = a_newCVariable;
        FFT                 = a_inpFFT;
        FFTEXEC             = a_inpFFTEXEC;
        RFFT                = a_inpRFFT;
        FFTDESTROY          = a_inpFFTDESTROY;
        Call                = a_inpCall;
    }

    int __declspec (dllexport) GetProcedureList(TFunctionLibrary* a_functionlibrary_reference)
    {
        CStringVec FunctionList;
        FunctionList.AddElement("DataIn(indataname'the input data', outputdataname'the output', activechannels'channels enable flag')");
        FunctionList.AddElement("MeanSpectrum(indataname'the input data', outputdataname'the output', nrhunits, nrfftunits'if bigger -> oversample')");
        FunctionList.AddElement("CopyForAllChannels(indataname'the input data', valuetocopy)");
        FunctionList.AddElement("DeleteData(indataname'the data to delete')");
        FunctionList.AddElement("CleanupCodec(invarname)"); /// The 'CleanupCodec' and 'CleanupFirstCodec' functions reset their internal buffers to a minimal size.
        FunctionList.AddElement("CleanupFirstCodec(dummy)");
        FunctionList.AddElement("InputFirstData(varnametostore, activechannels)");
        FunctionList.AddElement("GetDataName(indx)");
        FunctionList.AddElement("Hanning(varnametostore, samples)");
        FunctionList.AddElement("if(param)");
        FunctionList.AddElement("Filterx(src, des)");
        FunctionList.AddElement("CreateVariable(des, nrsamples, samplerates)");
        FunctionList.AddElement("FullSpectrum(indataname'the input data', outputdataname'the output', nrhunits, startstop'lower and upper bound of spectrum frequencies')");
        FunctionList.AddElement("AppendSine(outdataname, nrsamples, frequencies)");
        FunctionList.AddElement("CreateVector(des, list_of_elements)");
        FunctionList.AddElement("AlterSignal(data, value)");
        FunctionList.AddElement("IsEqual(data1, data2, a_tolerance)");
        FunctionList.AddElement("Spectrum_RT(data1, data2, data3)");
        FunctionList.AddElement("SpectrumTimeline_RT(data1, data2, data3)");
        FunctionList.AddElement("DateAndTime(prefix, suffix)");
        FunctionList.AddElement("for(a_statement_body, a_var_name, a_from, a_to, a_step)");
        FunctionList.AddElement("Iterator(a_iterator_varname)");
        FunctionList.AddElement("CatStrings(a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8)");
        FunctionList.AddElement("Spectrogram(indataname'the input data', a_channel, a_output'the output', a_block_len, a_spectrumbounds'lower and upper bound of spectrum frequencies', a_overlap, a_spectrum_presentation)");
        FunctionList.AddElement("Padding(a_varname, a_prepad, a_prepad_val, a_postpad, a_postpad_val)");
        FunctionList.AddElement("Truncate(a_varname, a_pretruncate, a_posttruncate)");
        FunctionList.AddElement("LoadJSON(outdataname, filename)");
        FunctionList.AddElement("SaveJSON(dataname, filename)");
        FunctionList.AddElement("TimeElapsed(time_var)");
        FunctionList.AddElement("Print(varname, y, x)");
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
