#define FILE triangle_pulse_dc
#define PATH c:\Tamas\mdedata\_extracted\
#define FILENAME CatStrings(PATH, FILE, .raw)
#define SAMPLING_RATE 500

FileOpen(FILENAME, opened_data);
DataIn(opened_data, FILE, 1);
DataDelete(opened_data);
DisplayData(FILE, fit_width,, A2, 001);
//----------------
DataIn(FILE, data);

CreateSomething(sim1, {"channel_params":[
{
    "number_of_samples" : 24000,
    "sampling_rate" : 500,
    "amplitude" : 3,
    "frequency" : 50,
    "modulation_frequency" : 0.02,
    "frequency_modulation_depth" : 50
}]});

Append(data, sim1);
Copy(data2, data, 0);

CreateFilter(filter_, butterworth hp, 2 SAMPLING_RATE, 0.01);
//CreateFilter(filter_, butterworth lp, 2 SAMPLING_RATE, 200);
//CreateFilter(filter_, butterworth bs, 2 SAMPLING_RATE, 49.9 50.1);
Copy(data_IIR, data, 0);
Copy(data_FIR2, data, 0);
Filter(data_IIR, filter_);
//FilterReverse(data, filter_);
CreateFIRFilter(fir_filter, 0, 500, 3901, 0.1);
//LoadAscii(datavartr, c:\Tamas\mdedata\_extracted\coeffs5.csv );
//Transpose(fir_filter, datavartr);
FIRFilter(data, fir_filter);

CreateFIRFilter(fir_filter2, 0, 500, 2501, 0.1);
FIRFilter(data_FIR2, fir_filter2);
Filter(data_FIR2, filter_);



DisplayData(data);
DisplayData(data2);
DisplayData(data_IIR);
DisplayData(data_FIR2);
DisplayData(fir_filter);


DisplayData(sim1,fit_width,, C, 000);