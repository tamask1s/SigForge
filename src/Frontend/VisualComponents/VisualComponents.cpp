#include <stdio.h>
#include <windows.h>
#include <vector>
#include <shlobj.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <Gdiplus.h>
using namespace std;
using namespace Gdiplus;

#include "fileio2.h"
#include "stringutils.h"
#include "MinVect.h"
#include "bitmap.h"
#include "Defines.h"
#include "Control_Base.h"
#include "datastructures.h"
#include "math.h"
#include "commonvisualcomponents.h"
#include "VisualComponents.h"

TVisualization::TVisualization()
{
    m_view_style        = 0;
    m_auto_offset_adjust = 1;
    m_horizontal_units_to_display = 1;
    m_global_magnifier     = 6 * 6 * 800;
    m_global_magnifier     = 6;
    m_seeking_allowed    = true;
    m_show_grid         = 0;
    m_line_surface_table    = 0;
}

TVisualization::~TVisualization()
{}

int TVisualization::GetActiveChNr ()
{
    int result = 0;
    for (unsigned int i = 0; i < m_visible_channels.m_size; i++)
        if (m_visible_channels.m_data[i] == 1)
            result++;
    return result;
}

PlotPanel::PlotPanel(const char* caption, int posx, int posy, int width, int height, CControl_Base* parent)
    : CControl_Base(0, 0, caption, 0, posx, posy, width, height, parent, false)
{
    m_codec       = 0;
    m_marker_indx = -1;
    m_vis         = 0;
    DrawLevel   = 4;
    RECT Rect;
    GetClientRect(hWnd, &Rect);
    Obj_OnResize(0, 0, Rect.right, Rect.bottom);
    m_font = CreateFont(12, 5, 0, 0, 700, 0, 0, 0, EASTEUROPE_CHARSET, 255, 255, /*19*/PROOF_QUALITY, 0, 0);
    m_white_brush = CreateSolidBrush(RGB(255, 255, 255));
    m_white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    m_black_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    m_blue_brush = CreateSolidBrush(RGB(192, 199, 230));
    m_startphysx = 0;
    m_stopphysx = 1;
    //MoveButton = MK_LBUTTON;
}

PlotPanel::~PlotPanel()
{
    DeleteObject(m_font);
    DeleteObject(m_white_brush);
    DeleteObject(m_white_pen);
    DeleteObject(m_blue_brush);
}

int PlotPanel::Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    int result = CControl_Base::Obj_OnMessage (a_hwnd, a_message, a_wparam, a_lparam);
    if (a_message == WM_ERASEBKGND)
        if (m_vis)
            m_vis->m_seeking_allowed = true;
    return result;
}

void PlotPanel::Obj_OnResize(int x, int y, int width, int height)
{
    BackBitmap.Rebuild(width, height);
    Paint();
    Refresh();
    return CControl_Base::Obj_OnResize(x, y, width, height);
}

void PlotPanel::CalculateCoordParameters(int scale_area_len, double a_min_phy_s_x, double a_max_phy_s_x, vector<double>& values, double& physxxwidth_div_intlen, double& hossz1_div_ints, double& rate1, double& physw)
{
    double physxxwidth, intlen, minphysxt, maxphysxt, hossz1, hossz2;

    if (a_min_phy_s_x > a_max_phy_s_x)
    {
        minphysxt = a_max_phy_s_x;
        maxphysxt = a_min_phy_s_x;
    }
    else
    {
        minphysxt = a_min_phy_s_x;
        maxphysxt = a_max_phy_s_x;
    }

    if ((a_min_phy_s_x == 0) && (a_max_phy_s_x == 0))
    {
        minphysxt = -1;
        maxphysxt = 1;
    }
    /** \param countl: how many times is the data magnified (multiplier). ---hanyszorosara van nagyitva az eredetihez kepest
    */
    int count1 = 1;
    physxxwidth = maxphysxt - minphysxt;
    if (physxxwidth == 0)
        physxxwidth = 1000;

    while (physxxwidth <= 100)
    {
        physxxwidth = maxphysxt - minphysxt;
        minphysxt = minphysxt * 10;
        maxphysxt = maxphysxt * 10;
        count1 = count1 * 10;
    }

    physxxwidth = maxphysxt - minphysxt;
    if (physxxwidth == 0)
        physxxwidth = 1;
    hossz1 = scale_area_len;
    int ints =(int)ceil(hossz1 / (999.999 / 13.0)) + 0; //5

    /**the lines above: only rounding the values of the parameters values
    * \param hossz1 the vertical size (hight) of the plotting area, so the 'client area' without the 'scaling area'
    * \param ints number of the scale units along the y axis. Divide the plotting area into ~100 pixel high units.-->intlen (later)
    */
    intlen = floor(physxxwidth / ints / pow(10, floor(log10(physxxwidth / ints)))) * pow(10, floor(log10(physxxwidth / ints)));
    ints = (int)floor(physxxwidth / intlen); /** < the meaning of ints doesn't change, only a little approx. happens*/
    hossz2 = floor(minphysxt / intlen) * intlen;
    rate1 = (minphysxt - hossz2) * hossz1 / physxxwidth;
    hossz1 = hossz1 * ints * intlen / physxxwidth;
    hossz1_div_ints = hossz1 / ints;

    /** < \param intlen~=width/ints. meaning: the hight of a unit on the scale.
    */

    physxxwidth_div_intlen = physxxwidth / intlen;
    values.reserve((int)floor(physxxwidth_div_intlen) + 2); /** < this will contain the numbers of the scale*/
    for (int i = 0; i < (int)floor(physxxwidth_div_intlen) + 2; i++) /** < same: i<values.size()*/
        if (((floor(i * hossz1_div_ints - rate1) + 1) > 0) && ((floor(i * hossz1_div_ints - rate1) < (scale_area_len + 1))))//ha az y menti skala teruleten vagyok
        {
            double value; /** <later we put it in the values[i] */
            if (a_min_phy_s_x > a_max_phy_s_x)
                value = floor(maxphysxt - (hossz2 + i * intlen) + minphysxt) / count1; /** < calculating the 'i'-th value(param) --kiszamolom az i-dik unit erteket, hogy ahol ez a beosztas van, az minek felel meg*/
            else
                value = floor(hossz2 + i * intlen) / count1;
            values [i] = value;
        }
    physw = a_max_phy_s_x - a_min_phy_s_x;
}

void PlotPanel::PaintVCoord(vector<double>& values, HDC hdc, RECT* rect, int* border, int scale_area_len, double a_min_phy_s_x, double physxxwidth_div_intlen, double hossz1_div_ints, double rate1, double physw)
{
    double clw = rect->bottom - rect->top - border[3]; /** < the hight of the area, on that we can plot */
    char   strt[64];
    SelectObject(hdc, m_blue_brush);
    Rectangle(hdc, rect->left + border[0], rect->top + border[1], rect->left + border[0] + border[2], rect->bottom - border[3] + 1);  /**< this draws actually the vertical blue scaling-area */

    HPEN tp;
    if ((m_vis->m_show_grid & 1) || (m_vis->m_show_grid & 2) || (m_vis->m_show_grid & 32) || (m_vis->m_show_grid & 64))
        tp = CreatePen(PS_DOT, 1, RGB(100, 100, 100));
    else
        tp = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN oldpen = (HPEN)SelectObject(BackBitmap.Hdc, tp);
    for (int i = 0; i < (int)floor(physxxwidth_div_intlen) + 2; i++) /** < iterate over values */
        if (((floor(i * hossz1_div_ints - rate1) + 1) > 0) && ((floor(i * hossz1_div_ints - rate1) < (scale_area_len + 1))))//ha az y menti skala teruleten vagyok
        {
            /** same condition as before, when we calculated values of 'values[]'*/
            sprintf(strt, "%1.5f", values[i]);
            int bigw = 5, smallw = 4, smallerw = 2;
            if (m_vis->m_show_grid & 1)
                bigw = rect->right;
            if (m_vis->m_show_grid & 2)
                smallw = bigw = rect->right;
            double x = (values[i] * clw / physw - a_min_phy_s_x * clw / physw);
            MoveToEx (hdc, border[2] - bigw + rect->left, rect->bottom - border[3] - (int)x, 0);
            LineTo   (hdc, border[2] + bigw + rect->left, rect->bottom - border[3] - (int)x); /**<small lines on the number lines*/
            double intenvalw = hossz1_div_ints / 10.0;
            int upb = 5;
            if (i > (int)floor(physxxwidth_div_intlen) - 1)
                upb = 7;
            for (int k = -5; k < upb; k++)
            {
                MoveToEx (hdc, rect->left + border[2] - ((k == 5 || k == -5) ? (smallw) : (smallerw)), rect->bottom - border[3] - (int)(x + ((double)k * intenvalw)), 0);
                LineTo (hdc, rect->left + border[2] + ((k == 5 || k == -5) ? (smallw) : (smallerw)), rect->bottom - border[3] - (int)(x + ((double)k * intenvalw)));
            }

            SelectObject(hdc, m_font);
            SetBkMode (hdc, TRANSPARENT);
            if (values[i] == 0)
                strt[strlen(strt) - 4] = 0;
            if (fabs(values[i]) > 0.1)
                strt[strlen(strt) - 2] = 0;
            if (fabs(values[i]) > 1)
                strt[strlen(strt) - 1] = 0;
            if (fabs(values[i]) > 50)
                strt[strlen(strt) - 1] = 0;
            if (fabs(values[i]) > 1000)
                strt[strlen(strt) - 2] = 0;
            if (!strcmp(&strt[strlen(strt) - 1], "."))
                strt[strlen(strt) - 1] = 0;

            HFONT Font2 = CreateFont(12, 5, 3600 / 4, 0, 700, 0, 0, 0, EASTEUROPE_CHARSET, 255, 255, /*19*/PROOF_QUALITY, 0, 0);
            SelectObject(hdc, Font2);

            SetTextColor(hdc, RGB(140, 20, 30));

            SetTextAlign(hdc,  VTA_CENTER		  );
            TextOut(hdc, rect->left + 2, rect->bottom - border[3] - (int)floor(i * hossz1_div_ints - rate1), strt, strlen(strt));

            DeleteObject(Font2);
        }
    SelectObject(BackBitmap.Hdc, oldpen);
    DeleteObject(tp);
}

void PlotPanel::DrawVCoord(HDC hdc, RECT* rect, double a_min_phy_s_x, double a_max_phy_s_x)
{
    /** This function calculates and draws the scale along the Y axis.
    */
    int border[4];

    /** < \param border[]: the borders of the vertical blue colored scale-area. \param border[0] the distance (of the beginning the blue area) from the left border. \param border[1] d (of beginning) from upper border. \param border[2] the distance (of the ending the blue area) from the left border. \param border[3] d (of ending) from lower border
    *so border[0],b[1] defines the upper left point of the hotizontal blue area, while the axes are the upper and the left axes. And b[2],b[3] defines the lower right point, while the axes are the lower and the left axes.
    */
    border[0] = 0;
    border[1] = 0;
    border[2] = 15;
    border[3] = 15;
    int scale_area_len = rect->bottom - border[1] - border[3];
    vector<double> values;
    double physxxwidth_div_intlen, hossz1_div_ints, rate1, physw;
    CalculateCoordParameters(scale_area_len, a_min_phy_s_x, a_max_phy_s_x, values, physxxwidth_div_intlen, hossz1_div_ints, rate1, physw);
    PaintVCoord(values, hdc, rect, border, scale_area_len, a_min_phy_s_x, physxxwidth_div_intlen, hossz1_div_ints, rate1, physw);
}

