#include <iostream>
#include <string.h>
#include <vector>
#include <map>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "datastructures.h"
#include "CodecLib.h"
#include "math.h"
#include "DataAq_Interface.h"
#include "FileBased_Codec.h"

void FileBasedCodec::CopyFromInternalCodec()
{
    unsigned int lNrChannels = m_internal_codec->GetNrChannels();
    m_total_samples.Rebuild(lNrChannels);
    m_sample_rates.Rebuild(lNrChannels);
    m_labels.Rebuild(lNrChannels);
    m_transducers.Rebuild(lNrChannels);
    m_vertical_units.Rebuild(lNrChannels);
    m_internal_codec->GetTotalSamples(m_total_samples.m_data);
    m_internal_codec->GetSampleRates(m_sample_rates.m_data);
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
    {
        m_internal_codec->GetLabel(i, m_labels.m_data[i].s);
        m_internal_codec->GetTransducer(i, m_transducers.m_data[i].s);
        m_internal_codec->GetVerticalUnit(i, m_vertical_units.m_data[i].s);
    }
    m_internal_codec->GetRecording(m_recording);
    m_internal_codec->GetPatient(m_patient);
    m_internal_codec->GetDate(m_date);
    m_internal_codec->GetTime(m_time);
    m_internal_codec->GetHorizontalUnit(m_horizontal_units);

    if (!strcmp(m_horizontal_units, "N/A"))
        strcpy(m_horizontal_units, "s");
}

bool FileBasedCodec::RefreshSource()
{
    m_internal_codec->SetSampleRates(m_sample_rates.m_data);
    for (unsigned int i = 0; i < m_total_samples.m_size; i++)
    {
        m_internal_codec->SetLabel(i, m_labels.m_data[i].s);
        m_internal_codec->SetTransducer(i, m_transducers.m_data[i].s);
        m_internal_codec->SetVerticalUnit(i, m_vertical_units.m_data[i].s);
    }
    m_internal_codec->SetRecording(m_recording);
    m_internal_codec->SetPatient(m_patient);
    m_internal_codec->SetDate(m_date);
    m_internal_codec->SetTime(m_time);
    m_internal_codec->SetHorizontalUnit(m_horizontal_units);
    m_internal_codec->WriteHeader();
    return true;
}

bool FileBasedCodec::OpenFile(char* a_filename, bool writeaccess)
{
    Deinitialize();
    bool result = false;
    char lExtension[32];
    strcpy(lExtension, a_filename + strlen(a_filename) - 4);
    for (unsigned int i = 0; lExtension[i]; i++)
        lExtension[i] = tolower(lExtension[i]);
    int lCodecID = QueryCodecIdByExtension(lExtension);
    if ((bool)(m_internal_codec = TDataType::CreateData(lCodecID)) && m_internal_codec->OpenFile (a_filename, writeaccess))
    {
        CopyFromInternalCodec();
        result = true;
    }
    else
        Deinitialize();
    return result;
}

void FileBasedCodec::Deinitialize()
{
    if (m_internal_codec)
    {
        m_internal_codec->CloseFile();
        m_internal_codec->DeleteData();
        delete m_internal_codec;
        m_internal_codec = 0;
    }
}

FileBasedCodec::FileBasedCodec()
    : ISignalCodec()
{
    m_internal_codec = 0;
    m_signalcodec_type = SignalCodecType_FILECODEC;
}

FileBasedCodec::~FileBasedCodec()
{
    Deinitialize();
}

bool FileBasedCodec::GetDataBlock(double** buffer, unsigned int* start, unsigned int* nrelements, int* enable)
{
    bool result = false;
    if (m_internal_codec && buffer)
    {
        CVector<unsigned int>temstops;
        temstops.Rebuild(m_total_samples.m_size);
        for (unsigned int i = 0; i < m_total_samples.m_size; i++)
            temstops.m_data[i] = start[i] + nrelements[i];
        if (enable)
            result = m_internal_codec->GetChannelRaw(buffer, enable, start, temstops.m_data);
        else
            result = m_internal_codec->GetDataRaw(buffer, start, temstops.m_data);
    }
    return result;
}

