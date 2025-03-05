// ECG R DETECTOR for "orig_sig"

#define SAMPLING_RATE 1000

CreateFilter(bandpass_filter, butterworth bp, 2 SAMPLING_RATE, 0.7 400);
Filter(orig_sig, bandpass_filter);
FilterReverse(orig_sig, bandpass_filter);

CreateFilter(bandstop_filter, butterworth bs, 2 SAMPLING_RATE, 49.9 50.1);
Filter(orig_sig, bandstop_filter);
FilterReverse(orig_sig, bandstop_filter);

Copy(sig_copy, orig_sig, 0); // copy 0th channel from orig_sig to sig_copy

def SpikeDetector(ekgsignal, spikes, spike_radius, density_threshold, bo_low, bo_high, show_details)
{
    CreateFilter(bandpass_filter, butterworth bp, 2 SAMPLING_RATE, bo_low bo_high);
    CreateFilter(derivative_filter, butterworth hp, 2 SAMPLING_RATE, 10);
    CreateFilter(integrative_filter, butterworth lp, 2 SAMPLING_RATE, 8);
    CreateFilter(threshold_filter, butterworth lp, 2 SAMPLING_RATE, 1.5);
    Filter(ekgsignal, bandpass_filter);
    FilterReverse(ekgsignal, bandpass_filter);
    Filter(ekgsignal, derivative_filter);
    FilterReverse(ekgsignal, derivative_filter);
    SQR_Inplace(ekgsignal); 
    Filter(ekgsignal, integrative_filter);
    FilterReverse(ekgsignal, integrative_filter);
    Copy(threshold_signal, ekgsignal);
    Filter(threshold_signal, threshold_filter);
    FilterReverse(threshold_signal, threshold_filter);
    DetectSpikes(spikes, ekgsignal, threshold_signal); // a thrashold signalhoz képest nézzük a jelet, így offsettel nem kell külön bajlódni. részletesen lentebb
    CleanupSpikes(ekgsignal, spikes, spike_radius, density_threshold);
    if (show_details)
    {
        Cat(threshold_signal, ekgsignal);
        Cat(threshold_signal, spikes);
        DisplayData(threshold_signal,,, B, 001);
    };
};

SpikeDetector(sig_copy, spikes, 0.3, 0.5, 8, 20, true);
RefineSpikes(orig_sig, spikes, 0.3);


https://tamask1s.github.io/SigForge/#DetectSpikes

.'Parameters'

* 'dst_name': the name of the output signal the detected spikes will be marked in
* 'spike_signal': the name of the original signal with spikes to be detected
* 'threshold_signal': a signal which serves as a threshold for spike detection
* 'marker_value':
** The spike locations will be marked by this value in the output variable
** Optional. Default value is '1'.
* 'refractory_period':
** After a spike was detected, within this period no new spike should be detected. The refractory_period is measured in the spike_signal's horizontal units (likely milliseconds).
** Optional. Default value is '240'.
* 'previous_spike_reference_ratio':
** The new spike, in order to be detected, should be greater than the previous spike multiplied by this ratio
** Optional. Default value is '0.5'.
* 'previous_spike_reference_attenuation':
** If there was no spike detected due to a high previous spike amplitude value, then the memorized spike amplitude value is attenuated (lowered) on every evaluation of a new sample by the following equation: *previous_spike_amplitude /= 1.0 + 'previous_spike_reference_attenuation' / sampling_rate*.
** Optional. Default value is '30'.

char* DetectSpikes(char* a_dst_name, char* a_spike_signal, char* a_threshold_signal, char* a_marker_val, char* a_refracter, char* a_previous_spike_reference_ratio, char* a_previous_spike_reference_attenuation)
{
    if (!a_dst_name || !a_spike_signal || !a_threshold_signal)
        return MakeString(m_newchar_proc, "ERROR: DetectSpikes: not enough arguments.");

    CVariable* spike_signal = m_variable_list_ref->variablemap_find(a_spike_signal);
    if (!spike_signal)
        return MakeString(m_newchar_proc, "ERROR: DetectSpikes ", a_spike_signal, " not found.");

    CVariable* threshold_signal = m_variable_list_ref->variablemap_find(a_threshold_signal);
    if (!threshold_signal)
        return MakeString(m_newchar_proc, "ERROR: DetectSpikes ", a_threshold_signal, " not found.");

    double marker_val = 1;
    double refracter = 240;
    double previous_spike_reference_ratio = 0.5;
    double previous_spike_reference_attenuation = 30;

    double_from_str(marker_val, a_marker_val);
    double_from_str(refracter, a_refracter);
    double_from_str(previous_spike_reference_ratio, a_previous_spike_reference_ratio);
    double_from_str(previous_spike_reference_attenuation, a_previous_spike_reference_attenuation);

    unsigned int nr_channels = spike_signal->m_total_samples.m_size;

    CVariable* dst = m_newvariable_proc();
    dst->Rebuild(nr_channels, spike_signal->m_total_samples.m_data);
    spike_signal->SCopyTo(dst);
    strcpy(dst->m_varname, a_dst_name);
    m_variable_list_ref->Insert(dst->m_varname, dst);

    for (unsigned int ch = 0; ch < nr_channels; ++ch)
    {
        bool searching_for_spikes = true;
        int refracter_samples = spike_signal->m_sample_rates.m_data[ch] / (1000.0 / refracter);
        double previous_spike_amplitude = 0;

        for (unsigned int i = 0; i < dst->m_total_samples.m_data[ch] - 1; ++i)
        {
            dst->m_data[ch][i] = 0;
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
                    searching_for_spikes = true;
                }
            }
        }
    }
    return 0;
}

/** removes spikes which has lower energy (RMS, normalized L2Norm) than the average. The thrashold ratio is given as parameter. spikes with much bigger energy are also removed. */
char* CleanupSpikes(char* a_var_name, char* a_spikes, char* a_spike_radius, char* a_density_threshold)
{
    if (!a_var_name || !a_spikes || !a_spike_radius || !a_density_threshold)
        return MakeString(m_newchar_proc, "ERROR: CleanupSpikes: not enough arguments.");

    CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
    if (!var)
        return MakeString(m_newchar_proc, "ERROR: CleanupSpikes ", a_var_name, " not found.");

    CVariable* spikes = m_variable_list_ref->variablemap_find(a_spikes);
    if (!spikes)
        return MakeString(m_newchar_proc, "ERROR: CleanupSpikes ", a_spikes, " not found.");

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
    return 0;
}

/** refines spike locations to the position of a nearby local minimum or maximum in the original variable. The maximum proximity of searching is defined by the radius. */
char* RefineSpikes(char* a_var_name, char* a_spikes, char* a_spike_radius, char* a_sign)
{
    if (!a_var_name || !a_spikes || !a_spike_radius)
        return MakeString(m_newchar_proc, "ERROR: RefineSpikes: not enough arguments.");

    CVariable* var = m_variable_list_ref->variablemap_find(a_var_name);
    if (!var)
        return MakeString(m_newchar_proc, "ERROR: RefineSpikes ", a_var_name, " not found.");

    CVariable* spikes = m_variable_list_ref->variablemap_find(a_spikes);
    if (!spikes)
        return MakeString(m_newchar_proc, "ERROR: RefineSpikes ", a_spikes, " not found.");

    int sign = 0;
    int_from_str(sign, a_sign);

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
    return 0;
}
