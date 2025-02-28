#ifndef scripteditor_h_
#define scripteditor_h_

typedef void (EObject::*On_RunScript) (CControl_Base* a_sender, const char* a_script);

class CScriptEditor: public CControl_Base
{
    HFONT                 m_font;
    const char*           m_function_library_list_ref;
    CControl_Base*        m_script_edit;
    CBitButton*           m_button_run;
    CBitButton*           m_button_load_script;
    CBitButton*           m_button_save_script;

    bool LoadScript(const char* a_file);
    void Obj_OnRunScript(const char* a_script);
    int  Obj_OnMessage(HWND a_hwnd, int a_message, int a_wparam, int a_lparam);
    void Obj_OnResize(int x, int y, int a_width, int a_height);

public:
    CList<On_RunScript> m_on_run_script;

    CScriptEditor(const char* a_function_library_list, const char* a_file, int a_posx, int a_posy, int a_width, int a_height, CControl_Base* a_parent, bool a_floating = true);
    virtual~CScriptEditor();
    void m_button_load_script_OnClick(CControl_Base* CBitButton, int a_button, int x, int y);
    void ButtonSaveScript_OnClick(CControl_Base* CBitButton, int a_button, int x, int y);
    void ButtonRunOnClick(CControl_Base* CBitButton, int a_button, int x, int y);
};

#endif // scripteditor_h_