void PlotPanel::DrawCoord(HDC hdc, RECT* rect, double a_min_phy_s_x, double a_max_phy_s_x, int a_data_size)
{
    /** This function calculates and draws the scale along the X axis.
    *the algorithm is the same as along y (DrawVCoord)
    */
    int clientwidth = rect->right - rect->left;
    int clientheight = rect->bottom;
    int border[4];
    border[0] = 0;
    border[1] = 0;
    border[2] = 0;
    border[3] = 15;
    int ints, count1;
    char strt[64];
    double minphysxt, maxphysxt, hossz1, hossz2, intlen, rate1, physxxwidth;
    {
        if (a_min_phy_s_x > a_max_phy_s_x)
        {
            minphysxt = a_max_phy_s_x;
            maxphysxt = a_min_phy_s_x;
        }
        else
        {
            minphysxt = a_min_phy_s_x;
            maxphysxt = a_max_phy_s_x;
        }

        if ((a_min_phy_s_x == 0) && (a_max_phy_s_x == 0))
        {
            minphysxt = 1;
            maxphysxt = a_data_size;
        }

        count1 = 1;
        physxxwidth = maxphysxt - minphysxt;

        while (physxxwidth <= 100)
        {
            physxxwidth = maxphysxt - minphysxt;
            minphysxt = minphysxt * 10;
            maxphysxt = maxphysxt * 10;
            count1 = count1 * 10;
        }

        physxxwidth = maxphysxt - minphysxt;
        hossz1 = (clientwidth - border[0] - border[2]);
        ints = (int)ceil(hossz1 / (999.999 / 10.0)) + 0;

        intlen = floor(physxxwidth / ints / pow(10, floor(log10(physxxwidth / ints)))) * pow(10, floor(log10(physxxwidth / ints)));
        ints = (int)floor(physxxwidth / intlen);
        hossz2 = floor(minphysxt / intlen) * intlen;
        rate1 = (minphysxt - hossz2) * hossz1 / physxxwidth;
        hossz1 = hossz1 * ints * intlen / physxxwidth;

        SelectObject(hdc, m_blue_brush);
        Rectangle(hdc, rect->left + border[0], rect->bottom - border[3], rect->right - border[2], rect->bottom);

        double values [(int)floor(physxxwidth / intlen) + 2];
        for (int i = 0; i < (int)floor(physxxwidth / intlen) + 2; i++)
            if (((border[0] + floor(i * hossz1 / ints - rate1) + 1) > border[0]) && ((border[0] - 1 + floor(i * hossz1 / ints - rate1) < (clientwidth - border[2]))))
            {
                double value;
                if (a_min_phy_s_x > a_max_phy_s_x)
                    value = floor(maxphysxt - (hossz2 + i * intlen) + minphysxt) / count1;
                else
                    value = floor(hossz2 + i * intlen) / count1;
                values [i] = value;
            }
        double physw = a_max_phy_s_x - a_min_phy_s_x;
        double clw = rect->right - rect->left;
        HPEN tp;
        if ((m_vis->m_show_grid & 1) || (m_vis->m_show_grid & 2) || (m_vis->m_show_grid & 32) || (m_vis->m_show_grid & 64))
            tp = CreatePen(PS_DOT, 1, RGB(100, 100, 100));
        else
            tp = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HPEN oldpen = (HPEN)SelectObject(BackBitmap.Hdc, tp);
        for (int i = 0; i < (int)floor(physxxwidth / intlen) + 2; i++)
            if (((border[0] + floor(i * hossz1 / ints - rate1) + 1) > border[0]) && ((border[0] - 1 + floor(i * hossz1 / ints - rate1) < (clientwidth - border[2]))))
            {
                sprintf(strt, "%f", values[i]);
                double x = (values[i] * clw / physw - a_min_phy_s_x * clw / physw);
                int bigw = 5, smallw = 4, smallerw = 2;
                if (m_vis->m_show_grid & 32)
                    bigw = rect->bottom;
                if (m_vis->m_show_grid & 64)
                    smallw = bigw = rect->bottom;

                MoveToEx (hdc, (int)x + rect->left, clientheight - border[3] - bigw, 0);
                LineTo   (hdc, (int)x + rect->left, clientheight - border[3] + bigw);

                double intenvalw = hossz1 / (double)ints / 10.0;
                for (int k = -10; k < 10; k++)
                {
                    int xx =    (int)(x + rect->left + ((double)k * intenvalw));
                    if (xx > rect->left)
                    {
                        MoveToEx (hdc, xx, clientheight - border[3] - ((k == 5 || k == -5) ? (smallw) : (smallerw)), 0);
                        LineTo (hdc, xx, clientheight - border[3] + ((k == 5 || k == -5) ? (smallw) : (smallerw)));
                    }
                }

                RECT tr;
                SelectObject(hdc, m_font);
                SetBkMode (hdc, TRANSPARENT);
                if (values[i] == 0)
                    strt[strlen(strt) - 4] = 0;
                if (fabs(values[i]) > 0.1)
                    strt[strlen(strt) - 2] = 0;
                if (fabs(values[i]) > 1)
                    strt[strlen(strt) - 1] = 0;
                if (fabs(values[i]) > 50)
                    strt[strlen(strt) - 1] = 0;
                if (fabs(values[i]) > 1000)
                    strt[strlen(strt) - 2] = 0;
                if (!strcmp(&strt[strlen(strt) - 1], "."))
                    strt[strlen(strt) - 1] = 0;

                SetTextAlign(hdc, TA_TOP);
                tr.left = (int)(border[0] + floor(i * hossz1 / ints - rate1) - 120) + rect->left;
                tr.right = (int)(border[0] + floor(i * hossz1 / ints - rate1) + 120) + rect->left;
                tr.top = (int)(clientheight - border[3] + 2);
                tr.bottom = (int)(clientheight - border[3] + 20);
                SetTextColor(hdc, RGB(140, 20, 30));
                DrawText(hdc, strt, strlen(strt), &tr, DT_CENTER);
            }
        SelectObject(BackBitmap.Hdc, oldpen);
        DeleteObject(tp);
    }
}

void PlotPanel::Obj_OnMouseMove(int button, int x, int y)
{
    CControl_Base::Obj_OnMouseMove(button, x, y);

    if (m_marker_indx > -1 && m_marker_indx < (int)m_vis->m_marker_refs.m_size)
    {
        if (m_start_stop == 3)
            CBSetCursor(SIZEALL);
        else
            CBSetCursor(SIZEWE);
        RECT Rect;
        GetClientRect(hWnd, &Rect);
        int minx = MAXINT;
        if (m_vis->GetActiveChNr() == 1 || m_vis->m_auto_offset_adjust == 3 || m_vis->m_auto_offset_adjust == 4)
        {
            x -= 14;
            Rect.left += 14;
        }
        for (unsigned int i = 0; i < m_vis->m_marker_refs.m_size; i++)
        {
            double rw = (Rect.right - Rect.left);
            double ratiox = rw / (m_stopphysx - m_startphysx) / m_max_stop_phys_x_3_div_stop_phys_x_2;
            int startx = Rect.left + (int)((m_vis->m_marker_refs.m_data[i]->m_start_sample - m_startphysx) * ratiox);
            if (minx > abs(startx - x))
                minx = startx;
        }
        if (m_vis->m_seeking_allowed)
        {
            m_vis->m_seeking_allowed = false;
            double rw = (Rect.right - Rect.left);
            double ratiox = rw / (m_stopphysx - m_startphysx) / m_max_stop_phys_x_3_div_stop_phys_x_2;
            if (m_start_stop == 1) // mouse is on start point
            {
                double stopsample = m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample + m_vis->m_marker_refs.m_data[m_marker_indx]->m_length;
                double laststop = m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample + m_vis->m_marker_refs.m_data[m_marker_indx]->m_length;
                m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample = (double)x / ratiox + m_startphysx;
                m_vis->m_marker_refs.m_data[m_marker_indx]->m_length = stopsample - m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample;
                if (m_vis->m_marker_refs.m_data[m_marker_indx]->m_length < 0)
                {
                    m_vis->m_marker_refs.m_data[m_marker_indx]->m_length = 0;
                    m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample = laststop;
                }
            }
            if (m_start_stop == 2) // mouse is on stop point
            {
                m_vis->m_marker_refs.m_data[m_marker_indx]->m_length = (double)x / ratiox + m_startphysx - m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample;
                if (m_vis->m_marker_refs.m_data[m_marker_indx]->m_length < 0)
                    m_vis->m_marker_refs.m_data[m_marker_indx]->m_length = 0;
            }
            if (m_start_stop == 3) // mouse is on the marker
            {
                m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample = m_vis->m_marker_refs.m_data[m_marker_indx]->m_start_sample + (double)(x - m_mouse_down_x) / ratiox;
                m_mouse_down_x = x;
            }
            Paint();
            Refresh();
        }
    }
    else
        SetCursors(x);
}

void PlotPanel::SetCursors(int x)
{
    int startstop;
    if (TMarkersHasPoint(x, 0, &startstop))
    {
        if (startstop == 3)
            CBSetCursor(SIZEALL);
        else
            CBSetCursor(SIZEWE);
    }
    else
        CBSetCursor(ARROW);
}

bool PlotPanel::TMarkersHasPoint(int x, int* markerindx, int* startstop, char* marker_label)
{
    RECT Rect;
    GetClientRect(hWnd, &Rect);
    if (m_vis->GetActiveChNr() == 1 || m_vis->m_auto_offset_adjust == 3 || m_vis->m_auto_offset_adjust == 4)
        Rect.left += 14;
    bool result = false;
    for (unsigned int i = 0; i < m_vis->m_marker_refs.m_size; i++)
    {
        double rw = (Rect.right - Rect.left);
        double ratiox = rw / (m_stopphysx - m_startphysx) / m_max_stop_phys_x_3_div_stop_phys_x_2;
        int startx = Rect.left + (int)((m_vis->m_marker_refs.m_data[i]->m_start_sample - m_startphysx) * ratiox);
        int stopx = Rect.left + (int)((m_vis->m_marker_refs.m_data[i]->m_start_sample + m_vis->m_marker_refs.m_data[i]->m_length - m_startphysx) * ratiox) + 1;
        if (x == startx && m_vis->m_marker_refs.m_data[i]->m_sizable_left)
        {
            result = true;
            if (markerindx)
                *markerindx = i;
            if (startstop)
                *startstop = 1; // mouse is on start point
        }
        if (x == stopx && m_vis->m_marker_refs.m_data[i]->m_sizable_right)
        {
            result = true;
            if (markerindx)
                *markerindx = i;
            if (startstop)
                *startstop = 2; // mouse is on stop point
        }
        if (x > startx && x < stopx && m_vis->m_marker_refs.m_data[i]->m_movable)
        {
            result = true;
            if (markerindx)
                *markerindx = i;
            if (startstop)
                *startstop = 3; // mouse is on the marker
        }
        if (marker_label && x >= startx && x <= stopx)
            strcpy(marker_label, m_vis->m_marker_refs.m_data[i]->m_label);
    }
    return result;
}

void PlotPanel::Obj_OnMouseDown(int button, int x, int y, int client)
{
    CControl_Base::Obj_OnMouseDown(button, x, y, client);
    if (TMarkersHasPoint(x, &m_marker_indx, &m_start_stop))
        SetCursors(x);
    m_mouse_down_x = x - 14;
}

void PlotPanel::Obj_OnMouseUp(int button, int x, int y, int client)
{
    CControl_Base::Obj_OnMouseDown(button, x, y, client);
    SetCursors(x);
    m_marker_indx = -1;
}

void PlotPanel::DrawVCoord(RECT* Rect, double offset, int magnifindx)
{
    double physminy = 0.5 / (-1.0 * m_vis->m_magnifiers.m_data[magnifindx] * m_vis->m_flips.m_data[0] * m_vis->m_global_magnifier) + offset;
    double physmaxy = 0.5 / (1.0 * m_vis->m_magnifiers.m_data[magnifindx] * m_vis->m_flips.m_data[0] * m_vis->m_global_magnifier) + offset;
    if (m_vis->m_auto_offset_adjust == 4)
    {
        physmaxy -= physminy;
        physminy = 0;
    }
    DrawVCoord(BackBitmap.Hdc, Rect, physminy, physmaxy);
}

void DrawArrow(HDC pdc, POINT* m_One, POINT* m_Two)
{
    double slopy, cosy, siny;
    double Par = 10.0; // length of Arrow (>)
    slopy = atan2( ( m_One->y - m_Two->y ), ( m_One->x - m_Two->x ) );
    cosy = cos( slopy );
    siny = sin( slopy ); // need math.h for these functions

    // draw a line between the 2 endpoint
    MoveToEx(pdc, m_One->x, m_One->y, 0);
    LineTo(pdc, m_Two->x, m_Two->y );

    // here is the tough part - actually drawing the arrows
    // a total of 6 lines drawn to make the arrow shape
    MoveToEx(pdc, m_One->x, m_One->y, 0);
    LineTo(pdc, m_One->x + int( - Par * cosy - ( Par / 5.0 * siny ) ), m_One->y + int( - Par * siny + ( Par / 5.0 * cosy ) ) );
    MoveToEx(pdc, m_One->x + int( - Par * cosy + ( Par / 5.0 * siny ) ), m_One->y - int( Par / 5.0 * cosy + Par * siny ), 0);
    LineTo(pdc, m_One->x, m_One->y );
    if (false) // arrow on other end
    {
        MoveToEx(pdc, m_Two->x, m_Two->y, 0);
        LineTo(pdc, m_Two->x + int( Par * cosy - ( Par / 2.0 * siny ) ), m_Two->y + int( Par * siny + ( Par / 2.0 * cosy ) ) );
        LineTo(pdc, m_Two->x + int( Par * cosy + Par / 2.0 * siny ), m_Two->y - int( Par / 2.0 * cosy - Par * siny ) );
        LineTo(pdc, m_Two->x, m_Two->y );
    }
}

