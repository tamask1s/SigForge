#include <iostream>
#include <map>
#include <vector>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <complex>
#include <cmath>
#include <cstring>
#include <random>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "minvect.h"
#include "datastructures.h"
#include "Distances.h"
#include "ZaxJsonParser.h"

class CBasicD: public CSignalProcessorBase
{
public:
    static CBasicD& BasicD()
    {
        static CBasicD singleton_instance;
        return singleton_instance;
    }

    void MovingWindow(double* a_dst, double* a_src, unsigned int a_data_size, double* a_kernel, unsigned int a_kernel_size /** must be an odd number */)
    {
        int border = (a_kernel_size - 1) / 2;
        for (unsigned int i = border; i < a_data_size - border; ++i)
        {
            a_dst[i] = 0;
            for (int j = -border; j < border; ++j)
            {
                a_dst[i] += a_kernel[j + border] * a_src[i + j] * 0.02;
            }
        }
        for (int i = 0; i < border; ++i)
        {
            a_dst[i] = a_src[i];
            a_dst[a_data_size - 1 - i] = a_src[a_data_size - 1 - i];
        }
    }

    char* TruncateSTDev(char* a_var, char* a_median, char* a_stdev, char* a_ratio)
    {
        char* res = 0;
        if (!a_var || !a_median || !a_stdev)
            res = MakeString(m_newchar_proc, "ERROR: TruncateSTDev: not enough arguments.");
        else
        {
            CVariable *var = m_variable_list_ref->variablemap_find(a_var);
            CVariable *median = m_variable_list_ref->variablemap_find(a_median);
            CVariable *stdev = m_variable_list_ref->variablemap_find(a_stdev);
            if (!var || !median || !stdev)
                res = MakeString(m_newchar_proc, "ERROR: TruncateSTDev: variable not found.");
            else
            {
                double stdevratio = 1;
                if (a_ratio && strlen(a_ratio))
                    stdevratio = atof(a_ratio);
                unsigned int nr_channels = var->m_total_samples.m_size;
                for (unsigned int ch = 0; ch < nr_channels; ++ch)
                {
                    double maxval = (median->m_data[ch][0] + stdev->m_data[ch][0]) * stdevratio;
                    double minval = (median->m_data[ch][0] - stdev->m_data[ch][0]) * stdevratio;
                    for (unsigned int i = 0; i < var->m_total_samples.m_data[ch]; ++i)
                        if (var->m_data[ch][i] > maxval)
                            var->m_data[ch][i] = maxval;
                        else if (var->m_data[ch][i] < minval)
                            var->m_data[ch][i] = minval;
//                        if (var->m_data[ch][i] > maxval)
//                            var->m_data[ch][i] = median->m_data[ch][0];
//                        else if (var->m_data[ch][i] < minval)
//                            var->m_data[ch][i] = median->m_data[ch][0];
                }
            }
        }
        return res;
    }

    char* NResultPerRow(char* a_dst_name, char* a_src_name, CVariable*& a_dst, CVariable*& a_src, unsigned int a_nr_res = 1)
    {
        char* res = 0;
        if (!a_dst_name || !a_src_name)
            res = MakeString(m_newchar_proc, "ERROR: NResultPerRow: not enough arguments.");
        else
        {
            a_src = m_variable_list_ref->variablemap_find(a_src_name);
            if (!a_src)
                res = MakeString(m_newchar_proc, "ERROR: NResultPerRow ", a_src_name, " not found.");
            else
            {
                unsigned int nr_channels = a_src->m_total_samples.m_size;
                unsigned int nr_dst_elements[nr_channels];
                for (unsigned int i = 0; i < nr_channels; ++i)
                    nr_dst_elements[i] = a_nr_res;
                a_dst = m_newvariable_proc();
                a_dst->Rebuild(nr_channels, nr_dst_elements);
                a_src->SCopyTo(a_dst);
                strcpy(a_dst->m_varname, a_dst_name);
                m_variable_list_ref->Insert(a_dst->m_varname, a_dst);
            }
        }
        return res;
    }

    char* STDev(char* a_dst_name, char* a_src_name, char* a_src_mean)
    {
        CVariable *dst, *src;
        char* res = NResultPerRow(a_dst_name, a_src_name, dst, src);
        if (!res)
        {
            CVariable* src_mean = m_variable_list_ref->variablemap_find(a_src_mean);
            if (!src_mean)
                res = MakeString(m_newchar_proc, "ERROR: STDev: mean variable not given.  ", a_src_mean, " not found.");
            else
            {
                unsigned int nr_channels = src->m_total_samples.m_size;
                for (unsigned int ch = 0; ch < nr_channels; ++ch)
                {
                    double var = 0;
                    for (unsigned int i = 0; i < src->m_total_samples.m_data[ch]; ++i)
                    {
                        var += (src->m_data[ch][i] - src_mean->m_data[ch][0]) * (src->m_data[ch][i] - src_mean->m_data[ch][0]);
                    }
                    var /= src->m_total_samples.m_data[ch];
                    dst->m_data[ch][0] = sqrt(var);
                }
            }
        }
        return res;
    }

    char* MaxVal(char* a_dst_name, char* a_src_name)
    {
        CVariable *dst, *src;
        char* res = NResultPerRow(a_dst_name, a_src_name, dst, src);
        if (!res)
        {
            unsigned int nr_channels = src->m_total_samples.m_size;
            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                double maxval = -10000000;
                int maxx = 0;
                for (unsigned int i = 0; i < src->m_total_samples.m_data[ch]; ++i)
                    if (maxval < src->m_data[ch][i])
                    {
                        maxval = src->m_data[ch][i];
                        maxx = i;
                    }
                dst->m_data[ch][0] = ((double)maxx) / src->m_sample_rates.m_data[ch];
            }
        }
        return res;
    }

    char* NrValuesAboveThershold(char* a_dst_name, char* a_src_name, char* a_threshold)
    {
        CVariable *dst, *src;
        double threshold = atof(a_threshold);
        char* res = NResultPerRow(a_dst_name, a_src_name, dst, src);
        if (!res)
        {
            for (unsigned int ch = 0; ch < src->m_total_samples.m_size; ++ch)
            {
                int vcount = 0;
                for (unsigned int i = 0; i < src->m_total_samples.m_data[ch]; ++i)
                    if (src->m_data[ch][i] > threshold)
                        vcount++;
                dst->m_data[ch][0] = vcount;
            }
        }
        return res;
    }

    char* FirstSpikeFromEnd(char* a_dst_name, char* a_src_name)
    {
        CVariable *dst, *src;
        char* res = NResultPerRow(a_dst_name, a_src_name, dst, src);
        if (!res)
//        {
//            unsigned int nr_channels = src->m_total_samples.m_size;
//            for (unsigned int ch = 0; ch < nr_channels; ++ch)
//            {
//                double maxval = -10000000;
//                int maxx = 0;
//                int started = 0;
//                for (unsigned int i = src->m_total_samples.m_data[ch] - 1; i < -1; --i)
//                {
//                    if (maxval < src->m_data[ch][i])
//                    {
//                        maxval = src->m_data[ch][i];
//                        maxx = i;
//                    }
//                    if (i > 0 && (src->m_data[ch][i - 1] > src->m_data[ch][i]))
//                    {
//                        cout << i <<  " / " << src->m_data[ch][i - 5] << " / " << src->m_data[ch][i] << " / " << ((double)i) / src->m_sample_rates.m_data[ch] << endl;
//                        started = 1;
//                        maxval = src->m_data[ch][i];
//                        maxx = i;
//                    }
//                    if (started == 1 && src->m_data[ch][i - 1] > src->m_data[ch][i])
//                    if (started && i > 0 && src->m_data[ch][i] < src->m_data[ch][i - 1])
//                        break;
//                }
//                dst->m_data[ch][0] = ((double)maxx) / src->m_sample_rates.m_data[ch];
//            }
//        }
        {
            unsigned int nr_channels = src->m_total_samples.m_size;
            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                double maxval = -10000000;
                int maxx = 0;
                int state = 0;
                cout << "------" << endl;
                for (int i = src->m_total_samples.m_data[ch] - 1; i < -1; --i)
                {
                    if (src->m_data[ch][i - 1] > src->m_data[ch][i]) /// felfele ivel
                    {
                        state = 2;
                        //cout << 2 << " / " << src->m_data[ch][i - 1] << " / " << src->m_data[ch][i] << " / " << ((double)i) / src->m_sample_rates.m_data[ch] << endl;
                    }
                    if (src->m_data[ch][i - 1] < src->m_data[ch][i]) /// lefele ivel
                    {
                        state = 1;
                        //cout << 1 << " / " << src->m_data[ch][i - 1] << " / " << src->m_data[ch][i] << " / " << ((double)i) / src->m_sample_rates.m_data[ch] << endl;
                    }

                    if (state == 2 && maxval < src->m_data[ch][i])
                    {
                        maxval = src->m_data[ch][i];
                        maxx = i;
                        //cout << "max" << " / " << src->m_data[ch][i - 1] << " / " << src->m_data[ch][i] << " / " << ((double)i) / src->m_sample_rates.m_data[ch] << endl;
                    }
                    if (state == 2 && i > 0 && src->m_data[ch][i] < src->m_data[ch][i - 1])
                        break;

                }
                dst->m_data[ch][0] = ((double)maxx) / src->m_sample_rates.m_data[ch];
            }
        }
        return res;
    }

    char* MinVal(char* a_dst_name, char* a_src_name)
    {
        CVariable *dst, *src;
        char* res = NResultPerRow(a_dst_name, a_src_name, dst, src);
        if (!res)
        {
            unsigned int nr_channels = src->m_total_samples.m_size;
            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                double minval = 10000000;
                int minx = 0;
                for (unsigned int i = 0; i < src->m_total_samples.m_data[ch]; ++i)
                    if (minval < src->m_data[ch][i])
                    {
                        minval = src->m_data[ch][i];
                        minx = i;
                    }
                dst->m_data[ch][0] = ((double)minx) / src->m_sample_rates.m_data[ch];
            }
        }
        return res;
    }

    char* Mean(char* a_dst_name, char* a_src_name)
    {
        CVariable *dst, *src;
        char* res = NResultPerRow(a_dst_name, a_src_name, dst, src);
        if (!res)
        {
            unsigned int nr_channels = src->m_total_samples.m_size;
            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                double var = 0;
                for (unsigned int i = 0; i < src->m_total_samples.m_data[ch]; ++i)
                {
                    var += src->m_data[ch][i];
                }
                dst->m_data[ch][0] = var / src->m_total_samples.m_data[ch];
            }
        }
        return res;
    }

    char* Median(char* a_dst_name, char* a_src_name)
    {
        CVariable *dst, *src;
        char* res = NResultPerRow(a_dst_name, a_src_name, dst, src);
        if (!res)
        {
            unsigned int nr_channels = src->m_total_samples.m_size;
            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                std::vector<double> v;
                for (unsigned int i = 0; i < src->m_total_samples.m_data[ch]; i += 10)
                    v.push_back(src->m_data[ch][i]);

                std::sort(v.begin(), v.end());
                if (v.size() % 2 == 0)
                    dst->m_data[ch][0] = (v[v.size() / 2 - 1] + v[v.size() / 2]) / 2;
                else
                    dst->m_data[ch][0] = v[v.size() / 2];
            }
        }
        return res;
    }

    char* MovingWindowFilter(char* a_dst_name, char* a_src_name, char* a_kernel_name)
    {
        char* res = 0;
        if (!a_dst_name || !a_src_name || !a_kernel_name)
            res = MakeString(m_newchar_proc, "ERROR: MovingWindowFilter: not enough arguments.");
        CVariable* src = m_variable_list_ref->variablemap_find(a_src_name);
        if (!src)
            res = MakeString(m_newchar_proc, "ERROR: MovingWindowFilter ", a_src_name, " not found.");
        CVariable* kernel = m_variable_list_ref->variablemap_find(a_kernel_name);
        if (!kernel)
            res = MakeString(m_newchar_proc, "ERROR: MovingWindowFilter ", a_kernel_name, " not found.");
        if (!res)
        {
            unsigned int nr_channels = src->m_total_samples.m_size;

            CVariable* dst = m_newvariable_proc();
            dst->Rebuild(nr_channels, src->m_total_samples.m_data);
            src->SCopyTo(dst);
            strcpy(dst->m_varname, a_dst_name);
            m_variable_list_ref->Insert(dst->m_varname, dst);

            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                MovingWindow(dst->m_data[ch], src->m_data[ch], dst->m_total_samples.m_data[ch], kernel->m_data[0], kernel->m_total_samples.m_data[0]);
            }
        }
        return res;
    }

    char* Copy(char* a_dst_name, char* a_src_name, char* a_channel_indexes)
    {
        char* res = 0;
        if (!a_dst_name || !a_src_name)
            res = MakeString(m_newchar_proc, "ERROR: Copy: not enough arguments.");
        CVariable* src = m_variable_list_ref->variablemap_find(a_src_name);
        if (!res && !src)
            res = MakeString(m_newchar_proc, "ERROR: Copy ", a_src_name, " not found.");
        if (!res)
        {
            CVariable* dst = m_newvariable_proc();
            if (!a_channel_indexes)
            {
                dst->Rebuild(src->m_total_samples.m_size, src->m_total_samples.m_data);
                src->SCopyTo(dst);
                for (unsigned int ch = 0; ch < src->m_total_samples.m_size; ++ch)
                    memcpy(dst->m_data[ch], src->m_data[ch], dst->m_total_samples.m_data[ch] * sizeof(double));
            }
            else
            {
                UIntVec out_ch;
                out_ch.RebuildFrom(a_channel_indexes);
                UIntVec new_sizes;
                IntVec active_channels(src->m_total_samples.m_size);
                new_sizes.Rebuild(out_ch.m_size);
                for (unsigned int i = 0; i < src->m_total_samples.m_size; ++i)
                    active_channels.m_data[i] = 0;
                for (unsigned int i = 0; i < new_sizes.m_size; ++i)
                {
                    if (src->m_total_samples.m_size > out_ch.m_data[i])
                    {
                        new_sizes.m_data[i] = src->m_total_samples.m_data[out_ch.m_data[i]];
                        active_channels.m_data[out_ch.m_data[i]] = 1;
                    }
                    else
                        res = MakeString(m_newchar_proc, "ERROR: Copy ", a_src_name, " channels overflow.");
                }

                if (!res)
                {
                    dst->Rebuild(new_sizes.m_size, new_sizes.m_data);
                    src->SCopyTo(dst, active_channels.m_data);
                    for (unsigned int ch = 0; ch < new_sizes.m_size; ++ch)
                    {
                        if (src->m_total_samples.m_size > out_ch.m_data[ch])
                            memcpy(dst->m_data[ch], src->m_data[out_ch.m_data[ch]], dst->m_total_samples.m_data[ch] * sizeof(double));
                        dst->m_sample_rates.m_data[ch] = src->m_sample_rates.m_data[out_ch.m_data[ch]];
                        strcpy(dst->m_vertical_units.m_data[ch].s, src->m_vertical_units.m_data[out_ch.m_data[ch]].s);
                        strcpy(dst->m_labels.m_data[ch].s, src->m_labels.m_data[out_ch.m_data[ch]].s);
                        strcpy(dst->m_transducers.m_data[ch].s, src->m_transducers.m_data[out_ch.m_data[ch]].s);
                    }
                }
            }
            strcpy(dst->m_varname, a_dst_name);
            m_variable_list_ref->Insert(dst->m_varname, dst);
        }
        return res;
    }

