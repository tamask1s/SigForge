#include <stdio.h>
#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include <shlobj.h>
#include <vector>
#include <iostream>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "minvect.h"
#include "datastructures.h"
#include "bitmap.h"
#include "Defines.h"
#include "Control_Base.h"
#include "commonvisualcomponents.h"
#include "VisualComponents.h"
#include "math.h"

CVariable_List*    m_variable_list_ref = 0; /// reference for the application's variable list.
CSignalCodec_List* m_data_list_ref = 0;     /// reference for the application's data list.
NewChar_Proc       NewChar;                 /// function call for application's "new char[n]" constructor. Strings that are going to be deleted by other parts of the system, needs to be allocated with this.
NewCVariable_Proc  NewCVariable;            /// function call for application's "new CVariable" constructor. Variables that are going to be deleted by other parts of the system (added to m_variable_list_ref), needs to be allocated with this.

char* CreateSpline(char* a_outdataname, char* n, char* xs, char* ys)
{
    DoubleVec Xs;
    DoubleVec Ys;
    int N1;
    sscanf(n, "%i", &N1);

    Xs.RebuildFrom(xs);
    Ys.RebuildFrom(ys);

    CVariable* output2 = NewCVariable();
    unsigned int output2sizes[1];
    output2sizes[0] = N1;
    output2->Rebuild(1, output2sizes);

    double xOutput[N1];
    for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
        xOutput[j] = j;
    SplineInterpolate(Xs.m_data, Ys.m_data, Xs.m_size, xOutput, output2->m_data[0], N1);

    output2->m_sample_rates.m_data[0] = 1;

    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(output2->m_varname, output2);
    return 0;
}

char* Extract(char* invarname, char* a_varnametostore, char* a_from, char* a_len)
{
    char* error = nullptr;
    if (!a_len)
        a_len = (char*)"-1";
    if (!invarname || !a_varnametostore || !a_from)
        error = MakeString(NewChar, "ERROR: Extract: not enough arguments");
    DoubleVec l_dfrom, l_dlen;
    if (!error)
    {
        l_dfrom.RebuildFrom(a_from);
        l_dlen.RebuildFrom(a_len);
    }
    if (l_dfrom.m_size && l_dlen.m_size && !error)
    {
        if (l_dfrom.m_data[0] < 0)
            error = MakeString(NewChar, "ERROR: Extract: the from argument can't be negative '", a_from, "'");
        CVariable* l_in_var = m_variable_list_ref->variablemap_find(invarname);
        if (l_in_var && !error)
        {
            for (unsigned l_channel_number = 0; l_channel_number < l_in_var->m_total_samples.m_size; ++l_channel_number)
                if (l_in_var->m_total_samples.m_data[l_channel_number] < (l_dfrom.m_data[0] + l_dlen.m_data[0]) * l_in_var->m_sample_rates.m_data[l_channel_number])
                    error = MakeString(NewChar, "ERROR: Extract: out of boundary");
            if (!error)
            {
                CVariable* spectrum = l_in_var->Extract(a_varnametostore, l_dfrom.m_data[0], l_dlen.m_data[0]);
                if (spectrum)
                    m_variable_list_ref->Insert(spectrum->m_varname, spectrum);
            }
        }
        else if (!error)
            error = MakeString(NewChar, "ERROR: Extract: can't find data in variablelist: '", invarname, "'");
    }
    else if (!error)
        error = MakeString(NewChar, "ERROR: Extract: the from/to argument not a valid number; From: '", a_from, "', Len: '", a_len, "'");

    return error;
}

char* Downsample(char* a_in_var_name, char* a_var_name_to_store, char* a_ratio)
{
    if (!a_in_var_name || !a_var_name_to_store || !a_ratio)
        return MakeString(NewChar, "ERROR: Downsample: not enough arguments!");
    DoubleVec dratio;
    dratio.RebuildFrom(a_ratio);
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_in_var_name))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[var->m_total_samples.m_size];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            output2sizes[i] = ((int)ceil((double)var->m_widths.m_data[i] / dratio.m_data[0]));

        output2->Rebuild(var->m_total_samples.m_size, output2sizes);
        var->SCopyTo(output2);
        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            output2->m_sample_rates.m_data[i] = var->m_sample_rates.m_data[i] * (double)output2->m_widths.m_data[i] / (double)var->m_widths.m_data[i];
            for (unsigned int j = 0; j < output2->m_widths.m_data[i]; ++j)
                output2->m_data[i][j] = var->m_data[i][(int)((double)j * dratio.m_data[0])];
        }
        strcpy(output2->m_varname, a_var_name_to_store);
        m_variable_list_ref->Insert(output2->m_varname, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Downsample: can't find data in variablelist: '", a_in_var_name, "'");
}

char* DownsampleToNrSamples(char* a_in_var_name, char* a_var_name_to_store, char* a_nr_samples)
{
    if (!a_in_var_name || !a_var_name_to_store || !a_nr_samples)
        return MakeString(NewChar, "ERROR: DownsampleToNrSamples: not enough arguments!");
    int dnr_samples = atof(a_nr_samples);
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_in_var_name))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[var->m_total_samples.m_size];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            output2sizes[i] = dnr_samples;

        output2->Rebuild(var->m_total_samples.m_size, output2sizes);
        var->SCopyTo(output2);
        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            output2->m_sample_rates.m_data[i] = var->m_sample_rates.m_data[i] * (double)output2->m_widths.m_data[i] / (double)var->m_widths.m_data[i];
            double l_ratio = ((double)var->m_widths.m_data[i]) / ((double)dnr_samples);
            for (unsigned int j = 0; j < output2->m_widths.m_data[i]; ++j)
                output2->m_data[i][j] = var->m_data[i][(int)((double)j * l_ratio)];
        }
        strcpy(output2->m_varname, a_var_name_to_store);
        m_variable_list_ref->Insert(output2->m_varname, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: DownsampleToNrSamples: can't find data in variablelist: '", a_in_var_name, "'");
}

char* DownsampleGauss(char* a_in_var_name, char* a_var_name_to_store, char* a_ratio)
{
    if (!a_in_var_name || !a_var_name_to_store || !a_ratio)
        return MakeString(NewChar, "ERROR: DownsampleGauss: not enough arguments!");
    DoubleVec dratio;
    dratio.RebuildFrom(a_ratio);
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_in_var_name))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[var->m_total_samples.m_size];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            output2sizes[i] = ((int)ceil((double)var->m_widths.m_data[i] / dratio.m_data[0]));
        output2->Rebuild(var->m_total_samples.m_size, output2sizes);
        var->SCopyTo(output2);
        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            output2->m_sample_rates.m_data[i] = output2->m_sample_rates.m_data[i] / dratio.m_data[0];
            int radius = 4;
            double GaussKernel [radius * 2 + 1];
            double GaussKernelsumm = 0;
            for (int k = -radius; k < radius + 1; ++k)
            {
                GaussKernel[k + radius] = exp(-((double)k / (double)(radius / 2.0)) * ((double)k / (double)(radius / 2.0)));
                GaussKernelsumm += GaussKernel[k + radius];
            }

            for (int j = 0; j < radius; ++j)
                output2->m_data[i][j] = 0;
            for (unsigned int j = output2->m_widths.m_data[i] - radius; j < output2->m_widths.m_data[i]; ++j)
                output2->m_data[i][j] = 0;

            for (unsigned int j = 0; j < output2->m_widths.m_data[i]; ++j)
            {
                output2->m_data[i][j] = 0;
                int indxstart = (int)((double)j * dratio.m_data[0]);
                for (int k = -radius; k < radius + 1; ++k)
                {
                    int tindx = indxstart + k;
                    if (tindx < 0)
                        tindx = 0;
                    if (tindx >= (int)var->m_widths.m_data[i])
                        tindx = var->m_widths.m_data[i] - 1;
                    output2->m_data[i][j] += var->m_data[i][tindx] * GaussKernel[k + radius];
                }
                output2->m_data[i][j] = output2->m_data[i][j] / GaussKernelsumm;
            }
        }
        strcpy(output2->m_varname, a_var_name_to_store);
        m_variable_list_ref->Insert(output2->m_varname, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: DownsampleGauss: can't find data in variablelist: '", a_in_var_name, "'");
}