void DrawArrow(HDC pdc, int x, int y, double len, double slopy)
{
    POINT m_One, m_Two;
    double cosy, siny;
    double Par = 4.0 + len / 1.50; // length of Arrow (>)

    cosy = cos(slopy);
    siny = sin(slopy);

    m_One.x = (int)(x + cosy * len);
    m_One.y = (int)(y + siny * len);
    m_Two.x = (int)(x - cosy * len);
    m_Two.y = (int)(y - siny * len);

    MoveToEx(pdc, m_One.x, m_One.y, 0);
    LineTo(pdc, m_Two.x, m_Two.y );

    MoveToEx(pdc, m_Two.x, m_Two.y, 0);
    LineTo(pdc, m_Two.x + int( Par * cosy - ( Par / 3.0 * siny ) ), m_Two.y + int( Par * siny + ( Par / 3.0 * cosy ) ) );
    MoveToEx(pdc, m_Two.x + int( Par * cosy + Par / 3.0 * siny ), m_Two.y - int( Par / 3.0 * cosy - Par * siny ), 0);
    LineTo(pdc, m_Two.x, m_Two.y );
}

CMatrix<double> splinebuff;

void PlotPanel::Paint(bool a_paint_hover_only)
{
    if (m_vis)
        if (m_vis->m_visible_channels.m_size)
        {
            RECT Rect;
            Rect.top = 0;
            Rect.left = 0;
            Rect.right = BackBitmap.Bmi.bmiHeader.biWidth / 1;
            Rect.bottom = BackBitmap.Bmi.bmiHeader.biHeight / 1;
            int lborder = 0;
            int rborder = 0;
            if (!a_paint_hover_only)
            {
                m_vis->m_last_hover_x = HOVER_NO_VALUE;
                m_vis->m_last_hover_y = HOVER_NO_VALUE;

                SelectObject(BackBitmap.Hdc, m_white_brush);
                HPEN tp = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                HPEN oldpen = (HPEN)SelectObject(BackBitmap.Hdc, tp);
                Rect.left += 0;

                Rectangle(BackBitmap.Hdc, Rect.left + lborder, Rect.top, Rect.right - rborder, Rect.bottom);
                SelectObject(BackBitmap.Hdc, oldpen);
                DeleteObject(tp);
            }
            lborder += 1;
            rborder += 1;

            if (!m_vis->m_visible_buffer.m_widths.m_size)
            {
                //printf("Error: PlotPanel::Paint: m_visible_buffer has not been built. \n");
                return;
            }
            int map2d_rulerwidth = 0;
            if (((m_vis->m_line_surface_table == 1) || (m_vis->m_line_surface_table == 2) || (m_vis->m_line_surface_table == 3)) && (Rect.bottom > 17) && (Rect.right > 3)) /// Surface
            {
                if (strlen(m_codec->m_surface2D_vert_units))
                    map2d_rulerwidth = 14;

                unsigned int VisNrchansVisible = m_vis->GetActiveChNr();
                unsigned int rectw = Rect.right - rborder - lborder - Rect.left - map2d_rulerwidth;
                if (splinebuff.m_height < rectw || splinebuff.m_width < VisNrchansVisible)
                    splinebuff.Rebuild(rectw, VisNrchansVisible);
                double** spin = splinebuff.m_data;
                dmx1.Rebuild(rectw, Rect.bottom - 14);

                int j = -1;
                //cout << m_vis->m_visible_buffer.m_widths.m_data[0] << endl;
                for (unsigned int x = 0; x < m_vis->m_visible_channels.m_size; x++) // prepare horizontal values
                    if (m_vis->m_visible_channels.m_data[x] == 1)
                    {
                        j++;
                        double xratio = (double)(m_vis->m_visible_buffer.m_widths.m_data[j] - 1) / (double)rectw;
                        if (xratio > 1) // decimate if bigger
                            for (unsigned int i = 0; i < rectw; i++)
                            {
                                int bufferx = (int)(i * xratio);
                                spin[i][j] = m_vis->m_visible_buffer.m_data[j][bufferx];
                            }
                        else // interpolate if smaller
                        {
//                            double xInput2[m_vis->m_visible_buffer.m_widths.m_data[j]];
//                            double xOutput2[rectw];
//                            double yOutput2[rectw];
//                            for (unsigned int i = 0; i < m_vis->m_visible_buffer.m_widths.m_data[j]; i++)
//                                xInput2[i] = (double)i / m_vis->m_visible_buffer.m_widths.m_data[j];
//
//                            for (unsigned int i = 0; i < rectw; i++)
//                                xOutput2[i] = (double)i / rectw;
//
//                            SplineInterpolate(xInput2, m_vis->m_visible_buffer.m_data[j], m_vis->m_visible_buffer.m_widths.m_data[j], xOutput2, yOutput2, rectw);
//                            for (unsigned int i = 0; i < rectw; i++)
//                                spin[i][j] = yOutput2[i];
                            for (unsigned int i = 0; i < rectw; ++i)
                            {
                                int bufferx = (int)(i * xratio);
                                spin[i][j] = m_vis->m_visible_buffer.m_data[j][bufferx];
                            }
                        }
                    }

                double xInput[VisNrchansVisible];
                double xOutput[Rect.bottom - 14];
                for (unsigned int i = 0; i < VisNrchansVisible; i++)
                    xInput[i] = (double)i / VisNrchansVisible;
                for (int i = 0; i < Rect.bottom - 14; i++)
                    xOutput[i] = (double)i / (Rect.bottom - 14);

                for (unsigned int i = 0; i < rectw; i++)
                    SplineInterpolate(xInput, spin[i], VisNrchansVisible, xOutput, dmx1.m_data[i], Rect.bottom - 14);

                double minval = MAXINT;
                double maxval = MININT;
                for (unsigned int j = 0; j < rectw / 1; j++)
                    for ( int i = 0; i < Rect.bottom - 14; i++)
                    {
                        if (minval > dmx1.m_data[j][i])
                            minval = dmx1.m_data[j][i];
                        if (maxval < dmx1.m_data[j][i])
                            maxval = dmx1.m_data[j][i];
                    }
                double szor = 255.0 / (maxval - minval);

                if (m_vis->m_line_surface_table == 3)
                {
                    int ColorMap[256];
                    {
                        float ft1 = 3.14 * 2 / 255;
                        int cont = 20;
                        for (unsigned int i = 0; i < 256; i++)
                        {
                            int t1 = 255 - (i / cont) * cont;
                            int t2 = ((byte)(128 - 127 * cos(t1 * ft1)) / cont) * cont;
                            int t3 = 255 - t1;
                            ColorMap[i] = RGB(t1, t2, t3);
                        }
                    }

                    for (int i = 1; i < Rect.bottom - 14; i++)
                    {
                        for (unsigned int j = 0; j < rectw / 1; j++)
                        {
                            byte val = (byte)((dmx1.m_data[j][i] - minval) * szor);
                            BackBitmap.BmRGBQuads[i][j + 1 + map2d_rulerwidth] = ColorMap[val];
                        }
                    }
                    for (int i = 0; i < Rect.bottom - 14 - 1; i++)
                        for (unsigned int j = 0; j < rectw - 1; j++)
                            if (BackBitmap.BmRGBQuads[i][j + map2d_rulerwidth] - BackBitmap.BmRGBQuads[i][j + 1 + map2d_rulerwidth] || BackBitmap.BmRGBQuads[i][j + map2d_rulerwidth] - BackBitmap.BmRGBQuads[i + 1][j + map2d_rulerwidth])
                                BackBitmap.BmRGBQuads[i][j + map2d_rulerwidth] = 0;
                }
                else
                {
                    int ColorMap[256];
                    {
                        float ft1 = 3.14 * 2 / 255;
                        int cont = 1;
                        for (unsigned int i = 0; i < 256; i++)
                        {
                            int t1 = 255 - (i / cont) * cont;
                            int t2 = ((byte)(128 - 127 * cos(t1 * ft1)) / cont) * cont;
                            int t3 = 255 - t1;
                            ColorMap[i] = RGB(t1, t2, t3);
                        }
                    }
                    for (int i = 1; i < Rect.bottom - 14; i++)
                    {
                        for (unsigned int j = 0; j < rectw / 1; j++)
                        {
                            byte val = (byte)((dmx1.m_data[j][i] - minval) * szor);
                            BackBitmap.BmRGBQuads[i][j + 1 + map2d_rulerwidth] = ColorMap[val];
                        }
                    }

                    int ho = 5;
                    for (int j = ho; j < Rect.bottom - 14 - ho; j += 20)
                        for (unsigned int i = ho; i < rectw - ho; i += 20)
                        {
                            int Mcx = 0, Mcy = 0, mcx, mcy;
                            double Mval = MININT;
                            double mval = MAXINT;
                            for (int k = j - ho; k < j + ho + 1; k++)
                            {
                                if (Mval < dmx1.m_data[i - ho][k])
                                {
                                    Mval = dmx1.m_data[i - ho][k];
                                    Mcx = i - ho;
                                    Mcy = k;
                                }
                                if (mval > dmx1.m_data[i - ho][k])
                                {
                                    mval = dmx1.m_data[i - ho][k];
                                    mcx = i - ho;
                                    mcy = k;
                                }
                                if (Mval < dmx1.m_data[i + ho][k])
                                {
                                    Mval = dmx1.m_data[i + ho][k];
                                    Mcx = i + ho;
                                    Mcy = k;
                                }
                                if (mval > dmx1.m_data[i + ho][k])
                                {
                                    mval = dmx1.m_data[i + ho][k];
                                    mcx = i + ho;
                                    mcy = k;
                                }
                            }

                            for (unsigned int k = i - ho; k < i + ho + 1; k++)
                            {
                                if (Mval < dmx1.m_data[k][j - ho])
                                {
                                    Mval = dmx1.m_data[k][j - ho];
                                    Mcx = k;
                                    Mcy = j - ho;
                                }
                                if (mval > dmx1.m_data[k][j - ho])
                                {
                                    mval = dmx1.m_data[k][j - ho];
                                    mcx = k;
                                    mcy = j - ho;
                                }
                                if (Mval < dmx1.m_data[k][j + ho])
                                {
                                    Mval = dmx1.m_data[k][j + ho];
                                    Mcx = k;
                                    Mcy = j + ho;
                                }
                                if (mval > dmx1.m_data[k][j + ho])
                                {
                                    mval = dmx1.m_data[k][j + ho];
                                    mcx = k;
                                    mcy = j + ho;
                                }
                            }

                            double slopy = atan2(  Mcy - mcy,  Mcx - mcx  );
                            double lenxy = Mval - mval;

                            lenxy *= 100.0 / (maxval - minval);
                            byte val = (byte)((dmx1.m_data[i][j] - minval) * szor);
                            lenxy = 20.0 * fabs(dmx1.m_data[i][j] - ((maxval - minval) / 2.0)) / (maxval - minval);
                            lenxy = fabs((double)val - 128.0) / 10.0;

                            if (lenxy > 0.8 && m_vis->m_line_surface_table == 2)
                                DrawArrow(BackBitmap.Hdc, i + map2d_rulerwidth, j, lenxy, slopy);
                        }
                }
                if (map2d_rulerwidth)
                    DrawVCoord(BackBitmap.Hdc, &Rect, m_codec->m_surface2D_vert_axis_min, m_codec->m_surface2D_vert_axis_max);
            } // EO Surface

            if (m_vis->m_line_surface_table == 4) /// Table
            {
                Rectangle(BackBitmap.Hdc, Rect.left + lborder, Rect.top, Rect.right - rborder, Rect.bottom);
                SelectObject(BackBitmap.Hdc, m_font);
                SetBkMode (BackBitmap.Hdc, TRANSPARENT);

                int j = -1;
                unsigned int max_channels_to_display = m_vis->m_visible_channels.m_size;
                if (max_channels_to_display > 16)
                    max_channels_to_display = 16;
                for (unsigned int x = 0; x < max_channels_to_display; x++) // prepare horizontal values
                    if (m_vis->m_visible_channels.m_data[x] == 1)
                    {
                        j++;
                        CStringVec crstr;
                        int nrelms = m_vis->m_visible_buffer.m_widths.m_data[j];
                        if (nrelms > 80)
                            nrelms = 80;
                        SetTextColor(BackBitmap.Hdc, RGB(0, 0, 0));
                        for (int i = 0; i < nrelms; i++)
                        {
                            float val = m_startphysx + (m_stopphysx - m_startphysx) / (double)(m_vis->m_visible_buffer.m_widths.m_data[j] - 1) * (double)i;
                            char tx[200];
                            sprintf(tx, "%0.2f", val);
                            RECT tr;
                            tr.left = (int)(j * 160 - 80 + 2);
                            tr.right = (int)((j + 1) * 160 - 80 + 2);
                            tr.top = (int)(i * 10);
                            tr.bottom = (int)((i + 1) * 10);
                            DrawText(BackBitmap.Hdc, tx, strlen(tx) - 1, &tr, DT_RIGHT);
                        }
                        SetTextColor(BackBitmap.Hdc, RGB(140, 20, 30));
                        crstr.RebuildFrom(m_vis->m_visible_buffer.m_data[j], nrelms);
                        for (unsigned int i = 0; i < crstr.m_size; i++)
                        {
                            RECT tr;
                            tr.left = (int)(j * 160 - 20 + 2);
                            tr.right = (int)((j + 1) * 160 - 20 + 2);
                            tr.top = (int)(i * 10);
                            tr.bottom = (int)((i + 1) * 10);
                            DrawText(BackBitmap.Hdc, crstr.m_data[i].s, strlen(crstr.m_data[i].s) - 1, &tr, DT_RIGHT);
                        }
                    }
                return;
            } /// EO Table

            float ft1;
            int VisNrchansVisible = m_vis->GetActiveChNr();
            double offsets[VisNrchansVisible];
            if (!m_vis->m_visible_buffer.m_widths.m_size)
                return;
            float datminC = 10000000;
            float datmaxC = -10000000;
            for (int j = 0; j < VisNrchansVisible ; j++)
            {
                if (m_vis->m_auto_offset_adjust == 1) // auto offset calculated from visible min and max
                {
                    float datmin = 10000000;
                    float datmax = -10000000;
                    for (unsigned int i = 0; i < m_vis->m_visible_buffer.m_widths.m_data[j]; i++)
                    {
                        ft1 = m_vis->m_visible_buffer.m_data[j][i];
                        if (datmin > ft1)
                            datmin = ft1;
                        if (datmax < ft1)
                            datmax = ft1;
                    }
                    offsets[j] = (datmax + datmin) / 2.0;
                }
                if (m_vis->m_auto_offset_adjust == 3) // auto offset calculated from visible min and max
                {
                    for (unsigned int i = 0; i < m_vis->m_visible_buffer.m_widths.m_data[j]; i++)
                    {
                        ft1 = m_vis->m_visible_buffer.m_data[j][i];
                        if (datminC > ft1)
                            datminC = ft1;
                        if (datmaxC < ft1)
                            datmaxC = ft1;
                    }
                    offsets[j] = (datmaxC + datminC) / 2.0;
                }
                if (m_vis->m_auto_offset_adjust == 2) // auto offset calculated from visible mean
                {
                    offsets[j] = 0;
                    double rat = 1.0 / (double)m_vis->m_visible_buffer.m_widths.m_data[j];
                    for (unsigned int i = 0; i < m_vis->m_visible_buffer.m_widths.m_data[j]; i++)
                        offsets[j] += m_vis->m_visible_buffer.m_data[j][i];
                    offsets[j] *= rat;
                }
                if (m_vis->m_auto_offset_adjust == 0 || m_vis->m_auto_offset_adjust == 4)
                    offsets[j] = 0; // no offset
            }
            if (m_vis->m_auto_offset_adjust == 3) // auto offset calculated from visible min and max
                for (int j = 0; j < VisNrchansVisible ; j++)
                    offsets[j] = (datmaxC + datminC) / 2.0;

            if (m_vis->m_line_surface_table == 0 || m_vis->m_line_surface_table == 5) // xy_plot
            {
                if (m_vis->GetActiveChNr() == 1)
                {
                    for (unsigned int x = 0; x < m_vis->m_visible_channels.m_size; x++)
                        if (m_vis->m_visible_channels.m_data[x] == 1)
                            DrawVCoord(&Rect, offsets[0], x);
                }
                else if (m_vis->m_auto_offset_adjust == 3 || m_vis->m_auto_offset_adjust == 4)
                    DrawVCoord(&Rect, offsets[0]);
                else if (m_vis->m_line_surface_table == 5) // xy_plot
                    DrawVCoord(&Rect, offsets[0], 0);
            }
            {
                if (1)
                {
                    unsigned int*start = new unsigned int[m_codec->m_total_samples.m_size];
                    unsigned int*nrelemnts = new unsigned int[m_codec->m_total_samples.m_size];
                    double ratios[m_vis->m_visible_buffer.m_widths.m_size];
                    double maxw = 0;
                    float maxsamplerate = 0;
                    int imaxindx = -1;
                    int j = 0;
                    for (unsigned int i = 0; i < m_vis->m_visible_channels.m_size; i++)
                    {
                        if (m_vis->m_visible_channels.m_data[i] == 1)
                        {
                            if (maxsamplerate < m_codec->m_total_samples.m_data[i] / m_codec->m_sample_rates.m_data[i])
                            {
                                maxsamplerate = m_codec->m_total_samples.m_data[i] / m_codec->m_sample_rates.m_data[i];
                                imaxindx = i;
                                maxw = m_vis->m_visible_buffer.m_widths.m_data[j];
                            }
                            j++;
                        }
                    }
                    for (unsigned int i = 0; i < m_vis->m_visible_buffer.m_widths.m_size; i++)
                        if (maxw)
                            ratios[i] = (double)m_vis->m_visible_buffer.m_widths.m_data[i] / maxw;
                        else
                            ratios[i] = 0;
                    j = 0;
                    for (unsigned int i = 0; i < m_vis->m_visible_channels.m_size; i++)
                    {
                        if (m_vis->m_visible_channels.m_data[i] == 1)
                        {
                            start[i] = 0 + (int)(0 * ratios[j]);
                            nrelemnts[i] = m_vis->m_visible_buffer.m_widths.m_data[j];
                            j++;
                        }
                        else
                        {
                            start[i] = 0;
                            nrelemnts[i] = 0;
                        }
                    }

                    Rect.bottom -= 14;
                    float vinterv = (float)Rect.bottom / (VisNrchansVisible + 1);
                    double maxm_stopphysx3pm_stopphysx2 = 1;
                    j = -1;
                    for (unsigned int x = 0; x < m_vis->m_visible_channels.m_size; x++)
                        if (m_vis->m_visible_channels.m_data[x] == 1)
                        {
                            j++;
                            double m_stopphysx2 = (double)(start[imaxindx] + nrelemnts[imaxindx] - 1.0/*-!!!*/) / ((double)m_codec->m_sample_rates.m_data[imaxindx]) + 0;
                            double m_stopphysx3 = (double)(start[x] + nrelemnts[x] - 1.0/*-!!!*/) / ((double)m_codec->m_sample_rates.m_data[x]) + 0;
                            double m_stopphysx3pm_stopphysx2 = m_stopphysx3 / m_stopphysx2;
                            if (maxm_stopphysx3pm_stopphysx2 < m_stopphysx3pm_stopphysx2)
                                maxm_stopphysx3pm_stopphysx2 = m_stopphysx3pm_stopphysx2;
                        }
                    m_max_stop_phys_x_3_div_stop_phys_x_2 = maxm_stopphysx3pm_stopphysx2;

                    if (m_vis->m_line_surface_table == 0)
                        if (m_vis->GetActiveChNr() == 1 || m_vis->m_auto_offset_adjust == 3 || m_vis->m_auto_offset_adjust == 4)
                            Rect.left += 14;
                    if ((m_vis->m_line_surface_table == 1) || (m_vis->m_line_surface_table == 2) || (m_vis->m_line_surface_table == 3))
                        Rect.left += map2d_rulerwidth;

                    Rect.bottom += 14;
                    if (!a_paint_hover_only)
                    {
                        SelectObject(BackBitmap.Hdc, m_black_pen);
                        // Marker
                        SetROP2(BackBitmap.Hdc, R2_MASKNOTPEN);
                        //HPEN tp = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
                        //HPEN oldpen = (HPEN)SelectObject(BackBitmap.Hdc, tp);
                        for (unsigned int i = 0; i < m_vis->m_marker_refs.m_size; i++)
                        {
                            HBRUSH TMarkerBrush = CreateSolidBrush(m_vis->m_marker_refs.m_data[i]->m_fill_color);
                            HPEN tp = CreatePen(PS_SOLID, 1, m_vis->m_marker_refs.m_data[i]->m_fill_color);

                            HBRUSH oldbrush = (HBRUSH)SelectObject(BackBitmap.Hdc, TMarkerBrush);
                            HPEN oldpen = (HPEN)SelectObject(BackBitmap.Hdc, tp);
                            double rw = (Rect.right - Rect.left);
                            double ratiox = rw / (m_stopphysx - m_startphysx);
                            int startx = Rect.left + (int)((m_vis->m_marker_refs.m_data[i]->m_start_sample - m_startphysx) * ratiox / maxm_stopphysx3pm_stopphysx2);
                            int stopx = Rect.left + (int)((m_vis->m_marker_refs.m_data[i]->m_start_sample + m_vis->m_marker_refs.m_data[i]->m_length - m_startphysx) * ratiox / maxm_stopphysx3pm_stopphysx2);
                            if (stopx <= startx)
                                stopx = startx + 1;
                            Rectangle(BackBitmap.Hdc, startx, Rect.top, stopx, Rect.bottom);
                            SelectObject(BackBitmap.Hdc, oldbrush);
                            SelectObject(BackBitmap.Hdc, oldpen);
                            DeleteObject(TMarkerBrush);
                            DeleteObject(tp);
                        }
                        SetROP2(BackBitmap.Hdc, R2_COPYPEN);
                        //SelectObject(BackBitmap.Hdc, oldpen);
                        //DeleteObject(tp);
                    } // EO TMarker

                    if (!a_paint_hover_only)
                    {
                        SelectObject(BackBitmap.Hdc, m_black_pen);
                            DrawCoord(BackBitmap.Hdc, &Rect, m_startphysx, m_startphysx + (m_stopphysx - m_startphysx)*maxm_stopphysx3pm_stopphysx2, 1000);
                    }

                    if (m_vis->m_line_surface_table == 0 || m_vis->m_line_surface_table == 5) /// multiline or XY plot
                    {
                        static int const PRECISE_DISPLAY_X_SIZE = 240;
                        static int const SMOOTH_DISPLAY_X_SIZE = 4096;
                        Rect.bottom -= 14;
                        j = -1;
                        int rrr = Rect.right;
                        double lastopp1 = -1;
                        Graphics* graphics = 0;
                        m_vis->m_line_style = 0;
                        if (m_vis->m_line_style == 0)
                        {
                            graphics = new Graphics(BackBitmap.Hdc);
                            graphics->SetSmoothingMode(SmoothingModeAntiAlias);
                        }
                        float last_xy_tmagn = 0;
                        double last_xy_coffset = 0;
                        for (unsigned int x = 0; x < m_vis->m_visible_channels.m_size; x++)
                        {
                            if (m_vis->m_visible_channels.m_data[x] == 1)
                            {
                                j++;
                                if (m_vis->m_visible_buffer.m_widths.m_data[j] > SMOOTH_DISPLAY_X_SIZE)
                                    m_vis->m_line_style = 1;
                                float tmagn = m_vis->m_magnifiers.m_data[x] * m_vis->m_flips.m_data[j] * m_vis->m_global_magnifier * (double)Rect.bottom;
                                double coffset = j * vinterv + vinterv;
                                if (m_vis->m_auto_offset_adjust == 3) // zero line is centered
                                    coffset = Rect.bottom / 2.0;
                                if (m_vis->m_auto_offset_adjust == 4) // zero line is on the bottom
                                    coffset = Rect.bottom;

                                double m_stopphysx2 = (double)(start[imaxindx] + nrelemnts[imaxindx] - 1) / (m_codec->m_sample_rates.m_data[imaxindx]) + 0;
                                double m_stopphysx3 = (double)(start[x] + nrelemnts[x] - 1) / (m_codec->m_sample_rates.m_data[x]) + 0;
                                double m_stopphysx3pm_stopphysx2 = m_stopphysx3 / m_stopphysx2;
                                Rect.right = (int)(rrr / maxm_stopphysx3pm_stopphysx2);
                                float opp1 = ((float)Rect.right - rborder - lborder - Rect.left) / (m_vis->m_visible_buffer.m_widths.m_data[j] - 1);

                                opp1 *= m_stopphysx3pm_stopphysx2;

                                if (!m_vis->m_points_ref.capacity())
                                    return;
                                unsigned int max_display = m_vis->m_visible_buffer.m_widths.m_data[j];
                                if (max_display > m_vis->m_points_ref.capacity())
                                    max_display = m_vis->m_points_ref.capacity();
                                if (m_vis->m_line_surface_table == 5) // xy_plot
                                {
                                    if (j % 2 == 0)
                                    {
                                        last_xy_tmagn = tmagn;
                                        last_xy_coffset = coffset;
                                    }
                                    else if (m_vis->m_visible_buffer.m_widths.m_data[j] == m_vis->m_visible_buffer.m_widths.m_data[j - 1])
                                    {
                                        for (unsigned int i = 0; i < max_display; i++)
                                        {
                                            m_vis->m_points_ref[i].x = (int)((m_vis->m_visible_buffer.m_data[j - 1][i]) * last_xy_tmagn + last_xy_coffset);
                                            m_vis->m_points_ref[i].y = (int)((offsets[j] - m_vis->m_visible_buffer.m_data[j][i]) * tmagn + coffset);
                                        }
                                    }
                                    else
                                        max_display = 0;
                                }
                                else if (m_vis->m_visible_buffer.m_widths.m_data[j] > PRECISE_DISPLAY_X_SIZE)
                                {
                                    if (lastopp1 == opp1)
                                        for (unsigned int i = 0; i < max_display; i++)
                                            m_vis->m_points_ref[i].y = (int)((offsets[j] - m_vis->m_visible_buffer.m_data[j][i]) * tmagn + coffset);
                                    else
                                        for (unsigned int i = 0; i < max_display; i++)
                                        {
                                            m_vis->m_points_ref[i].x = (int)(Rect.left + lborder + i * opp1);
                                            m_vis->m_points_ref[i].y = (int)((offsets[j] - m_vis->m_visible_buffer.m_data[j][i]) * tmagn + coffset);
                                        }
                                }
                                else
                                    for (unsigned int i = 0; i < max_display; i++)
                                    {
                                        m_vis->m_points_ref[i * 2].x = (int)(Rect.left + lborder + i * opp1);
                                        m_vis->m_points_ref[i * 2].y = (int)((offsets[j] - m_vis->m_visible_buffer.m_data[j][i]) * tmagn + coffset);
                                        m_vis->m_points_ref[i * 2 + 1].x = (int)(Rect.left + lborder + (i + 1) * opp1);
                                        m_vis->m_points_ref[i * 2 + 1].y = (int)((offsets[j] - m_vis->m_visible_buffer.m_data[j][i]) * tmagn + coffset);
                                    }
                                lastopp1 = opp1;

                                byte R = 255 * (x % 2);
                                byte G = 255 * (x % 3);
                                byte B = 255 - R;

                                R /= 1.3;
                                G /= 1.5;
                                B /= 1.3;
                                if (!a_paint_hover_only)
                                {
                                    if (m_vis->m_line_style == 1)
                                    {
                                        HPEN tp;
                                        if (m_vis->m_auto_offset_adjust == 3 || m_vis->m_auto_offset_adjust == 4)
                                            tp = CreatePen(PS_SOLID, m_vis->m_line_width, RGB(R, G, B));
                                        else
                                            tp = CreatePen(PS_SOLID, m_vis->m_line_width, RGB(0, 0, 0));
                                        HPEN oldpen = (HPEN)SelectObject(BackBitmap.Hdc, tp);
                                            Polyline(BackBitmap.Hdc, &m_vis->m_points_ref[0], m_vis->m_visible_buffer.m_widths.m_data[j] > PRECISE_DISPLAY_X_SIZE ? m_vis->m_visible_buffer.m_widths.m_data[j] : m_vis->m_visible_buffer.m_widths.m_data[j] * 2);
                                        SelectObject(BackBitmap.Hdc, oldpen);
                                        DeleteObject(tp);
                                    }
                                    else
                                    {
                                        Pen* pen;
                                        if (m_vis->m_auto_offset_adjust == 3 || m_vis->m_auto_offset_adjust == 4)
                                            pen = new Pen(Color(R,G,B), m_vis->m_line_width);
                                        else
                                            pen = new Pen(Color(0,0,0), m_vis->m_line_width);
                                        if (m_vis->m_line_surface_table == 5)
                                        {
                                            if (j % 2 == 1)
                                            {
                                                int radius = 3;
                                                SolidBrush brush(Color(255, 0, 0, 255));
                                                for (unsigned int i = 0; i < max_display; ++i)
                                                    graphics->FillEllipse(&brush, m_vis->m_points_ref[i].x - radius, m_vis->m_points_ref[i].y - radius, radius * 2, radius * 2);
                                            }
                                        }
                                        else
                                            graphics->DrawLines(pen, (Point*)&m_vis->m_points_ref[0], m_vis->m_visible_buffer.m_widths.m_data[j] > PRECISE_DISPLAY_X_SIZE ? m_vis->m_visible_buffer.m_widths.m_data[j] : m_vis->m_visible_buffer.m_widths.m_data[j] * 2);
                                        delete pen;
                                    }
                                }

                                unsigned int indx = (m_vis->m_hover_x - Rect.left - lborder) / opp1;
                                if (m_vis->m_hover_ch == j && indx < max_display)
                                {
                                    if (m_vis->m_visible_buffer.m_widths.m_data[j] <= PRECISE_DISPLAY_X_SIZE)
                                        indx *= 2;
                                    int m_last_cross_datapoints_indx = 0;
                                    if (m_vis->m_last_hover_x != HOVER_NO_VALUE && m_vis->m_last_hover_y != HOVER_NO_VALUE)
                                        if (m_vis->m_last_hover_x >= 0 && m_vis->m_last_hover_y >= 0 && m_vis->m_last_hover_y < BackBitmap.Bmi.bmiHeader.biHeight && m_vis->m_last_hover_x < BackBitmap.Bmi.bmiHeader.biWidth)
                                            for (int m = -15; m < 15; ++m)
                                            {
                                                if ((m_vis->m_last_hover_y + m) >= 0 && (m_vis->m_last_hover_y + m) < BackBitmap.Bmi.bmiHeader.biHeight)
                                                    BackBitmap.BmRGBQuads[m_vis->m_last_hover_y + m][m_vis->m_last_hover_x] = m_vis->m_last_cross_datapoints[m_last_cross_datapoints_indx++];
                                                if (m && (m_vis->m_last_hover_x + m) >= 0 && (m_vis->m_last_hover_x + m) < BackBitmap.Bmi.bmiHeader.biWidth)
                                                    BackBitmap.BmRGBQuads[m_vis->m_last_hover_y][m_vis->m_last_hover_x + m] = m_vis->m_last_cross_datapoints[m_last_cross_datapoints_indx++];
                                            }
                                    m_last_cross_datapoints_indx = 0;
                                    if (m_vis->m_hover_x >= 0 && m_vis->m_points_ref[indx].y >= 0 && m_vis->m_points_ref[indx].y < BackBitmap.Bmi.bmiHeader.biHeight && m_vis->m_hover_x < BackBitmap.Bmi.bmiHeader.biWidth)
                                        for (int m = -15; m < 15; ++m)
                                        {
                                            if ((m_vis->m_points_ref[indx].y + m) >= 0 && (m_vis->m_points_ref[indx].y + m) < BackBitmap.Bmi.bmiHeader.biHeight)
                                            {
                                                m_vis->m_last_cross_datapoints[m_last_cross_datapoints_indx++] = BackBitmap.BmRGBQuads[m_vis->m_points_ref[indx].y + m][m_vis->m_hover_x];
                                                BackBitmap.BmRGBQuads[m_vis->m_points_ref[indx].y + m][m_vis->m_hover_x] = RGB(0, 255, 0);
                                            }
                                            if (m && (m_vis->m_hover_x + m) >= 0 && (m_vis->m_hover_x + m) < BackBitmap.Bmi.bmiHeader.biWidth)
                                            {
                                                m_vis->m_last_cross_datapoints[m_last_cross_datapoints_indx++] = BackBitmap.BmRGBQuads[m_vis->m_points_ref[indx].y][m_vis->m_hover_x + m];
                                                BackBitmap.BmRGBQuads[m_vis->m_points_ref[indx].y][m_vis->m_hover_x + m] = RGB(0, 255, 0);
                                            }
                                        }
                                    if (m_last_cross_datapoints_indx >= 60)
                                        cout << "m_last_cross_datapoints_indx <= 60!" << endl;
                                    m_vis->m_last_hover_x = m_vis->m_hover_x;
                                    m_vis->m_last_hover_y = m_vis->m_points_ref[indx].y;
                                }
                            }
                        }
                        delete graphics;
                    }
                    delete[] start;
                    delete[] nrelemnts;
                }
            }
        }
}

