#include <string>
#include <vector>
#include <map>
#include <shlobj.h>
#include <commctrl.h>
#include <richedit.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <iostream>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "bitmap.h"
#include "Defines.h"
#include "Control_Base.h"
#include "CommonVisualComponents.h"
#include "datastructures.h"
#include "Backend.h"
#include "scripteditor.h"
#include "ShellObjects.h"
#include "ZaxJsonParser.h"

void HiglightText(HWND editctrl, const char* a_function_library_list)
{
    int endpos;
    SendMessage(editctrl, EM_GETSEL, 0, (int)&endpos);
    int txlen = GetWindowTextLength (editctrl);
    if (txlen)
    {
        char Script [txlen + 1];
        CHARFORMAT cf;
        ZeroMemory(&cf, sizeof(cf));
        COLORREF clrTxt[] = {RGB(250, 0, 0), RGB(0, 0, 250), RGB(0, 220, 0), RGB(150, 50, 50), RGB(50, 150, 50), RGB(150, 50, 150), RGB(230, 100, 0), RGB(50, 160, 255)};

        cf.cbSize = sizeof(cf);
        lstrcpy(cf.szFaceName, "MSSans");
        cf.bCharSet = ANSI_CHARSET;
        cf.bPitchAndFamily = FF_MODERN;
        //cf.dwEffects |= CFE_BOLD;
        cf.dwMask = CFM_COLOR | CFM_SIZE | CFM_FACE | CFM_EFFECTS;
        cf.crTextColor = clrTxt[2];
        cf.yHeight = 200;

        GetWindowText (editctrl, Script, txlen + 1);
        ReplaceStringWithString (Script, "\r", "");
        ReplaceCharactersWith(Script);

        if (a_function_library_list)
        {
            ZaxJsonTopTokenizer l_info(a_function_library_list, false);
            if (l_info.m_values["libraries"])
            {
                ZaxJsonTopTokenizer l_libs(l_info.m_values["libraries"], false);
                for (unsigned int s = 0; s < l_libs.m_list_values.size(); s++)
                {
                    ZaxJsonTopTokenizer l_lib(l_libs.m_list_values[s], false);
                    int ss = s;
                    while (ss > 7)
                        ss -= 8;
                    cf.crTextColor = clrTxt[ss];
                    ZaxJsonTopTokenizer l_functions(l_lib.m_values["functions"], false);
                    for (unsigned int i = 0; i < l_functions.m_list_values.size(); i++)
                    {
                        const char* funcname = l_functions.m_list_values[i];
                        char* pch = strstr (Script, funcname);
                        while (pch)
                        {
                            if ((*(pch + strlen(funcname)) != 32) && (*(pch + strlen(funcname)) != 10)) //" "
                                goto contin1;
                            if (pch != Script)
                            {
                                if ((*(pch - 1) != 32) && (*(pch - 1) != 59) && (*(pch - 1) != 10))
                                {
                                    goto contin1;
                                }
                            }
                            SendMessage(editctrl, EM_SETSEL, pch - Script, pch - Script + strlen(funcname));
                            SendMessage(editctrl, EM_SETCHARFORMAT, SCF_WORD | SCF_SELECTION, (LPARAM)&cf);
    contin1:
                            pch = strstr(pch + strlen(funcname), funcname);
                        }
                    }
                }
            }
        }
    }
    SendMessage(editctrl, EM_SETSEL, endpos, endpos);
    SetFocus(editctrl);
    SendMessage(editctrl, EM_EMPTYUNDOBUFFER, 0, 0);
}

CScriptEditor::CScriptEditor(const char* a_function_library_list, const char* a_file, int a_posx, int a_posy, int a_width, int a_height, CControl_Base* a_parent, bool a_floating)
    :CControl_Base(0, 0, "Script editor", 0, a_posx, a_posy, a_width, a_height, a_parent, a_floating),
     m_function_library_list_ref(0)
{
    if (a_floating)
        MoveButton = MK_LBUTTON;

    m_script_edit = new CControl_Base(0x00000200, "RichEdit20A", 0, 0x50A10004, 0, 0, a_width - 10, 250, this, false);
    m_font = CreateFont(16, 10, 0, 0, 400, 0, 0, 0, EASTEUROPE_CHARSET, 255, 255, PROOF_QUALITY, FF_MODERN, 0);
    SendMessage(m_script_edit->hWnd, WM_SETFONT, (int)m_font, 0);

    m_button_run = new CBitButton (0, 0, 70, 20, "RUN", this);
    m_button_load_script = new CBitButton (0, 0, 70, 20, "LOAD", this);
    m_button_save_script = new CBitButton (0, 0, 70, 20, "SAVE", this);
    m_button_run->OnClick += &CScriptEditor::ButtonRunOnClick;
    m_button_load_script->OnClick += &CScriptEditor::m_button_load_script_OnClick;
    m_button_save_script->OnClick += &CScriptEditor::ButtonSaveScript_OnClick;
    RECT Rect;
    GetClientRect(hWnd, &Rect);
    Obj_OnResize(0, 0, Rect.right, Rect.bottom);

    if (a_function_library_list)
        m_function_library_list_ref = a_function_library_list;

    LoadScript(a_file);
}

