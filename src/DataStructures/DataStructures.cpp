#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "minvect.h"
#include "datastructures.h"
#include "math.h"

TMarker::TMarker()
{
    m_start_sample  = 0;
    m_length        = 0;
    m_border_color  = 0;
    m_fill_color    = 0xeeeeee00;
    m_marker_id     = -1;
    m_label[0]      = 0;
    m_movable       = true;
    m_sizable_left  = true;
    m_sizable_right = true;
    m_left_snap_to  = ESnapMode_None;
    m_right_snap_to = ESnapMode_None;
}

ISignalCodec::~ISignalCodec()
{}

bool ISignalCodec::SCopyTo(ISignalCodec* output, int* activechannels)
{
    if (!output)
        return false;
    unsigned int i = 0;
    for (unsigned int j = 0; j < m_total_samples.m_size; j++)
    {
        output->m_sample_rates.m_data[i] = m_sample_rates.m_data[j];
        strcpy(output->m_vertical_units.m_data[i].s, m_vertical_units.m_data[j].s);
        strcpy(output->m_labels.m_data[i].s, m_labels.m_data[j].s);
        strcpy(output->m_transducers.m_data[i].s, m_transducers.m_data[j].s);
        if (activechannels)
        {
            if (activechannels[j] == 1)
                i++;
        }
        else
            i++;
        if (i >= output->m_total_samples.m_size)
            goto ex1;
    }
ex1:
    output->m_horizontal_scale_start = m_horizontal_scale_start;
    strcpy(output->m_varname, m_varname);
    strcpy(output->m_recording, m_recording);
    strcpy(output->m_patient, m_patient);
    strcpy(output->m_date, m_date);
    strcpy(output->m_time, m_time);
    strcpy(output->m_horizontal_units, m_horizontal_units);
    strcpy(output->m_surface2D_vert_units, m_surface2D_vert_units);
    output->m_surface2D_vert_axis_max = m_surface2D_vert_axis_max;
    output->m_surface2D_vert_axis_min = m_surface2D_vert_axis_min;
    return true;
}

ISignalCodec::ISignalCodec()
{
    m_horizontal_scale_start = 0;
    static int staticcounter = 0;
    m_signalcodec_type = SignalCodecType_NOTDEFINED;
    sprintf(m_varname, "%s%i", "DATASERIES", staticcounter++);
    m_patient         [0] = 0;
    m_recording       [0] = 0;
    m_date            [0] = 0;
    m_time            [0] = 0;
    strcpy(m_horizontal_units, "s");
    m_surface2D_vert_units[0] = 0;
    m_surface2D_vert_axis_max = 1;
    m_surface2D_vert_axis_min = 0;
}

void ISignalCodec::ResetData()
{}

bool ISignalCodec::RefreshDataFromSource()
{
    return false;
}

bool ISignalCodec::RefreshSource()
{
    return false;
}

CVariable::CVariable()
    : ISignalCodec()
{
    m_signalcodec_type = SignalCodecType_GVARCODEC;
}

void CVariable::Rebuild(int height, unsigned int* widths)
{
    CMatrixVarLen<double>::Rebuild(height, widths);
    m_total_samples.Rebuild(height);
    m_sample_rates.Rebuild(height);
    m_labels.Rebuild(height);
    m_transducers.Rebuild(height);
    m_vertical_units.Rebuild(height);
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
    {
        m_total_samples.m_data[i] = widths[i];
        m_sample_rates.m_data[i] = 1;
        m_labels.m_data[i].s[0] = 0;
        m_transducers.m_data[i].s[0] = 0;
        strcpy(m_vertical_units.m_data[i].s, "uV");
    }
}

CVariable* CVariable::Extract(char* varnametostore, double from, double nrhunits)
{
    CVariable* temmx = new CVariable;
    CVector <unsigned int> temwidhts;
    temwidhts.Rebuild (m_total_samples.m_size);
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
    {
        int startsample = from * m_sample_rates.m_data[i];
        int nr_samples = m_total_samples.m_data[i] - startsample;
        if (nrhunits != -1)
            nr_samples = ceil(m_sample_rates.m_data[i] * nrhunits);
        temwidhts.m_data[i] = nr_samples;
    }

    temmx->Rebuild(m_total_samples.m_size, temwidhts.m_data);
    SCopyTo(temmx);
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
        memcpy(temmx->m_data[i], m_data[i] + (int)(from * m_sample_rates.m_data[i]), temwidhts.m_data[i]*sizeof(double));

    temmx->m_horizontal_scale_start = from;
    strcpy(temmx->m_varname, varnametostore);
    return temmx;
}

void CVariable::CopyFrom(CVariable* temmx)
{
    CMatrixVarLen<double>::CopyFrom(temmx);
    m_total_samples.Rebuild(temmx->m_widths.m_size);
    m_sample_rates.Rebuild(temmx->m_widths.m_size);
    m_labels.Rebuild(temmx->m_widths.m_size);
    m_transducers.Rebuild(temmx->m_widths.m_size);
    m_vertical_units.Rebuild(temmx->m_widths.m_size);
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
    {
        m_total_samples.m_data[i] = temmx->m_total_samples.m_data[i];
        m_sample_rates.m_data[i] = temmx->m_sample_rates.m_data[i];
        strcpy(m_labels.m_data[i].s, temmx->m_labels.m_data[i].s);
        strcpy(m_transducers.m_data[i].s, temmx->m_transducers.m_data[i].s);
        strcpy(m_vertical_units.m_data[i].s, temmx->m_vertical_units.m_data[i].s);
    }
    strcpy(m_recording, temmx->m_recording);
    strcpy(m_patient, temmx->m_patient);
    strcpy(m_date, temmx->m_date);
    strcpy(m_time, temmx->m_time);
    strcpy(m_horizontal_units, temmx->m_horizontal_units);
    strcpy(m_surface2D_vert_units, temmx->m_surface2D_vert_units);
    m_surface2D_vert_axis_max = temmx->m_surface2D_vert_axis_max;
    m_surface2D_vert_axis_min = temmx->m_surface2D_vert_axis_min;
}

