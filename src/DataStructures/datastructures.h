#ifndef datastructures_h
#define datastructures_h

#define SignalCodecType_NOTDEFINED -1
#define SignalCodecType_GVARCODEC   0
#define SignalCodecType_FILECODEC   1

#define MAXSTRLEN  10000 //TODONOW: do something with this!

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct TMarker
{
    enum ESnapMode
    {
        ESnapMode_None = 1,
        ESnapMode_ToGrid = 2,
        ESnapMode_ToOthers = 4,
        ESnapMode_ToLinesOnly = 8
    };
    double m_start_sample; //in [horizontal units]
    double m_length;       //in [horizontal units]
    int    m_marker_id;
    int    m_border_color;
    int    m_fill_color;
    char   m_label[32];
    int    m_movable;
    int    m_sizable_left;
    int    m_sizable_right;
    int    m_left_snap_to;
    int    m_right_snap_to;
    int    m_channel;
    TMarker();
};

struct CMarkerList: public CList<TMarker>
{
    void AddTMarker(double startSample, double length, int markerID = -1, int borderColor = 0, int fillColor = 0x00aaaaee, char* a_label = 0, int a_channel = -1)
    {
        RebuildPreserve(m_size + 1);
        m_data[m_size - 1].m_start_sample = startSample;
        m_data[m_size - 1].m_length       = length;
        m_data[m_size - 1].m_border_color = borderColor;
        m_data[m_size - 1].m_fill_color   = fillColor;
        m_data[m_size - 1].m_marker_id    = markerID;
        m_data[m_size - 1].m_channel      = a_channel;
        if (a_label)
            strcpy(m_data[m_size - 1].m_label, a_label);
        else
            m_data[m_size - 1].m_label[0] = 0;
    }
    void GetTMarkers(CVector <TMarker*> * TMarkersRef, double start, double length)
    {
        if (TMarkersRef)
        {
            TMarkersRef->Rebuild(0);
            for (unsigned int i = 0; i < m_size; i++)
                if (((m_data[i].m_start_sample >= start) && (m_data[i].m_start_sample <= start + length)) || ((m_data[i].m_start_sample <= start) && (m_data[i].m_start_sample + m_data[i].m_length >= start)))
                    TMarkersRef->AddElement(&m_data[i]);
        }
    }
};

struct TString40
{
    char s[40];
};

struct TString84
{
    char s[84];
};

class ISignalCodec
{
public:
    int  m_signalcodec_type;
    char m_varname             [200];     // The ISignalCodec Object's identifier used in scripts
    char m_recording           [256];     // Recording info
    char m_patient             [256];     // patient info
    char m_date                [12];      // Start date of registration
    char m_time                [12];      // Start time of registration
    char m_horizontal_units    [12];      // Horizontal unit (usually Sec.)
    char m_surface2D_vert_units[12];      // In case of a 2D heat map, the vertical units
    double m_surface2D_vert_axis_min;     // In case of a 2D heat map, the ratio between channel indexes and vertical values
    double m_surface2D_vert_axis_max;     // In case of a 2D heat map, the ratio between channel indexes and vertical values
    double m_horizontal_scale_start;

    CVector<unsigned int> m_total_samples;// Total samples per channels
    CVector<double> m_sample_rates;       // Sample rates per channels (Nr of samples / one HorizontalUnit)
    CVector<TString40> m_vertical_units;  // Vertical units (physical dimensions)
    CVector<TString40> m_labels;          // Channel labels
    CVector<TString84> m_transducers;     // Channel transducers
    CMarkerList m_marker_list;            // List of markers

    virtual ~ISignalCodec();
    bool SCopyTo(ISignalCodec* output, int* activechannels = 0);
    ISignalCodec();

    virtual bool GetDataBlock   (double** buffer, unsigned int* start, unsigned int* nrelements, int* enable = 0) = 0;
    virtual bool WriteDataBlock (double** sbuffer, unsigned int* dstart, unsigned int* dnrelements, int* dactchans = 0) = 0; //todonow: implement these with similar behavior
    virtual bool AppendSamples  (double** buffer, unsigned int nrsamples) = 0; // todonow: this might not be needed
    virtual void ResetData();
    virtual bool RefreshDataFromSource();
    virtual bool RefreshSource();
};

class CVariable: public ISignalCodec, public CMatrixVarLen<double>
{
public:
    CVariable();
    virtual void Rebuild(int height, unsigned int* widths);
    virtual void CopyFrom(CVariable* temmx);
    virtual CVariable* Extract(char* varnametostore, double from, double nrhunits);
    virtual void RebuildPreserve(int height, unsigned int* widths);
    virtual bool GetDataBlock(double** buffer, unsigned int* start, unsigned int* nrelements, int* enable = 0);
    virtual bool WriteDataBlock(double** sbuffer, unsigned int* dstart, unsigned int* dnrelements, int* dactchans = 0);
    virtual bool AppendSamples(double** buffer, unsigned int nrsamples);
    virtual bool CopyFrom(ISignalCodec* indata, char* varnametostore, char* charactivechannels = 0);
};

