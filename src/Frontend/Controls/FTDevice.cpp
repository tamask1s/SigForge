#include <stdio.h>
#include <windows.h>
#include <string>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "bitmap.h"
#include "Defines.h"
#include "Control_Base.h"
#include "ftdevice.h"

#define FT_LIST_NUMBER_ONLY         0x80000000
#define FT_EVENT_RXCHAR		        1
#define FTMESSAGE_RECIEVED          WM_USER+10

typedef unsigned int FT_HANDLE;

enum FT_STATUS
{
    FT_OK,
    FT_INVALID_HANDLE,
    FT_DEVICE_NOT_FOUND,
    FT_DEVICE_NOT_OPENED,
    FT_IO_ERROR,
    FT_INSUFFICIENT_RESOURCES,
};

typedef FT_STATUS (*FT_ListDevices_PROCTYPE)         (void* pArg1, void* pArg2, unsigned int Flags);
typedef FT_STATUS (*FT_Open_PROCTYPE)                (int deviceNumber, FT_HANDLE *pHandle);
typedef FT_STATUS (*FT_Close_PROCTYPE)               (FT_HANDLE ftHandle);
typedef FT_STATUS (*FT_Read_PROCTYPE)                (FT_HANDLE ftHandle, void* lpBuffer, unsigned int nBufferSize, unsigned int*  lpBytesReturned);
typedef FT_STATUS (*FT_Write_PROCTYPE)               (FT_HANDLE ftHandle, void* lpBuffer, unsigned int nBufferSize, unsigned int*  lpBytesWritten);
typedef FT_STATUS (*FT_SetEventNotification_PROCTYPE)(FT_HANDLE ftHandle, unsigned int Mask, void* Param );
//typedef FT_STATUS (*FT_GetStatus_PROCTYPE)           (FT_HANDLE ftHandle, unsigned int *dwRxBytes, unsigned int *dwTxBytes, unsigned int *dwEventDWord);
typedef FT_STATUS (*FT_GetStatus_PROCTYPE)           (FT_HANDLE ftHandle, unsigned int *dwRxBytes);
typedef FT_STATUS (*FT_SetLatencyTimer_PROCTYPE)     (FT_HANDLE ftHandle, unsigned char ucLatency);
typedef FT_STATUS (*FT_SetUSBParameters_PROCTYPE)    (FT_HANDLE ftHandle, unsigned int ulInTransferSize, unsigned int ulOutTransferSize);

long WINAPI  EventWaiterThread  (long lParam);
HANDLE       hEventWaiterThread = 0;
HANDLE       hEvent             = 0;
unsigned int ThreadID           = 0;
unsigned int EventMask          = FT_EVENT_RXCHAR;

FT_ListDevices_PROCTYPE           FT_ListDevices_PROC          = 0;
FT_Open_PROCTYPE                  FT_Open_PROC                 = 0;
FT_Read_PROCTYPE                  FT_Read_PROC                 = 0;
FT_Write_PROCTYPE                 FT_Write_PROC                = 0;
FT_GetStatus_PROCTYPE             FT_GetStatus_PROC            = 0;
FT_Close_PROCTYPE                 FT_Close_PROC                = 0;
FT_SetEventNotification_PROCTYPE  FT_SetEventNotification_PROC = 0;
FT_SetLatencyTimer_PROCTYPE       FT_SetLatencyTimer_PROC      = 0;
FT_SetUSBParameters_PROCTYPE      FT_SetUSBParameters_PROC     = 0;

CControl_Base* MessengerWindow = 0;
CList <FTDevice*> Devices;

bool LoadFTProcs()
{
    HMODULE FTDll = LoadLibrary("FTD2XX.dll");
    if (!FTDll)
        return 0;
    FT_ListDevices_PROC          = (FT_ListDevices_PROCTYPE)          GetProcAddress(FTDll, "FT_ListDevices");
    FT_Open_PROC                 = (FT_Open_PROCTYPE)                 GetProcAddress(FTDll, TEXT("FT_Open"));
    FT_Read_PROC                 = (FT_Read_PROCTYPE)                 GetProcAddress(FTDll, TEXT("FT_Read"));
    FT_Write_PROC                = (FT_Write_PROCTYPE)                GetProcAddress(FTDll, TEXT("FT_Write"));
    FT_GetStatus_PROC            = (FT_GetStatus_PROCTYPE)            GetProcAddress(FTDll, TEXT("FT_GetStatus"));
    FT_GetStatus_PROC            = (FT_GetStatus_PROCTYPE)            GetProcAddress(FTDll, TEXT("FT_GetQueueStatus"));

    FT_Close_PROC                = (FT_Close_PROCTYPE)                GetProcAddress(FTDll, TEXT("FT_Close"));
    FT_SetEventNotification_PROC = (FT_SetEventNotification_PROCTYPE) GetProcAddress(FTDll, TEXT("FT_SetEventNotification"));
    FT_SetLatencyTimer_PROC      = (FT_SetLatencyTimer_PROCTYPE)      GetProcAddress(FTDll, TEXT("FT_SetLatencyTimer"));
    FT_SetUSBParameters_PROC     = (FT_SetUSBParameters_PROCTYPE)     GetProcAddress(FTDll, TEXT("FT_SetUSBParameters"));

    return true;
}

