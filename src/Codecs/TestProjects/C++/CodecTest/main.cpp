#include <iostream>
#include <math.h>
#include "../../../../CodecLib.h"

using namespace std;

void CreateNew(const char* filename, unsigned int nrchans, int codectype, bool closeFileAfterCreate)
{
    DDataType* InternalCodec = DDataType::CreateData(codectype);
    if (!InternalCodec)
    {
        cout << "DDataType::CreateData was not successful. The codec DLLs should be located in the 'CodecTest/bin/Codecs' directory." << endl;
        return;
    }
    if (!InternalCodec->CreateNewFile(filename, nrchans, true))
    {
        cout << "CreateNewFile was not successful." << endl;
        return;
    }
    else
    {
        double lRecordDuration = 2.0;
        unsigned int lRecordSamplesPerChannel = 2048;
        InternalCodec->SetRecordDuration(lRecordDuration);

        double InternalCodecSampleRates[nrchans];
        double InternalCodecPhysicalMin[nrchans];
        double InternalCodecPhysicalMax[nrchans];
        unsigned int InternalCodecRecordSamples[nrchans];

        for (unsigned int i = 0; i < nrchans; i++)
        {
            InternalCodecSampleRates[i] = lRecordSamplesPerChannel / lRecordDuration;
            InternalCodecPhysicalMin[i] = -100;
            InternalCodecPhysicalMax[i] = 100;
            InternalCodecRecordSamples[i] = (int)(InternalCodecSampleRates[i] * lRecordDuration);
        }

        InternalCodec->SetSampleRates(InternalCodecSampleRates);
        InternalCodec->SetPhysicalMin(InternalCodecPhysicalMin);
        InternalCodec->SetPhysicalMax(InternalCodecPhysicalMax);
        InternalCodec->SetRecordSamples(InternalCodecRecordSamples);

        if (!InternalCodec->WriteHeader())
        {}

        unsigned int nrrecs = 39;
        unsigned int hossz = lRecordSamplesPerChannel;
        double** tdat = new double* [nrchans];
        for (unsigned int i = 0; i < nrchans; i++)
            tdat[i] = new double [hossz];

        double it = 0.0;
        for (unsigned int recindx = 0; recindx < nrrecs; ++recindx)
        {
            for (unsigned int j = 0; j < nrchans; ++j)
            {
                for (unsigned int i = 0; i < lRecordSamplesPerChannel; ++i)
                {
                    it += 0.01;
                    //tdat[j][i] = sin ((double) i / 1024.0 * 2.0 * 3.14 * (double)(recindx / 1.0 + 1)) * 100.0;
                    tdat[j][i] = sin (it + (it * it) / 100.0) * 100.0;
                }

            }
            InternalCodec->AppendSamples(tdat, hossz);
        }
        for (unsigned int i = 0; i < nrchans; ++i)
            delete [] tdat[i];
        delete [] tdat;
        InternalCodec->WriteHeader();
        if (closeFileAfterCreate)
            InternalCodec->CloseFile();
    }
}

int main()
{
    LoadCodecTypes();
    CreateNew("FileName.bdf", 1, 4, true);
    cout << "File created." << endl;
    return 0;
}