class IntVec: public CVector<int>
{
public:
    IntVec();
    IntVec(unsigned int a_size);
    bool RebuildFrom(char* a_string);
    bool HasEelemnt(int a_element);
    char* ToString(const char* a_delimiter);
    bool AddElement(const char* a_element);
    void AddElement(int a_element);
};

class UIntVec: public CVector<unsigned int>
{
public:
    UIntVec();
    UIntVec(unsigned int a_size);
    bool RebuildFrom(char* a_string);
    bool HasEelemnt(unsigned int a_element);
    char* ToString(const char* a_delimiter);
    bool AddElement(const char* a_element);
    void AddElement(unsigned int a_element);
};

struct TStringMAX
{
    char s[MAXSTRLEN];
};

class CStringVec: public CVector<TStringMAX>
{
public:
    bool RebuildFrom(float* a_elms, int a_nrelms);
    bool RebuildFrom(double* a_elms, int a_nrelms);
    bool RebuildFrom(char* a_string);
    bool HasEelemnt(const char* a_element);
    void AddElement(const char* a_element);
};

class DoubleVec: public CVector<double>
{
public:
    bool RebuildFrom(char* a_string);
    bool HasEelemnt(double a_element);
    char* ToString(const char* a_delimiter);
    bool AddElement(const char* a_element);
    void AddElement(double a_element);
};

class CStringMx: public CVectorRS<CStringVec>
{};

class CDoubleMx: public CVectorRS<DoubleVec>
{};

class CFloatVec: public CVector<float>
{
public:
    bool RebuildFrom(char* a_string);
    bool HasEelemnt(float a_element);
    char* ToString(const char* a_delimiter);
};

typedef char*        (*SPR_PROC)             (int procindx, char* a_statement_body, char* param1, char* param2, char* param3, char* param4, char* param5, char* param6, char* param7, char* param8, char* a_param9, char* a_param10, char* a_param11, char* a_param12);
typedef bool         (*CopyrightInfo_Proc)   (CStringMx* a_copyrightinfo);
typedef CVariable*   (*NewCVariable_Proc)    (void);
typedef char*        (*NewChar_Proc)         (int n);
typedef int          (*Call_Proc)            (const char* a_script);

struct TFunctionParameter
{
    char ParameterName[MAXSTRLEN];
    char ParameterDescription[200];
    char Value[200];
};

class CFunctionDescription
{
public:
    char                        m_function_name[80];
    char                        m_function_description[200];
    vector <TFunctionParameter> m_parameter_list;
    string                      m_statement_body;
    bool ParseDescription(const char* a_descript);
    CFunctionDescription();
    ~CFunctionDescription();
    CFunctionDescription& operator = (const CFunctionDescription& a_vect)
    {
        memcpy(m_function_name, a_vect.m_function_name, sizeof(m_function_name));
        memcpy(m_function_description, a_vect.m_function_description, sizeof(m_function_description));
        m_parameter_list = a_vect.m_parameter_list;
        m_statement_body = a_vect.m_statement_body;
        return *this;
    }
    CFunctionDescription(const CFunctionDescription& a_functionDescription)
    {
        memcpy(m_function_name, a_functionDescription.m_function_name, sizeof(m_function_name));
        memcpy(m_function_description, a_functionDescription.m_function_description, sizeof(m_function_description));
        m_parameter_list = a_functionDescription.m_parameter_list;
        m_statement_body = a_functionDescription.m_statement_body;
    }
private:
    void FillParameterDescriptions(CStringVec* parameters);
};

struct TFunctionLibrary
{
    SPR_PROC                    m_base_function_proc;
    CopyrightInfo_Proc          m_copyright_info_proc;
    char                        m_library_name[200];
    char                        m_library_description[400];
    long long                   m_module_handle;
    vector<CFunctionDescription>  m_function_description_list;
    void ParseFunctionList(CStringVec* a_functionlist);
    TFunctionLibrary()
    {
        m_base_function_proc = NULL;
        m_copyright_info_proc = NULL;
        m_library_name[0] = 0;
        m_library_description[0] = 0;
        m_module_handle = 0;
    }
    TFunctionLibrary& operator = (const TFunctionLibrary& rhs)
    {
        m_base_function_proc = rhs.m_base_function_proc;
        m_copyright_info_proc = rhs.m_copyright_info_proc;
        memcpy(m_library_name, rhs.m_library_name, sizeof(m_library_name));
        memcpy(m_library_description, rhs.m_library_description, sizeof(m_library_description));
        m_module_handle = rhs.m_module_handle;
        m_function_description_list = rhs.m_function_description_list;
        return *this;
    }
    TFunctionLibrary(const TFunctionLibrary& rhs)
    {
        m_base_function_proc = rhs.m_base_function_proc;
        m_copyright_info_proc = rhs.m_copyright_info_proc;
        memcpy(m_library_name, rhs.m_library_name, sizeof(m_library_name));
        memcpy(m_library_description, rhs.m_library_description, sizeof(m_library_description));
        m_module_handle = rhs.m_module_handle;
        m_function_description_list = rhs.m_function_description_list;
    }
};