SetupChannels::SetupChannels(const char* caption, int posx, int posy, int width, int height, CControl_Base* parent, TVisualization* vis, ISignalCodec* signalcodec)
    : CControl_Base(0, 0, caption, 0, posx, posy, width, height, parent, true)
{
    m_vis = vis;
    MoveButton = MK_LBUTTON;
    m_checkboxes_start  = 40000;
    int nrrows = 15;
    for (unsigned int i = 0; i < m_vis->m_visible_channels.m_size; i++)
    {
        char dst [400];
        sprintf(dst, "%s%i%s", "Ch ", i + 1, ". ");

        if (signalcodec)
        {
            strcat(dst, signalcodec->m_labels.m_data[i].s);
            sprintf(dst + strlen(dst) - 1, "%s%s%s", ". |", signalcodec->m_vertical_units.m_data[i].s, "|");
            sprintf(dst + strlen(dst) - 1, "%s%f", "", signalcodec->m_sample_rates.m_data[i]);
            strcpy(dst + strlen(dst) - 3, "S/(");
            strcat(dst, signalcodec->m_horizontal_units);
            strcat(dst, ")|");
            sprintf(dst + strlen(dst) - 1, "%s%f", "", (float)signalcodec->m_total_samples.m_data[i] / signalcodec->m_sample_rates.m_data[i]);
            strcpy(dst + strlen(dst) - 5, "Tot.");
            strcat(dst, signalcodec->m_horizontal_units);
            strcat(dst, "|");
        }

        CheckBox* tCheckBox = new CheckBox(this, dst, 20 + (int)(i / nrrows) * 300, 20 + (i % nrrows) * 20, 300, 18);
        tCheckBox->EID = m_checkboxes_start + i;
        if (m_vis->m_visible_channels.m_data[i] == 1)
            tCheckBox->SetState(true);
        tCheckBox->OnClick += &SetupChannels::CheckBoxes_OnClick;
    }
    m_button_selectNone = new CBitButton (width - 125 - 8 - 12, height - 29 - 24 - 20, 120, 24, "Select none", this);
    m_button_selectNone->OnClick += &SetupChannels::Button_SelectNone_OnClick;
    m_button_selectAll = new CBitButton (width - 125 - 8 - 12 - 112, height - 29 - 24 - 20, 110, 24, "Select all", this);
    m_button_selectAll->OnClick += &SetupChannels::Button_SelectAll_OnClick;
}