void CVariable::RebuildPreserve(int height, unsigned int* widths)
{
    CMatrixVarLen<double>::RebuildPreserve(height, widths);
    int lastheight = m_total_samples.m_size;
    m_total_samples.RebuildPreserve(height);
    m_sample_rates.RebuildPreserve(height);
    m_labels.RebuildPreserve(height);
    m_transducers.RebuildPreserve(height);
    m_vertical_units.RebuildPreserve(height);
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
        m_total_samples.m_data[i] = widths[i];
    for (unsigned int i = lastheight; i < m_total_samples.m_size; i++)
    {
        m_sample_rates.m_data[i] = 1;
        m_labels.m_data[i].s[0] = 0;
        m_transducers.m_data[i].s[0] = 0;
        m_vertical_units.m_data[i].s[0] = 0;
    }
}

bool CVariable::GetDataBlock(double** buffer, unsigned int* start, unsigned int* nrelements, int* enable)
{
    if (buffer && m_data)
    {
        if (enable)
        {
            int j = 0;
            for (unsigned int i = 0; i < m_widths.m_size; i++)
                if (enable[i] == 1)
                {
                    if (start[i] + nrelements[i] <= m_widths.m_data[i] && nrelements[i])
                        memcpy(buffer[j], m_data[i] + start[i], nrelements[i]*sizeof(double));
                    else
                        return false;
                    j++;
                }
        }
        else
        {
            for (unsigned int i = 0; i < m_widths.m_size; i++)
                if (start[i] + nrelements[i] <= m_widths.m_data[i] && nrelements[i])
                    memcpy(buffer[i], m_data[i] + start[i], nrelements[i]*sizeof(double));
                else
                    return false;
        }
        return true;
    }
    else
        return false;

}

bool CVariable::WriteDataBlock(double** sbuffer, unsigned int* dstart, unsigned int* dnrelements, int* dactchans)
{
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
        if (m_total_samples.m_data[i])
            if (dstart[i] + dnrelements[i] > m_total_samples.m_data[i])
                return false;
    int j = 0;
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
    {
        bool cando = true;
        if (dactchans)
            if (dactchans[i] != 1)
                cando = false;
        if (cando)
        {
            if (dnrelements[i])
                memcpy(m_data[i] + dstart[i], sbuffer[j], dnrelements[i]*sizeof(double));
            j++;
        }
    }
    return true;
}

bool CVariable::AppendSamples(double** buffer, unsigned int nrsamples)
{
    unsigned int origwidths[m_widths.m_size];
    unsigned int newwidths[m_widths.m_size];
    for (unsigned int i = 0; i < m_widths.m_size; ++i)
    {
        origwidths[i] = m_widths.m_data[i];
        newwidths[i] = origwidths[i] + (nrsamples * origwidths[i]) / origwidths[0];
    }

    RebuildPreserve(m_widths.m_size, newwidths);

    for (unsigned int i = 0; i < m_widths.m_size; ++i)
    {
        int k = 0;
        for (unsigned int j = origwidths[i]; j < m_widths.m_data[i]; ++j)
            m_data[i][j] = buffer[i][k++];
    }
    return true;
}

int SearchLastPosition(const char* string, const char* stringtofind)
{
    int findlen = strlen(stringtofind);
    if (!findlen)
        return -1;
    if (!strlen(stringtofind))
        return -1;
    for (int i = strlen(string) - findlen; i > -1; i--)
    {
        char*pch = strstr (string + i, stringtofind);
        if (pch)
            return(i);
    }
    return -1;
}

void CutFileFormFullPath(char* fullpath)
{
    int lp = SearchLastPosition(fullpath, "\\");
    if (lp > -1)
        fullpath[lp + 1] = 0;
}

void CutLastDir(char* fullpath)
{
    int lp = SearchLastPosition(fullpath, "\\");
    if (lp > -1)
        fullpath[lp] = 0;
    else
        return;
    lp = SearchLastPosition(fullpath, "\\");
    if (lp > -1)
        fullpath[lp + 1] = 0;
    else
        strcat(fullpath, "\\");
}

char* IntVec::ToString(const char*delimiter)
{
    int offset = 0;
    char temstr[40];
    char result [m_size * 40];
    for (unsigned int i = 0; i < m_size; i++)
    {
        sprintf(temstr, "%i", m_data[i]);
        strcpy(result + offset, temstr);
        offset += strlen(temstr);
        strcpy(result + offset, delimiter);
        offset += strlen(delimiter);
    }
    char* result2 = new char[strlen(result) + 1];
    strcpy(result2, result);
    return result2;
}

bool DoubleVec::AddElement(const char* element)
{
    double elem;
    if (!element)
        return false;
    if (sscanf(element, "%lf", &elem))
    {
        CVector<double>::AddElement(elem);
        return true;
    }
    else
        return false;
}

void DoubleVec::AddElement(double element)
{
    CVector<double>::AddElement(element);
}

bool IntVec::AddElement(const char* element)
{
    int elem;
    if (!element)
        return false;
    if (sscanf(element, "%i", &elem))
    {
        CVector<int>::AddElement(elem);
        return true;
    }
    else
        return false;
}

IntVec::IntVec()
    : CVector()
{}

IntVec::IntVec(unsigned int a_size)
    : CVector(a_size)
{}

void IntVec::AddElement(int element)
{
    CVector<int>::AddElement(element);
}

bool IntVec::HasEelemnt(int element)
{
    bool result = false;
    for (unsigned int i = 0; i < m_size; i++)
        if (m_data[i] == element)
            result = true;
    return result;
}

