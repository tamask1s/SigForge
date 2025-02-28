#include <string.h>
#include <iostream>
using namespace std;

#include "stringutils.h"

void SplineDeriv2(double* x, double* y, int n, double yp1, double ypn, double* y2)
{
    int i, k;
    float p, qn, sig, un;
    float opx2mx1, opp;
    float u [n];
    if (yp1 > 0.99e30)
        y2[1] = u[1] = 0.0;
    else
    {
        y2[1] = -0.5;
        opx2mx1 = 1 / (x[2] - x[1]);
        u[1] = (3.0 * opx2mx1) * ((y[2] - y[1]) * opx2mx1 - yp1);
    }
    float opx[n];
    for (i = 1; i <= n - 1; i++)
        opx[i] = 1 / (x[i + 1] - x[i]);
    for (i = 2; i <= n - 1; i++)
    {
        opx2mx1 = 1 / (x[i + 1] - x[i - 1]);
        sig = (x[i] - x[i - 1]) * opx2mx1;
        p = sig * y2[i - 1] + 2.0;
        opp = 1 / p;
        y2[i] = (sig - 1.0) * opp;
        u[i] = (y[i + 1] - y[i]) * opx[i] - (y[i] - y[i - 1]) * opx[i - 1];
        u[i] = (6.0 * u[i] * opx2mx1 - sig * u[i - 1]) * opp;
    }
    if (ypn > 0.99e30)
        qn = un = 0.0;
    else
    {
        qn = 0.5;
        un = (3.0 / (x[n] - x[n - 1])) * (ypn - (y[n] - y[n - 1]) / (x[n] - x[n - 1]));
    }
    y2[n] = (un - qn * u[n - 1]) / (qn * y2[n - 1] + 1.0);
    for (k = n - 1; k >= 1; k--)
        y2[k] = y2[k] * y2[k + 1] + u[k];
}

void SplineInterpolate(double* ixInput, double *iyInput, int InputSN, double* ixOutput, double* iyOutput, int OutputSN)
{
    if (InputSN == 0)
        return;
    if (InputSN == 1)
    {
        for (int i = 0; i < OutputSN; i++)
            iyOutput[i] = iyInput[0];
        return;
    }

    double h, b, a, oph, y2Input [InputSN];
    SplineDeriv2(ixInput - 1, iyInput - 1, InputSN,  1.0e30, 1.0e30,  y2Input - 1);
    int klo, khi, k;

    for (int i = 0; i < OutputSN ; i++)
    {
        klo = 1;
        khi = InputSN;
        while (khi - klo > 1)
        {
            k = (khi + klo) >> 1;
            if (ixInput[k - 1] > ixOutput[i])
                khi = k;
            else
                klo = k;
        }
        khi--;
        klo--;
        h = ixInput[khi] - ixInput[klo];
        oph = 1.0 / h;
        a = (ixInput[khi] - ixOutput[i]) * oph;
        b = (ixOutput[i] - ixInput[klo]) * oph;
        iyOutput[i] = a * iyInput[klo] + b * iyInput[khi] + ((a * a * a - a) * y2Input[klo] + (b * b * b - b) * y2Input[khi]) * (h * h) * 0.1666666666666666666666666666666666666666666;
    }
    iyOutput[OutputSN - 1] = iyInput[InputSN - 1];
}

unsigned int NearestPow2(unsigned int aNum)
{
    unsigned int n = 1;
    while (n < aNum)
        n <<= 1;
    return n;
}

bool CheckFileExtension(char* a_filename, char* extension /*".jpg"*/)
{
    if (!strcmp(a_filename + strlen(a_filename) - strlen(extension), extension))
        return true;
    else
        return false;
}

void GetFileFromFullPath(char* fullpath, bool removeextension)
{
    char * pch;
    char tc2[] = "\\";
    pch = strpbrk (fullpath, tc2);
    char* lastpos;
    bool wasfoundatleastoneslash = false;
    while (pch != NULL)
    {
        wasfoundatleastoneslash = true;
        memcpy(pch, " ", 1);
        lastpos = pch;
        pch = strpbrk (pch + 1, tc2);
    }

    if (wasfoundatleastoneslash)
        memcpy(fullpath, lastpos + 1, strlen(fullpath) - (lastpos - fullpath));

    if (removeextension)
    {
        pch = strpbrk (fullpath, ".");
        if (pch)
            fullpath[pch - fullpath] = 0;
    }
}

int GetSomeColor(int i)
{
    int color;
    unsigned char value;
    value = 50 + 47 * i;
    ((unsigned char*)&color)[0] = 128 - value / 2;
    value = 57 * i;
    ((unsigned char*)&color)[1] = 128 - value / 2;
    value = 83 * i;
    ((unsigned char*)&color)[2] = 128 - value / 2;
    ((unsigned char*)&color)[3] = 128;
    return color;
}

void RemoveCharactersFromString(char* iString, const char* Key)
{
    char * pch;
    pch = strpbrk (iString, Key);
    if (!pch)
        return;
rep1:
    while (pch != NULL)
    {
        memcpy(pch, pch + 1, strlen(iString) - (pch - iString));
        pch = strpbrk (pch + 1, Key);
    }
    pch = strpbrk (iString, Key);
    if (pch)
        goto rep1;
}

void ReplaceCharactersWith(char* iString, const char* Key, const char* ReplaceChar)
{
    char * pch;
    pch = strpbrk (iString, Key);
    while (pch != NULL)
    {
        memcpy(pch, ReplaceChar, 1);
        pch = strpbrk (pch + 1, Key);
    }
}

void ReplaceCharactersEntersTooWith(char* iString, const char* Key2, const char* ReplaceChar)
{
    char* Key = new char[strlen(Key2) + 3];
    strcpy(Key, Key2);

    unsigned char tc1 = 13;
    unsigned char tc2 = 10;
    memcpy(Key + strlen(Key2), &tc1, 1);
    memcpy(Key + strlen(Key2) + 1, &tc2, 1);
    Key[strlen(Key2) + 2] = 0;

    char * pch;
    pch = strpbrk (iString, Key);
    while (pch != NULL)
    {
        memcpy(pch, ReplaceChar, 1);
        pch = strpbrk (pch + 1, Key);
    }
    delete[]Key;
}

void RemoveStringsFromString(char* iString, const char* Key)
{
    char * pch;
    pch = strstr (iString, Key);
rep1:
    while (pch != NULL)
    {
        memcpy(pch, pch + strlen(Key), strlen(iString) + 1 - (int)(pch - iString) - strlen(Key) + 1);
        pch = strstr (pch + 1, Key);
    }
    pch = strstr (iString, Key);
    if (pch)
        goto rep1;
}

void ReplaceStringWithString(char* iString, const char* Key, const char* replace)
{
    if (!Key || !replace || !strlen(Key))
        return;
    char* temstr = new char[strlen(iString) * 2];
    char * pch;
    pch = strstr (iString, Key);
    if (pch)
        while (pch != NULL)
        {
            memcpy(temstr, pch + strlen(Key), strlen(iString) - (pch - iString) - strlen(Key));
            temstr[strlen(iString) - (pch - iString) - strlen(Key)] = 0;
            memcpy(pch, replace, strlen(replace));
            strcpy(pch + strlen(replace), temstr);
            pch = strstr (pch + strlen(replace), Key);
        }
    delete[]temstr;
}

void CutFileFromFullPath(char* fullpath)
{
    int lp = SearchLastPosition(fullpath, "\\");
    if (lp > -1)
        fullpath[lp] = 0;
}