bool SetupChannels::Obj_OnClose()
{
    EmptyControls();
    return CControl_Base::Obj_OnClose();
}

SetupChannels::~SetupChannels()
{
    if (ParentObj)
    {
        if (ParentObj->ParentObj)
        {
            BringWindowToTop(ParentObj->ParentObj->hWnd);
            SetActiveWindow(ParentObj->ParentObj->hWnd);
        }
        BringWindowToTop(ParentObj->hWnd);
        SetActiveWindow(ParentObj->hWnd);
    }
    EmptyControls();
}

void SetupChannels::Obj_OnChanges (int chanibndx)
{
    BringWindowToTop(ParentObj->hWnd);
    SetActiveWindow(ParentObj->hWnd);
    for (unsigned int i = 0; i < OnChanges.m_size; i++)
        (ParentObj->*OnChanges.m_data[i])(this, chanibndx);
}

void SetupChannels::Button_SelectNone_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    for (unsigned int i = 0; i < Controls.m_size - 1; i++)
    {
        ((CheckBox*)Controls.m_data[i])->SetState(false);
        if (i < m_vis->m_visible_channels.m_size)
            m_vis->m_visible_channels.m_data[i] = 0;
        else
        {
            //cout << "Button_SelectNone_OnClick error:" << i << " size: " << m_vis->m_visible_channels.m_size << endl;
        }
    }
    Obj_OnChanges (0);
}

void SetupChannels::Button_SelectAll_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    for (unsigned int i = 0; i < Controls.m_size - 1; i++)
    {
        ((CheckBox*)Controls.m_data[i])->SetState(true);
        if (i < m_vis->m_visible_channels.m_size)
            m_vis->m_visible_channels.m_data[i] = 1;
        else
        {
            //cout << "Button_SelectAll_OnClick error:" << i << " size: " << m_vis->m_visible_channels.m_size << endl;
        }
    }
    Obj_OnChanges (0);
}

void SetupChannels::CheckBoxes_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    int btnindx = a_sender->EID - m_checkboxes_start;
    if (((CheckBox*)a_sender)->GetState())
        m_vis->m_visible_channels.m_data[btnindx] = 1;
    else
        m_vis->m_visible_channels.m_data[btnindx] = 0;
    Obj_OnChanges (0);
}

int SetupChannels::Obj_OnMessage (HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    int result = CControl_Base::Obj_OnMessage (a_hwnd, a_message, a_wparam, a_lparam);
    return result;
}