bool IntVec::RebuildFrom(char* string)
{
    bool result = false;
    if (!string)
        return result;
    if (string[0] == 0)
        return result;
    if ((strcmp(string, "") == 0) || (strcmp(string, " ") == 0))
        return result;
    CVector <TString40> result1;
    int numelms2 = 0;
    result1.Rebuild(60);
    for (unsigned int i = 0; i < result1.m_size; i++)
        result1.m_data[i].s[0] = 0;
    int offset1 = 0;
    int cycleindx = 0;
    {
repxx:
        int numelms = sscanf (string + offset1, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", result1.m_data[0 + cycleindx * 60].s, result1.m_data[1 + cycleindx * 60].s, result1.m_data[2 + cycleindx * 60].s, result1.m_data[3 + cycleindx * 60].s, result1.m_data[4 + cycleindx * 60].s, result1.m_data[5 + cycleindx * 60].s, result1.m_data[6 + cycleindx * 60].s, result1.m_data[7 + cycleindx * 60].s, result1.m_data[8 + cycleindx * 60].s, result1.m_data[9 + cycleindx * 60].s, result1.m_data[10 + cycleindx * 60].s, result1.m_data[11 + cycleindx * 60].s, result1.m_data[12 + cycleindx * 60].s, result1.m_data[13 + cycleindx * 60].s, result1.m_data[14 + cycleindx * 60].s, result1.m_data[15 + cycleindx * 60].s, result1.m_data[16 + cycleindx * 60].s, result1.m_data[17 + cycleindx * 60].s, result1.m_data[18 + cycleindx * 60].s, result1.m_data[19 + cycleindx * 60].s
                              , result1.m_data[20 + cycleindx * 60].s, result1.m_data[21 + cycleindx * 60].s, result1.m_data[22 + cycleindx * 60].s, result1.m_data[23 + cycleindx * 60].s, result1.m_data[24 + cycleindx * 60].s, result1.m_data[25 + cycleindx * 60].s, result1.m_data[26 + cycleindx * 60].s, result1.m_data[27 + cycleindx * 60].s, result1.m_data[28 + cycleindx * 60].s, result1.m_data[29 + cycleindx * 60].s, result1.m_data[30 + cycleindx * 60].s, result1.m_data[31 + cycleindx * 60].s, result1.m_data[32 + cycleindx * 60].s, result1.m_data[33 + cycleindx * 60].s, result1.m_data[34 + cycleindx * 60].s, result1.m_data[35 + cycleindx * 60].s, result1.m_data[36 + cycleindx * 60].s, result1.m_data[37 + cycleindx * 60].s, result1.m_data[38 + cycleindx * 60].s, result1.m_data[39 + cycleindx * 60].s
                              , result1.m_data[40 + cycleindx * 60].s, result1.m_data[41 + cycleindx * 60].s, result1.m_data[42 + cycleindx * 60].s, result1.m_data[43 + cycleindx * 60].s, result1.m_data[44 + cycleindx * 60].s, result1.m_data[45 + cycleindx * 60].s, result1.m_data[46 + cycleindx * 60].s, result1.m_data[47 + cycleindx * 60].s, result1.m_data[48 + cycleindx * 60].s, result1.m_data[49 + cycleindx * 60].s, result1.m_data[50 + cycleindx * 60].s, result1.m_data[51 + cycleindx * 60].s, result1.m_data[52 + cycleindx * 60].s, result1.m_data[53 + cycleindx * 60].s, result1.m_data[54 + cycleindx * 60].s, result1.m_data[55 + cycleindx * 60].s, result1.m_data[56 + cycleindx * 60].s, result1.m_data[57 + cycleindx * 60].s, result1.m_data[58 + cycleindx * 60].s, result1.m_data[59 + cycleindx * 60].s
                             );
        numelms2 += numelms;
        if (numelms > 59)
        {
            int endoffset = SearchLastPosition(string + offset1, result1.m_data[59 + cycleindx * 60].s);
            char lpstext [20];
            sprintf(lpstext, "%i %i %i", endoffset, 0, 0);

            endoffset += strlen(result1.m_data[59 + cycleindx * 60].s);
            offset1 += endoffset;
            cycleindx++;

            result1.RebuildPreserve(result1.m_size + 60);
            goto repxx;
        }
    }

    if (numelms2 > 0)
    {
        Rebuild(numelms2);
        int j = 0;
        for (int i = 0; i < numelms2; i++)
        {
            int elem;
            if (sscanf (result1.m_data[i].s, "%i", &elem))
                m_data[j++] = elem;
        }
        if (j != numelms2)
            RebuildPreserve(j);
        else
            result = true;
    }
    return result;
}

UIntVec::UIntVec()
    : CVector()
{}

UIntVec::UIntVec(unsigned int a_size)
    : CVector(a_size)
{}

bool UIntVec::AddElement(const char* element)
{
    unsigned int elem;
    if (!element)
        return false;
    if (sscanf(element, "%i", &elem))
    {
        CVector<unsigned int>::AddElement(elem);
        return true;
    }
    else
        return false;
}

void UIntVec::AddElement(unsigned int element)
{
    CVector<unsigned int>::AddElement(element);
}

bool UIntVec::HasEelemnt(unsigned int element)
{
    bool result = false;
    for (unsigned int i = 0; i < m_size; i++)
        if (m_data[i] == element)
            result = true;
    return result;
}