char* Oversample(char* a_invarname, char* a_varnametostore, char* a_ratio)
{
    DoubleVec ratio;
    ratio.RebuildFrom(a_ratio);
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        CVariable* input = var;
        CVariable* output = NewCVariable();
        unsigned int output_nr_samples[input->m_total_samples.m_size];
        for (unsigned int i = 0; i < input->m_total_samples.m_size; ++i)
            output_nr_samples[i] = ((int)ceil((double)input->m_widths.m_data[i] * ratio.m_data[0]));
        output->Rebuild(input->m_total_samples.m_size, output_nr_samples);
        input->SCopyTo(output);
        for (unsigned int i = 0; i < output->m_total_samples.m_size; ++i)
        {
            output->m_sample_rates.m_data[i] = output->m_sample_rates.m_data[i] * ratio.m_data[0];

            for (unsigned int j = 0; j < output->m_widths.m_data[i]; ++j)
            {
                int indx1 = (int)ceil((double)j / ratio.m_data[0]);
                int indx2 = (int)floor((double)j / ratio.m_data[0]);
                double dist1 = fabs(((double)j / ratio.m_data[0]) - (int)ceil((double)j / ratio.m_data[0]));
                double dist2 = fabs(((double)j / ratio.m_data[0]) - (int)floor((double)j / ratio.m_data[0]));

                if (indx1 >= (int)input->m_widths.m_data[i])
                    indx1 = input->m_widths.m_data[i] - 1;
                if (indx2 >= (int)input->m_widths.m_data[i])
                    indx2 = input->m_widths.m_data[i] - 1;
                //output->m_data[i][j] = (input->m_data[i][indx1] + input->m_data[i][indx2]) / 2.0;
                output->m_data[i][j] = (input->m_data[i][indx1] * (1 - dist1) + input->m_data[i][indx2] * (1 - dist2)) / ((1 - dist1) + (1 - dist2));
            }
        }
        strcpy(output->m_varname, a_varnametostore);
        m_variable_list_ref->Insert(output->m_varname, output);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: DownsampleGauss: while Downsampling '", a_invarname, "' to '", a_varnametostore, "': not enugh arguments!");
}

template<typename Type>
void interpolate2(Type* x, int lenx, Type* y, int leny)
{
    Type ratio = (Type)leny / (Type)(lenx);
    int lenxy = leny - ratio - 1;
    for (int i = 0; i < lenxy; ++i)
    {
        int step = i / (ratio);
        int substep = i % (int)(ratio);
        Type dy = (Type)(x[step + 1] - x[step]) / (Type)(ratio);
        y[i] = x[step] + (dy * substep);
    }
    for (int i = lenxy; i < leny; ++i)
        y[i] = x[lenx - 1];
}

template<typename Type>
void interpolate3(Type* x, int lenx, Type* y, int leny)
{
    Type h_ratio = (Type)leny / (Type)(lenx);
    unsigned int j = 0;
    for (int i = 0; i < lenx - 1; ++i)
    {
        unsigned int substep = 0;
        Type dy = (Type)(x[i + 1] - x[i]) / (Type)(h_ratio);
        //while (j < (Type)(i + 1) * h_ratio)
        for (; j < (Type)(i + 1) * h_ratio; ++j)
            y[j] = x[i] + (dy * substep++);
    }
    for (int i = j; i < leny; ++i)
        y[i] = x[lenx - 1];
}

char* Oversample2(char* a_invarname, char* a_varnametostore, char* a_ratio)
{
    DoubleVec ratio;
    ratio.RebuildFrom(a_ratio);
    double dratio = ratio.m_data[0];
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        CVariable* input = var;
        CVariable* output = NewCVariable();
        unsigned int output_nr_samples[input->m_total_samples.m_size];
        for (unsigned int i = 0; i < input->m_total_samples.m_size; ++i)
            output_nr_samples[i] = ((int)ceil((double)input->m_widths.m_data[i] * dratio));
        output->Rebuild(input->m_total_samples.m_size, output_nr_samples);
        input->SCopyTo(output);
        for (unsigned int i = 0; i < output->m_total_samples.m_size; ++i)
        {
            output->m_sample_rates.m_data[i] = output->m_sample_rates.m_data[i] * dratio;
            interpolate3(input->m_data[i], input->m_widths.m_data[i], output->m_data[i], output->m_widths.m_data[i]);
        }
        strcpy(output->m_varname, a_varnametostore);
        m_variable_list_ref->Insert(output->m_varname, output);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: DownsampleGauss: while Downsampling '", a_invarname, "' to '", a_varnametostore, "': not enugh arguments!");
}

char* RowMean(char* invarname, char* a_outdataname)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(invarname))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[1];
        output2sizes[0] = var->m_widths.m_data[0];
        output2->Rebuild(1, output2sizes);
        var->SCopyTo(output2);
        output2->m_sample_rates.m_data[0] = output2->m_sample_rates.m_data[0];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
                output2->m_data[0][j] += var->m_data[i][j];
        for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
            output2->m_data[0][j] = output2->m_data[0][j] / (double)var->m_total_samples.m_size;

        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(output2->m_varname, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: RowMean: can't find data in variablelist: '", invarname, "'");
}

char* RowMeanE(char* invarname, char* a_outdataname, char* enable)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(invarname))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[1];
        output2sizes[0] = var->m_widths.m_data[0];
        output2->Rebuild(1, output2sizes);
        var->SCopyTo(output2);
        output2->m_sample_rates.m_data[0] = output2->m_sample_rates.m_data[0];

        int nrrows = var->m_total_samples.m_size;
        if (enable)
        {
            IntVec activechannelindxs;
            activechannelindxs.RebuildFrom(enable);
            nrrows = 0;
            for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
                if (activechannelindxs.HasEelemnt(i + 1))
                {
                    for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
                        output2->m_data[0][j] += var->m_data[i][j];
                    nrrows++;
                }
        }
        else
        {
            for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
                for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
                    output2->m_data[0][j] += var->m_data[i][j];
        }
        if (nrrows)
        {
            for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
                output2->m_data[0][j] = output2->m_data[0][j] / (double)nrrows;
        }
        else
        {
            for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
                output2->m_data[0][j] = 0;
        }
        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(output2->m_varname, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: RowMeanE: can't find data in variablelist: '", invarname, "'");
}

class Terminator: public EObject
{
public:
    static bool releaseme;
    void EventHandler(CControl_Base*sender)
    {
        releaseme = true;
    }
};

bool Terminator::releaseme = false;