MagnifControlPanel::MagnifControlPanel(const char* caption, int posx, int posy, int width, int height, CControl_Base* parent)
    : CControl_Base(0, 0, caption, 0, posx, posy, width, height, parent, false)
{
    m_selector_btns_start  = 10000;
    m_magnif_scrolls_start = 20000;
    m_labels_start         = 30000;
    m_check_btn_start_indx    = 0;
//    LoadButton = new CBitButton    (0, 0, 15, 30, "load", this);
//    SaveButton = new CBitButton    (0, 0, 15, 30, "save", this);
}

void MagnifControlPanel::Obj_OnChanges (int chanibndx)
{
    for (unsigned int i = 0; i < OnChanges.m_size; i++)
        (ParentObj->*OnChanges.m_data[i])(this, chanibndx);
}

MagnifControlPanel::~MagnifControlPanel()
{
    EmptyControls();
}

void MagnifControlPanel::RebuildMagnifControls()
{
    EmptyControlsFromTo(m_selector_btns_start-1, m_selector_btns_start + 1000);
    EmptyControlsFromTo(m_magnif_scrolls_start-1, m_magnif_scrolls_start + 1000);
    EmptyControlsFromTo(m_labels_start-1, m_labels_start + 1000);
    if (m_vis)
    {
        int i = 0;
        bool waschecked = false;
        for (unsigned int j = 0; j < m_vis->m_visible_channels.m_size; j++)
        {
            if (m_vis->m_visible_channels.m_data[j] == 1)
            {
                char lpstext [20];
                sprintf(lpstext, "%u", (j + 1));
                CBitBtn* c1 = new CBitBtn (0, 0, 15, 30, lpstext, this, 0, 0, 0, 0, 0, m_selector_btns_start + i);
                c1->EID = m_selector_btns_start + i;
                if (m_check_btn_start_indx == j)
                {
                    c1->Check();
                    waschecked = true;
                }
                c1->OnClick += &MagnifControlPanel::SelectionBtns_OnClick;
                CControl_Base* c2 = new CControl_Base(0, "msctls_trackbar32", lpstext, 0x50000000, 0, 0, 15, 50, this, false, 0, 0, 0, false, m_magnif_scrolls_start + j);
                c2->EID = m_magnif_scrolls_start + j;
                SendMessage(c2->hWnd, TBM_SETPOS, true, 50);
                string label(m_codec->m_labels.m_data[j].s);
                if (strlen(m_codec->m_vertical_units.m_data[j].s))
                {
                    label += " [";
                    label += m_codec->m_vertical_units.m_data[j].s;
                    label += "]";
                }
                CBitBtn* c3 = new CBitBtn (0, 0, 15, 20, label.c_str(), this, 0, 0, 0, 0, 0, m_labels_start + i);
                c3->EID = m_labels_start + i;
                i++;
            }
        }
        if ((!waschecked) && (i) && (Controls.m_size > 2))
        {
            ((CBitBtn* )Controls.m_data[2])->Check();
            m_check_btn_start_indx = 0;
        }
    }
    RECT rc;
    GetClientRect(hWnd, &rc);
    Obj_OnResize(NOCHANGE, NOCHANGE, rc.right, rc.bottom);
}

bool MagnifControlPanel::Obj_OnClose()
{
    EmptyControls();
    return CControl_Base::Obj_OnClose();
}

void MagnifControlPanel::SelectionBtns_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    int btnindx = a_sender->EID - m_selector_btns_start;
    int i = 0;
    for (unsigned int j = 0; j < Controls.m_size; j++)
        if ((Controls.m_data[j]->EID >= m_selector_btns_start) && (Controls.m_data[j]->EID < m_magnif_scrolls_start))
        {
            if (i == btnindx)
            {
                ((CBitBtn* )Controls.m_data[j])->Check();
                m_check_btn_start_indx = i;
            }
            else
            {
                ((CBitBtn* )Controls.m_data[j])->UnCheck();
                Controls.m_data[j]->Refresh();
            }
            i++;
        }
}

void MagnifControlPanel::Obj_OnResize(int x, int y, int width, int height)
{
    height -= 15;
    CControl_Base::Obj_OnResize(x, y, width, height);
    int i = 0;
    if (m_vis)
        for (unsigned int j = 0; j < Controls.m_size; j++)
            if ((Controls.m_data[j]->EID >= m_selector_btns_start) && (Controls.m_data[j]->EID < m_magnif_scrolls_start))
            {
                float fi = height;
                float fnr = m_vis->GetActiveChNr();
                fi = fi / fnr;
                int h = (int)(fi);
                fnr = i;
                fi = fi * fnr;
                MoveWindow(Controls.m_data[j]->hWnd, width - 17, (int)fi, 17, h, false);
                MoveWindow(Controls.m_data[j + 1]->hWnd, 0, (int)fi - 1 + 21, width - 17, h + 1 - 21, false);
                MoveWindow(Controls.m_data[j + 2]->hWnd, 0, (int)fi, width - 17, 20, true);
                Controls.m_data[j]->Refresh();
                InvalidateRect(Controls.m_data[j + 1]->hWnd, NULL, false);
                i++;
            }
//    LoadButton->Move(0, height, width / 2, 14, true);
//    SaveButton->Move(width / 2, height, width / 2, 14, true);
}

int MagnifControlPanel::Obj_OnMessage(HWND a_hwnd, int a_message, int a_wparam, int a_lparam)
{
    int result = CControl_Base::Obj_OnMessage(a_hwnd, a_message, a_wparam, a_lparam);
    if (a_message == WM_HSCROLL)
    {
        CControl_Base* sender = 0;
        for (unsigned int i = 0; i < Controls.m_size; i++)
            if ((HWND) a_lparam == Controls.m_data[i]->hWnd)
                sender = Controls.m_data[i];
        if (sender)
            if (m_vis)
                if ((sender->EID >= m_magnif_scrolls_start) && (sender->EID < m_magnif_scrolls_start + 10000))
                {
                    if (m_vis->m_seeking_allowed)
                    {
                        m_vis->m_seeking_allowed = false;
                        int controlindx = sender->EID - m_magnif_scrolls_start;
                        int pos1 = SendMessage((HWND) a_lparam, TBM_GETPOS, 0, 0);
                        m_vis->m_magnifiers.m_data[controlindx] = (float)pos1 * 0.001;
                        Obj_OnChanges (sender->EID - m_magnif_scrolls_start);
                    }
                }
    }
    return result;
}

void CSignalDisplay::ResetCodec(ISignalCodec* a_codec)
{
    m_codec = a_codec;
    m_plot_panel->m_codec = a_codec;
    m_magnif_control_panel->m_codec = m_codec;
}

bool CSignalDisplay::Obj_OnClose()
{
    m_magnif_control_panel->EmptyControls();
    return CControl_Base::Obj_OnClose();
}

CSignalDisplay::CSignalDisplay(const char* caption, ISignalCodec* a_data_codec, int fitWidth, int displayType, char* a_align, char* a_grid, CControl_Base* mdiclientparent, CControl_Base* eparent)
{
    m_fit_width_type = (EFitWidthType)fitWidth;
    m_display_type = (EDisplayType)displayType;

    if (m_display_type == EDisplayType_2D_map)
        m_vis.m_line_surface_table = 1;
    else if (m_display_type == EDisplayType_2D_map_surface)
        m_vis.m_line_surface_table = 3;
    else if (m_display_type == EDisplayType_value_list)
        m_vis.m_line_surface_table = 4;
    else if (m_display_type == EDisplayType_xy_plot)
        m_vis.m_line_surface_table = 5;

    if (a_grid)
    {
        if (strlen(a_grid) > 0 && a_grid[0])
            m_vis.m_show_grid = (1 << (a_grid[0] - '0')) >> 1;
        if (strlen(a_grid) > 1 && a_grid[1])
            m_vis.m_show_grid |= (32 << (a_grid[1] - '0')) >> 1;
        if (strlen(a_grid) > 2)
            m_vis.m_line_style = (a_grid[2] - '0');
        if (strlen(a_grid) > 3)
            m_vis.m_line_width = (a_grid[3] - '0');
    }

    m_codec = a_data_codec;
    CreateComponent(caption, mdiclientparent, eparent);

    if (a_align)
    {
        if (a_align[0] == 'N')
        {
            m_vis.m_auto_offset_adjust = 0;
            SetWindowText(m_button_auto_offset_adjust->hWnd, "N"); // no offset
        }
        else if (a_align[0] == 'A' && a_align[0] == '1')
        {
            m_vis.m_auto_offset_adjust = 1;
            SetWindowText(m_button_auto_offset_adjust->hWnd, "A1"); // auto offset calculated from visible min and max
        }
        else if (a_align[0] == 'A' && a_align[0] == '2')
        {
            m_vis.m_auto_offset_adjust = 2;
            SetWindowText(m_button_auto_offset_adjust->hWnd, "A2"); // auto offset calculated from visible mean
        }
        else if (a_align[0] == 'C')
        {
            m_vis.m_auto_offset_adjust = 3;
            SetWindowText(m_button_auto_offset_adjust->hWnd, "C"); // zero line is centered
        }
        else if (a_align[0] == 'B')
        {
            m_vis.m_auto_offset_adjust = 4;
            SetWindowText(m_button_auto_offset_adjust->hWnd, "B"); // zero line is on the bottom
        }
    }

    SetDefaults();

    if (m_fit_width_type == EFitWidthType_fitWidth)
    {
        for (unsigned int i = 0; i < m_codec->m_sample_rates.m_size; i++)
            if (m_vis.m_visible_channels.m_data[i] == 1)
                m_vis.m_horizontal_units_to_display = m_codec->m_total_samples.m_data[i] / m_codec->m_sample_rates.m_data[i];
        ZoomButtons_OnClick(m_button_zoom_out, 0, 0, 0);
        ApproxGlobalMagnif();
        Replot();
    }
}

CSignalDisplay::CSignalDisplay(const char* caption, CControl_Base* mdiclientparent, CControl_Base* eparent)
{
    CreateComponent(caption, mdiclientparent, eparent);
}

void CSignalDisplay::CreateComponent(const char* caption, CControl_Base* mdiclientparent, CControl_Base* eparent)
{
    m_setup_channels = 0;
    m_now_scrolling = false;
    if (mdiclientparent)
    {
        Create(0x02000000, 0, caption, WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, mdiclientparent, true, 0, 0, 0, true);
        ParentObj = eparent;
        mdiclientparent->Controls -= this;
        eparent->Controls += this;
    }
    else
        Create(0x02000000, 0, caption, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, eparent, true);

    MoveButton = MK_LBUTTON;
    m_selector_btns_start  = 10000;
    m_magnif_scrolls_start = 20000;

    RECT Rect;
    GetClientRect(hWnd, &Rect);
    m_scrollbar = new CScrollBar(0, Rect.bottom - 15, Rect.right, 15, this, HORIZONTAL);
    m_scrollbar->OnChange += &CSignalDisplay::Scrollbar_OnChange;
    m_scrollbar->OnEndScroll += &CSignalDisplay::Scrollbar_OnEndScroll;

    m_plot_panel = new PlotPanel("this", 100, 100, 200, 200, this);
    m_plot_panel->m_vis = &m_vis;
    m_plot_panel->m_codec = m_codec;
    m_plot_panel->OnMouseMove += &CSignalDisplay::PlotPanel_OnDataHover;
    m_plot_panel->OnMouseDown += &CSignalDisplay::PlotPanel_OnMouseDown;

    m_button_magnif  = new CBitButton    (0, 0, 15, 25, "+", this);
    m_button_shrink  = new CBitButton    (0, 0, 15, 25, "-", this);
    m_button_zoom_in  = new CBitButton    (0, 0, 15, 25, "+", this);
    m_button_setup_visible_channels = new CBitBtn    (0, 0, 15, 25, "Se", this);
    m_button_zoom_out = new CBitButton    (0, 0, 15, 25, "-", this);
    m_button_auto_offset_adjust = new CBitBtn (0, 0, 15, 25, "A1", this);
    m_button_auto_offset_adjust->tristate = 0;

    m_button_hor_grid  = new CBitButton    (0, 0, 15, 25, "GH", this);
    m_button_ver_grid  = new CBitButton    (0, 0, 15, 25, "GV", this);
    m_button_line_surf_table  = new CBitButton    (0, 0, 15, 25, "Su", this);

    m_button_line_surf_table->OnClick += &CSignalDisplay::ButtonLineSurfTable_OnClick;

    m_button_hor_grid->OnClick += &CSignalDisplay::ButtonGrid_OnClick;
    m_button_ver_grid->OnClick += &CSignalDisplay::ButtonGrid_OnClick;

    m_button_magnif->OnClick += &CSignalDisplay::ButtonMagnif_OnClick;
    m_button_shrink->OnClick += &CSignalDisplay::ButtonMagnif_OnClick;

    m_button_zoom_in->OnClick += &CSignalDisplay::ZoomButtons_OnClick;
    m_button_zoom_out->OnClick += &CSignalDisplay::ZoomButtons_OnClick;

    m_button_setup_visible_channels->OnClick += &CSignalDisplay::SetupVisibleChannels_OnClick;
    m_button_auto_offset_adjust->OnClick += &CSignalDisplay::ButtonAutoOffsetAdjust_OnClick;

    m_magnif_control_panel = new MagnifControlPanel("no cap", 50, 50, 100, 300, this);
    m_magnif_control_panel->m_vis = &m_vis;
    m_magnif_control_panel->m_codec = m_codec;
    m_magnif_control_panel->OnChanges += &CSignalDisplay::MagnifControlPanel_On_Changes;

    m_v_splitter = new CSplitter(0, 0, Rect.right - 15, Rect.bottom, this, VERTICAL);
    m_v_splitter->MinSizes = -1;
    m_v_splitter->SpWeight = 10;
    m_v_splitter->SpSpace  = 0;
    SetWindowText(m_v_splitter->hWnd, "<");
    m_v_splitter->SetWindows(m_magnif_control_panel, m_plot_panel);
    m_splitterlastpos = 100;
    m_v_splitter->SetPos(0);

    m_v_splitter->OnClick += &CSignalDisplay::SnapButton1_OnClick;
    PostMessage(m_v_splitter->hWnd, WM_LBUTTONDOWN, 5, 5);
    PostMessage(m_v_splitter->hWnd, WM_LBUTTONUP, 5, 5);

    Obj_OnResize(0, 0, Rect.right, Rect.bottom);
}