bool UIntVec::RebuildFrom(char* string)
{
    bool result = false;
    if (!string)
        return result;
    if (string[0] == 0)
        return result;
    if ((strcmp(string, "") == 0) || (strcmp(string, " ") == 0))
        return result;
    CVector <TString40> result1;
    unsigned int numelms2 = 0;

    result1.Rebuild(60);
    for (unsigned int i = 0; i < result1.m_size; i++)
        result1.m_data[i].s[0] = 0;
    unsigned int offset1 = 0;
    unsigned int cycleindx = 0;
    {
repxx:
        int numelms = sscanf (string + offset1, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", result1.m_data[0 + cycleindx * 60].s, result1.m_data[1 + cycleindx * 60].s, result1.m_data[2 + cycleindx * 60].s, result1.m_data[3 + cycleindx * 60].s, result1.m_data[4 + cycleindx * 60].s, result1.m_data[5 + cycleindx * 60].s, result1.m_data[6 + cycleindx * 60].s, result1.m_data[7 + cycleindx * 60].s, result1.m_data[8 + cycleindx * 60].s, result1.m_data[9 + cycleindx * 60].s, result1.m_data[10 + cycleindx * 60].s, result1.m_data[11 + cycleindx * 60].s, result1.m_data[12 + cycleindx * 60].s, result1.m_data[13 + cycleindx * 60].s, result1.m_data[14 + cycleindx * 60].s, result1.m_data[15 + cycleindx * 60].s, result1.m_data[16 + cycleindx * 60].s, result1.m_data[17 + cycleindx * 60].s, result1.m_data[18 + cycleindx * 60].s, result1.m_data[19 + cycleindx * 60].s
                              , result1.m_data[20 + cycleindx * 60].s, result1.m_data[21 + cycleindx * 60].s, result1.m_data[22 + cycleindx * 60].s, result1.m_data[23 + cycleindx * 60].s, result1.m_data[24 + cycleindx * 60].s, result1.m_data[25 + cycleindx * 60].s, result1.m_data[26 + cycleindx * 60].s, result1.m_data[27 + cycleindx * 60].s, result1.m_data[28 + cycleindx * 60].s, result1.m_data[29 + cycleindx * 60].s, result1.m_data[30 + cycleindx * 60].s, result1.m_data[31 + cycleindx * 60].s, result1.m_data[32 + cycleindx * 60].s, result1.m_data[33 + cycleindx * 60].s, result1.m_data[34 + cycleindx * 60].s, result1.m_data[35 + cycleindx * 60].s, result1.m_data[36 + cycleindx * 60].s, result1.m_data[37 + cycleindx * 60].s, result1.m_data[38 + cycleindx * 60].s, result1.m_data[39 + cycleindx * 60].s
                              , result1.m_data[40 + cycleindx * 60].s, result1.m_data[41 + cycleindx * 60].s, result1.m_data[42 + cycleindx * 60].s, result1.m_data[43 + cycleindx * 60].s, result1.m_data[44 + cycleindx * 60].s, result1.m_data[45 + cycleindx * 60].s, result1.m_data[46 + cycleindx * 60].s, result1.m_data[47 + cycleindx * 60].s, result1.m_data[48 + cycleindx * 60].s, result1.m_data[49 + cycleindx * 60].s, result1.m_data[50 + cycleindx * 60].s, result1.m_data[51 + cycleindx * 60].s, result1.m_data[52 + cycleindx * 60].s, result1.m_data[53 + cycleindx * 60].s, result1.m_data[54 + cycleindx * 60].s, result1.m_data[55 + cycleindx * 60].s, result1.m_data[56 + cycleindx * 60].s, result1.m_data[57 + cycleindx * 60].s, result1.m_data[58 + cycleindx * 60].s, result1.m_data[59 + cycleindx * 60].s
                             );
        numelms2 += numelms;
        if (numelms > 59)
        {
            int endoffset = SearchLastPosition(string + offset1, result1.m_data[59 + cycleindx * 60].s);
            char lpstext [20];
            sprintf(lpstext, "%i %i %i", endoffset, 0, 0);

            endoffset += strlen(result1.m_data[59 + cycleindx * 60].s);
            offset1 += endoffset;
            cycleindx++;

            result1.RebuildPreserve(result1.m_size + 60);
            goto repxx;
        }
    }

    if (numelms2 > 0)
    {
        Rebuild(numelms2);
        unsigned int j = 0;
        for (unsigned int i = 0; i < numelms2; i++)
        {
            unsigned int elem;
            if (sscanf (result1.m_data[i].s, "%i", &elem))
                m_data[j++] = elem;
        }
        if (j != numelms2)
            RebuildPreserve(j);
        else
            result = true;
    }
    return result;
}

void CStringVec::AddElement(const char* element)
{
    RebuildPreserve(m_size + 1);
    strcpy(m_data[m_size - 1].s, element);
}

bool CStringVec::HasEelemnt(const char* element)
{
    bool result = false;
    for (unsigned int i = 0; i < m_size; i++)
        if (!strcmp(m_data[i].s, element))
            result = true;
    return result;
}

bool CStringVec::RebuildFrom(float* elms, int nrelms)
{
    Rebuild(nrelms);
    for (int i = 0; i < nrelms; i++)
        sprintf(m_data[i].s, "%0.4f", elms[i]);
    return true;
}
bool CStringVec::RebuildFrom(double* elms, int nrelms)
{
    Rebuild(nrelms);
    for (int i = 0; i < nrelms; i++)
        sprintf(m_data[i].s, "%0.4f", elms[i]);
    return true;
}

