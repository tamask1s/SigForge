#ifndef FTDevice_H_
#define FTDevice_H_

class FTDevice: public EObject
{
    typedef unsigned int FT_HANDLE;
    FT_HANDLE    devhandle;
    bool         EventsEnabled;
    int          FTMessenger_On_Message (CControl_Base* a_sender, int message, int wParam, int lParam);
public:
    EObject*     ParentObj;
    typedef void (EObject::*On_Recieve) (FTDevice* sender, unsigned char* buffer, int wParam, int lParam);
    CList   <On_Recieve>  OnRecieve;

    FTDevice                   (EObject* parentobj);
    virtual ~FTDevice          ();

    virtual void Obj_OnRecieve (unsigned char* buffer, int wParam, int lParam);

    bool         Open          ();
    void         Close         ();
    int          WriteBuffer   (unsigned char* buffer, unsigned int buffersize);
    int          ReadBuffer    (unsigned char* buffer, unsigned int buffersize);
    int          GetRxStatus   ();
    int          GetTxStatus   ();
    void         EnableEvents  ();
};

#endif // FTDevice_H_