void CSignalDisplay::SetupChannels_OnChanges(CControl_Base* a_sender, int indx)
{
    CheckHorizontalUnitsToDisplay();
    RebuildVisibleBuffer();
}

void CSignalDisplay::SnapButton1_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    if (m_splitterlastpos == m_v_splitter->GetPos())
    {
        if (m_v_splitter->GetPos())
        {
            m_v_splitter->SetPos(0);
            SetWindowText(m_v_splitter->hWnd, ">");
        }
    }
    else
    {
        if (!m_v_splitter->GetPos())
            m_v_splitter->SetPos(m_splitterlastpos);
        else
            m_splitterlastpos = m_v_splitter->GetPos();
        SetWindowText(m_v_splitter->hWnd, "<");
    }
    SendMessage(m_v_splitter->hWnd, WM_KILLFOCUS, 0, 0);
}

void CSignalDisplay::DataHover(int x, int y, int& a_data_channel, double& a_val_x, double& a_val_y, double& a_data_val, char* a_info)
{
    a_info[0] = 0;
    RECT Rect;
    GetClientRect(m_plot_panel->hWnd, &Rect);

    if ((m_vis.m_line_surface_table == 1) || (m_vis.m_line_surface_table == 2) || (m_vis.m_line_surface_table == 3)) /// Surface
    {
        bool display_y_axis = false;
        if (strlen(m_codec->m_surface2D_vert_units))
        {
            x -= 14;
            Rect.left += 14;
            display_y_axis = true;
        }
        a_val_x = (double)x * (double)(m_plot_panel->m_stopphysx - m_plot_panel->m_startphysx) / (double)(Rect.right - Rect.left) + m_plot_panel->m_startphysx;
        a_val_y = (double)y * (double)(m_codec->m_surface2D_vert_axis_min - m_codec->m_surface2D_vert_axis_max) / (double)(Rect.bottom - Rect.top - 14) + m_codec->m_surface2D_vert_axis_max;

        if (x >= 0 && y >= 0 && y < (int)m_plot_panel->dmx1.m_width && x < (int)m_plot_panel->dmx1.m_height)
        {
            a_data_val = m_plot_panel->dmx1.m_data[x][y];
            if (display_y_axis)
            {
                a_data_channel = -1;
                sprintf(a_info, R"({"x": %f, "y": %f, "val": %f})", a_val_x, a_val_y, a_data_val);
            }
            else
            {
                int height = Rect.bottom - Rect.top - 14;
                a_data_channel = (y * m_vis.m_visible_buffer.m_widths.m_size) / height;
                sprintf(a_info, R"({"x: %f, "ch": %d, "val": %f})", a_val_x, a_data_channel, a_data_val);
            }
        }
    }
    else /// multiline
    {
        unsigned int height = Rect.bottom - Rect.top - 14;
        a_data_channel = (y * m_vis.m_visible_buffer.m_widths.m_size) / height;
        if (a_data_channel >= (int)m_vis.m_visible_buffer.m_widths.m_size)
            a_data_channel = m_vis.m_visible_buffer.m_widths.m_size - 1;
        m_vis.m_hover_x = x;
        m_vis.m_hover_ch = a_data_channel;
        int realchannelindex = 0;
        for (unsigned int x = 0; x < m_vis.m_visible_channels.m_size; x++)
            if (m_vis.m_visible_channels.m_data[x] == 1)
                if (a_data_channel == realchannelindex++)
                {
                    realchannelindex = x;
                    break;
                }
        if (m_vis.GetActiveChNr() == 1 || m_vis.m_auto_offset_adjust == 3 || m_vis.m_auto_offset_adjust == 4)
        {
            x -= 14;
            Rect.left += 14;
        }
        if ((int)m_vis.m_visible_buffer.m_widths.m_size > a_data_channel && a_data_channel >= 0)
        {
            unsigned int indx = (double)x * (double)(m_vis.m_visible_buffer.m_widths.m_data[a_data_channel] - 1) / (double)(Rect.right - Rect.left);
            a_val_x = (double)x * (double)(m_plot_panel->m_stopphysx - m_plot_panel->m_startphysx) / (double)(Rect.right - Rect.left) + m_plot_panel->m_startphysx;
            a_val_y = -1;
            if (indx >= 0 && indx < m_vis.m_visible_buffer.m_widths.m_data[a_data_channel])
            {
                m_plot_panel->Paint(true);
                InvalidateRect(m_plot_panel->hWnd, 0, true);
                a_data_val = m_vis.m_visible_buffer.m_data[a_data_channel][indx];

                if (m_vis.m_visible_channels.m_size == 1)
                    sprintf(a_info, R"({"x": %f, "val": %f)", a_val_x, a_data_val);
                else
                    sprintf(a_info, R"({"x": %f, "channel": %d "val": %f)", a_val_x, realchannelindex, a_data_val);
            }
            int startstop;
            char label[256];
            if (m_plot_panel->TMarkersHasPoint(x, 0, &startstop, label))
            {
                strcat(a_info, R"(, "label": ")");
                strcat(a_info, label);
                strcat(a_info, "\"");
            }
            strcat(a_info, "}");
        }
    }
}

void CSignalDisplay::PlotPanel_OnDataHover(CControl_Base* a_sender, int button, int x, int y)
{
    char info[128];
    int data_channel;
    double val_x, val_y, data_val;
    DataHover(x, y, data_channel, val_x, val_y, data_val, info);
    for (unsigned int i = 0; i < OnDataHover.m_size; i++)
        (ParentObj->*OnDataHover.m_data[i])(this, info);
}

void CSignalDisplay::PlotPanel_OnMouseDown(CControl_Base* a_sender, int button, int x, int y, int client)
{
    char info[128];
    int data_channel;
    double val_x, val_y, data_val;
    DataHover(x, y, data_channel, val_x, val_y, data_val, info);
    for (unsigned int i = 0; i < OnDataEvent.m_size; i++)
        (ParentObj->*OnDataEvent.m_data[i])(this, button, val_x, val_y, data_channel, data_val, info);
}

int CSignalDisplay::Obj_OnSizing(RECT* newsizerect, int fwside)
{
    if ((newsizerect->bottom - newsizerect->top) < 125)
        newsizerect->bottom = newsizerect->top + 125;
    if ((newsizerect->right - newsizerect->left) < 145)
        newsizerect->right = newsizerect->left + 145;
    return CControl_Base::Obj_OnSizing(newsizerect, fwside);
}

void CSignalDisplay::Obj_OnResize(int x, int y, int width, int height)
{
    CControl_Base::Obj_OnResize(x, y, width, height);
    if (HasControl(m_scrollbar))
    {
        int husz = 30;

        MoveWindow(m_scrollbar->hWnd, 0, height - 15, width - 40, 15, true);
        m_scrollbar->Resize(width - husz - husz, 15);

        m_v_splitter->Move(0, 0, width - 15, height - 15);
        int harminc = 25;
        m_button_hor_grid->Move (width - 15, 0, NOCHANGE, NOCHANGE, true);
        m_button_ver_grid->Move(width - 15, 25, NOCHANGE, NOCHANGE, true);
        m_button_line_surf_table->Move(width - 15, 25 + 25, NOCHANGE, NOCHANGE, true);

        m_button_zoom_in->Move (width - husz - husz, height - 15, husz, 15, true);
        m_button_zoom_out->Move(width - husz, height - 15, husz, 15, true);

        m_button_magnif->Move (width - 15, height - 15 - harminc - harminc, NOCHANGE, NOCHANGE, true);
        m_button_shrink->Move (width - 15, height - 15 - harminc, NOCHANGE, NOCHANGE, true);
        m_button_setup_visible_channels->Move (width - 15, height - 15 - harminc - harminc - harminc, NOCHANGE, NOCHANGE, true);
        m_button_auto_offset_adjust->Move (width - 15, height - 15 - harminc - harminc - harminc - harminc, NOCHANGE, NOCHANGE, true);
        m_button_shrink->Refresh();
        m_button_magnif->Refresh();
        m_button_setup_visible_channels->Refresh();
        m_button_auto_offset_adjust->Refresh();

        m_button_zoom_in->Refresh();
        m_button_zoom_out->Refresh();
    }
}

void CSignalDisplay::ButtonLineSurfTable_OnClick(CBitBtn* a_sender, int button, int x, int y)
{
    m_vis.m_line_surface_table++;
    if (m_vis.m_line_surface_table > 5)
        m_vis.m_line_surface_table = 0;
    if (m_vis.m_line_surface_table == 4)
        m_scrollbar->SetLineStep(1.00);
    else
        m_scrollbar->SetLineStep(10.0);
    Replot();
}

void CSignalDisplay::ButtonAutoOffsetAdjust_OnClick(CBitBtn* a_sender, int button, int x, int y)
{
	if (m_vis.m_line_surface_table != 5) // xy_plot
        m_vis.m_line_surface_table = 0;
    m_vis.m_auto_offset_adjust++;
    if (m_vis.m_auto_offset_adjust > 4)
        m_vis.m_auto_offset_adjust = 0;
    if (m_vis.m_auto_offset_adjust == 0)
        SetWindowText(a_sender->hWnd, "N"); // no offset
    if (m_vis.m_auto_offset_adjust == 1)
        SetWindowText(a_sender->hWnd, "A1"); // auto offset calculated from visible min and max
    if (m_vis.m_auto_offset_adjust == 2)
        SetWindowText(a_sender->hWnd, "A2"); // auto offset calculated from visible mean
    if (m_vis.m_auto_offset_adjust == 3)
        SetWindowText(a_sender->hWnd, "C"); // zero line is centered
    if (m_vis.m_auto_offset_adjust == 4)
        SetWindowText(a_sender->hWnd, "B"); // zero line is on the bottom
    Replot();
}

void CSignalDisplay::SetupChannels_OnDestroy(CControl_Base* a_sender)
{
    if (HasControl(m_button_setup_visible_channels))
    {
        m_button_setup_visible_channels->UnCheck();
        m_button_setup_visible_channels->Refresh();
    }
}

void CSignalDisplay::SetupVisibleChannels_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    if (m_codec)
    {
        if (!HasControl(m_setup_channels))
        {
            m_setup_channels = new SetupChannels("Setup visible channels", 300, 300, 1200, 430, this, &m_vis, m_codec);
            m_setup_channels->OnChanges += &CSignalDisplay::SetupChannels_OnChanges;
            m_setup_channels->OnDestroy += &CSignalDisplay::SetupChannels_OnDestroy;
            m_button_setup_visible_channels->Check();
        }
        else
            DestroyWindow(m_setup_channels->hWnd);
    }
}

void CSignalDisplay::ButtonGrid_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    if (a_sender == m_button_hor_grid)
    {
        if (m_vis.m_show_grid & 1)
        {
            m_vis.m_show_grid -= 1;
            m_vis.m_show_grid += 2;
        }
        else
        {
            if (m_vis.m_show_grid & 2)
            {
                m_vis.m_show_grid -= 2;
            }
            else
            {
                m_vis.m_show_grid += 1;
            }
        }
    }
    else
    {
        if (m_vis.m_show_grid & 32)
        {
            m_vis.m_show_grid -= 32;
            m_vis.m_show_grid += 64;
        }
        else
        {
            if (m_vis.m_show_grid & 64)
            {
                m_vis.m_show_grid -= 64;
            }
            else
            {
                m_vis.m_show_grid += 32;
            }
        }
    }
    Replot();
}