bool CStringVec::RebuildFrom(char* string)
{
    bool result = false;
    if (!string)
        return result;
    if (string[0] == 0)
        return result;
    if ((strcmp(string, "") == 0) || (strcmp(string, " ") == 0))
        return result;
    CVector <TStringMAX> result1;
    int numelms2 = 0;
    result1.Rebuild(60);
    for (unsigned int i = 0; i < result1.m_size; i++)
        result1.m_data[i].s[0] = 0;
    int offset1 = 0;
    int cycleindx = 0;
    {
repxx:
        int numelms = sscanf (string + offset1, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", result1.m_data[0 + cycleindx * 60].s, result1.m_data[1 + cycleindx * 60].s, result1.m_data[2 + cycleindx * 60].s, result1.m_data[3 + cycleindx * 60].s, result1.m_data[4 + cycleindx * 60].s, result1.m_data[5 + cycleindx * 60].s, result1.m_data[6 + cycleindx * 60].s, result1.m_data[7 + cycleindx * 60].s, result1.m_data[8 + cycleindx * 60].s, result1.m_data[9 + cycleindx * 60].s, result1.m_data[10 + cycleindx * 60].s, result1.m_data[11 + cycleindx * 60].s, result1.m_data[12 + cycleindx * 60].s, result1.m_data[13 + cycleindx * 60].s, result1.m_data[14 + cycleindx * 60].s, result1.m_data[15 + cycleindx * 60].s, result1.m_data[16 + cycleindx * 60].s, result1.m_data[17 + cycleindx * 60].s, result1.m_data[18 + cycleindx * 60].s, result1.m_data[19 + cycleindx * 60].s
                              , result1.m_data[20 + cycleindx * 60].s, result1.m_data[21 + cycleindx * 60].s, result1.m_data[22 + cycleindx * 60].s, result1.m_data[23 + cycleindx * 60].s, result1.m_data[24 + cycleindx * 60].s, result1.m_data[25 + cycleindx * 60].s, result1.m_data[26 + cycleindx * 60].s, result1.m_data[27 + cycleindx * 60].s, result1.m_data[28 + cycleindx * 60].s, result1.m_data[29 + cycleindx * 60].s, result1.m_data[30 + cycleindx * 60].s, result1.m_data[31 + cycleindx * 60].s, result1.m_data[32 + cycleindx * 60].s, result1.m_data[33 + cycleindx * 60].s, result1.m_data[34 + cycleindx * 60].s, result1.m_data[35 + cycleindx * 60].s, result1.m_data[36 + cycleindx * 60].s, result1.m_data[37 + cycleindx * 60].s, result1.m_data[38 + cycleindx * 60].s, result1.m_data[39 + cycleindx * 60].s
                              , result1.m_data[40 + cycleindx * 60].s, result1.m_data[41 + cycleindx * 60].s, result1.m_data[42 + cycleindx * 60].s, result1.m_data[43 + cycleindx * 60].s, result1.m_data[44 + cycleindx * 60].s, result1.m_data[45 + cycleindx * 60].s, result1.m_data[46 + cycleindx * 60].s, result1.m_data[47 + cycleindx * 60].s, result1.m_data[48 + cycleindx * 60].s, result1.m_data[49 + cycleindx * 60].s, result1.m_data[50 + cycleindx * 60].s, result1.m_data[51 + cycleindx * 60].s, result1.m_data[52 + cycleindx * 60].s, result1.m_data[53 + cycleindx * 60].s, result1.m_data[54 + cycleindx * 60].s, result1.m_data[55 + cycleindx * 60].s, result1.m_data[56 + cycleindx * 60].s, result1.m_data[57 + cycleindx * 60].s, result1.m_data[58 + cycleindx * 60].s, result1.m_data[59 + cycleindx * 60].s
                             );
        numelms2 += numelms;
        if (numelms > 59)
        {
            int endoffset = SearchLastPosition(string + offset1, result1.m_data[59 + cycleindx * 60].s);
            char lpstext [20];
            sprintf(lpstext, "%i %i %i", endoffset, 0, 0);

            endoffset += strlen(result1.m_data[59 + cycleindx * 60].s);
            offset1 += endoffset;
            cycleindx++;

            result1.RebuildPreserve(result1.m_size + 60);
            goto repxx;
        }
    }
    Rebuild(numelms2);
    for (int i = 0; i < numelms2; i++)
        strcpy(m_data[i].s, result1.m_data[i].s);
    result = true;
    return result;
}

char* DoubleVec::ToString(const char*delimiter)
{
    int offset = 0;
    char temstr[40];
    char result [m_size * 40];
    for (unsigned int i = 0; i < m_size; i++)
    {
        sprintf(temstr, "%0.7f", m_data[i]);
        strcpy(result + offset, temstr);
        offset += strlen(temstr);
        strcpy(result + offset, delimiter);
        offset += strlen(delimiter);
    }
    char* result2 = new char[strlen(result) + 1];
    strcpy(result2, result);
    return result2;
}

bool DoubleVec::HasEelemnt(double element)
{
    bool result = false;
    for (unsigned int i = 0; i < m_size; i++)
        if (m_data[i] == element)
            result = true;
    return result;
}

void ReplaceCharactersWith_sameSize(char* iString, const char* Key, const char* ReplaceChar)
{
    char * pch;
    pch = strpbrk (iString, Key);
    while (pch != NULL)
    {
        memcpy(pch, ReplaceChar, 1);
        pch = strpbrk (pch + 1, Key);
    }
}

