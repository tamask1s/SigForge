#ifndef CODECLIB_H_
#define CODECLIB_H_

class TDataType
{
    int m_codec;
public:
    /* Methods */
    int  (*DInitData)();
    bool (*DOpenFile)(int, char*, const bool );
    bool (*DCreateNewFile)(int, const char*, int, const bool );
    bool (*DCloseFile)(int);
    bool (*DGetDataRaw)(int, double**, unsigned int*, unsigned int*);
    bool (*DGetChannelRaw)(int, double**, int*, unsigned int*, unsigned int*);
    bool (*DWriteHeader)(int);
    bool (*DWriteBlock)(int, double**, unsigned int*, unsigned int*);
    bool (*DAppendSamples)(int, double**, unsigned int);
    void (*DDeleteData) (int aData);
    int  (*DGetNrChannels)(int);
    bool (*DGetTotalSamples)(int, unsigned int*);
    bool (*DGetSampleRates)(int, double*);
    bool (*DGetLabel)(int, int, char*);
    bool (*DGetTransducer)(int, int, char*);
    bool (*DGetVerticalUnit)(int, int, char*);
    bool (*DGetHorizontalUnit)(int, char*);
    bool (*DGetRecording)(int, char*);
    bool (*DGetPatient)(int, char*);
    bool (*DGetDate)(int, char*);
    bool (*DGetTime)(int, char*);
    bool (*DGetFileName)(int, char*);
    bool (*DSetSampleRates)(int, double*);
    bool (*DSetPhysicalMin)(int, double*);
    bool (*DSetPhysicalMax)(int, double*);
    bool (*DSetRecordSamples)(int, unsigned int*);
    bool (*DSetRecordDuration)(int, double);
    bool (*DSetTotalSamples)(int, unsigned int*);
    bool (*DSetLabel)(int, int aChannelIndx, char* aLabel);
    bool (*DSetTransducer)(int, int aChannelIndx, char* aTransducer);
    bool (*DSetVerticalUnit)(int, int aChannelIndx, char* aVerticalUnit);
    bool (*DSetHorizontalUnit)(int, char* aHorizontalUnit);
    bool (*DSetRecording)(int, char* a_recording);
    bool (*DSetPatient)(int, char* a_patient);
    bool (*DSetDate)(int, char* a_date);
    bool (*DSetTime)(int, char* a_time);

    /* Wrappings */
    int  InitData();
    bool OpenFile(char* fullpath, const bool writeaccess);
    bool CreateNewFile(const char* fullpath, int nrchans, const bool rewrite);
    bool CloseFile();
    bool GetDataRaw(double** buffer, unsigned int* start, unsigned int* stop);
    bool GetChannelRaw(double** buffer, int* enable, unsigned int* start, unsigned int* stop);
    bool WriteHeader();
    bool WriteBlock(double** buffer, unsigned int* start, unsigned int* stop);
    bool AppendSamples(double** buffer, unsigned int nrsamples);
    void DeleteData();
    int  GetNrChannels();
    bool GetTotalSamples(unsigned int* a_total_samples);
    bool GetSampleRates(double* a_sample_rates);
    bool GetLabel(int, char* aLabel);
    bool GetTransducer(int, char* aTransducer);
    bool GetVerticalUnit(int, char* aVerticalUnit);
    bool GetHorizontalUnit(char* aHorizontalUnit);
    bool GetRecording(char* a_recording);
    bool GetPatient(char* a_recording);
    bool GetDate(char* a_date);
    bool GetTime(char* a_time);
    bool GetFileName(char* a_file_name);
    bool SetSampleRates(double* a_sample_rates);
    bool SetPhysicalMin(double* a_physical_min);
    bool SetPhysicalMax(double* a_physical_max);
    bool SetRecordSamples(unsigned int* a_record_samples);
    bool SetRecordDuration(double aRecordDuration);
    bool SetTotalSamples(unsigned int* a_total_samples);
    bool SetLabel(int aChannelIndx, char* aLabel);
    bool SetTransducer(int aChannelIndx, char* aTransducer);
    bool SetVerticalUnit(int aChannelIndx, char* aVerticalUnit);
    bool SetHorizontalUnit(char* aHorizontalUnit);
    bool SetRecording(char* a_recording);
    bool SetPatient(char* a_patient);
    bool SetDate(char* a_date);
    bool SetTime(char* a_time);
    static TDataType* CreateData(int);
};

int LoadCodecTypes();
void FreeCodecTypes();
int QueryCodecCount();
void QueryCodecExtensions(char* a_extensions);
int QueryCodecIndex(int);
int QueryCodecIdByExtension(char* aExtension);

#endif // CODECLIB_H_