char* UserInput(char* a_indataname, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid)
{
    char* l_result = nullptr;
    if (m_variable_list_ref)
    {
        CVariable* datacodec = m_variable_list_ref->variablemap_find(a_indataname);
        if (datacodec)
        {
            Terminator Termin;
            EDisplayType lDisplayType = EDisplayType::EDisplayType_1D_waveforms;
            if (a_display_type)
            {
                if (!strcmp(a_display_type, "2D_map"))
                    lDisplayType = EDisplayType::EDisplayType_2D_map;
                else if (!strcmp(a_display_type, "2D_map_surface"))
                    lDisplayType = EDisplayType::EDisplayType_2D_map_surface;
                else if (!strcmp(a_display_type, "value_list"))
                    lDisplayType = EDisplayType::EDisplayType_value_list;
            }
            EFitWidthType lfitWidth = EFitWidthType::EFitWidthType_normal;
            if (a_fit_width)
                if (!strcmp(a_fit_width, "fit_width") || !strcmp(a_fit_width, "true"))
                    lfitWidth = EFitWidthType::EFitWidthType_fitWidth;
            CSignalDisplay* child2 = new CSignalDisplay("NO DATA", datacodec, lfitWidth, lDisplayType, a_align, a_grid, 0, 0);
            child2->EID = 1000;
            child2->OnDestroy += &Terminator::EventHandler;
            BringWindowToTop(child2->hWnd);
            SetActiveWindow(child2->hWnd);
            SendMessage(child2->hWnd, WM_LBUTTONUP, 0, 0);
            RunAppDWhile(&Terminator::releaseme);
            Terminator::releaseme = false;
        }
        else
            l_result = MakeString(NewChar, "ERROR: UserInput: can't find '", a_indataname, "' in variablelist");
    }
    else
        l_result = MakeString(NewChar, "ERROR: UserInput: there is no variable list!");
    return l_result;
}

/*------------------------INTERSECT-----------------------------------------------------------*/
void GetQuad(double x1, double x2, double x3, double y1, double y2, double y3, double *a, double *b, double *c)
{
    double alph = (x3 * x3 - x1 * x1) / (x3 * x3 - x2 * x2);
    *b = (y3 - y1 - alph * y3 + alph * y2) / (alph * x2 - alph * x3 + x3 - x1);
    *a = (y1 - y2 - *b * x1 + *b * x2) / (x1 * x1 - x2 * x2);
    *c = y1 - *a * x1 * x1 - *b * x1;
}

bool Solve(double a, double b, double c, double y, double*x1, double*x2)
{
    double D = b * b - 4.0 * a * (c - y);
    if (D < 0)
        return false;
    else
    {
        if (!a) //ha linearis
        {
            if (!b) //ha meg vizszintes is
                *x1 = *x2 = y;
            else
                *x1 = *x2 = (y - c) / b;
        }
        else
        {
            *x1 = (-b + sqrt(D)) / (2.0 * a);
            *x2 = (-b - sqrt(D)) / (2.0 * a);
        }
        return true;
    }
}

int Intersect(double a, double b, double c, double y, double bound1, double bound2, double* intersectxpoint1, double* intersectxpoint2)
{
    double x1, x2;
    bound1 -= 0.00000000000001; // double correction
    bound2 += 0.00000000000001; // 0.000000->0.0   0.000->0.0
    if (Solve(a, b, c, y, &x1, &x2))
    {
        int result = 0;
        if ((x1 >= bound1) && (x1 <= bound2))
        {
            *intersectxpoint1 = x1;
            result++;
        }
        if ((x2 >= bound1) && (x2 <= bound2))
        {
            *intersectxpoint2 = x2;
            result++;
        }
        return result;
    }
    else
        return false;
}

bool IsEqual(double d1, double d2)
{
    bool result = false;
    if (fabs(d1 - d2) < 0.00000000000001)
        result = true;
    return result;
}

bool Solve(double b, double c, double y, double* x)
{
    if (!b)
        *x = y;
    else
        *x = (y - c) / b;
    return 0;
}

int Intersect(const double* input, const int inputSN, const double intersectypoint, double* out)
{
    int count = 0;
    for (int i = 0; i < inputSN - 1; ++i)
    {
        double b = (input[1 + i] - input[0 + i]);
        double c = input[0 + i];
        double intersectxpoint;
        Solve(b, c, intersectypoint, &intersectxpoint);

        if ((intersectxpoint >= 0) && (intersectxpoint <= 1))
            out[count++] = intersectxpoint + i;
    }
    return count;
}

char* Intersect(char* invarname, char* a_outvarname, char* value)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(invarname))
    {
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i) //!!! csak az utolso gorbet fogja intersectalni!!!
        {
            int SN = var->m_widths.m_data[i];
            double Data[SN];
            double Out[SN];
            for (int j = 0; j < SN; ++j)
            {
                Data[j] = var->m_data[i][j];
            }
            int count = Intersect(Data, SN, 0, Out);
            for (int k = 0; k < count; ++k)
                Out[k] = Out[k] / var->m_sample_rates.m_data[i] + var->m_horizontal_scale_start;

            CVariable* output2 = NewCVariable();
            unsigned int output2sizes[1];
            output2sizes[0] = count;
            output2->Rebuild(1, output2sizes);

            for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
            {
                output2->m_data[0][j] = Out[j];
                var->m_marker_list.AddTMarker(Out[j], 0, 15 + j, 20 + j, GetSomeColor(j), 0);
            }
            strcpy(output2->m_varname, a_outvarname);
            m_variable_list_ref->Insert(output2->m_varname, output2);
        }
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Intersect: can't find data in variablelist: '", invarname, "'");
}