bool DoubleVec::RebuildFrom(char* string)
{
    bool result = false;
    if (!string)
        return result;
    if (string[0] == 0)
        return result;
    if ((strcmp(string, "") == 0) || (strcmp(string, " ") == 0))
        return result;

    ReplaceCharactersWith_sameSize(string, ",", " ");

    CVector <TString40> result1;
    int numelms2 = 0;
    result1.Rebuild(60);
    for (unsigned int i = 0; i < result1.m_size; i++)
        result1.m_data[i].s[0] = 0;
    int offset1 = 0;
    int cycleindx = 0;
    {
repxx:
        int numelms = sscanf (string + offset1, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", result1.m_data[0 + cycleindx * 60].s, result1.m_data[1 + cycleindx * 60].s, result1.m_data[2 + cycleindx * 60].s, result1.m_data[3 + cycleindx * 60].s, result1.m_data[4 + cycleindx * 60].s, result1.m_data[5 + cycleindx * 60].s, result1.m_data[6 + cycleindx * 60].s, result1.m_data[7 + cycleindx * 60].s, result1.m_data[8 + cycleindx * 60].s, result1.m_data[9 + cycleindx * 60].s, result1.m_data[10 + cycleindx * 60].s, result1.m_data[11 + cycleindx * 60].s, result1.m_data[12 + cycleindx * 60].s, result1.m_data[13 + cycleindx * 60].s, result1.m_data[14 + cycleindx * 60].s, result1.m_data[15 + cycleindx * 60].s, result1.m_data[16 + cycleindx * 60].s, result1.m_data[17 + cycleindx * 60].s, result1.m_data[18 + cycleindx * 60].s, result1.m_data[19 + cycleindx * 60].s
                              , result1.m_data[20 + cycleindx * 60].s, result1.m_data[21 + cycleindx * 60].s, result1.m_data[22 + cycleindx * 60].s, result1.m_data[23 + cycleindx * 60].s, result1.m_data[24 + cycleindx * 60].s, result1.m_data[25 + cycleindx * 60].s, result1.m_data[26 + cycleindx * 60].s, result1.m_data[27 + cycleindx * 60].s, result1.m_data[28 + cycleindx * 60].s, result1.m_data[29 + cycleindx * 60].s, result1.m_data[30 + cycleindx * 60].s, result1.m_data[31 + cycleindx * 60].s, result1.m_data[32 + cycleindx * 60].s, result1.m_data[33 + cycleindx * 60].s, result1.m_data[34 + cycleindx * 60].s, result1.m_data[35 + cycleindx * 60].s, result1.m_data[36 + cycleindx * 60].s, result1.m_data[37 + cycleindx * 60].s, result1.m_data[38 + cycleindx * 60].s, result1.m_data[39 + cycleindx * 60].s
                              , result1.m_data[40 + cycleindx * 60].s, result1.m_data[41 + cycleindx * 60].s, result1.m_data[42 + cycleindx * 60].s, result1.m_data[43 + cycleindx * 60].s, result1.m_data[44 + cycleindx * 60].s, result1.m_data[45 + cycleindx * 60].s, result1.m_data[46 + cycleindx * 60].s, result1.m_data[47 + cycleindx * 60].s, result1.m_data[48 + cycleindx * 60].s, result1.m_data[49 + cycleindx * 60].s, result1.m_data[50 + cycleindx * 60].s, result1.m_data[51 + cycleindx * 60].s, result1.m_data[52 + cycleindx * 60].s, result1.m_data[53 + cycleindx * 60].s, result1.m_data[54 + cycleindx * 60].s, result1.m_data[55 + cycleindx * 60].s, result1.m_data[56 + cycleindx * 60].s, result1.m_data[57 + cycleindx * 60].s, result1.m_data[58 + cycleindx * 60].s, result1.m_data[59 + cycleindx * 60].s
                             );
        numelms2 += numelms;
        if (numelms > 59)
        {
            int endoffset = SearchLastPosition(string + offset1, result1.m_data[59 + cycleindx * 60].s);
            char lpstext [20];
            sprintf(lpstext, "%i %i %i", endoffset, 0, 0);

            endoffset += strlen(result1.m_data[59 + cycleindx * 60].s);
            offset1 += endoffset;
            cycleindx++;

            result1.RebuildPreserve(result1.m_size + 60);
            goto repxx;
        }
    }

    if (numelms2 > 0)
    {
        Rebuild(numelms2);
        int j = 0;
        for (int i = 0; i < numelms2; i++)
        {
            double elem;
            if (sscanf (result1.m_data[i].s, "%lf", &elem))
                m_data[j++] = elem;
        }
        if (j != numelms2)
            RebuildPreserve(j);
        else
            result = true;
    }
    return result;
}

char* CFloatVec::ToString(const char*delimiter)
{
    int offset = 0;
    char temstr[40];
    char result [m_size * 40];
    for (unsigned int i = 0; i < m_size; i++)
    {
        sprintf(temstr, "%0.7f", m_data[i]);
        strcpy(result + offset, temstr);
        offset += strlen(temstr);
        strcpy(result + offset, delimiter);
        offset += strlen(delimiter);
    }
    char* result2 = new char[strlen(result) + 1];
    strcpy(result2, result);
    return result2;
}

bool CFloatVec::HasEelemnt(float element)
{
    bool result = false;
    for (unsigned int i = 0; i < m_size; i++)
        if (m_data[i] == element)
            result = true;
    return result;
}

bool CFloatVec::RebuildFrom(char* string)
{
    bool result = false;
    if (!string)
        return result;
    if (string[0] == 0)
        return result;
    if ((strcmp(string, "") == 0) || (strcmp(string, " ") == 0))
        return result;
    CVector <TString40> result1;
    int numelms2 = 0;
    result1.Rebuild(60);
    for (unsigned int i = 0; i < result1.m_size; i++)
        result1.m_data[i].s[0] = 0;
    int offset1 = 0;
    int cycleindx = 0;
    {
repxx:
        int numelms = sscanf (string + offset1, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", result1.m_data[0 + cycleindx * 60].s, result1.m_data[1 + cycleindx * 60].s, result1.m_data[2 + cycleindx * 60].s, result1.m_data[3 + cycleindx * 60].s, result1.m_data[4 + cycleindx * 60].s, result1.m_data[5 + cycleindx * 60].s, result1.m_data[6 + cycleindx * 60].s, result1.m_data[7 + cycleindx * 60].s, result1.m_data[8 + cycleindx * 60].s, result1.m_data[9 + cycleindx * 60].s, result1.m_data[10 + cycleindx * 60].s, result1.m_data[11 + cycleindx * 60].s, result1.m_data[12 + cycleindx * 60].s, result1.m_data[13 + cycleindx * 60].s, result1.m_data[14 + cycleindx * 60].s, result1.m_data[15 + cycleindx * 60].s, result1.m_data[16 + cycleindx * 60].s, result1.m_data[17 + cycleindx * 60].s, result1.m_data[18 + cycleindx * 60].s, result1.m_data[19 + cycleindx * 60].s
                              , result1.m_data[20 + cycleindx * 60].s, result1.m_data[21 + cycleindx * 60].s, result1.m_data[22 + cycleindx * 60].s, result1.m_data[23 + cycleindx * 60].s, result1.m_data[24 + cycleindx * 60].s, result1.m_data[25 + cycleindx * 60].s, result1.m_data[26 + cycleindx * 60].s, result1.m_data[27 + cycleindx * 60].s, result1.m_data[28 + cycleindx * 60].s, result1.m_data[29 + cycleindx * 60].s, result1.m_data[30 + cycleindx * 60].s, result1.m_data[31 + cycleindx * 60].s, result1.m_data[32 + cycleindx * 60].s, result1.m_data[33 + cycleindx * 60].s, result1.m_data[34 + cycleindx * 60].s, result1.m_data[35 + cycleindx * 60].s, result1.m_data[36 + cycleindx * 60].s, result1.m_data[37 + cycleindx * 60].s, result1.m_data[38 + cycleindx * 60].s, result1.m_data[39 + cycleindx * 60].s
                              , result1.m_data[40 + cycleindx * 60].s, result1.m_data[41 + cycleindx * 60].s, result1.m_data[42 + cycleindx * 60].s, result1.m_data[43 + cycleindx * 60].s, result1.m_data[44 + cycleindx * 60].s, result1.m_data[45 + cycleindx * 60].s, result1.m_data[46 + cycleindx * 60].s, result1.m_data[47 + cycleindx * 60].s, result1.m_data[48 + cycleindx * 60].s, result1.m_data[49 + cycleindx * 60].s, result1.m_data[50 + cycleindx * 60].s, result1.m_data[51 + cycleindx * 60].s, result1.m_data[52 + cycleindx * 60].s, result1.m_data[53 + cycleindx * 60].s, result1.m_data[54 + cycleindx * 60].s, result1.m_data[55 + cycleindx * 60].s, result1.m_data[56 + cycleindx * 60].s, result1.m_data[57 + cycleindx * 60].s, result1.m_data[58 + cycleindx * 60].s, result1.m_data[59 + cycleindx * 60].s
                             );
        numelms2 += numelms;
        if (numelms > 59)
        {
            int endoffset = SearchLastPosition(string + offset1, result1.m_data[59 + cycleindx * 60].s);
            char lpstext [20];
            sprintf(lpstext, "%i %i %i", endoffset, 0, 0);

            endoffset += strlen(result1.m_data[59 + cycleindx * 60].s);
            offset1 += endoffset;
            cycleindx++;

            result1.RebuildPreserve(result1.m_size + 60);
            goto repxx;
        }
    }

    if (numelms2 > 0)
    {
        Rebuild(numelms2);
        int j = 0;
        for (int i = 0; i < numelms2; i++)
        {
            float elem;
            if (sscanf (result1.m_data[i].s, "%f", &elem))
                m_data[j++] = elem;
        }
        if (j != numelms2)
            RebuildPreserve(j);
        else
            result = true;
    }
    return result;
}

