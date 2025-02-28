#define SAMPR 2000
#define CHANNELS_TO_IMPORT 1 2
#define CHANNEL_INDX_MOTHER 1
#define CHANNEL_INDX_FETAL CHANNEL_INDX_MOTHER 


//#define DF d:\tamas\PSAA\Development\Test\Data\tasks_038\Bin\SeresJuditINA821_01.bdf
#define DF d:\tamas\PSAA\Development\Test\Data\3CH\2021-03-15\2021-03-15_18-14-46_Recording_FEBB_szep.bdf
//#define DF d:\tamas\PSAA\Development\Test\Data\2021-04-16_08-15-50_control_signal_no_fetal_ekg_noisy.bdf
//#define DF d:\tamas\PSAA\Development\Applications\SigForge\SigForge\App\2021-04-15_17-55-15_BB.bdf 
//#define DF d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Data\2021-04-12_13-08-12GGF.bdf
//#define DF d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Data\2021-04-22_13-19-18_HOE02.bdf


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

def FetalEkg()
{
    FileOpen(DF, sig);
    DataIn(sig, orig_sig_1, CHANNELS_TO_IMPORT);

    CreateFilter(bandpass_filter2, butterworth bp, 2 SAMPR, 0.7 200);
    Filter(orig_sig_1, bandpass_filter2);
    FilterReverse(orig_sig_1, bandpass_filter2);

    CreateFilter(bandstop_filter2, butterworth bs, 2 SAMPR, 49.9 50.1);
    Filter(orig_sig_1, bandstop_filter2);
    FilterReverse(orig_sig_1, bandstop_filter2);

    Copy(mother_sig1, orig_sig_1, CHANNEL_INDX_MOTHER);
    Copy(mother_sig2, orig_sig_1, CHANNEL_INDX_MOTHER);

    Copy(fetal_sig1, orig_sig_1, CHANNEL_INDX_FETAL);
    Copy(fetal_sig2, orig_sig_1, CHANNEL_INDX_FETAL);

    FetalSpikeDetector(mother_sig1, maternal_spikes, 8 20, 20, 3, 1.2 2.5, 0.4, MAVG, 0.21, 0.35, true);
    SubstractSignalAtTriggers(fetal_sig1, maternal_spikes, MAVG, 0.21, 0.35);
    
    Copy(orig_sig_5, fetal_sig1);
    FetalSpikeDetector(fetal_sig1, fetal_spikes, 20 100, 7, 1, 2 2.5, -0.1, FAVG, 0.25, 0.32, true);
       
    Cat(orig_sig_5, fetal_spikes);
    Cat(orig_sig_5, maternal_spikes);

    Cat(mother_sig2, maternal_spikes);
};

//DataDelete();
FetalEkg();
DisplayData(mother_sig2,,, 000);
DisplayData(MAVG, fit_width,, A1, 000);
DisplayData(MAVGOS, fit_width,, C, 001);
DisplayData(FAVG, fit_width,, A1, 000);
DisplayData(FAVGOS, fit_width,, C, 001);
//DisplayData(manage_windows, tile_vertically);