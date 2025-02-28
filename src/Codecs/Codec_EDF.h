/* EDF main header structure */
struct EDFMainHeader
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
EDFMainHeader, *pEDFMainHeader;
/* Offset values used when accessing the header */
#define VERSION		0
#define PATIENT		(VERSION	+ sizeof(EDFMainHeader.Version))
#define RECORDING	(PATIENT	+ sizeof(EDFMainHeader.Patient))
#define STARTDATE	(RECORDING	+ sizeof(EDFMainHeader.m_recording))
#define STARTTIME	(STARTDATE	+ sizeof(EDFMainHeader.StartDate))
#define NRBYTES		(STARTTIME	+ sizeof(EDFMainHeader.StartTime))
#define RESERVED	(NRBYTES	+ sizeof(EDFMainHeader.NrBytes))
#define NRRECORDS	(RESERVED	+ sizeof(EDFMainHeader.m_reserved))
#define DURATION	(NRRECORDS	+ sizeof(EDFMainHeader.NrRecords))
#define NRCHANNELS	(DURATION	+ sizeof(EDFMainHeader.Duration))

/* EDF Signal header structure */
struct EDFChannelHeader
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
EDFChannelHeader, *pEDFChannelHeader;
/* Offset values used when accessing the header */
#define LABEL		 0
#define TRANSDUCER	 (LABEL		   + sizeof(EDFChannelHeader.Label))
#define DIMENSION	 (TRANSDUCER   + sizeof(EDFChannelHeader.Transducer))
#define PHYSICALMIN	 (DIMENSION	   + sizeof(EDFChannelHeader.Dimension))
#define PHYSICALMAX	 (PHYSICALMIN  + sizeof(EDFChannelHeader.PhysicalMin))
#define DIGITALMIN	 (PHYSICALMAX  + sizeof(EDFChannelHeader.PhysicalMax))
#define DIGITALMAX	 (DIGITALMIN   + sizeof(EDFChannelHeader.DigitalMin))
#define PREFILTERING (DIGITALMAX   + sizeof(EDFChannelHeader.DigitalMax))
#define NRSAMPLES	 (PREFILTERING + sizeof(EDFChannelHeader.Prefiltering))
#define CRESERVED	 (NRSAMPLES    + sizeof(EDFChannelHeader.NrSamples))