class CSignalCodec_List: public map<string, ISignalCodec*>
{
public:
    ISignalCodec* datamap_find(const char* a_name)
    {
        if (!a_name)
            return 0;
        CSignalCodec_List::iterator found = map<string, ISignalCodec*>::find(a_name);
        if (found == map<string, ISignalCodec*>::end())
            return 0;
        else
            return found->second;
    }
};

ISignalCodec* CreateMemoryData(const char* a_name, unsigned int a_nr_channels, double* a_samplerates, double a_nr_hor_units);

class CVariable_List: public map<string, CVariable*>
{
public:
    CVariable* variablemap_find(const char* a_name)
    {
        if (!a_name)
            return 0;
        CVariable_List::iterator found = map<string, CVariable*>::find(a_name);
        if (found == map<string, CVariable*>::end())
            return 0;
        else
            return found->second;
    }
    CVariable* Insert(const char* a_name, CVariable* a_var)
    {
        CVariable_List::iterator found = map<string, CVariable*>::find(a_name);
        if (found != map<string, CVariable*>::end())
            delete found->second;
        (*this)[a_name] = a_var;
        return 0;
        /*std::pair<std::map<string, CVariable*>::iterator, bool> ret = insert(std::pair<string, CVariable*>(a_name, a_var));
        if (ret.second==false)
            return ret.first->second;
        else
            return 0;*/
    }
};

char* MakeString(NewChar_Proc newChar, const char* str1 = 0, const char* str2 = 0, const char* str3 = 0, const char* str4 = 0, const char* str5 = 0, const char* str6 = 0, const char* str7 = 0, const char* str8 = 0, const char* str9 = 0, const char* str10 = 0);
CStringVec* SplitString(char* string, const char* key);
void RemoveStringsFromEndAndBegin(CStringVec* sv1, const char* Key);

#define FFTW_ESTIMATE (1U << 6)
#define FFTW_MEASURE  (0U)
#define FFTW_PATIENT  (1U << 5)

typedef int* (*FFT_PROC) (int N, double* in, double* out, int sign, unsigned int flags);
typedef int* (*RFFT_PROC) (int N, double* in, double* out, unsigned int flags);
typedef void (*FFTEXEC_PROC) (int* fftplan);
typedef void (*FFTDESTROY_PROC) (int* fftplan);

/** Provides basic members and common functionality for signal processor libraries */
class CSignalProcessorBase
{
public:
    CVariable_List*    m_variable_list_ref; /** reference for the application's variable list. */
    CSignalCodec_List* m_data_list_ref;     /** reference for the application's data list. */
    NewChar_Proc       m_newchar_proc;      /** function call for application's "new char[n]" constructor. Strings that are going to be deleted by other parts of the system, needs to be allocated with this. */
    NewCVariable_Proc  m_newvariable_proc;  /** function call for application's "new CVariable" constructor. Variables that are going to be deleted by other parts of the system (added to m_variable_list_ref), needs to be allocated with this. */
    static Call_Proc   m_callscript_proc;   /** function implemented by the application. Executes a script in the current running script's context / sandbox. */
    static bool        error_already_displayed;   /** function implemented by the application. Executes a script in the current running script's context / sandbox. */

    /** function pointers loaded by the application and passed to signal processor modules. */
    FFT_PROC        m_fft_proc;
    RFFT_PROC       m_rfft_proc;
    FFTEXEC_PROC    m_fftexec_proc;
    FFTDESTROY_PROC m_fftdestroy_proc;

    /** \brief Executes a script in the currently running script's context. As a result, all the variables allocated by the caller script are available.
     * Can be used like the function "sprintf" to help the caller in formatting the script.
     * \param templ - Template for the script. Supports all the features "sprintf" supporting, like "%s", "%d" etc.
     * \param args - Parameter list. The number of arguments defined by the template.
     */
    template<class... Args>
    static void CallScript(const char* templ, Args&&... args)
    {
        int l_buffsize = strlen(templ) * 2;
        char* l_display_script = new char[l_buffsize + 1];
        int l_would_be_written = snprintf(l_display_script, l_buffsize, templ, std::forward<Args>(args)...);
        if (l_would_be_written > l_buffsize)
        {
            delete[] l_display_script;
            l_display_script = new char[l_would_be_written + 1];
            sprintf(l_display_script, templ, std::forward<Args>(args)...);
        }
        if (m_callscript_proc)
            m_callscript_proc(l_display_script);
        else if (!error_already_displayed)
        {
            std::cout << "ERROR: 'CallScript' function is NULL. Pease extend your plugin's InitLib() function with the following line: 'CSignalProcessorBase::m_callscript_proc = m_callscript_proc;'" << std::endl;
            error_already_displayed = true;
        }
        delete[] l_display_script;
    }

public:
    virtual void initialize(CSignalCodec_List* a_datalist, CVariable_List* a_variablelist, NewCVariable_Proc a_newCVariable, NewChar_Proc a_newChar, Call_Proc a_Call, FFT_PROC a_FFT, RFFT_PROC a_RFFT, FFTEXEC_PROC a_FFTEXEC, FFTDESTROY_PROC a_FFTDESTROY);
};

#endif /// datastructures_h