bool CVariable::CopyFrom(ISignalCodec* indata, char* varnametostore, char* charactivechannels)
{
    int* activechannels = 0;
    IntVec activechannelindxs;
    if (charactivechannels)
        activechannelindxs.RebuildFrom(charactivechannels);

    if (activechannelindxs.m_size)
    {
        activechannels = new int[indata->m_total_samples.m_size];
        for (unsigned int i = 0; i < indata->m_total_samples.m_size; i++)
            if (activechannelindxs.HasEelemnt(i + 1))
                activechannels[i] = 1;
            else
                activechannels[i] = 0;
    }

    CVector<unsigned int>temwidths;
    if (activechannels)
    {
        int nractivechans = 0;
        for (unsigned int i = 0; i < indata->m_total_samples.m_size; i++)
            if (activechannels[i])
                nractivechans++;
        temwidths.Rebuild(nractivechans);
        int j = 0;
        for (unsigned int i = 0; i < indata->m_total_samples.m_size; i++)
            if (activechannels[i])
            {
                temwidths.m_data[j] = indata->m_total_samples.m_data[i];
                j++;
            }
    }
    else
    {
        temwidths.Rebuild(indata->m_total_samples.m_size);
        for (unsigned int i = 0; i < indata->m_total_samples.m_size; i++)
            temwidths.m_data[i] = indata->m_total_samples.m_data[i];
    }


    Rebuild(temwidths.m_size, temwidths.m_data);
    indata->SCopyTo(this, activechannels);
    if (varnametostore)
        strcpy(m_varname, varnametostore);
    unsigned int starts    [indata->m_total_samples.m_size];
    unsigned int nrelements[indata->m_total_samples.m_size];
    for (unsigned int i = 0; i < indata->m_total_samples.m_size; i++)
    {
        starts[i] = 0;
        nrelements[i] = indata->m_total_samples.m_data[i];
    }
    indata->GetDataBlock(m_data, starts, nrelements, activechannels);
    if (activechannels)
        delete[]activechannels;

    return true;
}

ISignalCodec* CreateMemoryData(const char* a_name, unsigned int a_nr_channels, double* a_samplerates, double a_nr_hor_units)
{
    unsigned int l_nr_samples[a_nr_channels];

    for (unsigned int i = 0; i < a_nr_channels; ++i)
        l_nr_samples[i] = a_nr_hor_units * a_samplerates[i];

    CVariable* output = new CVariable();
    output->Rebuild(a_nr_channels, l_nr_samples);

    for (unsigned int i = 0; i < a_nr_channels; ++i)
        output->m_sample_rates.m_data[i] = a_samplerates[i];

    for (unsigned int i = 0; i < a_nr_channels; ++i)
        for (unsigned int j = 0; j < output->m_widths.m_data[i]; ++j)
            output->m_data[i][j] = 0;

    strcpy(output->m_varname, a_name);
    return output;
}

CFunctionDescription::CFunctionDescription()
{}

CFunctionDescription::~CFunctionDescription()
{}

char* strstrfromend(const char* string, const char* stringtofind, int findlen)
{
    if (!findlen)
        findlen = strlen(stringtofind);
    if (!findlen)
        return 0;
    if (!strlen(stringtofind))
        return 0;
    for (int i = strlen(string) - findlen; i > -1; i--)
    {
        char*pch = strstr (string + i, stringtofind);
        if (pch)
            return(pch);
    }
    return 0;
}

char* GetStringBetween(const char* string, const char* from, const char* to, char** ended)
{
    char* result = 0;
    if (char* fromplace = strstr (string, from))
        if (char* toplace = strstrfromend (string, to, 0))
            if (toplace - fromplace > 1)
            {
                result = new char[toplace - fromplace];
                strncpy(result, fromplace + 1, toplace - fromplace - 1);
                result[toplace - fromplace - 1] = 0;
                if (ended)
                    *ended = fromplace + (toplace - fromplace) + 1;
            }
    return result;
}

CStringVec* SplitString(char* string, const char* key)
{
    CStringVec * result = new CStringVec;
    char* pch = strstr (string, key);
    char* lastpos = string;
    while (pch != NULL)
    {
        lastpos[pch - lastpos] = 0;
        result->AddElement(lastpos);
        lastpos = pch + 1;
        pch = strstr (pch + 1, key);
    }
    result->AddElement(lastpos);
    return result;
}