void Deriv(double* input, int inputSN, double sr)
{
    if (inputSN < 1)
        return;
    if (inputSN < 2)
    {
        input[0] = 0;
        return;
    }
    if (inputSN < 3)
    {
        input[0] = (input[1] - input[0] * sr);
        input[1] = input[0] * sr;
        return;
    }
    double lastlastx;
    double iim3 = input[inputSN - 3];
    double lastx = input[0];
    input[0] = -input[2] * 0.5 - input[0] * 1.5 + input[1] + input[1];
    input[0] *= sr;
    int cy = ((inputSN - 2) / 8) * 8 + 1;
    double srd2 = sr * 0.5;
    for (int i = 1; i < cy; ++i)
    {
        lastlastx = input[i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;

        lastlastx = input[++i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;

        lastlastx = input[++i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;

        lastlastx = input[++i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;

        lastlastx = input[++i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;

        lastlastx = input[++i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;

        lastlastx = input[++i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;

        lastlastx = input[++i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;
    }
    for (int i = cy; i < inputSN - 1; ++i)
    {
        lastlastx = input[i];
        input[i] = (input[i + 1] - lastx) * srd2;
        lastx = lastlastx;
    }
    input[inputSN - 1] = input[inputSN - 1] * 1.5 + iim3 * 0.5 - lastx - lastx;
    input[inputSN - 1] *= sr;
}

char* Deriv(char* invarname)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(invarname))
    {
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            Deriv(var->m_data[i], var->m_widths.m_data[i], var->m_sample_rates.m_data[i]);
        //Spline2Deriv(var->m_data[i], var->m_widths.m_data[i],var->m_sample_rates.m_data[i]);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Deriv: Deriving : not enugh arguments!");
}

char* ToString(const char* a_indataname)
{
    if (!a_indataname)
        return MakeString(NewChar, "ERROR: ToString: not enough arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
        {
            DoubleVec dv1;
            dv1.Rebuild(var->m_widths.m_data[i]);
            for (unsigned int j = 0; j < var->m_widths.m_data[i]; ++j)
                dv1.m_data[j] = var->m_data[i][j];
            char* str1 = dv1.ToString(" ");
            if (str1)
            {
                char* str2 = MakeString(NewChar, "RESULT:", str1);
                delete[] str1;
                return str2;
            }
            else
                return 0; // MakeString(NewChar,"ERROR: ToString: Can't find data:'",a_indataname,"'");
        }
    }
    else
        return MakeString(NewChar, "ERROR: ToString: can't find data in variablelist: '", a_indataname, "'");
    return 0;
}

char* PutTMarkersOnValues(char* a_indataname, char* values)
{
    DoubleVec dv1;
    dv1.RebuildFrom(values);
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        for (unsigned int i = 0; i < dv1.m_size; ++i)
        {
            var->m_marker_list.AddTMarker(dv1.m_data[i], 0, 15 + i, 20 + i, GetSomeColor(i), 0);
            var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_movable = true;
            var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_left = true;
            var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_right = true;
        }
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: PutTMarkersOnValues: can't find data in variablelist: '", a_indataname, "'");
}

char* PutTwoSpindleTMarkers(char* a_indataname, char* RMDSGExtracted, char* Intersections)
{
    DoubleVec dv1;
    dv1.RebuildFrom (Intersections);
    CVariable* var = m_variable_list_ref->variablemap_find(a_indataname);
    CVariable* var2 = m_variable_list_ref->variablemap_find(RMDSGExtracted);
    if (var && var2)
    {
        double min1 = MAXINT;
        double min2 = MAXINT;
        unsigned int min1indx = 0;
        unsigned int min2indx = 0;
        double min3 = MAXINT;
        unsigned int min3indx = 0;

        for (unsigned int i = 0; i < var2->m_widths.m_data[0]; ++i)
            if (min1 > var2->m_data[0][i])
            {
                min1 = var2->m_data[0][i];
                min1indx = i;
            }
        for (unsigned int i = 0; i < var2->m_widths.m_data[0]; ++i)
            if ((min2 > var2->m_data[0][i]) && (i != min1indx))
            {
                min2 = var2->m_data[0][i];
                min2indx = i;
            }
        for (unsigned int i = 0; i < var2->m_widths.m_data[0]; ++i)
            if ((min3 > var2->m_data[0][i]) && (i != min1indx) && (i != min2indx))
            {
                min3 = var2->m_data[0][i];
                min3indx = i;
            }
        if (min1indx > min2indx)
        {
            int temval = min1indx;
            min1indx = min2indx;
            min2indx = temval;
        }
        if ((min1indx < var2->m_widths.m_data[0] / 2 + 4) && (min2indx < var2->m_widths.m_data[0] / 2 + 4))
            if (min3indx >= var2->m_widths.m_data[0] / 2 + 4)
            {
                min1indx = min2indx;
                min2indx = min3indx;
            }
        if ((min1indx > var2->m_widths.m_data[0] / 2 - 4) && (min2indx > var2->m_widths.m_data[0] / 2 - 4))
            if (min3indx <= var2->m_widths.m_data[0] / 2 - 4)
            {
                min2indx = min1indx;
                min1indx = min3indx;
            }
        double min1LBound = -1, min1UBound = -1, min2LBound, min2UBound;
        for (unsigned int i = 0; i < dv1.m_size; ++i)
        {
            if (dv1.m_data[i] > min1indx / var2->m_sample_rates.m_data[0] + var2->m_horizontal_scale_start)
            {
                min1UBound = dv1.m_data[i];
                goto ex1;
            }
        }
ex1:
        for (int i = dv1.m_size - 1; i > -1; i--)
            if (dv1.m_data[i] < min1indx / var2->m_sample_rates.m_data[0] + var2->m_horizontal_scale_start)
            {
                min1LBound = dv1.m_data[i];
                goto ex2;
            }
ex2:
        for (unsigned int i = 0; i < dv1.m_size; ++i)
        {
            if (dv1.m_data[i] > min2indx / var2->m_sample_rates.m_data[0] + var2->m_horizontal_scale_start)
            {
                min2UBound = dv1.m_data[i];
                goto ex3;
            }
        }
ex3:
        for (int i = dv1.m_size - 1; i > -1; i--)
            if (dv1.m_data[i] < min2indx / var2->m_sample_rates.m_data[0] + var2->m_horizontal_scale_start)
            {
                min2LBound = dv1.m_data[i];
                goto ex4;
            }
ex4:

        if(min1LBound < 9.2)
            min1LBound = 9.2;

        if(min1LBound > 14)
            min1LBound = 14;

        if(min2LBound > 15.5)
            min2LBound = 15.5;

        if(min2LBound <= min1LBound)
            min2LBound = min1LBound + 1.5;

        if(min1UBound > 15)
            min1UBound = 15;

        if(min1LBound > min1UBound)
            min1UBound = min1LBound + 0.7;

        if(min2LBound > 15.0)
            min2LBound = 15.0;

        if(min2UBound > 15.8)
            min2UBound = 15.8;

        if(min2UBound <= min1UBound)
            min2UBound = min1UBound + 0.5;

        if(min2LBound > min2UBound)
            min2UBound = min2LBound + 0.7;

        var->m_marker_list.AddTMarker(min1LBound, min1UBound - min1LBound, 15 + 0, 20 + 0, GetSomeColor(6), 0);
        var->m_marker_list.AddTMarker(min2LBound, min2UBound - min2LBound, 15 + 0, 20 + 0, GetSomeColor(9), 0);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: PutTMarkersOnValues: can't find data in variablelist: '", a_indataname, "'");
}

char* AddMarker(char* a_indataname, char* a_start_sample, char* a_length, char* a_label, char* a_channel, char* a_color, char* a_id)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        int s = var->m_marker_list.m_size;
        var->m_marker_list.AddTMarker(atof(a_start_sample), atof(a_length), a_id ? atoi(a_id) : (15 + s), 100 + s, a_color ? atoi(a_color) : GetSomeColor(s), a_label, a_channel ? atoi(a_channel) : -1);
        var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_movable = true;
        var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_left = true;
        var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_right = true;
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: PutTMarkersOnValues: can't find data in variablelist: '", a_indataname, "'");
}

char* AddMarkers(char* a_indataname, char* a_markers, char* a_max, char* a_min)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        if (CVariable* markers = m_variable_list_ref->variablemap_find(a_markers))
        {
            double maxval = markers->m_data[0][0];
            double minval = markers->m_data[0][0];

            for (unsigned int j = 0; j < markers->m_total_samples.m_data[0]; ++j)
            {
                if (markers->m_data[0][j] < minval)
                    minval = markers->m_data[0][j];
                if (markers->m_data[0][j] > maxval)
                    maxval = markers->m_data[0][j];
            }
            double amplitude = maxval - minval;
            for (unsigned int j = 0; j < markers->m_total_samples.m_data[0]; ++j)
            {
                double d_color = ((markers->m_data[0][j] - minval) / amplitude) * 511.0 - 256.0;
                if (markers->m_data[0][j] < 0)
                    d_color *= -0.999;
                unsigned char color = d_color;
                unsigned int i_color = (color << 8) | (color << 16);
                if (markers->m_data[0][j] < 0)
                    i_color = color | color << 8;
                var->m_marker_list.AddTMarker(j, 1, j, 0, i_color);
                var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_movable = true;
                var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_left = true;
                var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_right = true;
            }
            return 0;
        }
        else
            return MakeString(NewChar, "ERROR: AddMarkers: can't find data in variablelist: '", a_markers, "'");
    }
    else
        return MakeString(NewChar, "ERROR: AddMarkers: can't find data in variablelist: '", a_indataname, "'");
}

char* Annotate(char* a_indataname, char* a_start, char* a_length, char* a_label, char* a_channel, char* a_color, char* a_id)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        int s = var->m_marker_list.m_size;
        var->m_marker_list.AddTMarker(atof(a_start), atof(a_length), a_id ? atoi(a_id) : (15 + s), 100 + s, a_color ? atoi(a_color) : GetSomeColor(s), a_label, a_channel ? atoi(a_channel) : -1);
        var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_movable = true;
        var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_left = true;
        var->m_marker_list.m_data[var->m_marker_list.m_size - 1].m_sizable_right = true;
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: PutTMarkersOnValues: can't find data in variablelist: '", a_indataname, "'");
}

char* GetMarkerStart(char* a_indataname, char* a_marker_index)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        unsigned int marker_index = atoi(a_marker_index);
        if (marker_index < var->m_marker_list.m_size)
        {
            char tmp[32];
            sprintf(tmp, "%f", var->m_marker_list.m_data[marker_index].m_start_sample);
            return MakeString(NewChar, "RESULT:", tmp);
        }
        else
            return MakeString(NewChar, "ERROR: GetMarkerStart: marker index is out of range: '", a_indataname, "' / '",a_marker_index, "'");
    }
    else
        return MakeString(NewChar, "ERROR: GetMarkerStart: can't find data in variablelist: '", a_indataname, "'");
}

char* GetMarkerLength(char* a_indataname, char* a_marker_index)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        unsigned int marker_index = atoi(a_marker_index);
        if (marker_index < var->m_marker_list.m_size)
        {
            char tmp[32];
            sprintf(tmp, "%f", var->m_marker_list.m_data[marker_index].m_length);
            return MakeString(NewChar, "RESULT:", tmp);
        }
        else
            return MakeString(NewChar, "ERROR: GetMarkerLength: marker index is out of range: '", a_indataname, "' / '",a_marker_index, "'");
    }
    else
        return MakeString(NewChar, "ERROR: GetMarkerLength: can't find data in variablelist: '", a_indataname, "'");
}

