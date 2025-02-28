#include <windows.h>
#include <string.h>
#include <iostream>
using namespace std;

bool GetFileNameT(HWND hwnd, const LPSTR pszFileName, BOOL bSave, int filterindx, const char* types)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    pszFileName[0] = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = types;
    ofn.nFilterIndex    = filterindx;
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "";
    if (bSave)
    {
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                    OFN_OVERWRITEPROMPT;
        if (!GetSaveFileName(&ofn))
            return FALSE;
    }
    else
    {
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        if (!GetOpenFileName(&ofn))
            return FALSE;
    }
    return TRUE;
}

bool GetFileName(HWND hwnd, const LPSTR pszFileName, BOOL bSave, int filterindx)
{
    return GetFileNameT(hwnd, pszFileName, bSave, filterindx, "DADiSP Dat Files (*.dat)\0*.dat\0EDF Files (*.edf)\0*.edf\0BDF Files (*.bdf)\0*.bdf\0TDMS Files (*.tdms)\0*.tdms\0All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0\0");
}