//    char* CopyContent(char* a_dst_name, char* a_src_name, char* a_channel_indexes)
//    {
//        char* res = 0;
//        if (!a_dst_name || !a_src_name)
//            res = MakeString(m_newchar_proc, "ERROR: Copy: not enough arguments.");
//        CVariable* src = m_variable_list_ref->variablemap_find(a_src_name);
//        if (!res && !src)
//            res = MakeString(m_newchar_proc, "ERROR: Copy ", a_src_name, " not found.");
//        if (!res)
//        {
//            CVariable* dst = m_newvariable_proc();
//            if (!a_channel_indexes)
//            {
//                dst->Rebuild(src->m_total_samples.m_size, src->m_total_samples.m_data);
//                src->SCopyTo(dst);
//                for (unsigned int ch = 0; ch < src->m_total_samples.m_size; ++ch)
//                    memcpy(dst->m_data[ch], src->m_data[ch], dst->m_total_samples.m_data[ch] * sizeof(double));
//            }
//            else
//            {
//                UIntVec out_ch;
//                out_ch.RebuildFrom(a_channel_indexes);
//                UIntVec new_sizes;
//                IntVec active_channels(src->m_total_samples.m_size);
//                new_sizes.Rebuild(out_ch.m_size);
//                for (unsigned int i = 0; i < src->m_total_samples.m_size; ++i)
//                    active_channels.m_data[i] = 0;
//                for (unsigned int i = 0; i < new_sizes.m_size; ++i)
//                {
//                    if (src->m_total_samples.m_size > out_ch.m_data[i])
//                    {
//                        new_sizes.m_data[i] = src->m_total_samples.m_data[out_ch.m_data[i]];
//                        active_channels.m_data[out_ch.m_data[i]] = 1;
//                    }
//                    else
//                        res = MakeString(m_newchar_proc, "ERROR: Copy ", a_src_name, " channels overflow.");
//                }
//
//                if (!res)
//                {
//                    dst->Rebuild(new_sizes.m_size, new_sizes.m_data);
//                    src->SCopyTo(dst, active_channels.m_data);
//                    for (unsigned int ch = 0; ch < new_sizes.m_size; ++ch)
//                    {
//                        if (src->m_total_samples.m_size > out_ch.m_data[ch])
//                            memcpy(dst->m_data[ch], src->m_data[out_ch.m_data[ch]], dst->m_total_samples.m_data[ch] * sizeof(double));
//                        dst->m_sample_rates.m_data[ch] = src->m_sample_rates.m_data[out_ch.m_data[ch]];
//                        strcpy(dst->m_vertical_units.m_data[ch].s, src->m_vertical_units.m_data[out_ch.m_data[ch]].s);
//                        strcpy(dst->m_labels.m_data[ch].s, src->m_labels.m_data[out_ch.m_data[ch]].s);
//                        strcpy(dst->m_transducers.m_data[ch].s, src->m_transducers.m_data[out_ch.m_data[ch]].s);
//                    }
//                }
//            }
//            strcpy(dst->m_varname, a_dst_name);
//            m_variable_list_ref->Insert(dst->m_varname, dst);
//        }
//        return res;
//    }

    /** removes spikes which has lower normalized sqare of total energy (RMS, normalized L2Norm) than the average. The thrashold ratio is given as parameter. spikes with much bigger energy are also removed. */
    char* CleanupSpikes(char* a_var_name, char* a_spikes, char* a_spike_radius, char* a_density_threshold)
    {
        char* res = 0;
        if (!a_var_name || !a_spikes || !a_spike_radius || !a_density_threshold)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSpikes: not enough arguments.");
        CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
        if (!var)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSpikes ", a_var_name, " not found.");
        CVariable* spikes = m_variable_list_ref->variablemap_find(a_spikes);
        if (!spikes)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSpikes ", a_spikes, " not found.");

        if (!res)
        {
            unsigned int nr_channels = var->m_total_samples.m_size;
            double density_threshold = atof(a_density_threshold);

            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                double density_accum = 0;
                unsigned int spike_count = 0;
                int spike_radius = (atof(a_spike_radius) * var->m_sample_rates.m_data[ch]);
                for (unsigned int i = spike_radius; i < spikes->m_total_samples.m_data[ch] - spike_radius; ++i)
                    if (spikes->m_data[ch][i])
                    {
                        ++spike_count;
                        density_accum += PSignalMetrics::lp_norm(var->m_data[ch] + i - spike_radius, spike_radius * 2);
                    }
                density_accum /= spike_count;
                double density_threshold_value = density_accum * density_threshold;
                for (unsigned int i = spike_radius; i < spikes->m_total_samples.m_data[ch] - spike_radius; ++i)
                    if (spikes->m_data[ch][i] && density_threshold_value)
                    {
                        double density = PSignalMetrics::lp_norm(var->m_data[ch] + i - spike_radius, spike_radius * 2);
                        if (density < density_threshold_value || density > density_threshold_value * 5)
                            spikes->m_data[ch][i] = 0;
                    }
            }
        }
        return res;
    }

    /** refines spike locations to the position of a nearby local minimum or maximum in the original variable. The maximum proximity of searching is defined by the radius. */
    char* RefineSpikes(char* a_var_name, char* a_spikes, char* a_spike_radius, char* a_sign)
    {
        char* res = 0;
        if (!a_var_name || !a_spikes || !a_spike_radius)
            res = MakeString(m_newchar_proc, "ERROR: RefineSpikes: not enough arguments.");
        CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
        if (!var)
            res = MakeString(m_newchar_proc, "ERROR: RefineSpikes ", a_var_name, " not found.");
        CVariable* spikes = m_variable_list_ref->variablemap_find(a_spikes);
        if (!spikes)
            res = MakeString(m_newchar_proc, "ERROR: RefineSpikes ", a_spikes, " not found.");
        int sign = 0;
        if (a_sign && strlen(a_sign))
            sign = atoi(a_sign);
        if (!res)
        {
            for (unsigned int ch = 0; ch < var->m_total_samples.m_size; ++ch)
            {
                int spike_radius = (atof(a_spike_radius) * var->m_sample_rates.m_data[ch]);
                for (unsigned int i = spike_radius; i < spikes->m_total_samples.m_data[ch] - spike_radius; ++i)
                    if (spikes->m_data[ch][i])
                    {
                        double spikeval = var->m_data[ch][i];
                        unsigned int newspikeindx = i;
                        bool positive_spike = spikeval > 0;
                        if (sign)
                            positive_spike = sign > 0;
                        if (positive_spike)
                        {
                            for (int j = -1 * spike_radius; j < spike_radius; ++j)
                                if (var->m_data[ch][i + j] > spikeval)
                                {
                                    spikeval = var->m_data[ch][i + j];
                                    newspikeindx = i + j;
                                }
                        }
                        else
                        {
                            for (int j = -1 * spike_radius; j < spike_radius; ++j)
                                if (var->m_data[ch][i + j] < spikeval)
                                {
                                    spikeval = var->m_data[ch][i + j];
                                    newspikeindx = i + j;
                                }
                        }
                        if (i != newspikeindx)
                        {
                            spikes->m_data[ch][newspikeindx] = spikes->m_data[ch][i];
                            spikes->m_data[ch][i] = 0;
                        }
                    }
            }
        }
        return res;
    }

    /** refines spike locations to the position of a maximum correlation around the current spike. Correlation is calculated between the original var and kernel. The radius of searching can be set by radius. */
    char* RefineSpikesWithCorr(char* a_var_name, char* a_kernel, char* a_spikes, char* a_spike_radius, char* a_correlation_threshold, char* a_method)
    {
        char* res = 0;
        if (!a_var_name || !a_spikes || !a_spike_radius || !a_kernel)
            res = MakeString(m_newchar_proc, "ERROR: RefineSpikesWithCorr: not enough arguments.");
        CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
        if (!var)
            res = MakeString(m_newchar_proc, "ERROR: RefineSpikesWithCorr ", a_var_name, " not found.");
        CVariable* spikes = m_variable_list_ref->variablemap_find(a_spikes);
        if (!spikes)
            res = MakeString(m_newchar_proc, "ERROR: RefineSpikesWithCorr ", a_spikes, " not found.");
        CVariable* kernel = m_variable_list_ref->variablemap_find(a_kernel);
        if (!kernel)
            res = MakeString(m_newchar_proc, "ERROR: RefineSpikesWithCorr ", a_kernel, " not found.");
        int method = (a_method && strlen(a_method)) ? atoi(a_method) : PCC;
        double correlation_threshold = (a_correlation_threshold && strlen(a_correlation_threshold)) ? atof(a_correlation_threshold) : -1.0;

        if (!res)
        {
            for (unsigned int ch = 0; ch < var->m_total_samples.m_size; ++ch)
            {
                int spike_radius = (atof(a_spike_radius) * var->m_sample_rates.m_data[ch]);
                int kernel_radius = kernel->m_total_samples.m_data[0] / 2;
                for (unsigned int i = spike_radius + kernel_radius + 1; i < spikes->m_total_samples.m_data[ch] - spike_radius - kernel_radius - 1; ++i)
                    if (spikes->m_data[ch][i])
                    {
                        double corr = -100.0;
                        int corrindx = 0;
                        for (int j = -1 * spike_radius; j < spike_radius; ++j)
                        {
                            double corrtmp = PCorrelator::Distance(kernel->m_data[0], var->m_data[ch] + i + j - kernel_radius,  kernel_radius * 2, (DistanceMetric)method);
                            if (corrtmp > corr)
                            {
                                corr = corrtmp;
                                corrindx = j;
                            }
                        }
                        if (corr < correlation_threshold)
                            spikes->m_data[ch][i] = 0;
                        else if (corrindx)
                        {
                            spikes->m_data[ch][i] = 0;
                            spikes->m_data[ch][i + corrindx] = 1;
                        }
                    }
            }
        }
        return res;
    }

    void XCorr(double* a_lhs, unsigned int a_lhs_len, double* a_rhs, unsigned int a_rhs_len, double& a_xcorr, int& a_corr_indx)
    {
        a_xcorr = -10000000000;
        a_corr_indx = -1;
        for (unsigned int i = 0; i < a_rhs_len - a_lhs_len; ++i)
        {
            double actual_corr = 0;
            for (unsigned int j = 0; j < a_lhs_len; ++j)
                actual_corr += a_lhs[j] * a_rhs[i + j];
            if (a_xcorr < actual_corr)
            {
                a_xcorr = actual_corr;
                a_corr_indx = i;
            }
        }
    }

    char* CleanupSignal_Spike(char* a_var_name, char* a_spikes, char* a_spike_radius, char* a_2)
    {
        char* res = 0;
        if (!a_var_name || !a_spikes || !a_spike_radius)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSignal_Spike: not enough arguments.");
        CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
        if (!var)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSignal_Spike ", a_var_name, " not found.");
        CVariable* spikes = m_variable_list_ref->variablemap_find(a_spikes);
        if (!spikes)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSignal_Spike ", a_spikes, " not found.");
        if (!res)
        {
            unsigned int nr_channels = var->m_total_samples.m_size;
            CVariable* dst = m_newvariable_proc();
            CVariable* dst2 = m_newvariable_proc();
            unsigned int nr_ekgavg_samples = (atof(a_spike_radius) * var->m_sample_rates.m_data[0]) * 2.0;
            unsigned int sizes[2] = {nr_ekgavg_samples, nr_ekgavg_samples};
            dst->Rebuild(nr_channels, sizes);
            dst2->Rebuild(nr_channels, sizes);
            var->SCopyTo(dst);
            var->SCopyTo(dst2);
            strcpy(dst->m_varname, "EKGAVG");
            strcpy(dst2->m_varname, "EKGAVG2");
            m_variable_list_ref->Insert(dst->m_varname, dst);
            m_variable_list_ref->Insert(dst2->m_varname, dst2);

            for (unsigned int ch = 0; ch < nr_channels; ++ch)
            {
                int spike_radius = (atof(a_spike_radius) * var->m_sample_rates.m_data[ch]);
                //double nr_samples = spike_radius * 2.0;
                //double avgsignal[(int)nr_samples];
                double* avgsignal = dst->m_data[ch];
                unsigned int spike_count = 0;
                for (unsigned int i = spike_radius * 1.2; i < spikes->m_total_samples.m_data[ch] - spike_radius * 1.2; ++i)
                    if (spikes->m_data[ch][i])
                    {
                        if (!spike_count)
                            for (int j = -1 * spike_radius; j < spike_radius; ++j)
                            {
                                double avgratio = 0.3;
                                if (!spike_count)
                                    avgsignal[j + spike_radius] = var->m_data[ch][i + j];
                                else
                                    avgsignal[j + spike_radius] = avgsignal[j + spike_radius] * (1.0 - avgratio) + var->m_data[ch][i + j] * avgratio;
                            }
                        else
                        {
                            double corr;
                            int corr_indx;
                            XCorr(avgsignal, spike_radius * 2, var->m_data[ch] + i - (int)(spike_radius * 1.2), spike_radius * 1.2 * 2, corr, corr_indx);
                            corr_indx -= spike_radius * 0.2;
                            //corr_indx = 0;
                            //cout << "corr_indx " << corr_indx << " corr: " << corr << endl;
                            double avgratio = 0.3;
                            if (corr > 1)
                                for (int j = -1 * spike_radius; j < spike_radius; ++j)
                                    avgsignal[j + spike_radius] = avgsignal[j + spike_radius] * (1.0 - avgratio) + var->m_data[ch][i + j + corr_indx] * avgratio;

                            for (int j = -1 * spike_radius; j < spike_radius; ++j)
                                var->m_data[ch][i + j + corr_indx] -= avgsignal[j + spike_radius];
                        }

                        ++spike_count;
                    }
            }
        }
        return res;
    }

    char* gauss_1d(char* a_dst_name, char* a_nr_samples, char* a_sampling_rate, char* a_sigma, char* a_mu, char* a_sr)
    {
        char* res = 0;
        double sigma = atof(a_sigma);
        double mu = atof(a_mu);
        double sr = atof(a_sr);
        unsigned int nr_samples = atoi(a_nr_samples);

        CVariable* dst = m_newvariable_proc();
        unsigned int sizes[] = {nr_samples};
        dst->Rebuild(1, sizes);
        dst->m_sample_rates.m_data[0] = atof(a_sampling_rate);
        strcpy(dst->m_varname, a_dst_name);
        m_variable_list_ref->Insert(dst->m_varname, dst);

        PKernels::gauss_1d_pos(dst->m_data[0], nr_samples, sigma, mu, sr);
        return res;
    }

    char* xcorr(char* a_dst_name, char* a_var_name, char* a_kernel, char* a_method)
    {
        char* res = 0;
        if (!a_dst_name || !a_var_name || !a_kernel)
            res = MakeString(m_newchar_proc, "ERROR: xcorr: not enough arguments.");
        CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
        if (!var)
            res = MakeString(m_newchar_proc, "ERROR: xcorr ", a_var_name, " not found.");
        CVariable* kernel = m_variable_list_ref->variablemap_find(a_kernel);
        if (!kernel)
            res = MakeString(m_newchar_proc, "ERROR: xcorr ", a_kernel, " not found.");
        int method = PCC;
        if (a_method && strlen(a_method))
        {
            method = atoi(a_method);
        }

        unsigned int var_nr_channels = var->m_total_samples.m_size;

        CVariable* dst = m_newvariable_proc();
        dst->Rebuild(var_nr_channels, var->m_total_samples.m_data);
        var->SCopyTo(dst);
        strcpy(dst->m_varname, a_dst_name);
        m_variable_list_ref->Insert(dst->m_varname, dst);

        for (unsigned int ch = 0; ch < var_nr_channels; ++ch)
        {
            int kernel_ch = 0;
            if (var_nr_channels == kernel->m_total_samples.m_size)
                kernel_ch = ch;
            if (a_method)
            {
                //for (unsigned int k = 0; k < kernel->m_total_samples.m_data[kernel_ch]; ++k)
                //  kernel->m_data[kernel_ch][k] = 0;
                //kernel->m_data[kernel_ch][kernel->m_total_samples.m_data[kernel_ch] / 2] = 1;
            }

            PCorrelator::DistanceMap(dst->m_data[ch], kernel->m_data[kernel_ch], kernel->m_total_samples.m_data[kernel_ch], var->m_data[ch], var->m_total_samples.m_data[ch], (DistanceMetric)method);
        }

        return res;
    }

    char* DetectSpikes(char* a_dst_name, char* a_spike_signal, char* a_threshold_signal, char* a_marker_val, char* a_refracter, char* a_previous_spike_reference_ratio, char* a_previous_spike_reference_attenuation)
    {
        char* res = 0;
        if (!a_dst_name || !a_spike_signal || !a_threshold_signal)
            res = MakeString(m_newchar_proc, "ERROR: DetectSpikes: not enough arguments.");
        CVariable* spike_signal = m_variable_list_ref->variablemap_find(a_spike_signal);
        if (!spike_signal)
            res = MakeString(m_newchar_proc, "ERROR: DetectSpikes ", a_spike_signal, " not found.");

        CVariable* threshold_signal = m_variable_list_ref->variablemap_find(a_threshold_signal);
        if (!threshold_signal)
            res = MakeString(m_newchar_proc, "ERROR: DetectSpikes ", a_threshold_signal, " not found.");

        double marker_val = 1;
        double refracter = 240;
        double previous_spike_reference_ratio = 0.5;
        double previous_spike_reference_attenuation = 30;

        if (a_marker_val && strlen(a_marker_val))
            marker_val = atof(a_marker_val);
        if (a_refracter && strlen(a_refracter))
            refracter = atof(a_refracter);
        if (a_previous_spike_reference_ratio && strlen(a_previous_spike_reference_ratio))
            previous_spike_reference_ratio = atof(a_previous_spike_reference_ratio);
        if (a_previous_spike_reference_attenuation && strlen(a_previous_spike_reference_attenuation))
            previous_spike_reference_attenuation = atof(a_previous_spike_reference_attenuation);

        unsigned int nr_channels = spike_signal->m_total_samples.m_size;

        CVariable* dst = m_newvariable_proc();
        dst->Rebuild(nr_channels, spike_signal->m_total_samples.m_data);
        spike_signal->SCopyTo(dst);
        strcpy(dst->m_varname, a_dst_name);
        m_variable_list_ref->Insert(dst->m_varname, dst);

//        CVariable* deb = m_newvariable_proc();
//        deb->Rebuild(nr_channels, spike_signal->m_total_samples.m_data);
//        spike_signal->SCopyTo(deb);
//        strcpy(deb->m_varname, "debugs");
//        m_variable_list_ref->Insert(deb->m_varname, deb);

        for (unsigned int ch = 0; ch < nr_channels; ++ch)
        {
            bool searching_for_spikes = true;
            int refracter_samples = spike_signal->m_sample_rates.m_data[ch] / (1000.0 / refracter);
            double previous_spike_amplitude = 0;

            for (unsigned int i = 0; i < dst->m_total_samples.m_data[ch] - 1; ++i)
            {
                dst->m_data[ch][i] = 0;
//                deb->m_data[ch][i] = 0;
                if (searching_for_spikes)
                {
                    if (spike_signal->m_data[ch][i] > threshold_signal->m_data[ch][i] * 1.5)
                    {
                        if (spike_signal->m_data[ch][i] > spike_signal->m_data[ch][i + 1])
                        {
                            if ((previous_spike_amplitude == 0) || (spike_signal->m_data[ch][i] > previous_spike_amplitude * previous_spike_reference_ratio))
                            {
                                previous_spike_amplitude = spike_signal->m_data[ch][i];
                                dst->m_data[ch][i] = marker_val;
                                searching_for_spikes = false;
                                i += refracter_samples;
                            }
                            else
                                previous_spike_amplitude /= 1.0 + previous_spike_reference_attenuation / spike_signal->m_sample_rates.m_data[ch];
                        }
                    }
                }
                else
                {
                    if (spike_signal->m_data[ch][i] < threshold_signal->m_data[ch][i] )
                    {
//                        deb->m_data[ch][i] = marker_val / 2;
                        searching_for_spikes = true;
                    }
                }
            }
        }
        return res;
    }

    char* AverageSignalAroundTrigger(char* a_src, char* a_dst, char* a_trigger, char* a_radius_left, char* a_radius_right, char* a_correlation_treshold, char* a_all_correlated_slices_varname) /// trigger must be 1 channel wih proper size
    {
        char* res = 0;
        if (!a_src || !a_dst || !a_trigger || !a_radius_left || !a_radius_right)
            res = MakeString(m_newchar_proc, "ERROR: AverageSignalAroundTrigger: not enough arguments.");
        CVariable* src = m_variable_list_ref->variablemap_find(a_src);
        if (!src)
            res = MakeString(m_newchar_proc, "ERROR: AverageSignalAroundTrigger ", a_src, " not found.");
        CVariable* trigger = m_variable_list_ref->variablemap_find(a_trigger);
        if (!trigger)
            res = MakeString(m_newchar_proc, "ERROR: AverageSignalAroundTrigger ", a_trigger, " not found.");
        if (!res)
        {
            unsigned int sizes[src->m_total_samples.m_size];
            for (unsigned int ch = 0; ch < src->m_total_samples.m_size; ++ch)
                sizes[ch] = (atof(a_radius_left) * src->m_sample_rates.m_data[ch]) + (atof(a_radius_right) * src->m_sample_rates.m_data[ch]);
            CVariable* dst = m_newvariable_proc();
            strcpy(dst->m_varname, a_dst);
            m_variable_list_ref->Insert(dst->m_varname, dst);
            dst->Rebuild(src->m_total_samples.m_size, sizes);
            for (unsigned int ch = 0; ch < src->m_total_samples.m_size; ++ch)
                memset(dst->m_data[ch], 0, dst->m_total_samples.m_data[ch] * sizeof(double));

            unsigned int triggercount = 0;
            int radius_left = (atof(a_radius_left) * src->m_sample_rates.m_data[0]);
            int radius_right = (atof(a_radius_right) * src->m_sample_rates.m_data[0]);
            for (unsigned int i = radius_left; i < trigger->m_total_samples.m_data[0] - radius_right; ++i)
                if (trigger->m_data[0][i])
                    ++triggercount;
            unsigned int sizes2[triggercount];
            CVariable* dst2 = 0;
            if (a_all_correlated_slices_varname && strlen(a_all_correlated_slices_varname))
            {
                dst2 = m_newvariable_proc();
                strcpy(dst2->m_varname, a_all_correlated_slices_varname);
                m_variable_list_ref->Insert(dst2->m_varname, dst2);
                for (unsigned int ch = 0; ch < triggercount; ++ch)
                    sizes2[ch] = (atof(a_radius_left) * src->m_sample_rates.m_data[0]) + (atof(a_radius_right) * src->m_sample_rates.m_data[0]);
                dst2->Rebuild(triggercount, sizes2);
                for (unsigned int ch = 0; ch < triggercount; ++ch)
                    dst2->m_sample_rates.m_data[ch] = src->m_sample_rates.m_data[0];
            }

            for (unsigned int ch = 0; ch < src->m_total_samples.m_size; ++ch)
            {
                int triggercount = 0;
                double avgsignal[dst->m_total_samples.m_data[0]];
                int radius_left = (atof(a_radius_left) * src->m_sample_rates.m_data[ch]);
                int radius_right = (atof(a_radius_right) * src->m_sample_rates.m_data[ch]);

                for (unsigned int i = radius_left; i < trigger->m_total_samples.m_data[0] - radius_right; ++i)
                {
                    if (trigger->m_data[0][i])
                    {
                        if (a_correlation_treshold && strlen(a_correlation_treshold))
                        {
                            //cout << "a_correlation_treshold: " << a_correlation_treshold << " len: " << strlen(a_correlation_treshold) << endl;
                            double corr_treshold = atof(a_correlation_treshold);
                            double corr = 100.0;
                            if (triggercount)
                            {
                                double mult = 1.0 / double(triggercount);
                                for (int j = -1 * radius_left; j < radius_right; ++j)
                                    avgsignal[j + radius_left] = dst->m_data[ch][j + radius_left] * mult;
                                corr = PCorrelator::Distance(avgsignal, src->m_data[ch] + i - radius_left, radius_left + radius_right);
                            }
                            if (corr > corr_treshold)
                            {
                                for (int j = -1 * radius_left; j < radius_right; ++j)
                                {
                                    dst->m_data[ch][j + radius_left] += src->m_data[ch][i + j];
                                    if (ch == 0 && a_all_correlated_slices_varname)
                                        dst2->m_data[triggercount][j + radius_left] = src->m_data[ch][i + j];
                                }
                                ++triggercount;
                            }
                        }
                        else
                        {
                            for (int j = -1 * radius_left; j < radius_right; ++j)
                            {
                                dst->m_data[ch][j + radius_left] += src->m_data[ch][i + j];
                                if (ch == 0 && a_all_correlated_slices_varname)
                                    dst2->m_data[triggercount][j + radius_left] = src->m_data[ch][i + j];
                            }
                            ++triggercount;
                        }
                    }
                }
                if (triggercount)
                {
                    double mult = 1.0 / double(triggercount);
                    for (int j = -1 * radius_left; j < radius_right; ++j)
                        dst->m_data[ch][j + radius_left] *= mult;
                }
                if (ch == 0 && a_all_correlated_slices_varname)
                    dst2->RebuildPreserve(triggercount, sizes2);
            }
        }
        return res;
    }

    char* SubstractSignalAtTriggers(char* a_var_name, char* a_trigger, char* a_signal, char* a_radius_left, char* a_radius_right)
    {
        char* res = 0;
        if (!a_var_name || !a_trigger || !a_signal)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSignal_Spike: not enough arguments.");
        CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
        if (!var)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSignal_Spike ", a_var_name, " not found.");
        CVariable* trigger = m_variable_list_ref->variablemap_find(a_trigger);
        if (!trigger)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSignal_Spike ", a_trigger, " not found.");
        CVariable* signal = m_variable_list_ref->variablemap_find(a_signal);
        if (!signal)
            res = MakeString(m_newchar_proc, "ERROR: CleanupSignal_Spike ", a_signal, " not found.");
        if (!res)
        {
            for (unsigned int ch = 0; ch < var->m_total_samples.m_size; ++ch)
            {
                int radius_left = (atof(a_radius_left) * var->m_sample_rates.m_data[ch]);
                int radius_right = (atof(a_radius_right) * var->m_sample_rates.m_data[ch]);
                for (unsigned int i = radius_left; i < trigger->m_total_samples.m_data[0] - radius_right; ++i)
                {
                    if (trigger->m_data[0][i])
                    {
                        for (int j = -1 * radius_left; j < radius_right; ++j)
                        {
                            var->m_data[ch][i + j] -= signal->m_data[ch][j + radius_left];
                        }
                    }
                }
            }
        }
        return res;
    }

    char* SetValue(char* a_data_name, char* a_channel, char* a_x, char* a_val, char* a_radius)
    {
        char* res = 0;
        if (!a_data_name || !a_channel || !a_x || !a_val)
            res = MakeString(m_newchar_proc, "ERROR: SetValue: not enough arguments.");
        ISignalCodec* data = m_data_list_ref->datamap_find(a_data_name);
        if (!data)
            res = MakeString(m_newchar_proc, "ERROR: SetValue ", a_data_name, " not found.");
        if (!a_radius)
            a_radius = (char*)"0.0";
        if (!res)
        {
            unsigned int nrchannels = data->m_total_samples.m_size;
            unsigned int starts[nrchannels];
            unsigned int nrelements[nrchannels];
            int dactchans[nrchannels];

            for (unsigned int i = 0; i < nrchannels; ++i)
            {
                int radius_samples = atof(a_radius) * data->m_sample_rates.m_data[i];
                if (!radius_samples)
                    radius_samples = 1;
                int start = atof(a_x) * data->m_sample_rates.m_data[i] - (radius_samples / 2);
                starts[i] = start >= 0 ? start : 0;
                nrelements[i] = radius_samples;
                dactchans[i] = 0;
            }
            dactchans[atoi(a_channel)] = 1;
            CMatrixVarLen<double>datt;
            datt.Rebuild(nrchannels, nrelements);
            for (unsigned int i = 0; i < nrchannels; ++i)
                for (unsigned int j = 0; j < nrelements[i]; ++j)
                    datt.m_data[i][j] = atof(a_val);

            data->WriteDataBlock(datt.m_data, starts, nrelements, dactchans);
        }
        return res;
    }

    bool is_equal_tolerance(const double lhs, const double rhs, const double tolerance)
    {
        if (lhs - rhs <= tolerance && rhs - lhs <= tolerance)
            return true;
        else
            return false;
    }

    char* CompareSpikes(char* a_dst_name, char* a_ref_spikes, char* a_spike_channel_ref, char* a_det_spikes, char* a_spike_channel_det, char* a_tolerance_radius_mhu, char* a_errors)
    {
        char* res = 0;
        if (!a_dst_name || !a_ref_spikes || !a_det_spikes || !a_spike_channel_ref || !a_spike_channel_det)
            res = MakeString(m_newchar_proc, "ERROR: CompareSpikes: not enough arguments.");
        CVariable* ref_spikes = m_variable_list_ref->variablemap_find(a_ref_spikes);
        if (!ref_spikes)
            res = MakeString(m_newchar_proc, "ERROR: CompareSpikes: ", a_ref_spikes, " not found.");
        CVariable* det_spikes = m_variable_list_ref->variablemap_find(a_det_spikes);
        if (!det_spikes)
            res = MakeString(m_newchar_proc, "ERROR: CompareSpikes: ", a_det_spikes, " not found.");

        if (!res)
        {
            CVariable* errors = 0;
            if (a_errors)
            {
                errors = m_newvariable_proc();
                errors->Rebuild(1, ref_spikes->m_total_samples.m_data);
                ref_spikes->SCopyTo(errors);
                for (unsigned int i = 0; i < ref_spikes->m_total_samples.m_data[0]; ++i)
                    errors->m_data[0][i] = 0;
                strcpy(errors->m_varname, a_errors);
                m_variable_list_ref->Insert(errors->m_varname, errors);
            }
            unsigned int spike_channel_ref = atoi(a_spike_channel_ref);
            unsigned int spike_channel_det = atoi(a_spike_channel_det);
            unsigned int ref_spikes_nr_samples = ref_spikes->m_total_samples.m_data[spike_channel_ref];
            unsigned int det_spikes_nr_samples = det_spikes->m_total_samples.m_data[spike_channel_det];
            int radius = 1;
            if (a_tolerance_radius_mhu && strlen(a_tolerance_radius_mhu))
                radius = atof(a_tolerance_radius_mhu) * ref_spikes->m_sample_rates.m_data[spike_channel_ref];
            if (ref_spikes_nr_samples != det_spikes_nr_samples)
                res = MakeString(m_newchar_proc, "ERROR: CompareSpikes: number of samples did not match.");
            if (!res)
            {
                int reference_spike_count = 0;
                int detected_spike_count_out_of_reference_spikes = 0;
                int detected_spike_count = 0;
                int reference_spike_count_out_of_detected_spikes = 0;

                for (unsigned int i = radius; i < ref_spikes_nr_samples - radius; ++i)
                {
                    bool is_ok = true;
                    if (is_equal_tolerance(ref_spikes->m_data[spike_channel_ref][i], 1.0, 0.1))
                    {
                        reference_spike_count++;
                        is_ok = false;
                        for (int j = -radius; j < radius; ++j)
                        {
                            if (is_equal_tolerance(det_spikes->m_data[spike_channel_det][i + j], 1.0, 0.1) || is_equal_tolerance(det_spikes->m_data[spike_channel_det][i + j], 0.5, 0.1))
                            {
                                is_ok = true;
                                break;
                            }
                        }
                        if (is_ok)
                            detected_spike_count_out_of_reference_spikes++;
                    }
                    if (!is_ok && errors)
                        errors->m_data[0][i] = 1;
                }
                for (unsigned int i = radius; i < ref_spikes_nr_samples - radius; ++i)
                {
                    bool is_ok = true;
                    if (is_equal_tolerance(det_spikes->m_data[spike_channel_ref][i], 1.0, 0.1))
                    {
                        detected_spike_count++;
                        is_ok = false;
                        for (int j = -radius; j < radius; ++j)
                        {
                            if (is_equal_tolerance(ref_spikes->m_data[spike_channel_det][i + j], 1.0, 0.1) || is_equal_tolerance(ref_spikes->m_data[spike_channel_det][i + j], 0.5, 0.1))
                            {
                                is_ok = true;
                                break;
                            }
                        }
                        if (is_ok)
                            reference_spike_count_out_of_detected_spikes++;
                    }
                    if (!is_ok && errors)
                        errors->m_data[0][i] = 0.5;
                }
                int skipped_detections = reference_spike_count - detected_spike_count_out_of_reference_spikes;
                int false_detections = detected_spike_count - reference_spike_count_out_of_detected_spikes;

                double detected_percentage = 0;
                if (reference_spike_count)
                    detected_percentage = (100.0 * detected_spike_count_out_of_reference_spikes) / reference_spike_count;
                double error_percentage = 100.0;
                if (reference_spike_count)
                    error_percentage = ((skipped_detections + false_detections) * 100.0) / reference_spike_count;

                CVariable* dst = m_newvariable_proc();
                unsigned int sizes[6] = {1, 1, 1, 1, 1, 1};
                dst->Rebuild(6, sizes);
                strcpy(dst->m_varname, a_dst_name);
                m_variable_list_ref->Insert(dst->m_varname, dst);

                dst->m_data[0][0] = reference_spike_count;
                dst->m_data[1][0] = skipped_detections;
                dst->m_data[2][0] = false_detections;
                dst->m_data[3][0] = detected_percentage;
                dst->m_data[4][0] = error_percentage;
                dst->m_data[5][0] = 100 - error_percentage;

                strcpy(dst->m_labels.m_data[0].s, "Nr reference spikes");
                strcpy(dst->m_labels.m_data[1].s, "Nr skipped spikes");
                strcpy(dst->m_labels.m_data[2].s, "Nr false detections");
                strcpy(dst->m_labels.m_data[3].s, "Detection percentage");
                strcpy(dst->m_labels.m_data[4].s, "All error percentage");
                strcpy(dst->m_labels.m_data[5].s, "Total performance");

                strcpy(dst->m_vertical_units.m_data[0].s, "");
                strcpy(dst->m_vertical_units.m_data[1].s, "");
                strcpy(dst->m_vertical_units.m_data[2].s, "");
                strcpy(dst->m_vertical_units.m_data[3].s, "%");
                strcpy(dst->m_vertical_units.m_data[4].s, "%");
                strcpy(dst->m_vertical_units.m_data[5].s, "%");
            }
        }
        return res;
    }

    char* SetLabel(char* a_var_name, char* a_channel_indx, char* a_label)
    {
        char* res = 0;
        ISignalCodec* data = m_variable_list_ref->variablemap_find(a_var_name);
        if (!data)
            res = MakeString(m_newchar_proc, "ERROR: SetLabel: ", a_var_name, " not found.");
        else
            strcpy(data->m_labels.m_data[atoi(a_channel_indx)].s, a_label);
        return res;
    }

    char* SetVerticalUnit(char* a_var_name, char* a_channel_indx, char* a_vertical_unit, char* a_label)
    {
        char* res = 0;
        ISignalCodec* data = m_variable_list_ref->variablemap_find(a_var_name);
        if (!data)
            res = MakeString(m_newchar_proc, "ERROR: SetVerticalUnit: ", a_var_name, " not found.");
        else
        {
            strcpy(data->m_vertical_units.m_data[atoi(a_channel_indx)].s, a_vertical_unit);
            if (a_label && strlen(a_label))
                strcpy(data->m_labels.m_data[atoi(a_channel_indx)].s, a_label);
        }
        return res;
    }

    char* RR_Distances(char* a_in_var_name, char* a_var_name_to_store, char* a_channel_index)
    {
        if (!a_in_var_name || !a_var_name_to_store)
            return MakeString(m_newchar_proc, "ERROR: Downsample: not enough arguments!");
        int channel_index = atoi(a_channel_index);
        if (CVariable* var = m_variable_list_ref->variablemap_find(a_in_var_name))
        {
            int r_count = 0;
            for (unsigned int j = 0; j < var->m_widths.m_data[channel_index]; ++j)
                if (var->m_data[channel_index][j] > 0)
                    r_count++;
            cout << "r_count: " << r_count << endl;

            CVariable* output2 = m_newvariable_proc();
            unsigned int output2sizes[1];
            output2sizes[0] = r_count - 2;
            output2->Rebuild(1, output2sizes);
            var->SCopyTo(output2);
            output2->m_sample_rates.m_data[0] = 1;

            ///output2->m_sample_rates.m_data[i] = var->m_sample_rates.m_data[i] * (double)output2->m_widths.m_data[i] / (double)var->m_widths.m_data[i];

            int last_r_index = 0;
            r_count = 0;
            for (unsigned int j = 0; j < var->m_widths.m_data[channel_index]; ++j)
                if (var->m_data[channel_index][j] > 0)
                {
                    if (r_count >= 2)
                        output2->m_data[0][r_count - 2] = (double)(j - last_r_index) / var->m_sample_rates.m_data[channel_index];
                    r_count++;
                    last_r_index = j;
                }

            strcpy(output2->m_varname, a_var_name_to_store);
            //strcpy(output2->m_horizontal_units, "s");
            strcpy(output2->m_vertical_units.m_data[0].s, "s");
            m_variable_list_ref->Insert(output2->m_varname, output2);
            return 0;
        }
        else
            return MakeString(m_newchar_proc, "ERROR: Downsample: can't find data in variablelist: '", a_in_var_name, "'");
    }

    // Lineáris interpolácio fuggvénye
    static inline double linear_interpolate(double x0, double y0, double x1, double y1, double x)
    {
        return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
    }

    // Resampling algoritmus
    std::vector<double> resample_nn_intervals(double* nn_intervals, int size, double target_frequency)
    {
        // Összes ido a NN intervallumokban (osszegzés)
        double total_time = 0.0;
        for (int i = 0; i < size; i++)
            total_time += nn_intervals[i];

        // A cél egyenletes idokozok kiszámitása
        double dt = 1.0 / target_frequency;  // Cél idobeli mintavételi idokoz (pl. 4 Hz -> dt = 0.25 másodperc)
        int num_samples = static_cast<int>(total_time / dt);  // A resampled minták száma

        std::vector<double> resampled_intervals;

        // Elindulunk a 0 idoponttol, és minden dt lépésben interpoláljuk az uj NN értékeket
        double current_time = 0.0;
        int current_nn_index = 0;
        double previous_time = 0.0;

        for (int i = 0; i < num_samples; i++)
        {
            current_time = i * dt;  // Kovetkezo resampled idopont

            // Keressuk meg a kovetkezo két NN intervallumot, amelyek kozé esik a current_time
            while (current_time > previous_time + nn_intervals[current_nn_index] && current_nn_index < size - 1)
            {
                previous_time += nn_intervals[current_nn_index];
                current_nn_index++;
            }

            // Interpoláljuk az értéket az aktuális NN intervallumban
            double interpolated_value = linear_interpolate(
                                            previous_time, nn_intervals[current_nn_index],
                                            previous_time + nn_intervals[current_nn_index], nn_intervals[current_nn_index + 1],
                                            current_time
                                        );

            // Hozzáadjuk az interpolált értéket a resampled listához
            resampled_intervals.push_back(interpolated_value);
        }

        return resampled_intervals;
    }

    /// --------- spline

    // Spline struktura a spline koefficienseinek tárolására
    struct Spline
    {
        std::vector<double> a, b, c, d;  // spline koefficiensek
        std::vector<double> x;           // eredeti x pontok (idopontok)
    };

    // A spline koefficiensek kiszámitása
    Spline cubic_spline(const std::vector<double>& x, const std::vector<double>& y)
    {
        int n = x.size() - 1;
        std::vector<double> a(n + 1), b(n), d(n), h(n), alpha(n), c(n + 1), l(n + 1), mu(n), z(n + 1);

        // Inicializáljuk a 'a' vektor az y pontokkal
        for (int i = 0; i <= n; i++)
        {
            a[i] = y[i];
        }

        // Lépés 1: h(i) és alpha(i) kiszámitása
        for (int i = 0; i < n; i++)
        {
            h[i] = x[i + 1] - x[i];
        }

        for (int i = 1; i < n; i++)
        {
            alpha[i] = (3.0 / h[i]) * (a[i + 1] - a[i]) - (3.0 / h[i - 1]) * (a[i] - a[i - 1]);
        }

        // Lépés 2: Tridiagonális egyenletrendszer megoldása
        l[0] = 1.0;
        mu[0] = 0.0;
        z[0] = 0.0;

        for (int i = 1; i < n; i++)
        {
            l[i] = 2.0 * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }

        l[n] = 1.0;
        z[n] = 0.0;
        c[n] = 0.0;

        // Lépés 3: c(i), b(i), d(i) kiszámitása
        for (int j = n - 1; j >= 0; j--)
        {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (a[j + 1] - a[j]) / h[j] - h[j] * (c[j + 1] + 2.0 * c[j]) / 3.0;
            d[j] = (c[j + 1] - c[j]) / (3.0 * h[j]);
        }

        // A spline koefficienseket visszaadjuk
        return {a, b, c, d, x};
    }

    // Spline értékének kiértékelése adott ponton
    double evaluate_spline(const Spline& spline, double xi)
    {
        int n = spline.x.size() - 1;
        int i = n - 1;

        // Megkeressuk, melyik intervallumban van xi
        for (int j = 0; j < n; j++)
        {
            if (xi >= spline.x[j] && xi <= spline.x[j + 1])
            {
                i = j;
                break;
            }
        }

        double dx = xi - spline.x[i];
        return spline.a[i] + spline.b[i] * dx + spline.c[i] * dx * dx + spline.d[i] * dx * dx * dx;
    }

    // NN resampling cubic spline-nal
    std::vector<double> resample_nn_spline(const double* nn_intervals, int size, double target_fs = 4.0)
    {
        // Eredeti idopontok kiszámitása az NN intervallumokbol
        std::vector<double> original_times(size);
        original_times[0] = 0.0;
        for (int i = 1; i < size; i++)
        {
            original_times[i] = original_times[i - 1] + nn_intervals[i - 1];
        }

        // Az interpolált idopontok (egyenletes mintavételezés)
        double total_time = original_times.back();
        int num_samples = static_cast<int>(total_time * target_fs);  // Minták száma
        std::vector<double> resampled_times(num_samples);
        std::vector<double> resampled_nn(num_samples);

        for (int i = 0; i < num_samples; i++)
        {
            resampled_times[i] = i / target_fs;
        }

        // Cubic spline koefficiensek kiszámitása
        std::vector<double> nn_vec(nn_intervals, nn_intervals + size);
        Spline spline = cubic_spline(original_times, nn_vec);

        // Interpolácio a resample-elt idopontokra
        for (int i = 0; i < num_samples; i++)
        {
            resampled_nn[i] = evaluate_spline(spline, resampled_times[i]);
        }

        return resampled_nn;
    }

    /// --------- eo spline

    char* ResampleNNIntervals(char* a_in_var_name, char* a_out_var_name, char* a_target_frequency)
    {
        double target_frequency = atof(a_target_frequency);
        if (CVariable* input = m_variable_list_ref->variablemap_find(a_in_var_name))
        {
            //std::vector<double> resampled = resample_nn_intervals(input->m_data[0], input->m_total_samples.m_data[0] - 1, target_frequency);
            std::vector<double> resampled = resample_nn_spline(input->m_data[0], input->m_total_samples.m_data[0], target_frequency);
            cout << "resampled: " << resampled.size() << endl;
            CVariable* output = m_newvariable_proc();
            unsigned int output_nr_samples[1] {resampled.size()};
            output->Rebuild(input->m_total_samples.m_size, output_nr_samples);
            output->m_sample_rates.m_data[0] = target_frequency;

            for (unsigned int j = 0; j < output->m_total_samples.m_data[0]; ++j)
                output->m_data[0][j] = resampled[j];

            strcpy(output->m_varname, a_out_var_name);
            strcpy(output->m_vertical_units.m_data[0].s, input->m_vertical_units.m_data[0].s);
            m_variable_list_ref->Insert(output->m_varname, output);
            return 0;
        }
        else
            return MakeString(m_newchar_proc, "ERROR: ResampleNNIntervals: while resampling '", a_in_var_name, "' to '", a_out_var_name, "': bad arguments!");
    }

    static inline void remove_dc_component(double* signal, int length)
    {
        double mean = 0.0;
        for (int i = 0; i < length; ++i)
            mean += signal[i];
        mean /= length;
        for (int i = 0; i < length; ++i)
            signal[i] -= mean;
    }

    char* RemoveDCComponent(char* a_in_var_name)
    {
        if (CVariable* input = m_variable_list_ref->variablemap_find(a_in_var_name))
        {
            for (unsigned int i = 0; i < input->m_total_samples.m_size; ++i)
                remove_dc_component(input->m_data[i], input->m_total_samples.m_data[i]);
            return 0;
        }
        else
            return MakeString(m_newchar_proc, "ERROR: RemoveDCComponent: '", a_in_var_name, "': not enugh arguments!");
    }

