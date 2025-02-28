#define OUTPUT_FILE d:\tamas\PSAA\Development\Applications\SigForge\SigForge\App\spikes_reviewed.bdf, 100, 4
def OnDataMousedown(a_data_name, a_button, a_val_x, a_val_y, a_data_channel, a_data_val)
{
    if (IsEqual(a_button, 1))
    {
        SetValue(a_data_name, a_data_channel, a_val_x, 1);
        RefreshDataWindow(a_data_name, false);        
        SaveDataToFile(a_data_name, OUTPUT_FILE);
    };
    if (IsEqual(a_button, 16))
    {
        SetValue(a_data_name, a_data_channel, a_val_x, 0.5);
        RefreshDataWindow(a_data_name, false);        
        SaveDataToFile(a_data_name, OUTPUT_FILE);
    };
    if (IsEqual(a_button, 2))
    {
        SetValue(a_data_name, a_data_channel, a_val_x, 0, 0.1);
        RefreshDataWindow(a_data_name, false);
        SaveDataToFile(a_data_name, OUTPUT_FILE);
    };
};
FileOpen(d:\tamas\PSAA\Development\Applications\SigForge\SigForge\App\spikes.bdf, data1);
DataIn(data1, data2);
DataDelete(data1);
DisplayData(data2);
OnDataEvent(data2, mousedown, OnDataMousedown);