bool FileBasedCodec::WriteDataBlock(double** sbuffer, unsigned int* dstart, unsigned int* dnrelements, int* dactchans) //todonow: implement this, investigate (sin() is used below)
{
//        CVector<unsigned int>temstops;
//        temstops.Rebuild(m_total_samples.m_size);
//        for (unsigned int i=0;i<m_total_samples.m_size;i++)
//            temstops.m_data[i]=dstart[i]+dnrelements[i];
//
//        float* bufftowrite [m_total_samples.m_size];
//        int j=0;
//        for (unsigned int i=0;i<m_total_samples.m_size;i++)
//        {
//            if (dactchans)
//            {
//                //Debug.Print("yy1");
//                if (dactchans[i]==1)
//                {
//                    //Debug.Print("yy2");
//                    bufftowrite[i]=sbuffer[j];
//                    j++;
//                }
//                else
//                {
//                    bufftowrite[i]=new float[dnrelements[i]];
//                    //bufftowrite[i]=0;
//                    //temstops.m_data[i]=dstart[i];
//                }
//            }
//            else
//                bufftowrite[i]=sbuffer[i];
//        }
//        //m_internal_codec->WriteHeader();
//        m_internal_codec->WriteBlock(bufftowrite,dstart,temstops.m_data);

    unsigned int lNrChannels = m_internal_codec->GetNrChannels();
    m_internal_codec->WriteHeader();
    int nrrecs = 5;
    int hossz = 256; //!!!
    double** tdat = new double* [lNrChannels];
    for (unsigned int i = 0; i < lNrChannels; i++)
        tdat[i] = new double [hossz];

    unsigned int start [lNrChannels];
    unsigned int stop  [lNrChannels];

    for (int recindx = 0; recindx < nrrecs - 1; recindx++)
    {
        for (unsigned int j = 0; j < lNrChannels; j++)
        {
            for (unsigned int i = 0; i < 256; i++)
                tdat[j][i] = sin((double)i / 1024.0 * 2.0 * 3.14 * (double)(recindx + 1)) * 100.0;
            start[j] = recindx * hossz;
            stop[j] = hossz + recindx * hossz;
        }
        m_internal_codec->WriteBlock(tdat, start, stop);
    }
    for (unsigned int i = 0; i < lNrChannels; i++)
        delete[] tdat[i];
    delete[] tdat;
    CopyFromInternalCodec();
    return 0;
}

bool FileBasedCodec::CreateNewFile(const char* fullpath, int nrchans, const bool rewrite, int codectype)
{
    m_internal_codec = TDataType::CreateData(codectype);
    if (!m_internal_codec)
        std::cout << "m_internal_codec not created succesfully, dll exports are probably faulty." << std::endl;
    if (!m_internal_codec || !m_internal_codec->CreateNewFile(fullpath, nrchans, rewrite))
        return false;
    else
        CopyFromInternalCodec();
    return true;
}

bool FileBasedCodec::SetRecordDuration(double aRecordDuration)
{
    bool res = m_internal_codec->SetRecordDuration(aRecordDuration);
    CopyFromInternalCodec();
    return res;
}

bool FileBasedCodec::SetRecordSamples(unsigned int* a_record_samples)
{
    bool res = m_internal_codec->SetRecordSamples(a_record_samples);
    CopyFromInternalCodec();
    return res;
}

bool FileBasedCodec::SetSampleRates(double* a_sample_rates)
{
    bool res = m_internal_codec->SetSampleRates(a_sample_rates);
    CopyFromInternalCodec();
    return res;
}

bool FileBasedCodec::SetPhysicalMin(double* a_physical_min)
{
    bool res = m_internal_codec->SetPhysicalMin(a_physical_min);
    CopyFromInternalCodec();
    return res;
}

bool FileBasedCodec::SetPhysicalMax(double* a_physical_max)
{
    bool res = m_internal_codec->SetPhysicalMax(a_physical_max);
    CopyFromInternalCodec();
    return res;
}

bool FileBasedCodec::WriteHeader()
{
    bool res = m_internal_codec->WriteHeader();
    CopyFromInternalCodec();
    return res;
}

void FileBasedCodec::DeleteData()
{
    return m_internal_codec->DeleteData();
}

bool FileBasedCodec::AppendSamples(double** buffer, unsigned int nrsamples)
{
    bool res = m_internal_codec->AppendSamples(buffer, nrsamples);
    CopyFromInternalCodec();
    return res;
}