CScriptEditor::~CScriptEditor()
{
    if (m_font)
        DeleteObject(m_font);
}

void CScriptEditor::Obj_OnRunScript (const char* a_script)
{
    for (unsigned int i = 0; i < m_on_run_script.m_size; i++)
    {
        (ParentObj->*m_on_run_script.m_data[i])(this, a_script);
    }
}

bool CScriptEditor::LoadScript(const char* a_file)
{
    int size;
    if (CheckFile(a_file ? a_file : "../Scripts/003_DataAq.txt"))
        if (byte* buff = LoadBuffer(a_file ? a_file : "../Scripts/003_DataAq.txt", 0, &size))
        {
            char Script [size + 1] ;
            memcpy(Script, buff, size);
            Script[size] = 0;
            SetWindowText(m_script_edit->hWnd, (char*)Script);
            delete[] buff;
            SetFocus(hWnd);
            HiglightText(m_script_edit->hWnd, m_function_library_list_ref);
            InvalidateRect(m_script_edit->hWnd, 0, true);
        }
    return true;
}

void CScriptEditor::m_button_load_script_OnClick(CControl_Base* CBitButton, int button, int x, int y)
{
    char FileName[MAX_PATH];
    if (GetFileNameT(hWnd, FileName, FALSE, 2, "All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0\0"))
    {
        int size;
        byte* buff = LoadBuffer (FileName, 0, &size);
        if (buff)
        {
            char Script [size + 1] ;
            memcpy(Script, buff, size);
            Script[size] = 0;
            SetWindowText(m_script_edit->hWnd, (char*)Script);
            delete[] buff;
        }
    }
    SetFocus(hWnd);
    HiglightText(m_script_edit->hWnd, m_function_library_list_ref);
}

void CScriptEditor::ButtonSaveScript_OnClick(CControl_Base* CBitButton, int button, int x, int y)
{
    int txlen = GetWindowTextLength(m_script_edit->hWnd);
    char FileName[MAX_PATH];
    if (GetFileNameT(hWnd, FileName, true, 2, "All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0\0") && txlen)
    {
        char Script [txlen + 1];
        GetWindowText(m_script_edit->hWnd, Script, txlen + 1);
        SaveBuffer (FileName, (byte*)Script, txlen, 0, true);
    }
    SetFocus(hWnd);
    HiglightText(m_script_edit->hWnd, m_function_library_list_ref);
}

void CScriptEditor::ButtonRunOnClick(CControl_Base* CBitButton, int button, int x, int y)
{
    int txlen = GetWindowTextLength(m_script_edit->hWnd);
    char Script [txlen + 1];
    GetWindowText(m_script_edit->hWnd, Script, txlen + 1);
    Obj_OnRunScript (Script);
    BringWindowToTop(hWnd);
    SetActiveWindow(hWnd);
    SendMessage (hWnd, WM_LBUTTONUP, 0, 0);
    SetFocus(hWnd);
    HiglightText(m_script_edit->hWnd, m_function_library_list_ref);
}

void CScriptEditor::Obj_OnResize(int x, int y, int width, int height)
{
    CControl_Base::Obj_OnResize(x, y, width, height);
    m_script_edit->Move(0, 0, width - 1, height - 40);
    m_button_run->Move (width - 80, height - 30, NOCHANGE, NOCHANGE, true);
    m_button_load_script->Move (width - 80 - 72, height - 30, NOCHANGE, NOCHANGE, true);
    m_button_save_script->Move (width - 80 - 72 - 72, height - 30, NOCHANGE, NOCHANGE, true);
    m_button_load_script->Refresh();
    m_button_save_script->Refresh();
}

int CScriptEditor::Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    int result = CControl_Base::Obj_OnMessage (a_hwnd, a_message, a_wparam, a_lparam);
    return result;
}
