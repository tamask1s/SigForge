#define SAMPR 2000
#define CHANNELS_TO_IMPORT 1 2
#define CHANNEL_INDX_MOTHER 1
#define CHANNEL_INDX_FETAL CHANNEL_INDX_MOTHER 
#define OUT2FILE c:\tamas\Data\Results\Spikes\default_output.bdf

#define FILE d:\tamas\PSAA\Development\Test\Data\tasks_038\Bin\SeresJuditINA821_01.bdf
//#define FILE d:\tamas\PSAA\Development\Test\Data\3CH\2021-03-15\2021-03-15_18-14-46_Recording_FEBB_szep.bdf
//#define FILE d:\tamas\PSAA\Development\Test\Data\tasks_038\GulyasDzseniferINA821_01.bdf
//#define FILE d:\tamas\PSAA\Development\Test\Data\2021-04-16_08-15-50_control_signal_no_fetal_ekg_noisy.bdf
//#define FILE d:\tamas\PSAA\Development\Applications\SigForge\SigForge\App\2021-04-15_17-55-15_BB.bdf 
//#define FILE d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Data\2021-04-12_13-08-12GGF.bdf
//#define FILE d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Data\2021-04-22_13-19-18_HOE02.bdf

def FetalSpikeDetector(in_sig, spikes, freq_bounds, kern_theta, kern_rad, hr_fr_bounds, ptrh, mom_avg, avg_rad_l, avg_rad_r, details)
{
    Copy(in_sigtmp, in_sig);

    CreateFilter(bandpass_filter, butterworth bp, 2 SAMPR, freq_bounds);
    CreateFilter(integrative_filter, butterworth lp, 2 SAMPR, 8);
    CreateFilter(threshold_filter, butterworth lp, 2 SAMPR, 1.5);   
    Filter(in_sig, bandpass_filter);
    FilterReverse(in_sig, bandpass_filter);
    
    gauss_1d(FKernel, 480, SAMPR, 5, -kern_theta, kern_rad);
    gauss_1d(FKernelD, 480, SAMPR, 5, kern_theta, kern_rad);
    IProd(FKernelD, -1);
    Add(FKernel, FKernelD);

    Filter(in_sig, bandpass_filter);
    FilterReverse(in_sig, bandpass_filter);
    SQR_Inplace(in_sig);
    Filter(in_sig, integrative_filter);
    FilterReverse(in_sig, integrative_filter);

    CreateFilter(bandpass_filter3, butterworth bp, 1 SAMPR, hr_fr_bounds);
    Filter(in_sig, bandpass_filter3);
    FilterReverse(in_sig, bandpass_filter3);

    Copy(in_sig_threshold, in_sig);
    Filter(in_sig_threshold, threshold_filter);
    FilterReverse(in_sig_threshold, threshold_filter);
    DetectSpikes(spikes, in_sig, in_sig_threshold,, 0);
    RefineSpikesWithCorr(in_sigtmp, FKernel, spikes, 0.2, ptrh);

    AverageSignalAroundTrigger(in_sigtmp, mom_avg, spikes, 0.04, 0.04,, mom_avgOS);
    RefineSpikesWithCorr(in_sigtmp, mom_avg, spikes, 0.01, 0.6);
    
    AverageSignalAroundTrigger(in_sigtmp, mom_avg, spikes, avg_rad_l, avg_rad_r,, mom_avgOS);    

    if (details)
    {
        Cat(in_sig_threshold, in_sigtmp);
        Cat(in_sig_threshold, in_sig);
        Cat(in_sig_threshold, spikes);
        Cat(in_sig_threshold, debugs);

        DisplayData(in_sig_threshold, fit_width,, C, 001);
        DisplayData(FKernel, fit_width,, A1, 000);
    };
};

def FetalEkg(a_fie, a_channels_to_import, a_channel_indx_maternal, a_channel_index_fetal, a_outfile, details)
{
    FileOpen(a_fie, sig);
    DataIn(sig, orig_sig_1, a_channels_to_import);

    CreateFilter(bandpass_filter2, butterworth bp, 2 SAMPR, 0.7 200);
    Filter(orig_sig_1, bandpass_filter2);
    FilterReverse(orig_sig_1, bandpass_filter2);

    CreateFilter(bandstop_filter2, butterworth bs, 2 SAMPR, 49.9 50.1);
    Filter(orig_sig_1, bandstop_filter2);
    FilterReverse(orig_sig_1, bandstop_filter2);

    Copy(mother_sig1, orig_sig_1, a_channel_indx_maternal);
    Copy(mother_sig2, orig_sig_1, a_channel_indx_maternal);

    Copy(fetal_sig1, orig_sig_1, a_channel_index_fetal);
    Copy(fetal_sig2, orig_sig_1, a_channel_index_fetal);

    FetalSpikeDetector(mother_sig1, maternal_spikes, 8 20, 20, 3, 1.2 2.5, 0.4, MAVG, 0.21, 0.35, details);
    SubstractSignalAtTriggers(fetal_sig1, maternal_spikes, MAVG, 0.21, 0.35);
    
    Copy(orig_sig_5, fetal_sig1);
    FetalSpikeDetector(fetal_sig1, fetal_spikes, 20 100, 7, 1, 2 2.5, -0.1, FAVG, 0.25, 0.32, details);
       
    Cat(orig_sig_5, fetal_spikes);
    Cat(orig_sig_5, maternal_spikes);

    Cat(mother_sig2, maternal_spikes);
    Cat(mother_sig2, fetal_spikes);
    SaveVarToFile(mother_sig2, a_outfile, 100, 4);
};

def OnSignalWaveMousedown(a_data_name, a_button, a_val_x, a_val_y, a_data_channel, a_data_val, a_outfile)
{
    if (IsEqual(a_button, 1))
    {
        SetValue(a_data_name, a_data_channel, a_val_x, 1);
        RefreshDataWindow(a_data_name, false);
        SaveDataToFile(a_data_name, a_outfile, 100, 4);
    };
    if (IsEqual(a_button, 16))
    {
        SetValue(a_data_name, a_data_channel, a_val_x, 0.5);
        RefreshDataWindow(a_data_name, false);
        SaveDataToFile(a_data_name, a_outfile, 100, 4);
    };
    if (IsEqual(a_button, 2))
    {
        SetValue(a_data_name, a_data_channel, a_val_x, 0, 0.1);
        RefreshDataWindow(a_data_name, false);
        SaveDataToFile(a_data_name, a_outfile, 100, 4);
    };
};

def DetectAndDisplay(a_file, a_channels_to_import, a_channel_indx_maternal, a_channel_index_fetal, a_outfile)
{
    //DataDelete();
    FetalEkg(a_file, a_channels_to_import, a_channel_indx_maternal, a_channel_index_fetal, a_outfile, true);
    DisplayData(mother_sig2,,, 000);
    OnDataEvent(mother_sig2, mousedown, OnSignalWaveMousedown, a_outfile);
    DisplayData(MAVG, fit_width,, A1, 000);
    DisplayData(MAVGOS, fit_width,, C, 001);
    DisplayData(FAVG, fit_width,, A1, 000);
    DisplayData(FAVGOS, fit_width,, C, 001);
    //DisplayData(manage_windows, tile_vertically);
};
//#include d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Scripts\FetalEKG\sketch6.py
main DetectAndDisplay(FILE, CHANNELS_TO_IMPORT, CHANNEL_INDX_MOTHER, CHANNEL_INDX_FETAL, OUT2FILE);