// Burg algoritmus az AR koefficiensek kiszámításához (double*)
    std::vector<double> burg_algorithm(const double* x, int n, int order)
    {
        std::vector<double> a(order + 1, 0.0);
        std::vector<double> e_forward(n, 0.0), e_backward(n, 0.0);
        std::vector<double> k(order + 1, 0.0);  // Reflection coefficients

        // Initialization
        a[0] = 1.0;  // a_0 is always 1
        for (int i = 0; i < n; i++)
        {
            e_forward[i] = x[i];
            e_backward[i] = x[i];
        }

        double E = 0.0;
        for (int i = 0; i < n; i++)
        {
            E += x[i] * x[i];
        }

        // Burg recursion
        for (int m = 1; m <= order; m++)
        {
            double num = 0.0, denom = 0.0;
            for (int i = m; i < n; i++)
            {
                num += e_forward[i] * e_backward[i - 1];
                denom += e_forward[i] * e_forward[i] + e_backward[i - 1] * e_backward[i - 1];
            }

            // Reflection coefficient
            k[m] = -2.0 * num / denom;

            // Update forward and backward errors
            for (int i = m; i < n; i++)
            {
                double e_fwd_prev = e_forward[i];
                e_forward[i] = e_fwd_prev + k[m] * e_backward[i - 1];
                e_backward[i - 1] = e_backward[i - 1] + k[m] * e_fwd_prev;
            }

            // Update AR coefficients
            for (int i = 1; i <= m / 2; i++)
            {
                double temp = a[i] + k[m] * a[m - i];
                a[m - i] = a[m - i] + k[m] * a[i];
                a[i] = temp;
            }
            a[m] = k[m];

            // Update prediction error
            E *= (1.0 - k[m] * k[m]);
        }

        return a;  // Az AR koefficiensek
    }