bool DataAqCodec::initCodec(const char* a_file_name, int aCFileCodectype)
{
    if (!(m_codecCreated = CreateNewFile(a_file_name, m_nr_channels, true, aCFileCodectype))) ///{ <------- API call ------- }
    {
        cout << "DataAqCodec: Can't create file\n";
        return false;
    }
    SetRecordDuration(m_milliSeconds_to_read / 1000.0);  ///{ <------- API call ------- }
    double DoubleBuf[m_nr_channels];
    unsigned int NrRecordSamples[m_nr_channels];
    ///{ Setup file writing }
    for (unsigned int i = 0; i < m_nr_channels; ++i)
    {
        NrRecordSamples[i] = (m_sampling_rates.m_data[i] * m_milliSeconds_to_read) / 1000;
        if (!NrRecordSamples[i])
            NrRecordSamples[i] = 1;
    }

    SetRecordSamples(NrRecordSamples);  ///{ <------- API call ------- }
    for (unsigned int i = 0; i < m_nr_channels; ++i)
        DoubleBuf[i] = m_sampling_rates.m_data[i];
    SetSampleRates(DoubleBuf);  ///{ <------- API call ------- }
    for (unsigned int i = 0; i < m_nr_channels; ++i)
        DoubleBuf[i] = -5000;//todonow
    SetPhysicalMin(DoubleBuf);  ///{ <------- API call ------- }
    for (unsigned int i = 0; i < m_nr_channels; ++i)
        DoubleBuf[i] = 5000;
    SetPhysicalMax(DoubleBuf);  ///{ <------- API call ------- }
    WriteHeader();   ///{ <------- API call ------- }
    return true;
}

DataAqCodec::DataAqCodec(int a_dataAq_device_type, const char* a_dataAq_params, unsigned int a_nr_channels, int* aSamplingRates, const int* a_gain, const int* a_physical_mapping, int aMilliSecondsToRead, int aCFileCodectype, const char* a_file_name)
    : FileBasedCodec(),
      m_recording(false),
      m_nr_channels(a_nr_channels),
      m_milliSeconds_to_read(aMilliSecondsToRead)
{
    m_sampling_rates.Rebuild(a_nr_channels);
    m_gain.Rebuild(a_nr_channels);
    m_physical_mapping.Rebuild(a_nr_channels);
    for (unsigned int i = 0; i < a_nr_channels; ++i)
    {
        m_sampling_rates.m_data[i] = aSamplingRates[i];
        m_gain.m_data[i] = a_gain[i];
        m_physical_mapping.m_data[i] = a_physical_mapping[i];
    }

    m_device = IDataAq::New(a_dataAq_device_type);
    if (!m_device->connectDevice(a_dataAq_params))
    {
        IDataAq::Delete(m_device);
        m_device = 0;
    }
    else
    {
        m_data_chunk = new double* [m_nr_channels];
        unsigned int lTotalSampleNumber = 0;
        for (unsigned int i = 0; i < m_nr_channels; ++i)
        {
            unsigned int lNrSamplesPerActualChannel = (m_sampling_rates.m_data[i] * m_milliSeconds_to_read) / 1000;
            lTotalSampleNumber += lNrSamplesPerActualChannel;
            m_data_chunk[i] = new double[lNrSamplesPerActualChannel];
        }

        m_read_data = new double[lTotalSampleNumber];

        initCodec(a_file_name, aCFileCodectype);
    }
}

void DataAqCodec::startReading()
{
    if (!m_recording && m_device)
    {
        int samplingRates[m_nr_channels];

        for (unsigned int i = 0; i < m_nr_channels; ++i)
            samplingRates[i] = m_sampling_rates.m_data[i];

        m_device->setupReading(m_nr_channels, samplingRates, m_gain.m_data, m_physical_mapping.m_data, m_milliSeconds_to_read);       ///{ <------- API call ------- }
        m_device->startReading();       ///{ <------- API call ------- }
        m_recording = true;
    }
}

void DataAqCodec::stopReading()
{
    if (m_recording && m_device)
    {
        m_recording = false;
        m_device->stopReading();
        WriteHeader();  /// { <------- API call ------- }
    }
}

bool DataAqCodec::RefreshDataFromSource()
{
    int res = false;
    if (m_recording && m_device)
    {
        if (m_device->analogRead(m_read_data) == 0)  /// { <------- API call ------- }
        {
            res =  true;
            unsigned int lOffset = 0;
            for (unsigned int i = 0; i < m_nr_channels; ++i)
            {
                unsigned int lNrSamplesPerActualChannel = (m_sampling_rates.m_data[i] * m_milliSeconds_to_read) / 1000;
                for (unsigned int j = 0; j < lNrSamplesPerActualChannel; ++j)
                    m_data_chunk[i][j] = m_read_data[lOffset++];
            }
            if (m_codecCreated)
            {
                AppendSamples(m_data_chunk, (m_sampling_rates.m_data[0] * m_milliSeconds_to_read) / 1000);  /// { <------- API call ------- }
                WriteHeader();   ///{ <------- API call ------- }
            }
        }
    }
    return res;
}

DataAqCodec::~DataAqCodec()
{
    if (m_device)
    {
        m_device->stopReading();
        m_device->disconnectDevice();
        IDataAq::Delete(m_device);
        for (unsigned int i = 0; i < m_nr_channels; ++i)
            delete[] m_data_chunk[i];
        delete[] m_data_chunk;
        delete[] m_read_data;
    }
}
