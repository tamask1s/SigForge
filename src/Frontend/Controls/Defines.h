#ifndef __typesandconsts
#define __typesandconsts

#define CLASS_NAME  "CControl_BaseCLASS"
#define CLASS_NAME_TRANS  "CLASS_TRANSCControl_Base"

#define HORIZONTAL  16
#define VERTICAL    32

#define AUTO        -1
#define FIXED       -2
#define DEFAULT     -3
#define NOCHANGE    -4

#define MAXIMUMSIZE -1
#define MINIMUMSIZE -2

#define MAXINT      2147483647
#define MININT      -2147483647

#define NOREDRAW    -1
#define ALLLAYERS   -2

typedef struct tagWINDOWRECT
{
    int left;
    int top;
    int width;
    int height;
} WINDOWRECT, *PWINDOWRECT, *LPWINDOWRECT;

#define POPUP_STYLES   (WS_POPUP |WS_CLIPCHILDREN)
#define POPUP_EXSTYLES (WS_EX_TOOLWINDOW)
#define CHILD_STYLES   (WS_CHILD |WS_CLIPCHILDREN)
#define CHILD_EXSTYLES (0)

#define TOP	         1
#define LEFT	     2
#define BOTTOM	     4
#define RIGHT	     8

#define TOPLEFT      3
#define TOPRIGHT     9
#define BOTTOMLEFT   6
#define BOTTOMRIGHT	 12
#define TOPBOTTOM    5
#define LEFTRIGHT	 10

#define APPSTARTING	 0
#define ARROW	     1
#define CROSS	     2
#define IBEAM	     3
#define ICON	     4
#define NO	         5
#define SIZE         6
#define SIZEALL	     7
#define SIZENESW     8
#define SIZENS	     9
#define SIZENWSE     10
#define SIZEWE	     11
#define UPARROW	     12
#define WAIT	     13
#define HAND         14

#define CursorVectorSize    20
#endif