void CSignalDisplay::ButtonMagnif_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    if (a_sender == m_button_magnif)
        m_vis.m_global_magnifier *= 1.2;
    else
        m_vis.m_global_magnifier /= 1.2;
    Replot();
}

void CSignalDisplay::CheckHorizontalUnitsToDisplay()
{
    if (m_codec)
    {
        double minsamplerate = MAXINT;
        for (unsigned int i = 0; i < m_codec->m_sample_rates.m_size; i++)
            if (m_vis.m_visible_channels.m_data[i] == 1)
                if (minsamplerate > m_codec->m_sample_rates.m_data[i])
                    minsamplerate = m_codec->m_sample_rates.m_data[i];
        if (minsamplerate)
            if (m_vis.m_horizontal_units_to_display <= 2.0 / minsamplerate)
                m_vis.m_horizontal_units_to_display = 2.0 / minsamplerate + 0.0000001;
        for (unsigned int i = 0; i < m_codec->m_sample_rates.m_size; i++)
            if (m_vis.m_visible_channels.m_data[i] == 1)
                if (m_vis.m_horizontal_units_to_display > m_codec->m_total_samples.m_data[i] / m_codec->m_sample_rates.m_data[i])
                    m_vis.m_horizontal_units_to_display = m_codec->m_total_samples.m_data[i] / m_codec->m_sample_rates.m_data[i];
    }
}

void CSignalDisplay::ZoomButtons_OnClick(CControl_Base* a_sender, int button, int x, int y)
{
    if (a_sender == m_button_zoom_in)
        m_vis.m_horizontal_units_to_display /= 2;
    else
        m_vis.m_horizontal_units_to_display *= 2;
    CheckHorizontalUnitsToDisplay();
    RebuildVisibleBuffer();
    Replot();
    if (m_vis.m_line_surface_table == 2)
        m_scrollbar->SetLineStep(1.00);
    else
        m_scrollbar->SetLineStep(10.0);
}

void CSignalDisplay::MagnifControlPanel_On_Changes (CControl_Base* a_sender, int chanibndx)
{
    Replot();
}

void CSignalDisplay::Replot(bool a_rebuild_visual_buffer)
{
    if (a_rebuild_visual_buffer)
        RebuildVisibleBuffer();
    m_plot_panel->Paint();
    m_plot_panel->Refresh();
}

void CSignalDisplay::Obj_OnMouseDown(int button, int x, int y, int client)
{
    if (client && button == 2)
    {}
    CControl_Base::Obj_OnMouseDown(button, x, y, client);
}

void CSignalDisplay::Scrollbar_OnEndScroll(CControl_Base* a_sender, double pos, double lastpos)
{
    m_now_scrolling = false;
}

void CSignalDisplay::Scrollbar_OnChange(CControl_Base* a_sender, double curpos, int button)
{
    if (button)
        m_now_scrolling = true;
    if (m_codec)
        if (m_codec->m_total_samples.m_size)
            if (m_vis.m_seeking_allowed || (m_scrollbar->GetMaxF() == curpos) || curpos == m_scrollbar->GetMinF() || curpos == 0)
            {
                m_vis.m_seeking_allowed = false;
                unsigned int*start = new unsigned int[m_codec->m_total_samples.m_size];
                unsigned int*nrelemnts = new unsigned int[m_codec->m_total_samples.m_size];
                float maxsamplerate = 0;
                int imaxindx = -1;
                int j = 0;

                for (unsigned int i = 0; i < m_codec->m_total_samples.m_size; i++)
                {
                    if (m_vis.m_visible_channels.m_data[i] == 1)
                    {
                        if (maxsamplerate < m_codec->m_total_samples.m_data[i] /*/ m_codec->m_sample_rates.m_data[i]*/)
                        {
                            maxsamplerate = m_codec->m_total_samples.m_data[i] /*/ m_codec->m_sample_rates.m_data[i]*/;
                            imaxindx = i;
                        }
                        j++;
                    }
                }

                if (imaxindx == -1)
                {
                    delete[]start;
                    delete[]nrelemnts;
                    return;
                }

                j = 0;
                if (!m_vis.m_visible_buffer.m_widths.m_size)
                {
                    delete[]start;
                    delete[]nrelemnts;
                    printf("Error: CSignalDisplay::Scrollbar_OnChange: m_visible_buffer has not been built. \n");
                    return;
                }
                for (unsigned int i = 0; i < m_codec->m_total_samples.m_size; i++)
                {
                    if (m_vis.m_visible_channels.m_data[i] == 1)
                    {
                        start[i] = (int)(curpos * m_codec->m_sample_rates.m_data[i] / m_codec->m_sample_rates.m_data[imaxindx]);
                        nrelemnts[i] = m_vis.m_visible_buffer.m_widths.m_data[j];
                        j++;
                    }
                    else
                    {
                        start[i] = (int)(curpos * m_codec->m_sample_rates.m_data[i] / m_codec->m_sample_rates.m_data[imaxindx]);
                        nrelemnts[i] = (int)ceil((float)m_codec->m_sample_rates.m_data[i] * m_vis.m_horizontal_units_to_display);
                        if (nrelemnts[i]  > m_codec->m_total_samples.m_data[i])
                            nrelemnts[i]  = m_codec->m_total_samples.m_data[i];
                    }
                }

                m_codec->GetDataBlock(m_vis.m_visible_buffer.m_data, start, nrelemnts, m_vis.m_visible_channels.m_data);
                m_plot_panel->m_startphysx = 0;
                m_plot_panel->m_stopphysx = 1;

                m_plot_panel->m_startphysx = (double)start[imaxindx] / (m_codec->m_sample_rates.m_data[imaxindx]) + m_codec->m_horizontal_scale_start;
                m_plot_panel->m_stopphysx = (double)(start[imaxindx] + nrelemnts[imaxindx] - 1/*-!!!*/) / (m_codec->m_sample_rates.m_data[imaxindx]) + m_codec->m_horizontal_scale_start;

                delete[]start;
                delete[]nrelemnts;
                m_codec->m_marker_list.GetTMarkers(&m_vis.m_marker_refs, m_plot_panel->m_startphysx, m_plot_panel->m_stopphysx);
                Replot();
            }
    double pos_percentage = (m_scrollbar->GetPosF() - m_scrollbar->GetMinF()) * 100.0 / (m_scrollbar->GetMaxF() - m_scrollbar->GetMinF());
    for (unsigned int i = 0; i < OnDataScrolled.m_size; i++)
        (ParentObj->*OnDataScrolled.m_data[i])(this, pos_percentage);
}

void CSignalDisplay::SetScrollPosPercentage(double pos_percentage)
{
    m_scrollbar->SetPos((pos_percentage * (m_scrollbar->GetMaxF() - m_scrollbar->GetMinF())) / 100.0 + m_scrollbar->GetMinF(), true);
}

CSignalDisplay::~CSignalDisplay()
{
    if (HasControl(m_setup_channels))
        DestroyWindow(m_setup_channels->hWnd);
}

void CSignalDisplay::ApproxGlobalMagnif()
{
    bool is_bottom_or_center_aligned = m_vis.m_auto_offset_adjust == 4 || m_vis.m_auto_offset_adjust == 3;
    int j = -1;
    m_vis.m_global_magnifier = 0;
    double MaxVal = -MAXINT;
    double MinVal = MAXINT;
    for (unsigned int x = 0; x < m_vis.m_visible_channels.m_size; x++)
        if (m_vis.m_visible_channels.m_data[x] == 1)
        {
            j++;
            if (!m_vis.m_visible_buffer.m_widths.m_size)
                break;
            for (unsigned int i = 0; i < m_vis.m_visible_buffer.m_widths.m_data[j]; i++)
            {
                if (MaxVal < m_vis.m_visible_buffer.m_data[j][i])
                    MaxVal = m_vis.m_visible_buffer.m_data[j][i];
                if (MinVal > m_vis.m_visible_buffer.m_data[j][i])
                    MinVal = m_vis.m_visible_buffer.m_data[j][i];
            }
        }
    if (MaxVal - MinVal)
        m_vis.m_global_magnifier = 8.0 / (MaxVal - MinVal) / (is_bottom_or_center_aligned ? 0.5 : m_vis.m_visible_channels.m_size);
    if (m_vis.m_global_magnifier < 0.0004)
        m_vis.m_global_magnifier = 0.0004;
}

void CSignalDisplay::ResetHorizontal()
{
    m_vis.m_horizontal_units_to_display = 5;
    ZoomButtons_OnClick(m_button_zoom_in, 0, 0, 0);
}

void CSignalDisplay::SetDefaults()
{
    if (m_now_scrolling)
        return;
    if (m_codec)
    {
        m_vis.m_visible_channels.Rebuild (m_codec->m_total_samples.m_size);
        m_vis.m_magnifiers.Rebuild     (m_codec->m_total_samples.m_size);
        m_vis.m_flips.Rebuild          (m_codec->m_total_samples.m_size);

        for (unsigned int i = 0; i < m_codec->m_total_samples.m_size; i++)
        {
            m_vis.m_magnifiers.m_data[i] = 0.05;
            m_vis.m_flips.m_data[i] = 1;
            m_vis.m_visible_channels.m_data[i] = 1;
        }
        ResetHorizontal();
        ApproxGlobalMagnif();
        Replot();
        SetWindowText(hWnd, m_codec->m_varname);
    }
}

void CSignalDisplay::RebuildDataWindow(bool a_jump_to_end)
{
    if (m_now_scrolling)
        return;
    if (m_codec)
    {
        if (m_vis.m_visible_channels.m_size != m_codec->m_total_samples.m_size)
            SetDefaults();
        float maxsamplerate = 0;
        int jmaxindx = -1;
        int imaxindx = -1;
        CVector <unsigned int> temwidhts;
        temwidhts.Rebuild (m_vis.GetActiveChNr());
        int j = 0;

        for (unsigned int i = 0; i < m_codec->m_total_samples.m_size; i++)
        {
            if (m_vis.m_visible_channels.m_data[i] == 1)
            {
                temwidhts.m_data[j] = (int)ceil((float)m_codec->m_sample_rates.m_data[i] * m_vis.m_horizontal_units_to_display);
                if (temwidhts.m_data[j] > m_codec->m_total_samples.m_data[i])
                    temwidhts.m_data[j] = m_codec->m_total_samples.m_data[i]; // double correction
                if (maxsamplerate < m_codec->m_total_samples.m_data[i] /*/ m_codec->m_sample_rates.m_data[i]*/)
                {
                    maxsamplerate = m_codec->m_total_samples.m_data[i] /*/ m_codec->m_sample_rates.m_data[i]*/;
                    jmaxindx = j;
                    imaxindx = i;
                }
                j++;
            }
        }

        m_vis.m_seeking_allowed = true;
        if ((m_codec) && (jmaxindx > -1))
        {
            m_vis.m_visible_buffer.Rebuild(temwidhts.m_size, temwidhts.m_data);
            m_vis.m_points_ref.reserve((int)(m_codec->m_sample_rates.m_data[imaxindx] * 1 * m_vis.m_horizontal_units_to_display) * 2 + 2); //!!!+1 think
            m_scrollbar->SetRange(0, m_codec->m_total_samples.m_data[imaxindx] - m_vis.m_visible_buffer.m_widths.m_data[jmaxindx]);
            m_scrollbar->SetLineStep(10.0);
            m_scrollbar->SetPageStep(m_scrollbar->GetLineStep() * 20.0);
            if (a_jump_to_end)
                m_scrollbar->SetPos(m_codec->m_total_samples.m_data[imaxindx] - m_vis.m_visible_buffer.m_widths.m_data[jmaxindx]);
            else
                m_scrollbar->SetPos(m_scrollbar->ActualPos);
        }
        else
        {
            Replot();
        }
    }
}

void CSignalDisplay::RebuildVisibleBuffer()
{
    if (m_codec)
    {
        RebuildDataWindow();
        if (m_codec->m_total_samples.m_size <= 40)
            m_magnif_control_panel->RebuildMagnifControls();
        else if (m_splitterlastpos == m_v_splitter->GetPos())
            SnapButton1_OnClick(0, 0, 0, 0);
    }
}

void CSignalDisplay::SetupVisibleChannels(unsigned int* a_channels)
{
    for (unsigned int i = 0; i < m_vis.m_visible_channels.m_size; i++)
    {
        m_vis.m_visible_channels.m_data[i] = a_channels[i];
    }
    ZoomButtons_OnClick(m_button_zoom_out, 0, 0, 0);
    ApproxGlobalMagnif();
    Replot();
}
