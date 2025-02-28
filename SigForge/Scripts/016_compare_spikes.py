FileOpen(d:\tamas\PSAA\Development\Applications\SigForge\SigForge\App\spikes_reviewed_ninfea_58.bin, refspikesdata);
FileOpen(d:\tamas\PSAA\Development\Applications\SigForge\SigForge\App\spikes_ch20_ninfea_58.bin, spikesdata);
DataIn(refspikesdata, refspikes);
DataIn(spikesdata, spikes);

CompareSpikes(comp_res_maternal, refspikes, 1, spikes, 1, 0.05);
CompareSpikes(comp_res_fetal, refspikes, 2, spikes, 2, 0.05);

DisplayData(comp_res_maternal,, value_list);
DisplayData(comp_res_fetal,, value_list);
