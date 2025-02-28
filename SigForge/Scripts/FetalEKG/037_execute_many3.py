#include d:\tamas\PSAA\Development\Applications\SigForge\SigForge\Scripts\FetalEKG\sketch7.py
#define DATADIR c:\tamas\Data\LabeledSignals\Data
#define RESULTDIR c:\tamas\Data\LabeledSignals\Output

DataDelete();

FetalEkg(DATADIR\dat_CinC_a01.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a01.bin);
FetalEkg(DATADIR\dat_CinC_a02.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a02.bin);
FetalEkg(DATADIR\dat_CinC_a03.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a03.bin);
FetalEkg(DATADIR\dat_CinC_a04.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a04.bin);
FetalEkg(DATADIR\dat_CinC_a05.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a05.bin);
FetalEkg(DATADIR\dat_CinC_a06.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a06.bin);
FetalEkg(DATADIR\dat_CinC_a07.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a07.bin);
FetalEkg(DATADIR\dat_CinC_a08.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a08.bin);
FetalEkg(DATADIR\dat_CinC_a09.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a09.bin);
FetalEkg(DATADIR\dat_CinC_a10.bin, 1 2, 0, 0, RESULTDIR\out_hyb1_CinC_a10.bin);

//----------------------------------------

#define REFDIR c:\tamas\Data\LabeledSignals\Reference
#define RESDIR c:\tamas\Data\LabeledSignals\Output

def Compare2Spikes(a_ref, a_spikes)
{
    FileOpen(a_ref, refspikesdata);
    FileOpen(a_spikes, spikesdata);
    DataIn(refspikesdata, refspikes);
    DataIn(spikesdata, spikes);

    CompareSpikes(comp_res_maternal, refspikes, 1, spikes, 1, 0.05);
    CompareSpikes(comp_res_fetal, refspikes, 2, spikes, 2, 0.05);

    Append(comp_res_maternal_all, comp_res_maternal);
    Append(comp_res_fetal_all, comp_res_fetal);
};

Compare2Spikes(REFDIR\ref_CinC_a01.bin, RESDIR\out_hyb1_CinC_a01.bin);
Compare2Spikes(REFDIR\ref_CinC_a02.bin, RESDIR\out_hyb1_CinC_a02.bin);
Compare2Spikes(REFDIR\ref_CinC_a03.bin, RESDIR\out_hyb1_CinC_a03.bin);
Compare2Spikes(REFDIR\ref_CinC_a04.bin, RESDIR\out_hyb1_CinC_a04.bin);
Compare2Spikes(REFDIR\ref_CinC_a05.bin, RESDIR\out_hyb1_CinC_a05.bin);
Compare2Spikes(REFDIR\ref_CinC_a06.bin, RESDIR\out_hyb1_CinC_a06.bin);
Compare2Spikes(REFDIR\ref_CinC_a07.bin, RESDIR\out_hyb1_CinC_a07.bin);
Compare2Spikes(REFDIR\ref_CinC_a08.bin, RESDIR\out_hyb1_CinC_a08.bin);
Compare2Spikes(REFDIR\ref_CinC_a09.bin, RESDIR\out_hyb1_CinC_a09.bin);
Compare2Spikes(REFDIR\ref_CinC_a10.bin, RESDIR\out_hyb1_CinC_a10.bin);

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