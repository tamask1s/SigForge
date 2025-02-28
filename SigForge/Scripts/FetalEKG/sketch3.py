#define SAMPR 2000
#define CHANNELS_TO_IMPORT 1 20
#define CHANNEL_INDX_MOTHER 0
#define CHANNEL_INDX_FETAL 0
#define OUT2FILE c:\tamas\Data\Results\Spikes\default_output.bdf

//#define FILE d:\tamas\PSAA\Development\Test\Data\tasks_038\Bin\SeresJuditINA821_01.bdf
//#define FILE d:\tamas\PSAA\Development\Test\Data\3CH\2021-03-15\2021-03-15_18-14-46_Recording_FEBB_szep.bdf
//#define FILE d:\tamas\PSAA\Development\Test\Data\tasks_038\GulyasDzseniferINA821_01.bdf
//#define FILE d:\tamas\PSAA\Development\Test\Data\2021-04-16_08-15-50_control_signal_no_fetal_ekg_noisy.bdf
//#define FILE d:\tamas\PSAA\Development\Applications\SigForge\SigForge\App\2021-04-15_17-55-15_BB.bdf 
//#define FILE d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Data\2021-04-12_13-08-12GGF.bdf
//#define FILE d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Data\2021-04-22_13-19-18_HOE02.bdf
#define FILE d:\ninfea-non-invasive-multimodal-foetal-ecg-doppler-dataset-for-antenatal-cardiology-research-1.0.0\bin_format_ecg_and_respiration\58.bin
//#define FILE c:\tamas\Data\ForTest\58.bin

#define MCorr 0.7
#define FCorr -0.5

def SpikeDetector0(ekgsignal, spikes, spike_radius, density_threshold, bo_low, bo_high, details)
{
    CreateFilter(bandpass_filter, butterworth bp, 2 2000, bo_low bo_high);
    CreateFilter(derivative_filter, butterworth hp, 2 2000, 10);
    CreateFilter(integrative_filter, butterworth lp, 2 2000, 8);
    CreateFilter(threshold_filter, butterworth lp, 2 2000, 1.05);
    Filter(ekgsignal, bandpass_filter);
    FilterReverse(ekgsignal, bandpass_filter);
    Filter(ekgsignal, derivative_filter);
    FilterReverse(ekgsignal, derivative_filter);
    SQR_Inplace(ekgsignal);
    Filter(ekgsignal, integrative_filter);
    FilterReverse(ekgsignal, integrative_filter);
    Copy(ekgsignal_threshold, ekgsignal);
    Filter(ekgsignal_threshold, threshold_filter);
    FilterReverse(ekgsignal_threshold, threshold_filter);
    DetectSpikes(spikes, ekgsignal, ekgsignal_threshold);
    //CleanupSpikes(ekgsignal, spikes, spike_radius, density_threshold);
    if (details)
    {
        Cat(ekgsignal_threshold, ekgsignal);
        DisplayData(ekgsignal_threshold, fit_width,, C, 001);
    };
};

def SpikeDetector(ekgsignal, spikes, spike_radius, density_threshold, bo_low, bo_high, details)
{
    CreateFilter(bandpass_filter, butterworth bp, 2 2000, bo_low bo_high);
    CreateFilter(integrative_filter, butterworth lp, 2 2000, 8);
    CreateFilter(threshold_filter, butterworth lp, 2 2000, 1.5);   
    Filter(ekgsignal, bandpass_filter);
    FilterReverse(ekgsignal, bandpass_filter);
    Copy(ekgsignaltmp, ekgsignal);

    gauss_1d(FKernel, 1480, 1, 5, -7, 1.95);
    gauss_1d(FKernelD, 1480, 1, 5, 7, 1.95);
    IProd(FKernelD, -1);
    Add(FKernel, FKernelD);
    xcorr(correlated1, ekgsignal, FKernel);

    Copy(correlated, correlated1);

    Filter(correlated, bandpass_filter);
    FilterReverse(correlated, bandpass_filter);
    SQR_Inplace(correlated);
    Filter(correlated, integrative_filter);
    FilterReverse(correlated, integrative_filter);

    Copy(correlated2, correlated);

    CreateFilter(bandpass_filter3, butterworth bp, 1 2000, 2 2.5);
    Filter(correlated2, bandpass_filter3);
    FilterReverse(correlated2, bandpass_filter3);
    Copy(correlated, correlated2);

    Copy(ekgsignal_threshold, correlated);
    Filter(ekgsignal_threshold, threshold_filter);
    FilterReverse(ekgsignal_threshold, threshold_filter);
    DetectSpikes(spikes, correlated, ekgsignal_threshold);
    RefineSpikesWithCorr(ekgsignaltmp, FKernel, spikes, 0.1);
};