// Az AR-spektrum kiszámítása (Single-sided spektrum)
    std::vector<double> calculate_ar_psd(const std::vector<double>& a, double sigma2, int num_points, double fs)
    {
        std::vector<double> psd(num_points / 2, 0.0);  // Csak a fél spektrumra koncentrálunk
        double df = fs / num_points;

        for (int i = 0; i < num_points / 2; i++)
        {
            double f = i * df;
            std::complex<double> denom(1.0, 0.0);

            for (unsigned int k = 1; k < a.size(); k++)
            {
                denom += std::complex<double>(a[k], 0.0) * std::polar(1.0, -2.0 * M_PI * f * k);
            }

            psd[i] = sigma2 / std::norm(denom);  // PSD
        }

        return psd;
    }

// LF és HF sávok spektrális teljesítménye
    double integrate_band(const std::vector<double>& psd, double df, double f_low, double f_high)
    {
        double total_power = 0.0;
        int n_low = static_cast<int>(f_low / df);
        int n_high = static_cast<int>(f_high / df);

        for (int i = n_low; i <= n_high; i++)
        {
            total_power += psd[i];
        }

        return total_power * df;  // Teljesítmény integrálása
    }


    std::vector<double> ARSpectrum(double* nn_intervals, int size, int order = 16, double fs = 4.0, int num_points = 256)
    {
        // Az NN intervallumok kozépértéktol való eltérésének kiszámítása
        std::vector<double> nn_deviation(size);
        double mean_nn = 0.0;
        for (int i = 0; i < size; i++)
        {
            mean_nn += nn_intervals[i];
        }
        mean_nn /= size;
        for (int i = 0; i < size; i++)
        {
            nn_deviation[i] = nn_intervals[i] - mean_nn;
        }

        // Az AR modell illesztése a Burg-algoritmussal
        double sigma2 = 1.0;
        std::vector<double> ar_coeffs = burg_algorithm(nn_deviation.data(), size, order/*, sigma2*/);

        // A spektrum suruségének kiszámítása az AR modellel
        std::vector<double> psd = calculate_ar_psd(ar_coeffs, sigma2, num_points, fs);

        // LF (0.04 - 0.15 Hz) és HF (0.15 - 0.4 Hz) sávok teljesítménye
        double df = fs / num_points;
        double lf_power = integrate_band(psd, df, 0.04, 0.15/*, fs*/);
        double hf_power = integrate_band(psd, df, 0.15, 0.4/*, fs*/);

        std::cout << "LF Power: " << lf_power << std::endl;
        std::cout << "HF Power: " << hf_power << std::endl;

        // LF/HF arány
        if (hf_power != 0.0)
        {
            double lf_hf_ratio = lf_power / hf_power;
            std::cout << "LF/HF Ratio: " << lf_hf_ratio << std::endl;
        }
        else
        {
            std::cout << "HF Power is zero, cannot compute LF/HF ratio." << std::endl;
        }
        return psd;
    }

    char* ARSpectrum(char* a_in_var_name, char* a_ar_spectrum, char* a_order, char* a_fs, char* a_num_points)
    {
        double fs = atof(a_fs);
        int order = atoi(a_order);
        int num_points = atoi(a_num_points);
        if (CVariable* input = m_variable_list_ref->variablemap_find(a_in_var_name))
        {
            CVariable* output = m_newvariable_proc();
            unsigned int output_nr_samples[1];
            output_nr_samples[0] = num_points;
            output->Rebuild(input->m_total_samples.m_size, output_nr_samples);
            input->SCopyTo(output);
            output->m_sample_rates.m_data[0] = fs;

            std::vector<double> psd = ARSpectrum(input->m_data[0], input->m_total_samples.m_data[0], order, fs, num_points);

            for (unsigned int j = 0; j < output->m_total_samples.m_data[0]; ++j)
                output->m_data[0][j] = psd[j];

            strcpy(output->m_varname, a_ar_spectrum);
            m_variable_list_ref->Insert(output->m_varname, output);
            return 0;
        }
        else
            return MakeString(m_newchar_proc, "ERROR: DownsampleGauss: while Downsampling '", a_in_var_name, "' to '", a_ar_spectrum, "': not enugh arguments!");
    }

    void print_gauss(double* data, size_t data_samples, double amplitude)
    {
        // A Gauss-gorbe kozéppontja a tomb kozepén van
        double mean = static_cast<double>(data_samples - 1) / 2.0;

        // Szórás (sigma) meghatározása: az 1/6 részével osztjuk el a tartományt, hogy a szélei 0 legyenek
        double sigma = mean / 3.0;

        // Gauss-gorbe generálása minden adatmintára
        for (size_t i = 0; i < data_samples; ++i)
        {
            double x = static_cast<double>(i);
            data[i] = amplitude * exp(-0.5 * pow((x - mean) / sigma, 2));
        }
    }

    void print_line(double* data, size_t data_samples, double amplitude)
    {
        if (data_samples < 2)
            return;
        double step = amplitude / static_cast<double>(data_samples - 1);
        for (size_t i = 1; i < data_samples; ++i)
            data[i] = data[0] + step * i;
    }

    void print_curve(double* data, size_t data_samples, double amplitude)
    {
        if (data_samples < 2)
            return;

        // Görbe karakterisztikájának szabályozása (nagyobb érték laposabb véget eredményez)
        double curvature = 5.0; // Beállítható görbe karakter
        double factor = curvature / (data_samples - 1);

        for (size_t i = 0; i < data_samples; ++i)
        {
            // Görbe értéke exponenciális függvény alapján
            double t = i * factor;
            data[i] = data[0] + amplitude * (1.0 - exp(-t));
        }
    }

    struct qrs_params
    {
        double amplitude_p = 0.1;
        double amplitude_q = -0.1;
        double amplitude_r = 1.0;
        double amplitude_s = -0.2;
        double amplitude_t = 0.2;
        double len_p = 0.08;
        double len_pq = 0.08;
        double len_q = 0.007;
        double len_r = 0.1;
        double len_s = 0.007;
        double len_st = 0.1;
        double len_t = 0.16;
        ZAX_JSON_SERIALIZABLE(qrs_params, JSON_PROPERTY(amplitude_p), JSON_PROPERTY(amplitude_q), JSON_PROPERTY(amplitude_r), JSON_PROPERTY(amplitude_s), JSON_PROPERTY(amplitude_t), JSON_PROPERTY(len_p), JSON_PROPERTY(len_pq), JSON_PROPERTY(len_q), JSON_PROPERTY(len_r), JSON_PROPERTY(len_s), JSON_PROPERTY(len_st), JSON_PROPERTY(len_t))
    };

    void simulate_qrs(double* data, size_t data_samples, size_t sampling_rate, double amp_mod, qrs_params& params)
    {
        size_t samples_p = params.len_p * sampling_rate;
        size_t samples_pq = params.len_pq * sampling_rate;
        size_t samples_q = params.len_q * sampling_rate;
        size_t samples_r = params.len_r * sampling_rate;
        size_t samples_s = params.len_s * sampling_rate;
        size_t samples_st = params.len_st * sampling_rate;
        size_t samples_t = params.len_t * sampling_rate;

        print_gauss(data, samples_p, params.amplitude_p * amp_mod); /// P
        print_line(data + samples_p + samples_pq, samples_q, params.amplitude_q * amp_mod); /// Q |
        print_line(data + samples_p + samples_pq + samples_q - 1, samples_r / 2 + 1, (params.amplitude_r - params.amplitude_q) * amp_mod); /// R /
        print_line(data + samples_p + samples_pq + samples_q + samples_r / 2 - 1, samples_r / 2 + 1, (-params.amplitude_r + params.amplitude_s) * amp_mod); /// S |
        print_curve(data + samples_p + samples_pq + samples_q + samples_r / 2 + samples_r / 2 - 1, samples_s + 1, -params.amplitude_s * amp_mod);
        //print_line(data + samples_p + samples_pq + samples_q + samples_r / 2 + samples_r / 2 - 1, samples_s / 4 + 1, (-params.amplitude_s * amp_mod * 3) / 4); /// ST /
        //print_line(data + samples_p + samples_pq + samples_q + samples_r / 2 + samples_r / 2 - 1 + samples_s / 4 + 1 - 1, (samples_s * 3) / 4, -params.amplitude_s * amp_mod / 4); /// ST /
        print_gauss(data + samples_p + samples_pq + samples_q + samples_r + samples_s + samples_st, samples_t, params.amplitude_t * amp_mod); /// T
    }

    char* GenerateECG(char* a_outdataname, char* a_number_of_samples, char* a_sample_rates, char* a_frequency)
    {
        qrs_params qrs_pars;
        double frequency = atof(a_frequency);
        UIntVec number_of_samples;
        DoubleVec Samplerates;

        number_of_samples.RebuildFrom(a_number_of_samples);
        Samplerates.RebuildFrom(a_sample_rates);

        CVariable* output2 = m_newvariable_proc();
        output2->Rebuild(number_of_samples.m_size, number_of_samples.m_data);

        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            output2->m_sample_rates.m_data[i] = Samplerates.m_data[i];
            size_t interval_samples = Samplerates.m_data[i] / frequency;
            for (unsigned int j = 0; j < output2->m_widths.m_data[i] - interval_samples; j+= interval_samples)
                simulate_qrs(output2->m_data[i] + j, interval_samples, Samplerates.m_data[i], 1, qrs_pars);
        }
        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(a_outdataname, output2);
        return 0;
    }

    struct ecg_simulation_params
    {
        double heartbeat_frequency = 1.0;
        double alteration_frequency_for_DC_component = 0.01;
        double alteration_amplitude_for_DC_component = 0.2;
        double alteration_phase_for_DC_component_in_radians = 0;
        double amplitude_modulation_depth_for_QRS_by_HF = 0.2;
        double frequency_HF = 0.3;
        double frequency_LF = 0.12;
        double frequency_modulation_depth_HF = 0.3;
        double frequency_modulation_depth_LF = 0.3;
        double phase_HF_radians = 0;
        double phase_LF_radians = 0;
        double extrasys_frequency = 0.1;
        double extrasys_shift_after_last_QRS = 0.5;
        double QRS_interval_standard_deviation = 0.1;
        int skip_one_QRS_at_every = 7;
        ZAX_JSON_SERIALIZABLE(ecg_simulation_params, JSON_PROPERTY(heartbeat_frequency), JSON_PROPERTY(alteration_frequency_for_DC_component), JSON_PROPERTY(alteration_amplitude_for_DC_component), JSON_PROPERTY(amplitude_modulation_depth_for_QRS_by_HF), JSON_PROPERTY(frequency_HF), JSON_PROPERTY(frequency_LF), JSON_PROPERTY(frequency_modulation_depth_HF), JSON_PROPERTY(frequency_modulation_depth_LF), JSON_PROPERTY(phase_HF_radians), JSON_PROPERTY(phase_LF_radians), JSON_PROPERTY(alteration_phase_for_DC_component_in_radians), JSON_PROPERTY(extrasys_frequency), JSON_PROPERTY(extrasys_shift_after_last_QRS), JSON_PROPERTY(QRS_interval_standard_deviation), JSON_PROPERTY(skip_one_QRS_at_every))
    };

    char* GenerateModulatedECG(char* a_outdataname, char* a_number_of_samples, char* a_sample_rates, char* additional_params, char* a_qrs_params)
    {
        const double PI = 3.14159265358979323846;

        ecg_simulation_params add_params = additional_params;
        qrs_params qrs_pars = a_qrs_params;

        std::default_random_engine generator;
        std::normal_distribution<double> distribution(1.0, add_params.QRS_interval_standard_deviation);

        UIntVec number_of_samples;
        DoubleVec Samplerates;

        number_of_samples.RebuildFrom(a_number_of_samples);
        Samplerates.RebuildFrom(a_sample_rates);

        CVariable* output2 = m_newvariable_proc();
        output2->Rebuild(number_of_samples.m_size, number_of_samples.m_data);

        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            output2->m_sample_rates.m_data[i] = Samplerates.m_data[i];
            int interval_samples = Samplerates.m_data[i] / add_params.heartbeat_frequency;
            int extra_last_simulated_qrs_index = 0;
            int extrasys_samples = 1.0 / add_params.extrasys_frequency * Samplerates.m_data[i];
            int extrasys_shift = add_params.extrasys_shift_after_last_QRS * Samplerates.m_data[i];
            int QRS_count = 0;
            for (unsigned int j = 0; j < output2->m_widths.m_data[i] - interval_samples - extrasys_shift; j+= interval_samples)
            {
                double HF_sin_coeff = sin(add_params.phase_HF_radians + j * 2 * PI * add_params.frequency_HF / Samplerates.m_data[i]);
                double HF_sin_coeff_plus_pi = sin(add_params.phase_HF_radians + j * 2 * PI * add_params.frequency_HF / Samplerates.m_data[i] + PI);
                double LF_sin_coeff = sin(add_params.phase_LF_radians + j * 2 * PI * add_params.frequency_LF / Samplerates.m_data[i]);
                interval_samples = Samplerates.m_data[i] / add_params.heartbeat_frequency;
                interval_samples += HF_sin_coeff * interval_samples * add_params.frequency_modulation_depth_HF;
                interval_samples += LF_sin_coeff * interval_samples * add_params.frequency_modulation_depth_LF;
                interval_samples *= distribution(generator);
                if ((!add_params.skip_one_QRS_at_every) || ((QRS_count++) % add_params.skip_one_QRS_at_every))
                {
                    simulate_qrs(output2->m_data[i] + j, interval_samples, Samplerates.m_data[i], 1 + HF_sin_coeff_plus_pi * add_params.amplitude_modulation_depth_for_QRS_by_HF, qrs_pars);
                    if (j > extra_last_simulated_qrs_index + extrasys_samples)
                    {
                        simulate_qrs(output2->m_data[i] + j + extrasys_shift, interval_samples, Samplerates.m_data[i], 1 + HF_sin_coeff_plus_pi * add_params.amplitude_modulation_depth_for_QRS_by_HF, qrs_pars);
                        extra_last_simulated_qrs_index = j;
                    }
                }
            }
            for (unsigned int j = 0; j < output2->m_widths.m_data[i]; j++)
                output2->m_data[i][j] += sin(add_params.alteration_phase_for_DC_component_in_radians + j * 2 * PI * add_params.alteration_frequency_for_DC_component / Samplerates.m_data[i]) * add_params.alteration_amplitude_for_DC_component;
        }
        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(a_outdataname, output2);
        return 0;
    }

    void add_noise_to_signal(double* data, size_t data_samples, double noise_amplitude, size_t sampling_rate, double noise_frequency = 0.0)
    {
        // Véletlen generátor beállítása szinuszos zaj fázisához
        std::default_random_engine generator;
        std::uniform_real_distribution<double> distribution(-noise_amplitude, noise_amplitude);

        for (size_t i = 0; i < data_samples; ++i)
        {
            double noise_value;

            if (noise_frequency > 0.0)
            {
                // Ha zaj frekvencia meg van adva, szinuszos zajt adunk hozzá
                double phase = static_cast<double>(i) / sampling_rate;
                noise_value = noise_amplitude * sin(2 * M_PI * noise_frequency * phase);
            }
            else
            {
                // Ha nincs megadva zaj frekvencia, fehér zajt adunk hozzá
                noise_value = distribution(generator);
            }

            data[i] += noise_value;
        }
    }

    char* AddNoise(char* a_invarname, char* a_noise_amplitude, char* a_noise_frequency)
    {
        char* l_error = 0;
        if (!a_invarname || !a_noise_amplitude)
            l_error = MakeString(m_newchar_proc, "ERROR: AddNoise: not enough arguments, 'invarname' or 'value' is missing!");
        else
        {
            CVariable* l_invar = m_variable_list_ref->variablemap_find(a_invarname);
            if (!l_invar)
                l_error = MakeString(m_newchar_proc, "ERROR: AddNoise: can't find '", a_invarname, "' in variablelist");
            else
            {
                double noise_amplitude = atof(a_noise_amplitude);
                double noise_frequency = a_noise_frequency ? atof(a_noise_frequency) : 0;
                for (unsigned int ch = 0; ch < l_invar->m_total_samples.m_size; ch++)
                    add_noise_to_signal(l_invar->m_data[ch], l_invar->m_widths.m_data[ch], noise_amplitude, l_invar->m_sample_rates.m_data[ch], noise_frequency);
            }
        }
        return l_error;
    }

    // Precízebb 2. rendu Butterworth sávszuro
    void butterworth_bandpass_filter(double* data, size_t data_samples, double sampling_rate, double low_freq, double high_freq)
    {
        // Frekvencia- és sávszélesség-számítás (Nyquist-normalizáció nélkül)
        double center_freq = M_PI * ((high_freq + low_freq) / sampling_rate);
        double bandwidth = M_PI * ((high_freq - low_freq) / sampling_rate);

        // Szuro koefficiensek számítása
        double alpha = sin(center_freq) * sinh(log(2.0) / 2.0 * bandwidth / sin(center_freq));

        double b0 = alpha;
        double b1 = 0;
        double b2 = -alpha;
        double a0 = 1 + alpha;
        double a1 = -2 * cos(center_freq);
        double a2 = 1 - alpha;

        // Normalizálás
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;

        // Elso szurés (idoben elore), eredmény a `filtered` tömbbe kerül
        std::vector<double> filtered(data_samples, 0.0);
        for (size_t i = 2; i < data_samples; ++i)
            filtered[i] = b0 * data[i] + b1 * data[i - 1] + b2 * data[i - 2] - a1 * filtered[i - 1] - a2 * filtered[i - 2];

        // Második szurés (idoben visszafelé), eredmény közvetlenül a `data` tömbbe kerül
        for (int i = data_samples - 3; i >= 0; --i)
            data[i] = b0 * filtered[i] + b1 * filtered[i + 1] + b2 * filtered[i + 2] - a1 * data[i + 1] - a2 * data[i + 2];

        // Az elso két érték közvetlenül másolása a `data` tömbbe
        data[0] = filtered[0];
        data[1] = filtered[1];
    }

    // Butterworth sávszuro alkalmazása - 2. rendu példa
    void butterworth_bandpass_filter_(double* data, size_t data_samples, size_t sampling_rate, double low_freq, double high_freq)
    {
        // Normalizált frekvenciák számítása
        double low = low_freq / sampling_rate;
        double high = high_freq / sampling_rate;

        // Példa paraméterek - Butterworth 2. rendu sávszuro koefficiensei
        double Q = 0.707; // Minoségi tényezo
        double omega = M_PI * (high + low);
        double bw = M_PI * (high - low) / Q;
        double alpha = sin(omega) * sinh(log(2.0) / 2.0 * bw / sin(omega));

        // Szuro koefficiensek
        double b0 = alpha, b1 = 0, b2 = -alpha;
        double a0 = 1 + alpha, a1 = -2 * cos(omega), a2 = 1 - alpha;

        // Normalizált koefficiensek
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;

        // Elso szurés (idoben elore), eredmény a `filtered` tömbbe kerül
        std::vector<double> filtered(data_samples, 0.0);
        for (size_t i = 2; i < data_samples; ++i)
            filtered[i] = b0 * data[i] + b1 * data[i - 1] + b2 * data[i - 2] - a1 * filtered[i - 1] - a2 * filtered[i - 2];

        // Második szurés (idoben visszafelé), eredmény közvetlenül a `data` tömbbe kerül
        for (int i = data_samples - 3; i >= 0; --i)
            data[i] = b0 * filtered[i] + b1 * filtered[i + 1] + b2 * filtered[i + 2] - a1 * data[i + 1] - a2 * data[i + 2];

        // Az elso két érték közvetlenül másolása a `data` tömbbe
        data[0] = filtered[0];
        data[1] = filtered[1];
    }

    void add_bandlimited_noise(double* data, size_t data_samples, double noise_amplitude, double sampling_rate, double low_freq, double high_freq)
    {
        // Véletlen generátor fehér zajhoz
        std::default_random_engine generator;
        std::uniform_real_distribution<double> distribution(-noise_amplitude, noise_amplitude);

        // Fehér zaj generálása
        std::vector<double> noise(data_samples);
        for (size_t i = 0; i < data_samples; ++i)
        {
            noise[i] = distribution(generator);
        }

        // Sávszuro alkalmazása a fehér zajra
        butterworth_bandpass_filter(noise.data(), data_samples, sampling_rate, low_freq, high_freq);

        // A szurt zaj hozzáadása az eredeti jelhez
        for (size_t i = 0; i < data_samples; ++i)
        {
            data[i] += noise[i];
        }
    }

    void add_bandlimited_noise_not_really_band_limited(double* data, size_t data_samples, double noise_amplitude, size_t sampling_rate, double low_freq, double high_freq)
    {
        // Véletlen generátor beállítása az amplitudók és fázisok számára
        std::default_random_engine generator;
        std::uniform_real_distribution<double> amplitude_distribution(0, noise_amplitude);
        std::uniform_real_distribution<double> phase_distribution(0, 2 * M_PI);

        // Számoljuk ki az adott frekvenciatartományban lévo komponensek számát
        int num_components = 10; // Példa: 10 összetevo szinuszjel a frekvenciasávban
        double freq_step = (high_freq - low_freq) / num_components;

        // Iteráljunk végig az adatmintákon, és minden mintához adjunk egy sávszurt zajértéket
        for (size_t i = 0; i < data_samples; ++i)
        {
            double noise_value = 0.0;

            // Egy sávszurt zaj komponenseinek összegzése
            for (int j = 0; j < num_components; ++j)
            {
                double freq = low_freq + j * freq_step;            // Frekvencia az adott sávban
                double amplitude = amplitude_distribution(generator); // Random amplitudó a zavarás erossége szerint
                double phase = phase_distribution(generator);          // Véletlen fázis

                // Zaj komponens hozzáadása az aktuális mintához
                noise_value += amplitude * sin(2 * M_PI * freq * i / sampling_rate + phase);
            }

            // A végso zaj érték hozzáadása az eredeti adathoz
            data[i] += noise_value;
        }
    }

    char* AddBandLimitedNoise(char* a_invarname, char* a_noise_amplitude, char* a_noise_frequency_low, char* a_noise_frequency_high)
    {
        char* l_error = 0;
        if (!a_invarname || !a_noise_amplitude || !a_noise_frequency_low || !a_noise_frequency_high)
            l_error = MakeString(m_newchar_proc, "ERROR: AddBandLimitedNoise: not enough arguments, 'invarname' or 'value' is missing!");
        else
        {
            CVariable* l_invar = m_variable_list_ref->variablemap_find(a_invarname);
            if (!l_invar)
                l_error = MakeString(m_newchar_proc, "ERROR: AddBandLimitedNoise: can't find '", a_invarname, "' in variablelist");
            else
            {
                double noise_amplitude = atof(a_noise_amplitude);
                double noise_frequency_low = atof(a_noise_frequency_low);
                double noise_frequency_high = atof(a_noise_frequency_high);
                for (unsigned int ch = 0; ch < l_invar->m_total_samples.m_size; ch++)
                    add_bandlimited_noise(l_invar->m_data[ch], l_invar->m_widths.m_data[ch], noise_amplitude, l_invar->m_sample_rates.m_data[ch], noise_frequency_low, noise_frequency_high);
            }
        }
        return l_error;
    }

    struct sine_sim_params
    {
        unsigned int number_of_samples = 240000;
        unsigned int sampling_rate = 1000;
        double amplitude = 0.1;
        double frequency = -0.1;
        double modulation_frequency = 1.0;
        double frequency_modulation_depth = -0.2;
        ZAX_JSON_SERIALIZABLE(sine_sim_params, JSON_PROPERTY(number_of_samples), JSON_PROPERTY(sampling_rate), JSON_PROPERTY(amplitude), JSON_PROPERTY(frequency), JSON_PROPERTY(modulation_frequency), JSON_PROPERTY(frequency_modulation_depth))
    };

    struct modulated_sine_params
    {
        vector<sine_sim_params> channel_params;
        ZAX_JSON_SERIALIZABLE(modulated_sine_params, JSON_PROPERTY(channel_params))
    };

    char* CreateModulatedSine(char* a_outdataname, char* a_modulated_sine_params)
    {
        modulated_sine_params params = a_modulated_sine_params;
        vector<unsigned int> number_of_samples(params.channel_params.size());
        for (unsigned int i = 0; i < params.channel_params.size(); ++i)
            number_of_samples[i] = params.channel_params[i].number_of_samples;

        CVariable* output2 = m_newvariable_proc();
        output2->Rebuild(number_of_samples.size(), number_of_samples.data());

        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            output2->m_sample_rates.m_data[i] = params.channel_params[i].sampling_rate;
            cout << "params.channel_params[i].sampling_rate: " << params.channel_params[i].sampling_rate << endl;
            for (unsigned int j = 0; j < output2->m_widths.m_data[i]; ++j)
            {
                double p2t = j / output2->m_sample_rates.m_data[i] * 2.0 * M_PI;
                double phi = p2t * params.channel_params[i].frequency +
                             params.channel_params[i].frequency_modulation_depth / params.channel_params[i].modulation_frequency * sin(p2t * params.channel_params[i].modulation_frequency);
                output2->m_data[i][j] = sin(phi) * params.channel_params[i].amplitude;
            }
        }
        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(a_outdataname, output2);
        return 0;
    }


    void create_sine(double* data, unsigned int data_len, int sampling_rate, double frequency, double amplitude)
    {
        double p = (2.0 * M_PI * frequency) / sampling_rate;
        amplitude /= 2.0;
        for (unsigned int i = 0; i < data_len; ++i)
            data[i] = (sin(i * p) + 1.0) * amplitude;
    }

    void print_line2(double* data, size_t data_samples, double amplitude)
    {
        if (data_samples < 2)
            return;
        double step = amplitude / static_cast<double>(data_samples - 1);
        for (size_t i = 1; i < data_samples; ++i)
            data[i] = data[0] + step * i;
    }

    void create_triangle(double* data, unsigned int data_len, int sampling_rate, double silence_before_triangle_in_msec, double duration_in_msec, double amplitude)
    {
        memset(data, 0, sizeof(data[0] * data_len));
        unsigned int triangle_starting_point = (silence_before_triangle_in_msec * sampling_rate) / 1000.0;
        unsigned int duration_samples = (duration_in_msec * sampling_rate) / 1000.0;
        print_line2(data + triangle_starting_point - 1, duration_samples / 2 + 1, amplitude);
        print_line2(data + triangle_starting_point + duration_samples / 2 - 1, duration_samples / 2 + 2, -amplitude);
    }

    void create_pulse(double* data, unsigned int data_len, int sampling_rate, double silence_before_pulse_in_msec, double duration_in_msec, double amplitude)
    {
        memset(data, 0, sizeof(data[0] * data_len));
        unsigned int pulse_starting_point = (silence_before_pulse_in_msec * sampling_rate) / 1000.0;
        unsigned int duration_samples = (duration_in_msec * sampling_rate) / 1000.0;
        print_line2(data + pulse_starting_point - 1, 2, amplitude);
        print_line2(data + pulse_starting_point, duration_samples, 0);
    }


    char* CreateSomething(char* a_outdataname, char* a_modulated_sine_params)
    {
        cout << "a_modulated_sine_params: " << a_modulated_sine_params << endl;

        modulated_sine_params params = a_modulated_sine_params;
        vector<unsigned int> number_of_samples(params.channel_params.size());
        for (unsigned int i = 0; i < params.channel_params.size(); ++i)
            number_of_samples[i] = params.channel_params[i].number_of_samples;

        CVariable* output2 = m_newvariable_proc();
        output2->Rebuild(number_of_samples.size(), number_of_samples.data());

        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
        {
            cout << "params.channel_params[i].sampling_rate: " << params.channel_params[i].sampling_rate << endl;
            output2->m_sample_rates.m_data[i] = params.channel_params[i].sampling_rate;
            //create_sine(output2->m_data[i], output2->m_widths.m_data[i], output2->m_sample_rates.m_data[i], params.channel_params[i].frequency, params.channel_params[i].amplitude);
            create_triangle(output2->m_data[i], output2->m_widths.m_data[i], output2->m_sample_rates.m_data[i], 200, 100, 0.7);
//            create_sine(int* data, unsigned int data_len, int sampling_rate, double frequency, double amplitude)
            //create_pulse(output2->m_data[i], output2->m_widths.m_data[i], output2->m_sample_rates.m_data[i], 400, 100, 0.5);
            create_pulse(output2->m_data[i], output2->m_widths.m_data[i], output2->m_sample_rates.m_data[i], 4000, 10000, params.channel_params[i].amplitude);
        }
        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(a_outdataname, output2);
        return 0;
    }

    // FIR szuro tervezése (Hamming-ablak használatával)
    void designFIRLowPass(double* h, double fc, double fs, int taps)
    {
        //std::vector<double> h(taps);
        double norm_fc = fc / (fs / 2.0); // Normalizált cutoff frekvencia
        int mid = taps / 2;

        for (int n = 0; n < taps; ++n)
        {
            if (n == mid)
            {
                // Impulzusválasz a 0 pontban
                h[n] = 2 * norm_fc;
            }
            else
            {
                // Sinc függvény
                double k = M_PI * (n - mid);
                h[n] = sin(2 * norm_fc * k) / k;
            }
            // Hamming ablak alkalmazása
            h[n] *= 0.54 - 0.46 * cos(2 * M_PI * n / (taps - 1));
        }

        // Normalizálás (egységnyi nyereség)
        double sum = 0.0;
        for (int i = 0; i < taps; ++i)
            sum += h[i];

        for (int i = 0; i < taps; ++i)
            h[i] /= sum;
    }

    void firHighPassCoefficients(double* coefficients, double cutoffFreq, double sampleRate, int numTaps)
    {
        double normCutoff = cutoffFreq / (sampleRate / 2.0); // Normalized cutoff frequency
        int midPoint = numTaps / 2;

        // Generate low-pass coefficients first
        for (int n = 0; n < numTaps; ++n)
        {
            if (n == midPoint)
            {
                coefficients[n] = 2.0 * normCutoff; // sinc(0) for low-pass
            }
            else
            {
                double k = n - midPoint;
                coefficients[n] = sin(2.0 * M_PI * normCutoff * k) / (M_PI * k); // sinc for low-pass
                coefficients[n] *= 0.54 - 0.46 * cos(2.0 * M_PI * n / (numTaps - 1)); // Hamming window
            }
        }

        // Transform to high-pass coefficients
        for (int n = 0; n < numTaps; ++n)
        {
            coefficients[n] = (n == midPoint ? 1.0 : 0.0) - coefficients[n];
        }
    }

    char* CreateFIRFilter(char* a_outdataname, char* a_type, char* a_sampling_rate, char* a_kernel_size, char* a_cutoff1, char* a_cutoff2)
    {
        double sampling_rate = atof(a_sampling_rate);
        unsigned int kernel_size = atoi(a_kernel_size);
        double cutoff1 = atof(a_cutoff1);
        //double cutoff2 = atof(a_cutoff2);
        unsigned int number_of_samples[1] = {kernel_size};

        CVariable* output2 = m_newvariable_proc();
        output2->Rebuild(1, number_of_samples);

        firHighPassCoefficients(output2->m_data[0], cutoff1, sampling_rate, kernel_size);
//        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
//        {
//            output2->m_sample_rates.m_data[i] = params.channel_params[i].sampling_rate;
//            //create_sine(output2->m_data[i], output2->m_widths.m_data[i], output2->m_sample_rates.m_data[i], params.channel_params[i].frequency, params.channel_params[i].amplitude);
//            create_triangle(output2->m_data[i], output2->m_widths.m_data[i], output2->m_sample_rates.m_data[i], 200, 100, 0.7);
////            create_sine(int* data, unsigned int data_len, int sampling_rate, double frequency, double amplitude)
//            create_pulse(output2->m_data[i], output2->m_widths.m_data[i], output2->m_sample_rates.m_data[i], 400, 100, 0.5);
//        }
        strcpy(output2->m_varname, a_outdataname);
        m_variable_list_ref->Insert(a_outdataname, output2);
        return 0;
    }