void CFunctionDescription::FillParameterDescriptions(CStringVec* parameters)
{
    m_parameter_list.resize(parameters->m_size);
    for (unsigned int i = 0; i < parameters->m_size; i++)
    {
        strcpy(m_parameter_list[i].ParameterName, parameters->m_data[i].s);
        char* paramsdescription = GetStringBetween(parameters->m_data[i].s, "'", "'", 0);
        if (paramsdescription)
        {
            RemoveStringFromBegin(paramsdescription, " ");
            RemoveStringFromEnd(paramsdescription, " ");
            strcpy(m_parameter_list[i].ParameterDescription, paramsdescription);
            char* pch = strstr(m_parameter_list[i].ParameterName, "'");
            *pch = 0;
            delete[]paramsdescription;
        }
        else
            m_parameter_list[i].ParameterDescription[0] = 0;
    }
}

void RemoveStringFromBegin(char* iString, const char*Key)
{
    char* pch = strstr (iString, Key);
    while (pch)
    {
        if (pch == iString)
            strcpy(iString, iString + strlen(Key));
        else
            return;
        pch = strstr (iString, Key);
    }
}

void RemoveStringFromEnd(char* iString, const char*Key)
{
    char* pch = strstrfromend (iString, Key, 0);
    while (pch)
    {
        if (pch == iString + strlen(iString) - strlen(Key))
            iString[strlen(iString) - strlen(Key)] = 0;
        else
            return;
        pch = strstrfromend (iString, Key, 0);
    }
}

void RemoveStringsFromEndAndBegin(CStringVec* sv1, const char* Key)
{
    for (unsigned int i = 0; i < sv1->m_size; i++)
    {
        RemoveStringFromBegin(sv1->m_data[i].s, Key);
        RemoveStringFromEnd(sv1->m_data[i].s, Key);
    }
}

CStringVec* SplitParameter(char* parameters)
{
    CStringVec * result = new CStringVec;
    char* pch = strstr(parameters, ",");
    char* bracelet = strstr(parameters, "{");
    if (bracelet && pch > bracelet)
    {
        char* matchingBrace = findMatchingBracelet(bracelet);
        if (matchingBrace)
            pch = strstr(matchingBrace, ",");
    }

    char* lastpos = parameters;
    while (pch != NULL)
    {
        lastpos[pch - lastpos] = 0;
        result->AddElement(lastpos);
        lastpos = pch + 1;
        char* to_search_from = pch + 1;
        pch = strstr(to_search_from, ",");
        char* bracelet = strstr(to_search_from, "{");
        if (bracelet && pch > bracelet)
        {
            char* matchingBrace = findMatchingBracelet(bracelet);
            if (matchingBrace)
                pch = strstr(matchingBrace, ",");
        }
    }
    result->AddElement(lastpos);
    return result;
}

bool CFunctionDescription::ParseDescription(const char* descript)
{
    char* ended;
    char  functionname[80];
    char* nameend = strstr(descript, "(");
    if (!nameend)
        return false;
    strncpy(functionname, descript, nameend - descript + 1);
    functionname[nameend - descript] = 0;
    strcpy(m_function_name, functionname);
    RemoveStringFromBegin(m_function_name, " ");
    RemoveStringFromEnd(m_function_name, " ");

    m_function_description[0] = 0;
    char* params = GetStringBetween(descript, "(", ")", &ended);
    if (params)
    {
        RemoveStringFromBegin(params, " ");
        RemoveStringFromEnd(params, " ");
        CStringVec* sv1 = SplitParameter(params);
        if (!sv1)
        {
            delete[]params;
            return false;
        }
        RemoveStringsFromEndAndBegin(sv1, " ");
        FillParameterDescriptions(sv1);
        delete sv1;
        delete[]params;
        return true;
    }
    else
        return true;
}

void TFunctionLibrary::ParseFunctionList(CStringVec* functionlist)
{
    m_function_description_list.resize(functionlist->m_size);
    for (unsigned int i = 0; i < functionlist->m_size; i++)
        m_function_description_list[i].ParseDescription(functionlist->m_data[i].s);
}

char* MakeString(NewChar_Proc newChar, const char* str1, const char* str2, const char* str3, const char* str4, const char* str5, const char* str6, const char* str7, const char* str8, const char* str9, const char* str10)
{
    char* result;
    if (newChar)
        result = newChar(14000);
    else
        result = new char[14000];
    result[0] = 0;
    if (str1)
        strcat(result, str1);
    if (str2)
        strcat(result, str2);
    if (str3)
        strcat(result, str3);
    if (str4)
        strcat(result, str4);
    if (str5)
        strcat(result, str5);
    if (str6)
        strcat(result, str6);
    if (str7)
        strcat(result, str7);
    if (str8)
        strcat(result, str8);
    if (str9)
        strcat(result, str9);
    if (str10)
        strcat(result, str10);
    return result;
}

void CSignalProcessorBase::initialize(CSignalCodec_List* a_datalist, CVariable_List* a_variablelist, NewCVariable_Proc a_newCVariable, NewChar_Proc a_newChar, Call_Proc a_Call, FFT_PROC a_FFT, RFFT_PROC a_RFFT, FFTEXEC_PROC a_FFTEXEC, FFTDESTROY_PROC a_FFTDESTROY)
{
    m_variable_list_ref = a_variablelist;
    m_data_list_ref     = a_datalist;
    m_newchar_proc      = a_newChar;
    m_newvariable_proc  = a_newCVariable;
    m_fft_proc          = a_FFT;
    m_fftexec_proc      = a_FFTEXEC;
    m_rfft_proc         = a_RFFT;
    m_fftdestroy_proc   = a_FFTDESTROY;
    m_callscript_proc   = a_Call;
}

Call_Proc CSignalProcessorBase::m_callscript_proc = 0;
bool CSignalProcessorBase::error_already_displayed = false;
