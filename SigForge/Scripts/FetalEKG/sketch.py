//DataDelete();

#define MCorr 0.7
#define FCorr -0.5

FileOpen(d:\tamas\PSAA\Development\Test\Data\3CH\2021-03-15\2021-03-15_18-14-46_Recording_FEBB_szep.bdf, sig);
DataIn(sig, orig_sig_1, 1 2);
//Copy(original, orig_sig_1); DisplayData(original,,,, 000);
CreateFilter(bandpass_filter2, butterworth bp, 2 2000, 0.7 500);
Filter(orig_sig_1, bandpass_filter2);
FilterReverse(orig_sig_1, bandpass_filter2);

Copy(mother_sig1, orig_sig_1, 0);
Copy(mother_sig2, orig_sig_1, 0);

Copy(fetal_sig1, orig_sig_1, 1);
Copy(fetal_sig2, orig_sig_1, 1);

def SpikeDetector(ekgsignal, spikes, spike_radius, density_threshold, bo_low, bo_high, details)
{
    CreateFilter(bandpass_filter, butterworth bp, 2 2000, bo_low bo_high);
    CreateFilter(derivative_filter, butterworth hp, 2 2000, 10);
    CreateFilter(integrative_filter, butterworth lp, 2 2000, 8);
    CreateFilter(threshold_filter, butterworth lp, 2 2000, 1.5);
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
    CleanupSpikes(ekgsignal, spikes, spike_radius, density_threshold);
    if (details)
    {
        Cat(ekgsignal_threshold, ekgsignal);
        DisplayData(ekgsignal_threshold, fit_width,, C, 001);
    };
};

def SpikeDetector2(ekgsignal, spikes, spike_radius, density_threshold, bo_low, bo_high, details)
{
    CreateFilter(bandpass_filter, butterworth bp, 2 2000, bo_low bo_high);
    CreateFilter(derivative_filter, butterworth hp, 2 2000, 10);
    CreateFilter(integrative_filter, butterworth lp, 2 2000, 8);
    CreateFilter(threshold_filter, butterworth lp, 2 2000, 1.5);
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
    CleanupSpikes(ekgsignal, spikes, spike_radius, density_threshold);
    if (details)
    {
        Cat(ekgsignal_threshold, ekgsignal);
        DisplayData(ekgsignal_threshold, fit_width,, C, 001);
    };
};

SpikeDetector(mother_sig1, maternal_spikes, 0.3, 0.5, 8, 20);
RefineSpikes(mother_sig2, maternal_spikes, 0.3);

AverageSignalAroundTrigger(fetal_sig1, MAVG, maternal_spikes, 0.21, 0.35, MCorr, AVGOS);
SubstractSignalAtTriggers(fetal_sig1, maternal_spikes, MAVG, 0.21, 0.35);
Copy(orig_sig_5, fetal_sig1);
SpikeDetector(fetal_sig1, fetal_spikes, 0.25, 0.3, 16, 40, true);
//RefineSpikes(orig_sig_5, fetal_spikes, 0.2);

AverageSignalAroundTrigger(orig_sig_5, FAVG, fetal_spikes, 0.15, 0.22, FCorr, FAVGOS);

CreateFilter(bandpass_filter3, butterworth bp, 2 2000, 20 100);
Filter(orig_sig_5, bandpass_filter3);
FilterReverse(orig_sig_5, bandpass_filter3);


xcorr(correlated, orig_sig_5, FAVG);

gauss_1d(FKernel, 1480, 2000, 5, -10, 1);
gauss_1d(FKernelD, 1480, 2000, 5, 10, 1);
IProd(FKernelD, -1);
Add(FKernel, FKernelD);
xcorr(correlated2, orig_sig_5, FKernel);

gauss_1d(FKernel2, 1480, 2000, 5, -10, 0.5);
gauss_1d(FKernel2D, 1480, 2000, 5, 10, 0.5);
IProd(FKernel2D, -1);
Add(FKernel2, FKernel2D);
xcorr(correlated3, orig_sig_5, FKernel2);

Cat(orig_sig_5, fetal_spikes);
Cat(orig_sig_5, correlated2);
Cat(orig_sig_5, correlated3);
Cat(orig_sig_5, maternal_spikes);
//Cat(orig_sig_5, fetal_sig2);

Cat(mother_sig2, maternal_spikes);

DisplayData(correlated,,, 000);
DisplayData(orig_sig_5,,, 000);
DisplayData(MAVG, fit_width,, A1, 000);
DisplayData(AVGOS, fit_width,, C, 001);
DisplayData(FAVG, fit_width,, A1, 000);
DisplayData(FAVGOS, fit_width,, C, 001);
//DisplayData(manage_windows, tile_vertically);




DataIn(FAVG, favg);
gauss_1d(gauss, 740, 1, 5, -85, 2.4);
gauss_1d(gauss2, 740, 1, 5, -65, 2.4);
IProd(gauss2, -1);
Add(gauss, gauss2);
IAdd(favg, 0.01);
Cat(gauss, favg);
DisplayData(gauss, fit_width,, A1, 000);


gauss_1d(gauss3, 1480, 2000, 5, -10, 1.2);
gauss_1d(gauss4, 1480, 2000, 5, 10, 1.2);
IProd(gauss3, -1);
Add(gauss3, gauss4);
DisplayData(gauss3, fit_width,, A1, 000);