#define MAXINT      2147483647
#define MININT      -2147483647

    void normalize(double* data, size_t data_samples, double minimum, double maximum)
    {
        double lmin = MAXINT;
        double lmax = MININT;
        for (unsigned int j = 0; j < data_samples; j++)
        {
            if (lmin > data[j])
                lmin = data[j];
            if (lmax < data[j])
                lmax = data[j];
        }
        double amp_inv = 0;
        if (lmax - lmin)
            amp_inv = (1.0 / (lmax - lmin)) * (maximum - minimum);
        for (unsigned int j = 0; j < data_samples; j++)
            //data[j] = (data[j] - lmin) * amp_inv + minimum;
            data[j] = ceil((data[j] - lmin) * amp_inv + minimum);
    }

    char* Normalize(char* a_invarname, char* a_minimum, char* a_maximum)
    {
        char* l_error = 0;
        if (!a_invarname)
            l_error = MakeString(m_newchar_proc, "ERROR: Normalize: not enough arguments, 'invarname' or 'value' is missing!");
        else
        {
            double minimum = 0;
            double maximum = 0;
            if (a_minimum)
                minimum = atof(a_minimum);
            if (a_maximum)
                maximum = atof(a_maximum);
            CVariable* l_invar = m_variable_list_ref->variablemap_find(a_invarname);
            if (!l_invar)
                l_error = MakeString(m_newchar_proc, "ERROR: Normalize: can't find '", a_invarname, "' in variablelist");
            else
            {
                for (unsigned int ch = 0; ch < l_invar->m_total_samples.m_size; ch++)
                    normalize(l_invar->m_data[ch], l_invar->m_widths.m_data[ch], minimum, maximum);
            }
        }
        return l_error;
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
            return CBasicD::BasicD().MovingWindowFilter(a_param1, a_param2, a_param3);
        case 1:
            return CBasicD::BasicD().Copy(a_param1, a_param2, a_param3);
        case 2:
            return CBasicD::BasicD().DetectSpikes(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7);
        case 3:
            return CBasicD::BasicD().STDev(a_param1, a_param2, a_param3);
        case 4:
            return CBasicD::BasicD().Mean(a_param1, a_param2);
        case 5:
            return CBasicD::BasicD().Median(a_param1, a_param2);
        case 6:
            return CBasicD::BasicD().TruncateSTDev(a_param1, a_param2, a_param3, a_param4);
        case 7:
            return CBasicD::BasicD().CleanupSpikes(a_param1, a_param2, a_param3, a_param4);
        case 8:
            return CBasicD::BasicD().CleanupSignal_Spike(a_param1, a_param2, a_param3, a_param4);
        case 9:
            return CBasicD::BasicD().RefineSpikes(a_param1, a_param2, a_param3, a_param4);
        case 10:
            return CBasicD::BasicD().AverageSignalAroundTrigger(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7);
        case 11:
            return CBasicD::BasicD().SubstractSignalAtTriggers(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 12:
            return CBasicD::BasicD().xcorr(a_param1, a_param2, a_param3, a_param4);
        case 13:
            return CBasicD::BasicD().gauss_1d(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6);
        case 14:
            return CBasicD::BasicD().RefineSpikesWithCorr(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6);
        case 15:
            return CBasicD::BasicD().SetValue(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 16:
            return CBasicD::BasicD().CompareSpikes(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param7);
        case 17:
            return CBasicD::BasicD().SetLabel(a_param1, a_param2, a_param3);
        case 18:
            return CBasicD::BasicD().SetVerticalUnit(a_param1, a_param2, a_param3, a_param4);
        case 19:
            return CBasicD::BasicD().MaxVal(a_param1, a_param2);
        case 20:
            return CBasicD::BasicD().MinVal(a_param1, a_param2);
        case 21:
            return CBasicD::BasicD().FirstSpikeFromEnd(a_param1, a_param2);
        case 22:
            return CBasicD::BasicD().NrValuesAboveThershold(a_param1, a_param2, a_param3);
        case 23:
            return CBasicD::BasicD().RR_Distances(a_param1, a_param2, a_param3);
        case 24:
            return CBasicD::BasicD().ResampleNNIntervals(a_param1, a_param2, a_param3);
        case 25:
            return CBasicD::BasicD().RemoveDCComponent(a_param1);
        case 26:
            return CBasicD::BasicD().ARSpectrum(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 27:
            return CBasicD::BasicD().GenerateECG(a_param1, a_param2, a_param3, a_param4);
        case 28:
            return CBasicD::BasicD().GenerateModulatedECG(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 29:
            return CBasicD::BasicD().AddNoise(a_param1, a_param2, a_param3);
        case 30:
            return CBasicD::BasicD().AddBandLimitedNoise(a_param1, a_param2, a_param3, a_param4);
        case 31:
            return CBasicD::BasicD().CreateModulatedSine(a_param1, a_param2);
        case 32:
            return CBasicD::BasicD().CreateSomething(a_param1, a_param2);
        case 33:
            return CBasicD::BasicD().CreateFIRFilter(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6);
        case 34:
            return CBasicD::BasicD().Normalize(a_param1, a_param2, a_param3);
        }
        return 0;
    }

    void __declspec (dllexport) InitLib(CSignalCodec_List* a_datalist, CVariable_List* a_variablelist, NewCVariable_Proc a_newCVariable, NewChar_Proc a_newChar, Call_Proc a_inpCall, FFT_PROC a_inpFFT, RFFT_PROC a_inpRFFT, FFTEXEC_PROC a_inpFFTEXEC, FFTDESTROY_PROC a_inpFFTDESTROY)
    {
        CBasicD::BasicD().initialize(a_datalist, a_variablelist, a_newCVariable, a_newChar, a_inpCall, a_inpFFT, a_inpRFFT, a_inpFFTEXEC, a_inpFFTDESTROY);
    }

    int __declspec (dllexport) GetProcedureList(TFunctionLibrary* a_functionlibrary_reference)
    {
        CStringVec FunctionList;
        FunctionList.AddElement("MovingWindowFilter(a_src_dataname'the input data', a_dst_dataname'the output', kernel'the filtering kernel')");
        FunctionList.AddElement("Copy(a_dst_name, a_src_name, a_channel_indexes)");
        FunctionList.AddElement("DetectSpikes(a_dst_name, a_spike_signal, a_threshold_signal)");
        FunctionList.AddElement("STDev(dst_name, src_name, src_mean)");
        FunctionList.AddElement("Mean(dst_name, src_name)");
        FunctionList.AddElement("Median(dst_name, src_name)");
        FunctionList.AddElement("TruncateSTDev(var, median, stdev, ratio)");
        FunctionList.AddElement("CleanupSpikes(var_name, spikes, spike_radius, density_threshold)");
        FunctionList.AddElement("CleanupSignal_Spike(var_name, spikes, a1, a2)");
        FunctionList.AddElement("RefineSpikes(var_name, spikes, spike_radius, a_sign)");
        FunctionList.AddElement("AverageSignalAroundTrigger(a_src, a_dst, a_trigger, radius_left, radius_right, a_correlation_treshold, a_all_correlated_slices_varname)");
        FunctionList.AddElement("SubstractSignalAtTriggers(a_var_name, a_trigger, a_signal, a_radius_left, a_radius_right)");
        FunctionList.AddElement("xcorr(a_dst_name, a_var_name, a_kernel, a_method)");
        FunctionList.AddElement("gauss_1d(a_dst_name, a_nr_samples, a_sampling_rate, a_sigma, a_mu, a_sr)");
        FunctionList.AddElement("RefineSpikesWithCorr(a_var_name, a_kernel, a_spikes, a_spike_radius, a_correlation_threshold, a_method)");
        FunctionList.AddElement("SetValue(a_data_name, a_channel, a_x, a_val, a_radius)");
        FunctionList.AddElement("CompareSpikes(a_dst_name, a_ref_spikes, a_spike_channel_ref, a_det_spikes, a_spike_channel_det, a_tolerance_radius_mhu)");
        FunctionList.AddElement("SetLabel(a_var_name, a_channel_indx, a_label)");
        FunctionList.AddElement("SetVerticalUnit(a_var_name, a_channel_indx, a_vertical_unit, a_label)");
        FunctionList.AddElement("MaxVal(a_dst_name, a_src_name)");
        FunctionList.AddElement("MinVal(a_dst_name, a_src_name)");
        FunctionList.AddElement("FirstSpikeFromEnd(a_dst_name, a_src_name)");
        FunctionList.AddElement("NrValuesAboveThershold(a_dst_name, a_src_name, a_threshold)");
        FunctionList.AddElement("RR_Distances(a_in_var_name, a_var_name_to_store, a_channel_index)");
        FunctionList.AddElement("ResampleNNIntervals(a_in_var_name, a_out_var_name, a_target_frequency)");
        FunctionList.AddElement("RemoveDCComponent(a_in_var_name)");
        FunctionList.AddElement("ARSpectrum(a_in_var_name, a_ar_spectrum, a_order, a_fs, a_num_points)");
        FunctionList.AddElement("GenerateECG(a_outdataname, number_of_samples, a_sample_rates, a_frequency)");
        FunctionList.AddElement("GenerateModulatedECG(out_data_name, number_of_samples, sample_rates, simulation_params, qrs_params)");
        FunctionList.AddElement("AddNoise(invarname, noise_amplitude, noise_frequency)");
        FunctionList.AddElement("AddBandLimitedNoise(invarname, noise_amplitude, noise_frequency_low, noise_frequency_high)");
        FunctionList.AddElement("CreateModulatedSine(outdataname, modulated_sine_params)");
        FunctionList.AddElement("CreateSomething(outdataname, modulated_sine_params)");
        FunctionList.AddElement("CreateFIRFilter(outdataname, type, sampling_rate, kernel_size, cutoff1, cutoff2)");
        FunctionList.AddElement("Normalize(dataname, min, max)");

        a_functionlibrary_reference->ParseFunctionList(&FunctionList);
        return FunctionList.m_size;
    }

    bool __declspec (dllexport) CopyrightInfo(CStringMx* a_copyrightinfo)
    {
        a_copyrightinfo->RebuildPreserve(a_copyrightinfo->m_size + 1);
        a_copyrightinfo->m_data[a_copyrightinfo->m_size - 1]->AddElement("This library is.");
        return 0;
    }

    bool __stdcall DllMain(int hInst, int reason, int reserved)
    {
        return true;
    }
}
