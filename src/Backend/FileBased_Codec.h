#ifndef FILEBASED_CODEC_H_
#define FILEBASED_CODEC_H_

class FileBasedCodec: public ISignalCodec
{
    TDataType* m_internal_codec;
    void CopyFromInternalCodec();

public:
    bool OpenFile(char* a_filename, bool writeaccess = false);
    void Deinitialize();
    FileBasedCodec();
    virtual~FileBasedCodec();
    virtual bool GetDataBlock(double** buffer, unsigned int* start, unsigned int* nrelements, int* enable = 0);
    virtual bool WriteDataBlock(double** sbuffer, unsigned int* dstart, unsigned int* dnrelements, int* dactchans = 0);
    bool CreateNewFile(const char* fullpath, int nrchans, const bool rewrite, int codectype);
    bool SetRecordDuration(double aRecordDuration);
    bool SetRecordSamples(unsigned int* a_record_samples);
    bool SetSampleRates(double* a_sample_rates);
    bool SetPhysicalMin(double* a_physical_min);
    bool SetPhysicalMax(double* a_physical_max);
    bool WriteHeader();
    void DeleteData();
    bool AppendSamples(double** buffer, unsigned int nrsamples);
    bool RefreshSource();
};

class DataAqCodec: public FileBasedCodec
{
    IDataAq*     m_device;
    bool         m_recording;
    double*      m_read_data;
    unsigned int m_nr_channels;
    IntVec       m_sampling_rates;
    IntVec       m_gain;
    IntVec       m_physical_mapping;
    int          m_milliSeconds_to_read;
    double**     m_data_chunk;
    bool         m_codecCreated;
    bool initCodec(const char* a_file_name, int aCFileCodectype);

public:
    DataAqCodec(int a_dataAq_device_type, const char* a_dataAq_params, unsigned int a_nr_channels, int* aSamplingRates, const int* a_gain, const int* a_physical_mapping, int aMilliSecondsToRead, int aCFileCodectype, const char* a_file_name);
    void startReading();
    void stopReading();
    bool RefreshDataFromSource();
    virtual ~DataAqCodec();
};

#endif /// FILEBASED_CODEC_H_
