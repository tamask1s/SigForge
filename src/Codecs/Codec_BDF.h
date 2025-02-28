/* BDF main header structure */
struct BDFMainHeader
{
    char Version[8];
    char Patient[80];
    char m_recording[80];
    char StartDate[8];
    char StartTime[8];
    char NrBytes[8];
    char m_reserved[44];
    char NrRecords[8];
    char Duration[8];
    char NrChannels[4];
}
BDFMainHeader, *pBDFMainHeader;
/* Offset values used when accessing the header */
#define VERSION		0
#define PATIENT		(VERSION	+ sizeof(BDFMainHeader.Version))
#define RECORDING	(PATIENT	+ sizeof(BDFMainHeader.Patient))
#define STARTDATE	(RECORDING	+ sizeof(BDFMainHeader.m_recording))
#define STARTTIME	(STARTDATE	+ sizeof(BDFMainHeader.StartDate))
#define NRBYTES		(STARTTIME	+ sizeof(BDFMainHeader.StartTime))
#define RESERVED	(NRBYTES	+ sizeof(BDFMainHeader.NrBytes))
#define NRRECORDS	(RESERVED	+ sizeof(BDFMainHeader.m_reserved))
#define DURATION	(NRRECORDS	+ sizeof(BDFMainHeader.NrRecords))
#define NRCHANNELS	(DURATION	+ sizeof(BDFMainHeader.Duration))

/* BDF Signal header structure */
struct BDFChannelHeader
{
    char Label[16];
    char Transducer[80];
    char Dimension[8];
    char PhysicalMin[8];
    char PhysicalMax[8];
    char DigitalMin[8];
    char DigitalMax[8];
    char Prefiltering[80];
    char NrSamples[8];
    char m_reserved[32];
}
BDFChannelHeader, *pBDFChannelHeader;
/* Offset values used when accessing the header */
#define LABEL		 0
#define TRANSDUCER	 (LABEL		   + sizeof(BDFChannelHeader.Label))
#define DIMENSION	 (TRANSDUCER   + sizeof(BDFChannelHeader.Transducer))
#define PHYSICALMIN	 (DIMENSION	   + sizeof(BDFChannelHeader.Dimension))
#define PHYSICALMAX	 (PHYSICALMIN  + sizeof(BDFChannelHeader.PhysicalMin))
#define DIGITALMIN	 (PHYSICALMAX  + sizeof(BDFChannelHeader.PhysicalMax))
#define DIGITALMAX	 (DIGITALMIN   + sizeof(BDFChannelHeader.DigitalMin))
#define PREFILTERING (DIGITALMAX   + sizeof(BDFChannelHeader.DigitalMax))
#define NRSAMPLES	 (PREFILTERING + sizeof(BDFChannelHeader.Prefiltering))
#define CRESERVED	 (NRSAMPLES    + sizeof(BDFChannelHeader.NrSamples))

