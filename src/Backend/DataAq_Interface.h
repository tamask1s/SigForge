#ifndef DATAAQ_INTERFACE_H_
#define DATAAQ_INTERFACE_H_

class IDataAq
{
public:
    virtual int connectDevice(const char* a_connection_parameter) = 0;
    virtual int disconnectDevice() = 0;
    virtual int setupReading(int a_nr_channels_to_use, const int* a_sample_rates, const int* a_gain, const int* a_physical_mapping, int a_milliseconds_to_read) = 0;
    virtual int startReading() = 0;
    virtual int stopReading() = 0;
    virtual int digitalRead(int* a_data, int a_samples_per_channel_to_read, double a_timeout) = 0;
    virtual int digitalRange(int* a_digital_min, int* a_digital_max) = 0;
    virtual int analogRead(double* aData) = 0;
public:
    static int LoadDataAqModules();
    static void FreeDataAqModules();
    static string GetModuleDescriptions();
    static IDataAq* New(unsigned int a_dataAq_id);
    static void Delete(IDataAq* a_instance);
};

#endif // DATAAQ_INTERFACE_H_