def SpikeDetecto2(ekgsignal, spikes, spike_radius, density_threshold, bo_low, bo_high, details)
{
    CreateFilter(bandpass_filter, butterworth bp, 2 2000, bo_low bo_high);
    CreateFilter(integrative_filter, butterworth lp, 2 2000, 8);
    CreateFilter(threshold_filter, butterworth lp, 2 2000, 1.5);   
    CreateFilter(derivative_filter, butterworth bp, 2 2000, 15 49);
    Filter(ekgsignal, bandpass_filter);
    FilterReverse(ekgsignal, bandpass_filter);
    Copy(ekgsignaltmp, ekgsignal);
    gauss_1d(FKernel, 1480, 2000, 5, -7, 1.0);
    gauss_1d(FKernelD, 1480, 2000, 5, 7, 1.0);
    IProd(FKernelD, -1);
    Add(FKernel, FKernelD);
    
    xcorr(correlated1, ekgsignal, FKernel);
    Copy(correlated, correlated1);
    
    //Filter(ekgsignal, derivative_filter);
    //FilterReverse(ekgsignal, derivative_filter);
    //Copy(correlated, ekgsignal);
    
    Filter(correlated, bandpass_filter);
    FilterReverse(correlated, bandpass_filter);
    SQR_Inplace(correlated);
    Filter(correlated, integrative_filter);
    FilterReverse(correlated, integrative_filter);

    Copy(correlated2, correlated);

    CreateFilter(bandpass_filter3, butterworth bp, 1 2000, 2 2.5);
    Filter(correlated2, bandpass_filter3);
    FilterReverse(correlated2, bandpass_filter3);
    Copy(correlated, correlated2);

    Copy(ekgsignal_threshold, correlated);
    Filter(ekgsignal_threshold, threshold_filter);
    FilterReverse(ekgsignal_threshold, threshold_filter);
    DetectSpikes(spikes, correlated, ekgsignal_threshold);
    RefineSpikesWithCorr(ekgsignaltmp, FKernel, spikes, 0.1);
    if (details)
    {
        Cat(ekgsignal_threshold, ekgsignaltmp);
        Cat(ekgsignal_threshold, correlated);
		Cat(ekgsignal_threshold, spikes);
        Cat(ekgsignal_threshold, debugs);

        DisplayData(ekgsignal_threshold, fit_width,, C, 001);
        DisplayData(FKernel, fit_width,, A1, 000);
    };
};

def FetalEkg(a_fie, a_channels_to_import, a_channel_indx_maternal, a_channel_index_fetal, a_outfile, details)
{
    FileOpen(a_fie, sig);
    DataIn(sig, orig_sig_1, a_channels_to_import);
    //Copy(original, orig_sig_1); DisplayData(original,,,, 000);
    CreateFilter(bandpass_filter2, butterworth bp, 2 2000, 0.7 500);
    Filter(orig_sig_1, bandpass_filter2);
    FilterReverse(orig_sig_1, bandpass_filter2);

    CreateFilter(bandstop_filter2, butterworth bs, 2 2000, 49.9 50.1);
    Filter(orig_sig_1, bandstop_filter2);
    FilterReverse(orig_sig_1, bandstop_filter2);

    Copy(mother_sig1, orig_sig_1, a_channel_indx_maternal);
    Copy(mother_sig2, orig_sig_1, a_channel_indx_maternal);

    Copy(fetal_sig1, orig_sig_1, a_channel_index_fetal);
    Copy(fetal_sig2, orig_sig_1, a_channel_index_fetal);
    
    SpikeDetector0(mother_sig1, maternal_spikes, 0.3, 0.5, 11, 23, true);
    //RefineSpikes(mother_sig2, maternal_spikes, 0.3);

    AverageSignalAroundTrigger(fetal_sig1, MAVG, maternal_spikes, 0.21, 0.35,, AVGOS);
    SubstractSignalAtTriggers(fetal_sig1, maternal_spikes, MAVG, 0.21, 0.35);
    Copy(orig_sig_5, fetal_sig1);
    SpikeDetecto2(fetal_sig1, fetal_spikes, 0.25, 0.5, 20, 100, details);
    //RefineSpikes(orig_sig_5, fetal_spikes, 0.2);

    AverageSignalAroundTrigger(orig_sig_5, FAVG, fetal_spikes, 0.25, 0.32,, FAVGOS);

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
    FetalEkg(a_file, a_channels_to_import, a_channel_indx_maternal, a_channel_index_fetal, a_outfile, true);
    DisplayData(correlated,,, 000);
    DisplayData(mother_sig2,,, 000);
    OnDataEvent(mother_sig2, mousedown, OnSignalWaveMousedown, a_outfile);
    DisplayData(MAVG, fit_width,, A1, 000);
    DisplayData(AVGOS, fit_width,, C, 001);
    DisplayData(FAVG, fit_width,, A1, 000);
    DisplayData(FAVGOS, fit_width,, C, 001);
    //DisplayData(manage_windows, tile_vertically);
};

main DetectAndDisplay(FILE, CHANNELS_TO_IMPORT, CHANNEL_INDX_MOTHER, CHANNEL_INDX_FETAL, OUT2FILE);