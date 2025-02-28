#include d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Scripts\FetalEKG\sketch7.py
#define DATADIR c:\tamas\Data\ForTest
#define RESULTDIR c:\tamas\Data\Results\Spikes

DataDelete();

DetectAndDisplay(DATADIR\recording_2021-03-15_18-14-46_Recording_FEBB.bdf, 1 2, 1, 1, RESULTDIR\46_Recording_FEBB.bin);
//DetectAndDisplay(DATADIR\recording_2021-03-15_18-14-46_Recording_FEBB.bdf, 1 3, 0, 1, RESULTDIR\46_Recording_FEBB2.bin);
//DetectAndDisplay(DATADIR\recording_GulyasDzseniferINA821_01.bdf, 1 2, 1, 1, RESULTDIR\recording_GulyasDzseniferINA821_01.bin);
//DetectAndDisplay(DATADIR\58.bin, 1 20, 0, 0, RESULTDIR\58.bin);
//DetectAndDisplay(DATADIR\58.bin, 1 20, 1, 1, RESULTDIR\582.bin);
//DetectAndDisplay(DATADIR\recording_SeresJuditINA821_01.bdf, 1 2, 0, 0, RESULTDIR\recording_SeresJuditINA821_01.bin);
//DetectAndDisplay(DATADIR\recording_SeresJuditINA821_01.bdf, 1 2, 1, 1, RESULTDIR\recording_SeresJuditINA821_012.bin);

//----------------------------------------

#define REFDIR c:\tamas\Data\Results\SpikesReviewed
#define RESDIR c:\tamas\Data\Results\Spikes

def Compare2Spikes(a_ref, a_spikes)
{
    FileOpen(a_ref, refspikesdata);
    FileOpen(a_spikes, spikesdata);
    DataIn(refspikesdata, refspikes);
    DataIn(spikesdata, spikes);
    DataDelete(refspikesdata);
    DataDelete(spikesdata);

    CompareSpikes(comp_res_maternal, refspikes, 1, spikes, 1, 0.07, error_maternal);
    CompareSpikes(comp_res_fetal, refspikes, 2, spikes, 2, 0.07, error_fetal);

    Append(comp_res_maternal_all, comp_res_maternal);
    Append(comp_res_fetal_all, comp_res_fetal);
    
    Cat(spikes, error_maternal);
    Cat(spikes, error_fetal);
    DisplayData(spikes);
};

Compare2Spikes(REFDIR\spikes_reviewed_2021-03-15_18-14-46_Recording_FEBB.bdf, RESDIR\46_Recording_FEBB.bin, spikesdata);
//Compare2Spikes(REFDIR\spikes_reviewed_2021-03-15_18-14-46_Recording_FEBB.bdf, RESDIR\46_Recording_FEBB2.bin, spikesdata);
//Compare2Spikes(REFDIR\spikes_reviewed_GulyasDzseniferINA821_01.bdf, RESDIR\recording_GulyasDzseniferINA821_01.bin);
//Compare2Spikes(REFDIR\spikes_reviewed_ninfea_58.bin, RESDIR\58.bin);
//Compare2Spikes(REFDIR\spikes_reviewed_ninfea_58.bin, RESDIR\582.bin);
//Compare2Spikes(REFDIR\spikes_reviewed_SeresJuditINA821_01.bdf, RESDIR\recording_SeresJuditINA821_01.bin);
//Compare2Spikes(REFDIR\spikes_reviewed_SeresJuditINA821_01.bdf, RESDIR\recording_SeresJuditINA821_012.bin);

DisplayData(comp_res_maternal_all, fit_width, value_list);
DisplayData(comp_res_fetal_all, fit_width, value_list);