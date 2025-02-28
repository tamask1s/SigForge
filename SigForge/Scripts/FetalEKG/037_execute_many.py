#include d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Scripts\FetalEKG\sketch3.py
#define DATADIR c:\tamas\Data\ForTest
#define RESULTDIR c:\tamas\Data\Results\Spikes

DataDelete();

FetalEkg(DATADIR\recording_2021-03-15_18-14-46_Recording_FEBB.bdf, 1 2, 1, 1, RESULTDIR\46_Recording_FEBB.bdf);
FetalEkg(DATADIR\recording_2021-03-15_18-14-46_Recording_FEBB.bdf, 1 3, 0, 1, RESULTDIR\46_Recording_FEBB2.bdf);
FetalEkg(DATADIR\recording_GulyasDzseniferINA821_01.bdf, 1 2, 1, 1, RESULTDIR\recording_GulyasDzseniferINA821_01.bdf);
FetalEkg(DATADIR\58.bin, 1 20, 0, 0, RESULTDIR\58.bdf);
FetalEkg(DATADIR\58.bin, 1 20, 1, 1, RESULTDIR\582.bdf);
FetalEkg(DATADIR\recording_SeresJuditINA821_01.bdf, 1 2, 0, 0, RESULTDIR\recording_SeresJuditINA821_01.bdf);
FetalEkg(DATADIR\recording_SeresJuditINA821_01.bdf, 1 2, 1, 1, RESULTDIR\recording_SeresJuditINA821_012.bdf);

//----------------------------------------

#define REFDIR c:\tamas\Data\Results\SpikesReviewed
#define RESDIR c:\tamas\Data\Results\Spikes

def Compare2Spikes(a_ref, a_spikes)
{
    FileOpen(a_ref, refspikesdata);
    FileOpen(a_spikes, spikesdata);
    DataIn(refspikesdata, refspikes);
    DataIn(spikesdata, spikes);

    CompareSpikes(comp_res_maternal, refspikes, 1, spikes, 1, 0.07);
    CompareSpikes(comp_res_fetal, refspikes, 2, spikes, 2, 0.07);

    Append(comp_res_maternal_all, comp_res_maternal);
    Append(comp_res_fetal_all, comp_res_fetal);
};

Compare2Spikes(REFDIR\spikes_reviewed_2021-03-15_18-14-46_Recording_FEBB.bdf, RESDIR\46_Recording_FEBB.bdf, spikesdata);
Compare2Spikes(REFDIR\spikes_reviewed_2021-03-15_18-14-46_Recording_FEBB.bdf, RESDIR\46_Recording_FEBB2.bdf, spikesdata);
Compare2Spikes(REFDIR\spikes_reviewed_GulyasDzseniferINA821_01.bdf, RESDIR\recording_GulyasDzseniferINA821_01.bdf);
Compare2Spikes(REFDIR\spikes_reviewed_ninfea_58.bin, RESDIR\58.bdf);
Compare2Spikes(REFDIR\spikes_reviewed_ninfea_58.bin, RESDIR\582.bdf);
Compare2Spikes(REFDIR\spikes_reviewed_SeresJuditINA821_01.bdf, RESDIR\recording_SeresJuditINA821_01.bdf);
Compare2Spikes(REFDIR\spikes_reviewed_SeresJuditINA821_01.bdf, RESDIR\recording_SeresJuditINA821_012.bdf);

Mean(comp_res_maternal_avg, comp_res_maternal_all);
Mean(comp_res_fetal_avg, comp_res_fetal_all);

Append(comp_res_maternal_all, comp_res_maternal_avg);
Append(comp_res_fetal_all, comp_res_fetal_avg);

Transpose(maternaltr, comp_res_maternal_all);
Transpose(fetaltr, comp_res_fetal_all);
WriteAscii(maternaltr, c:\tamas\data\comp_res_maternal_all.txt);
WriteAscii(fetaltr, c:\tamas\data\comp_res_fetal_all.txt);

DataDelete();
DisplayData(comp_res_maternal_all, fit_width, value_list);
DisplayData(comp_res_fetal_all, fit_width, value_list);
DisplayData(manage_windows, tile_horizontally);