long WINAPI EventWaiterThread(long lParam)
{
    while(true)
    {
        WaitForSingleObject(hEvent, INFINITE);
        SendMessage(MessengerWindow->hWnd, FTMESSAGE_RECIEVED, 0, 0);
    }
}

int  FTDevice::FTMessenger_On_Message (CControl_Base* a_sender, int message, int wParam, int lParam)
{
    if (message == FTMESSAGE_RECIEVED)
        for (unsigned int i = 0; i < Devices.m_size; i++)
            if (Devices.m_data[i]->EventsEnabled)
                Devices.m_data[i]->Obj_OnRecieve (0, wParam, lParam);
    return 0;
}

void FTDevice::EnableEvents ()
{
    EventsEnabled = true;
    MessengerWindow->OnMessage += &FTDevice::FTMessenger_On_Message;

    if (!hEventWaiterThread)
        hEventWaiterThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EventWaiterThread, NULL, 0, (LPDWORD)&ThreadID);
    if (!hEvent)
        hEvent  = CreateEvent (NULL, false, false, "");

    FT_SetEventNotification_PROC(devhandle, EventMask, hEvent);
}

FTDevice::FTDevice(EObject* parentobj)
{
    if (!MessengerWindow)
    {
        MessengerWindow = new CControl_Base(0, 0, "FT Messenger Window", 0, 0, 0, 220, 100, 0, true, 0, 0);
        MessengerWindow->Hide();
    }
    devhandle = 0;
    EventsEnabled = false;
    ParentObj = parentobj;
    Devices += this;
}

bool FTDevice::Open()
{
    if (!FT_ListDevices_PROC)
        if (!LoadFTProcs())
            return false;

    unsigned int rxtxbuffersize = 526 + 18;
    unsigned int numDevs;

    FT_STATUS status = FT_ListDevices_PROC(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
    if ((numDevs == 0) | (status != FT_OK))
        return 0;

    FT_Open_PROC(0, &devhandle);
    FT_SetLatencyTimer_PROC(devhandle, 1);
    FT_SetUSBParameters_PROC(devhandle, rxtxbuffersize, rxtxbuffersize);
    return devhandle;
}

FTDevice::~FTDevice()
{
    Devices -= this;
    MessengerWindow->OnMessage -= &FTDevice::FTMessenger_On_Message;
    Close();
}

void FTDevice::Close()
{
    if (devhandle == 0)
        return;
    FT_Close_PROC(devhandle);
}

int FTDevice::GetRxStatus()
{
    if (devhandle == 0)
        return -1;
    unsigned int rx;
    FT_GetStatus_PROC(devhandle, &rx);
    return rx;
}

int FTDevice::GetTxStatus()
{
    if (devhandle == 0)
        return -1;
    return 0;
}

void FTDevice::Obj_OnRecieve(byte* buffer, int wParam, int lParam)
{
    for (unsigned int i = 0; i < OnRecieve.m_size; i++)
        (ParentObj->*OnRecieve.m_data[i])(this, buffer, wParam, lParam);
}

int FTDevice::WriteBuffer(byte* buffer, unsigned int buffersize)
{
    unsigned int lpdwBytesWritten = 0;
    if (devhandle == 0)
        return -1;
    FT_Write_PROC(devhandle, buffer, buffersize, &lpdwBytesWritten);
    return lpdwBytesWritten;
}

int FTDevice::ReadBuffer(byte* buffer, unsigned int buffersize)
{
    unsigned int lpdwBytesRead = 0;
    if (devhandle == 0)
        return -1;
    FT_Read_PROC(devhandle, buffer, buffersize, &lpdwBytesRead);
    return 0;
}