char* GetMarkerChannel(char* a_indataname, char* a_marker_index)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
    {
        unsigned int marker_index = atoi(a_marker_index);
        if (marker_index < var->m_marker_list.m_size)
        {
            char tmp[32];
            sprintf(tmp, "%d", var->m_marker_list.m_data[marker_index].m_channel);
            return MakeString(NewChar, "RESULT:", tmp);
        }
        else
            return MakeString(NewChar, "ERROR: GetMarkerChannel: marker index is out of range: '", a_indataname, "' / '",a_marker_index, "'");
    }
    else
        return MakeString(NewChar, "ERROR: GetMarkerChannel: can't find data in variablelist: '", a_indataname, "'");
}

char* Cat(char* a_indataname, char* a_indatanametocat)
{
    if (!a_indataname || !a_indatanametocat)
        return MakeString(NewChar, "ERROR: Cat: not enugh arguments!");
    CVariable* var = m_variable_list_ref->variablemap_find(a_indataname);
    CVariable* var2 = m_variable_list_ref->variablemap_find(a_indatanametocat);
    if (var && var2)
    {
        unsigned int widths2[var->m_total_samples.m_size + var2->m_total_samples.m_size];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            widths2[i] = var->m_widths.m_data[i];
        int lastsize = var->m_total_samples.m_size;
        for (unsigned int i = var->m_total_samples.m_size; i < var->m_total_samples.m_size + var2->m_total_samples.m_size; ++i)
        {
            widths2[i] = var2->m_widths.m_data[i - var->m_total_samples.m_size];
        }
        var->RebuildPreserve(var->m_total_samples.m_size + var2->m_total_samples.m_size, widths2);
        for (unsigned int i = lastsize; i < var->m_total_samples.m_size; ++i)
        {
            memcpy(var->m_data[i], var2->m_data[i - lastsize], var->m_widths.m_data[i]*sizeof(double));
            var->m_sample_rates.m_data[i] = var2->m_sample_rates.m_data[i - lastsize];
            strcpy(var->m_vertical_units.m_data[i].s, var2->m_vertical_units.m_data[i - lastsize].s);
            if (strlen(var2->m_labels.m_data[i - lastsize].s))
                strcpy(var->m_labels.m_data[i].s, var2->m_labels.m_data[i - lastsize].s);
            else
                strcpy(var->m_labels.m_data[i].s, var2->m_varname);
        }
        strcpy(var->m_horizontal_units, var2->m_horizontal_units);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Cat: can't find data in variablelist: '", a_indataname, "' or '", a_indatanametocat, "'");
}

char* Append(char* a_indataname, char* a_indatanametocat)
{
    if (!a_indataname || !a_indatanametocat)
        return MakeString(NewChar, "ERROR: Append: not enough arguments!");
    if (CVariable* var2 = m_variable_list_ref->variablemap_find(a_indatanametocat))
    {
        CVariable* var = m_variable_list_ref->variablemap_find(a_indataname);
        unsigned int nrchannels = var2->m_total_samples.m_size;
        if (!var)
        {
            CVariable* output2 = NewCVariable();
            unsigned int output2sizes[nrchannels];
            memset(output2sizes, 0, sizeof(output2sizes));
            output2->Rebuild(nrchannels, output2sizes);
            var2->SCopyTo(output2);
            strcpy(output2->m_varname, a_indataname);
            m_variable_list_ref->Insert(output2->m_varname, output2);
            var = m_variable_list_ref->variablemap_find(a_indataname);
        }
        if (nrchannels != var->m_total_samples.m_size)
            return MakeString(NewChar, "ERROR: Append: different channel numbers!");

        unsigned int orig_output_sizes[nrchannels];
        unsigned int new_output_sizes[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
        {
            orig_output_sizes[i] = var->m_total_samples.m_data[i];
            new_output_sizes[i] = orig_output_sizes[i] + var2->m_total_samples.m_data[i];
        }
        var->RebuildPreserve(nrchannels, new_output_sizes);

        for (unsigned int i = 0; i < nrchannels; ++i)
            memcpy(&var->m_data[i][orig_output_sizes[i]], &var2->m_data[i][0], var2->m_total_samples.m_data[i] * sizeof(double));

        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Append: can't find variable in variablelist: '", a_indataname, "' or '", a_indatanametocat, "'");
}

char* PopFront(char* a_indataname, char* a_nr_elements)
{
    if (!a_indataname || !a_nr_elements)
        return MakeString(NewChar, "ERROR: PopFront: not enough arguments!");
    if (CVariable* var2 = m_variable_list_ref->variablemap_find(a_indataname))
    {
        int nr_elements = atoi(a_nr_elements);
        unsigned int nrchannels = var2->m_total_samples.m_size;
        unsigned int new_output_sizes[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
            new_output_sizes[i] = var2->m_total_samples.m_data[i] - nr_elements;
        for (unsigned int i = 0; i < nrchannels; ++i)
            memmove(&var2->m_data[i][0], &var2->m_data[i][nr_elements], new_output_sizes[i] * sizeof(double));
        var2->RebuildPreserve(nrchannels, new_output_sizes);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: PopFront: can't find variable in variablelist: '", a_indataname, "'.");
}

char* PopBack(char* a_indataname, char* a_nr_elements)
{
    if (!a_indataname || !a_nr_elements)
        return MakeString(NewChar, "ERROR: PopBack: not enough arguments!");
    if (CVariable* var2 = m_variable_list_ref->variablemap_find(a_indataname))
    {
        int nr_elements = atoi(a_nr_elements);
        unsigned int nrchannels = var2->m_total_samples.m_size;
        unsigned int new_output_sizes[nrchannels];
        for (unsigned int i = 0; i < nrchannels; ++i)
            new_output_sizes[i] = var2->m_total_samples.m_data[i] - nr_elements;
        var2->RebuildPreserve(nrchannels, new_output_sizes);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: PopBack: can't find variable in variablelist: '", a_indataname, "'.");
}

double integral_fast_(double* data, unsigned int nr_samples, double nr_units_between_samples)
{
    double res = (data[0] + data[nr_samples]) * 0.5;
    for (unsigned int i = 1; i < nr_samples; ++i)
        res += data[i];
    return res * nr_units_between_samples;
}

char* integral(char* a_in_var_name, char* a_unit_start, char* a_unit_stop, char* a_output_var_name)
{
    char* error = nullptr;
    if (!a_in_var_name || !a_unit_start || !a_unit_stop || !a_output_var_name)
        error = MakeString(NewChar, "ERROR: integral: not enough arguments");
    CVariable* l_input = m_variable_list_ref->variablemap_find(a_in_var_name);
    if (!l_input)
        error = MakeString(NewChar, "ERROR: integral: can't find variable: '", a_in_var_name, "'");

    double l_unitstart = atof(a_unit_start);
    double l_unitstop = atof(a_unit_stop);
    if (l_unitstart < 0)
        error = MakeString(NewChar, "ERROR: integral: negative start index is not valid ", a_unit_start);
    if (l_unitstop < 0)
        error = MakeString(NewChar, "ERROR: integral: negative stop index is not valid ", a_unit_stop);
    if (!error)
    {
        CVariable* output = NewCVariable();
        unsigned int outputsizes[l_input->m_total_samples.m_size];
        for (unsigned int i = 0; i < l_input->m_total_samples.m_size; ++i)
            outputsizes[i] = 1;
        output->Rebuild(l_input->m_total_samples.m_size, outputsizes);

        for (unsigned int i = 0; i < l_input->m_total_samples.m_size; ++i)
        {
            unsigned int l_startsample = l_unitstart * l_input->m_sample_rates.m_data[i];
            unsigned int l_stopsample = l_unitstop * l_input->m_sample_rates.m_data[i];
            if (l_input->m_total_samples.m_data[i] > l_startsample + l_stopsample)
                output->m_data[i][0] = integral_fast_(l_input->m_data[i] + l_startsample, l_stopsample - l_startsample, 1.0 / l_input->m_sample_rates.m_data[i]);
            else
            {
                error = MakeString(NewChar, "ERROR: integral: boundary out of range: '", a_in_var_name, "' / ");
                break;
            }

        }
        if (!error)
        {
            strcpy(output->m_varname, a_output_var_name);
            m_variable_list_ref->Insert(output->m_varname, output);
        }
    }
    return error;
}

char* IntegTrapezoid(char* a_indataname, char* a_outdataname, char* spectrum)
{
    CVariable* var = m_variable_list_ref->variablemap_find(a_indataname);
    CVariable* var2 = m_variable_list_ref->variablemap_find(spectrum);
    if (var && var2)
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[4];
        for (unsigned int i = 0; i < 4; ++i)
            output2sizes[i] = var2->m_total_samples.m_size;
        output2->Rebuild(4, output2sizes);
        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(output2->m_varname, output2);
        double szurokernel [var2->m_widths.m_data[0]];
        for (unsigned int i = 0; i < var2->m_widths.m_data[0]; ++i)
        {
            double x = (double)i / var2->m_sample_rates.m_data[0];
            szurokernel[i] = exp(-(((x - 0.9) * 2.0) * ((x - 0.9) * 2.0)));
        }
        double min1LBound = var->m_marker_list.m_data[0].m_start_sample;
        double min1Len    = var->m_marker_list.m_data[0].m_length;
        double min2LBound = var->m_marker_list.m_data[1].m_start_sample;
        double min2Len    = var->m_marker_list.m_data[1].m_length;
        for (unsigned int i = 0; i < var2->m_total_samples.m_size; ++i)
        {
            unsigned int min1Lindx = (int) ( ( min1LBound - var->m_horizontal_scale_start ) * var->m_sample_rates.m_data[i]);
            if (min1Lindx >= var->m_widths.m_data[i])
                min1Lindx = var->m_widths.m_data[i] - 1;
            unsigned int min1Uindx = (int) ( ( min1LBound + min1Len - var->m_horizontal_scale_start ) * var->m_sample_rates.m_data[i]);
            if (min1Uindx >= var->m_widths.m_data[i])
                min1Uindx = var->m_widths.m_data[i] - 1;
            double trapezcenterheight = ( var->m_data[i][min1Lindx] + var->m_data[i][min1Uindx] ) / 2.0;
            output2->m_data[0][i] = trapezcenterheight * min1Len * var->m_sample_rates.m_data[i];

            unsigned int min2Lindx = (int) ( ( min2LBound - var->m_horizontal_scale_start ) * var->m_sample_rates.m_data[i]);
            if (min2Lindx >= var->m_widths.m_data[i])
                min2Lindx = var->m_widths.m_data[i] - 1;
            unsigned int min2Uindx = (int) ( ( min2LBound + min2Len - var->m_horizontal_scale_start ) * var->m_sample_rates.m_data[i]);
            if (min2Uindx >= var->m_widths.m_data[i])
                min2Uindx = var->m_widths.m_data[i] - 1;
            trapezcenterheight = ( var->m_data[i][min2Lindx] + var->m_data[i][min2Uindx] ) / 2.0;
            output2->m_data[1][i] = trapezcenterheight * min2Len * var->m_sample_rates.m_data[i];

            double szorzat [var2->m_widths.m_data[0]];
            for (unsigned int j = 0; j < var2->m_widths.m_data[0]; ++j)
                szorzat[j] = szurokernel[j] * var2->m_data[i][j];
            double somethingtrapezwidth = var2->m_sample_rates.m_data[i] * 0.72;

            unsigned int min3Lindx = (int) ceil( ( 0.54 - var2->m_horizontal_scale_start ) * var2->m_sample_rates.m_data[i]);
            if (min3Lindx >= var2->m_widths.m_data[i])
                min3Lindx = var2->m_widths.m_data[i] - 1;
            unsigned int min3Uindx = (int) floor( ( 1.25 - var2->m_horizontal_scale_start ) * var2->m_sample_rates.m_data[i]);
            if (min3Uindx >= var2->m_widths.m_data[i])
                min3Uindx = var2->m_widths.m_data[i] - 1;

            trapezcenterheight = ( szorzat [min3Lindx] + szorzat [min3Uindx] ) / 2.0;
            output2->m_data[2][i] = trapezcenterheight * somethingtrapezwidth * 8.0;
        }

        output2->m_data[3][0] = min1LBound;
        output2->m_data[3][1] = min1LBound + min1Len;
        output2->m_data[3][2] = min2LBound;
        output2->m_data[3][3] = min2LBound + min2Len;

        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: PutTMarkersOnValues: can't find data in variablelist: '", a_indataname, "'");
}

char* mxtostring(double** mx, int height, int width, bool as_integer) // cols (channels) to rows (samples)
{
    if (height * width == 0)
        return 0;
    char* result = new char[height * width * 16];
    result[0] = 0;
    char actualnum[16];
    int dstlen = 0;
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            if (as_integer)
                sprintf(actualnum, "%d", (int)mx[j][i]);
            else
                sprintf(actualnum, "%0.11f", mx[j][i]);
            int actuallen = strlen(actualnum);
            sprintf(result + dstlen, "%s", actualnum);
            dstlen += actuallen;
            if (i == width - 1)
                sprintf(result + dstlen, "\n");
            else
                sprintf(result + dstlen, "\t");
            ++dstlen;
        }
    }
    return result;
}

char* ColFromMxToStringRow(double** mx, int height, int width, int colindx, char* name = 0)
{
    if (height * width == 0)
        return 0;
    char* result = new char [height * width * 100];
    result [0] = 0;
    char actualnum [100];
    if (name)
    {
        strcat(result, name);
        strcat(result, "\t");
    }
    for (int i = 0; i < height; ++i)
    {
        sprintf (actualnum, "%0.7f", mx[i][colindx]);
        strcat(result, actualnum);
        if (i == height - 1)
            strcat(result, "\n");
        else
            strcat(result, "\t");
    }
    return result;
}

char* GetVarName(char* a_indataname)
{
    if (!a_indataname)
        return MakeString(NewChar, "ERROR: GetVarName: not enough arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_indataname))
        return MakeString(NewChar, "RESULT:", var->m_varname);
    else
        return MakeString(NewChar, "ERROR: GetVarName: can't find data in variablelist: '", a_indataname, "'");
}

char* CatToFile (char* a_var_name, char* a_indexes, char* a_file_name, char* a_name)
{
    if (!a_var_name || !a_file_name || !a_indexes)
        return MakeString(NewChar, "ERROR: CatToFile: Not enough arguments");
    IntVec indexes;
    indexes.RebuildFrom(a_indexes);
    if (!indexes.m_size)
        return MakeString(NewChar, "ERROR: CatToFile");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_var_name))
    {
        int l_index = indexes.m_data[0];
        int l_max_data_width = var->m_widths.m_data[0];
        if (l_index < 0)
            return MakeString(NewChar, "ERROR: CatToFile: negative index is not applicable");
        if (l_index > l_max_data_width - 1)
            return MakeString(NewChar, "ERROR: CatToFile: the given index is bigger than the max index in the input variable");
        char* str1 = ColFromMxToStringRow(var->m_data, var->m_total_samples.m_size, l_max_data_width, l_index, a_name);
        if (str1)
        {
            SaveBuffer(a_file_name, (byte*)str1, strlen(str1), ENDOFFILE, true);
            delete[] str1;
            return 0;
        }
        else
            return MakeString(NewChar, "ERROR: CatToFile: can't convert variable data to string");
    }
    else
        return MakeString(NewChar, "ERROR: CatToFile: can't find data in the variablelist: '", a_var_name, "'");
}

char* WriteAscii(char* a_input_var_name, char* a_file_name, char* type)
{
    if (!a_input_var_name || !a_file_name)
        return MakeString(NewChar, "ERROR: WriteAscii: Not enough argument");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_input_var_name))
    {
        bool as_integer = false;
        if (type && (!strcmp(type, "int")||(!strcmp(type, "integer"))))
            as_integer = true;
        char* str1 = mxtostring(var->m_data, var->m_total_samples.m_size, var->m_widths.m_data[0], as_integer);
        if (str1)
        {
            SaveBuffer(a_file_name, (byte*)str1, strlen(str1), 0, true);
            delete[] str1;
            return 0;
        }
        else
            return MakeString(NewChar, "ERROR: WriteAscii: can't create string from data to save file");
    }
    else
        return MakeString(NewChar, "ERROR: WriteAscii: can't find data in variablelist: '", a_input_var_name, "'");
}

inline bool float_scan(const char* wcs, float* val)
{
    int hdr = 0;
    while (wcs[hdr] == L' ')
        hdr++;

    int cur = hdr;

    bool negative = false;

    if (wcs[cur] == L'+' || wcs[cur] == L'-')
    {
        negative = wcs[cur] == L'-';
        cur++;
    }

    int quot_digs = 0;
    int frac_digs = 0;

    bool full = false;

    int binexp = 0;
    int decexp = 0;
    unsigned long value = 0;

    while (wcs[cur] >= L'0' && wcs[cur] <= L'9')
    {
        if (!full)
        {
            if ((value >= 0x19999999 && wcs[cur] - L'0' > 5) || value > 0x19999999)
            {
                full = true;
                decexp++;
            }
            else
                value = value * 10 + wcs[cur] - L'0';
        }
        else
            decexp++;

        quot_digs++;
        cur++;
    }

    if (wcs[cur] == L'.' || wcs[cur] == L',')
    {
        cur++;

        while (wcs[cur] >= L'0' && wcs[cur] <= L'9')
        {
            if (!full)
            {
                if ((value >= 0x19999999 && wcs[cur] - L'0' > 5) || value > 0x19999999)
                    full = true;
                else
                {
                    decexp--;
                    value = value * 10 + wcs[cur] - L'0';
                }
            }

            frac_digs++;
            cur++;
        }
    }

    if (!quot_digs && !frac_digs)
        return false;

    int decexp2 = 0; // explicit exponent
    bool exp_negative = false;
    int exp_digs = 0;

// even if value is 0, we still need to eat exponent chars
    if (wcs[cur] == L'e' || wcs[cur] == L'E')
    {
        cur++;

        if (wcs[cur] == L'+' || wcs[cur] == L'-')
        {
            exp_negative = wcs[cur] == '-';
            cur++;
        }

        while (wcs[cur] >= L'0' && wcs[cur] <= L'9')
        {
            if (decexp2 >= 0x19999999)
                return false;
            decexp2 = 10 * decexp2 + wcs[cur] - L'0';
            exp_digs++;
            cur++;
        }

        if (exp_negative)
            decexp -= decexp2;
        else
            decexp += decexp2;
    }

// end of wcs scan, cur contains value's tail

    if (value)
    {
        while (value <= 0x19999999)
        {
            decexp--;
            value = value * 10;
        }

        if (decexp)
        {
            // ensure 1bit space for mul by something lower than 2.0
            if (value & 0x80000000)
            {
                value >>= 1;
                binexp++;
            }

            if (decexp > 308 || decexp < -307)
                return false;

            // convert exp from 10 to 2 (using FPU)
            int E;
            double v = pow(10.0, decexp);
            double m = frexp(v, &E);
            m = 2.0 * m;
            E--;
            value = (unsigned long)floor(value * m);

            binexp += E;
        }

        binexp += 23; // rebase exponent to 23bits of mantisa


        // so the value is: +/- VALUE * pow(2,BINEXP);
        // (normalize manthisa to 24bits, update exponent)
        while (value & 0xFE000000)
        {
            value >>= 1;
            binexp++;
        }
        if (value & 0x01000000)
        {
            if (value & 1)
                value++;
            value >>= 1;
            binexp++;
            if (value & 0x01000000)
            {
                value >>= 1;
                binexp++;
            }
        }

        while (!(value & 0x00800000))
        {
            value <<= 1;
            binexp--;
        }

        if (binexp < -127)
        {
            // underflow
            value = 0;
            binexp = -127;
        }
        else if (binexp > 128)
            return false;

        //exclude "implicit 1"
        value &= 0x007FFFFF;

        // encode exponent
        unsigned long exponent = (binexp + 127) << 23;
        value |= exponent;
    }

// encode sign
    unsigned long sign = negative << 31;
    value |= sign;

    if (val)
    {
        *(unsigned long*)val = value;
    }

    return true;
}

char* LoadAscii(char* a_out_var_name, char* asciifilename, char* a_is_fast)
{
    int len;
    char* buff = (char*)LoadBuffer(asciifilename, 0, &len);
    if (buff)
    {
        char* buff2 = new char[len + 2];
        memcpy(buff2, buff, len);
        buff2[len] = 0;
        buff2[len + 1] = 0;
        delete[] buff;
        buff = buff2;
        if (buff[len - 1] != '\n')
            buff[len] = '\n';

        int nrrows = 0;
        int nrcols = 0;
        char* tmpb = buff;
        while ((tmpb = strchr(tmpb, '\n')))
        {
            ++nrrows;
            ++tmpb;
        }
        unsigned int output_sizes[nrrows];
        tmpb = buff;
        int chindx = 0;
        while (char* actuap_new_line = strchr(tmpb, '\n'))
        {
            nrcols = 1;
            while ((tmpb = strchr(tmpb, '\t')))
            {
                if (tmpb > actuap_new_line)
                    break;
                ++nrcols;
                ++tmpb;
            }
            output_sizes[chindx++] = nrcols;
            tmpb = actuap_new_line + 1;
        }

        CVariable* output = NewCVariable();
        output->Rebuild(nrrows, output_sizes);
        if (a_is_fast && (!strcmp(a_is_fast, "true") || !strcmp(a_is_fast, "fast")))
        {
            for (unsigned int i = 0; i < output->m_total_samples.m_size; ++i)
            {
                float elem = 0;
                for (unsigned int j = 0; j < output->m_total_samples.m_data[i]; ++j)
                {
                    float_scan(buff, &elem);
                    if (j == output->m_total_samples.m_data[i] - 1)
                        buff = strchr(buff + 1, '\n');
                    else
                        buff = strchr(buff + 1, '\t');
                    ++buff;
                    output->m_data[i][j] = elem;
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < output->m_total_samples.m_size; ++i)
            {
                float elem = 0;
                for (unsigned int j = 0; j < output->m_total_samples.m_data[i]; ++j)
                {
                    elem = atof(buff);
                    if (j == output->m_total_samples.m_data[i] - 1)
                        buff = strchr(buff + 1, '\n');
                    else
                        buff = strchr(buff + 1, '\t');
                    ++buff;
                    output->m_data[i][j] = elem;
                }
            }
        }

        strcpy(output->m_varname, a_out_var_name);
        m_variable_list_ref->Insert(output->m_varname, output);
    }
    return 0;
}

extern "C"
{
    char* __declspec (dllexport) Procedure(int procindx, char* a_statement_body, char* param1, char* param2, char* param3, char* param4, char* param5, char* param6, char* param7, char* param8, char* a_param9, char* a_param10, char* a_param11, char* a_param12)
    {
        switch (procindx)
        {
        case 0:
            return UserInput(param1, param2, param3, param4, param5);
        case 1:
            return Extract(param1, param2, param3, param4);
        case 2:
            return Downsample(param1, param2, param3);
        case 3:
            return Deriv(param1);
        case 4:
            return RowMean(param1, param2);
        case 5:
            return PutTMarkersOnValues(param1, param2);
        case 6:
            return DownsampleGauss(param1, param2, param3);
        case 7:
            return Intersect(param1, param2, param3);
        case 8:
            return ToString(param1);
        case 9:
            return CreateSpline(param1, param2, param3, param4);
        case 10:
            return Cat(param1, param2);
        case 11:
            return PutTwoSpindleTMarkers(param1, param2, param3);
        case 12:
            return IntegTrapezoid(param1, param2, param3);
        case 13:
            return WriteAscii(param1, param2, param3);
        case 14:
            return RowMeanE(param1, param2, param3);
        case 15:
            return CatToFile(param1, param2, param3, param4);
        case 16:
            return GetVarName(param1);
        case 17:
            return LoadAscii(param1, param2, param3);
        case 18:
            return Append(param1, param2);
        case 19:
            return Oversample(param1, param2, param3);
        case 20:
            return integral(param1, param2, param3, param4);
        case 21:
            return AddMarker(param1, param2, param3, param4, param5, param6, param7);
        case 22:
            return GetMarkerStart(param1, param2);
        case 23:
            return GetMarkerLength(param1, param2);
        case 24:
            return GetMarkerChannel(param1, param2);
        case 25:
            return DownsampleToNrSamples(param1, param2, param3);
        case 26:
            return Annotate(param1, param2, param3, param4, param5, param6, param7);
        case 27:
            return Oversample2(param1, param2, param3);
        case 28:
            return AddMarkers(param1, param2, param3, param4);
        case 29:
            return PopBack(param1, param2);
        case 30:
            return PopFront(param1, param2);
        }
        return 0;
    }

    void __declspec (dllexport) InitLib(CSignalCodec_List* a_datalist, CVariable_List* a_variablelist, NewCVariable_Proc a_newCVariable, NewChar_Proc a_newChar, Call_Proc a_inpCall, FFT_PROC a_inpFFT, RFFT_PROC a_inpRFFT, FFTEXEC_PROC a_inpFFTEXEC, FFTDESTROY_PROC a_inpFFTDESTROY)
    {
        m_variable_list_ref = a_variablelist;
        m_data_list_ref     = a_datalist;
        NewChar             = a_newChar;
        NewCVariable        = a_newCVariable;
    }

    int __declspec (dllexport) GetProcedureList(TFunctionLibrary* a_functionlibrary_reference)
    {
        CStringVec FunctionList;
        FunctionList.AddElement("UserInput(indataname'the input data', a_fit_width, a_display_type, a_align, a_grid)");
        FunctionList.AddElement("Extract(indataname'the input data', outputdataname'the output', from, len)");
        FunctionList.AddElement("Downsample(indataname'the input data', outputdataname'the output', ratio)");
        FunctionList.AddElement("Deriv(indataname'the input data')");
        FunctionList.AddElement("RowMean(indataname'the input data', outdataname)");
        FunctionList.AddElement("PutTMarkersOnValues(indataname'the input data', values)");
        FunctionList.AddElement("DownsampleGauss(indataname'the input data', outputdataname'the output',ratio)");
        FunctionList.AddElement("Intersect(indataname'the input data', outputdataname'the output', value)");
        FunctionList.AddElement("ToString(indataname'the input data')");
        FunctionList.AddElement("CreateSpline(outdataname'the output data', N, xs, ys)");
        FunctionList.AddElement("Cat(indataname'the input data', indatatocat)");
        FunctionList.AddElement("PutTwoSpindleTMarkers(indataname'the input data', RMDSGExtracted, Intersections)");
        FunctionList.AddElement("IntegTrapezoid(indataname'the input data', outputdataname'the output'), spectrum");
        FunctionList.AddElement("WriteAscii(invarname, filename, type)");
        FunctionList.AddElement("RowMeanE(invarname, outdataname, enable)");
        FunctionList.AddElement("CatToFile (invarname, indxs, filename, name'name of row: optional')");
        FunctionList.AddElement("GetVarName(indataname)");
        FunctionList.AddElement("LoadAscii(outdataname, asciifilename, is_fast)");
        FunctionList.AddElement("Append(destination'the input data', data_to_append)");
        FunctionList.AddElement("Oversample(indataname'the input data', outputdataname'the output',ratio)");
        FunctionList.AddElement("integral(indataname'the input data', unitstart, unitstop, outputdataname'the output')");
        FunctionList.AddElement("AddMarker(a_indataname, a_start_sample, a_length, a_label, a_channel, a_color, a_id)");
        FunctionList.AddElement("GetMarkerStart(char* a_indataname, char* a_marker_index)");
        FunctionList.AddElement("GetMarkerLength(char* a_indataname, char* a_marker_index)");
        FunctionList.AddElement("GetMarkerChannel(char* a_indataname, char* a_marker_index)");
        FunctionList.AddElement("DownsampleToNrSamples(a_in_var_name, a_var_name_to_store, a_nr_samples)");
        FunctionList.AddElement("Annotate(a_indataname, a_start, a_length, a_label, a_channel, a_color, a_id)");
        FunctionList.AddElement("Oversample2(indataname'the input data', outputdataname'the output',ratio)");
        FunctionList.AddElement("AddMarkers(indataname, markers, max, min)");
        FunctionList.AddElement("PopBack(indataname, nr_elements)");
        FunctionList.AddElement("PopFront(indataname, nr_elements)");
        a_functionlibrary_reference->ParseFunctionList(&FunctionList);
        return FunctionList.m_size;
    }

    bool __declspec (dllexport) CopyrightInfo(CStringMx* a_copyrightinfo)
    {
        a_copyrightinfo->RebuildPreserve(a_copyrightinfo->m_size + 1);
        a_copyrightinfo->m_data[a_copyrightinfo->m_size - 1]->AddElement("This library is taking part of the DB's basic signal processing libraries.");
        return 0;
    }

    bool APIENTRY DllMain(HINSTANCE a_hInst, DWORD a_reason, LPVOID a_reserved)
    {
        return true;
    }
}
