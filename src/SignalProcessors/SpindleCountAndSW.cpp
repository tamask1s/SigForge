#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <math.h>
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "fileio2.h"
#include "stringutils.h"
#include "minvect.h"
#include "datastructures.h"
#include "ellf_design_filter.h"

#ifndef MAXINT
#define MAXINT      2147483647
#endif
#ifndef MININT
#define MININT      -2147483647
#endif

CVariable_List*    m_variable_list_ref = 0; /// reference for the application's variable list.
CSignalCodec_List* m_data_list_ref = 0;     /// reference for the application's data list.
NewChar_Proc       NewChar;                 /// function call for application's "new char[n]" constructor. Strings that are going to be deleted by other parts of the system, needs to be allocated with this.
NewCVariable_Proc  NewCVariable;            /// function call for application's "new CVariable" constructor. Variables that are going to be deleted by other parts of the system (added to m_variable_list_ref), needs to be allocated with this.
Call_Proc          Call;                    /// function implemented by the application. Executes a script in the current running script's context / sandbox.

char* CreateFilter(char* a_outdataname, char* a_shape_kind, char* a_order_samplingrate, char* a_edges, char* a_iripple, char* a_iellipstopbandedge)
{
    int kind = -1;
    int shape = -1;
    int order = 2;
    double ripple = 1;
    double samplingrate = 1;
    double edge1 = 1;
    double edge2 = -1;
    double ellipstopbandedge = 3;

    CStringVec shape_kind;
    DoubleVec order_samplingrate;
    DoubleVec Edges;
    DoubleVec Ripple;
    DoubleVec Ellipstopbandedge;

    shape_kind.RebuildFrom(a_shape_kind);
    order_samplingrate.RebuildFrom(a_order_samplingrate);
    Edges.RebuildFrom(a_edges);
    Ripple.RebuildFrom(a_iripple);
    Ellipstopbandedge.RebuildFrom(a_iellipstopbandedge);

    if (Ripple.m_size)
        ripple = Ripple.m_data[0];
    if (Ellipstopbandedge.m_size)
        ellipstopbandedge = Ellipstopbandedge.m_data[0];

    if (shape_kind.m_size > 1)
    {
        if (!strcmp(shape_kind.m_data[0].s, "butter") || !strcmp(shape_kind.m_data[0].s, "butt") || !strcmp(shape_kind.m_data[0].s, "butterworth") || !strcmp(shape_kind.m_data[0].s, "1"))
            kind = 1;
        if (!strcmp(shape_kind.m_data[0].s, "chebyshev") || !strcmp(shape_kind.m_data[0].s, "cheby") || !strcmp(shape_kind.m_data[0].s, "2"))
            kind = 2;
        if (!strcmp(shape_kind.m_data[0].s, "elliptic") || !strcmp(shape_kind.m_data[0].s, "ellip") || !strcmp(shape_kind.m_data[0].s, "3"))
            kind = 3;
        if (kind == -1)
            return MakeString(NewChar, "ERROR: CreateFilter: Unknown filter kind: '", shape_kind.m_data[0].s, "', HINT: Filter kind could be 'butt', 'cheby' or 'ellip'. ");

        if (!strcmp(shape_kind.m_data[1].s, "lowpass") || !strcmp(shape_kind.m_data[1].s, "lp") || !strcmp(shape_kind.m_data[1].s, "1"))
            shape = 1;
        if (!strcmp(shape_kind.m_data[1].s, "bandpass") || !strcmp(shape_kind.m_data[1].s, "bp") || !strcmp(shape_kind.m_data[1].s, "2"))
            shape = 2;
        if (!strcmp(shape_kind.m_data[1].s, "highpass") || !strcmp(shape_kind.m_data[1].s, "hp") || !strcmp(shape_kind.m_data[1].s, "3"))
            shape = 3;
        if (!strcmp(shape_kind.m_data[1].s, "bandstop") || !strcmp(shape_kind.m_data[1].s, "bs") || !strcmp(shape_kind.m_data[1].s, "4"))
            shape = 4;
        if (shape == -1)
            return MakeString(NewChar, "ERROR: CreateFilter: Unknown filter type: '", shape_kind.m_data[1].s, "', HINT: filter type could be ('lowpass', 'bandpass', 'highpass' or 'bandstop'). ");
    }
    else
        return MakeString(NewChar, "ERROR: CreateFilter: Not enough parameters! Filter kind and/or type not provided!");

    if (order_samplingrate.m_size > 1)
    {
        order = (int) order_samplingrate.m_data[0];
        if (order <= 0)
            return MakeString(NewChar, "ERROR: CreateFilter: Invalid filter order!");
        samplingrate = order_samplingrate.m_data[1];
        if (samplingrate <= 0)
            return MakeString(NewChar, "ERROR: CreateFilter: Invalid sampling rate!");
    }
    else
        return MakeString(NewChar, "ERROR: CreateFilter: Not enough parameters! Filter order and/or sampling rate not provided!");

    if (Edges.m_size > 0)
    {
        edge1 = Edges.m_data[0];
        if (Edges.m_size > 1)
            edge2 = Edges.m_data[1];
        if (shape == 2 || shape == 4)
        {
            if (Edges.m_size <= 1)
                return MakeString(NewChar, "ERROR: CreateFilter: Not enough parameters! Not enough edges provided!");
            if (edge1 >= edge2)
                return MakeString(NewChar, "ERROR: CreateFilter: Invalid edges given: ", a_edges);
        }
    }
    else
        return MakeString(NewChar, "ERROR: CreateFilter: Not enough parameters! No edges provided!");

    int coefmultiplier = 1;
    if (shape == 2 || shape == 4)
        coefmultiplier = 2;
    int nrcoefs = order * coefmultiplier + 1;
    double denominator[nrcoefs];
    double numerator[nrcoefs];
    ellf_design_filter(kind, shape, order, ripple, samplingrate, edge1, edge2, ellipstopbandedge, denominator, numerator);

    UIntVec N;
    N.Rebuild(2);
    N.m_data[0] = nrcoefs;
    N.m_data[1] = nrcoefs;
    CVariable* output2 = NewCVariable();
    output2->Rebuild(N.m_size, N.m_data);

    for (unsigned int j = 0; j < output2->m_widths.m_data[0]; ++j)
    {
        output2->m_data[0][j] = denominator[j];
        output2->m_data[1][j] = numerator[j];
//       std::cout.precision(17);
//       std::cout << j << " th denom: " << denominator[j] << " numer: " << numerator[j] << std::endl;
    }
    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(a_outdataname, output2);
    return 0;
}

/** \brief In place Infinite Impulse Response Filter.
 *
 * \param[inout] x The data array to be processed.
 * \param[in] a_len The length of the data array to be processed.
 * \param[in] n Filter numerator coefficients.
 * \param[in] d Filter denominator coefficients.
 * \param[in] a_nr_coefs Number of filter coefficients.
 * \return void.
 */
void IIRFilter(double* x, const unsigned int a_len, const double* n, double* d, const unsigned int a_nr_coefs)
{
    if (a_nr_coefs > 1)
    {

        double* y = new double[a_len];
        memset(y, 0, a_len * sizeof(double));
        x[0] = x[1];

        for (unsigned int i = 0; i < a_nr_coefs && a_nr_coefs < 8; i++)
        {
            y[i] = 0;
            for (unsigned int j = 0; j <= i; j++)
                y[i] += n[j] * x[i - j];
            for (unsigned int j = 1; j <= i; j++)
                y[i] -= d[j] * y[i - j];
        }
        switch (a_nr_coefs)
        {
        case 2:
            for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                y[i] = n[0] * x[i] + n[1] * x[i - 1] - d[1] * y[i - 1];
            break;
        case 3:
            for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] - d[1] * y[i - 1] - d[2] * y[i - 2];
            break;
        case 4:
//            y[0] = n[0] * x[0];
//            y[1] = n[0] * x[1] + n[1] * x[0] - d[1] * y[0];
//            y[2] = n[0] * x[2] + n[1] * x[1] + n[2] * x[0] - d[1] * y[1] - d[2] * y[0];
            for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3];
            break;
        case 5:
//            y[0] = n[0] * x[0];
//            y[1] = n[0] * x[1] + n[1] * x[0] - d[1] * y[0];
//            y[2] = n[0] * x[2] + n[1] * x[1] + n[2] * x[0] - d[1] * y[1] - d[2] * y[0];
//            y[3] = n[0] * x[3] + n[1] * x[2] + n[2] * x[1] + n[3] * x[0] - d[1] * y[2] - d[2] * y[1]  - d[3] * y[0];
            for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4];
            break;

        case 6:
            for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] + n[5] * x[i - 5] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4] - d[5] * y[i - 5];
            break;
        case 7:
            for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
                y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] + n[5] * x[i - 5] + n[6] * x[i - 6] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4] - d[5] * y[i - 5] - d[6] * y[i - 6];
            break;
        }

        memcpy(x, y, a_len * sizeof(double));
        delete[] y;
    }
}

void IIRFilterZeroPhase(double* x, unsigned int a_len, double* n, double* d, const unsigned int a_nr_coefs)
{
    double* y = new double [a_len];

    y[0] = n[0] * x[0];
    y[1] = n[0] * x[1] + n[1] * x[0] - d[1] * y[0];

    if (a_nr_coefs > 2)
        y[2] = n[0] * x[2] + n[1] * x[1] + n[2] * x[0] - d[1] * y[1] - d[2] * y[0];
    if (a_nr_coefs > 3)
        y[3] = n[0] * x[3] + n[1] * x[2] + n[2] * x[1] + n[3] * x[0] - d[1] * y[2] - d[2] * y[1] - d[3] * y[0];
    if (a_nr_coefs > 4)
        y[4] = n[0] * x[4] + n[1] * x[3] + n[2] * x[2] + n[3] * x[1] + n[4] * x[0] - d[1] * y[3] - d[2] * y[2] - d[3] * y[1] - d[4] * y[0] ;
    if (a_nr_coefs > 5)
        y[5] = n[0] * x[5] + n[1] * x[4] + n[2] * x[3] + n[3] * x[2] + n[4] * x[1] + n[5] * x[0] - d[1] * y[4] - d[2] * y[3] - d[3] * y[2] - d[4] * y[1] - d[5] * y[0];

    if (a_nr_coefs == 2)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] - d[1] * y[i - 1];
    if (a_nr_coefs == 3)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] - d[1] * y[i - 1] - d[2] * y[i - 2];
    if (a_nr_coefs == 4)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3];
    if (a_nr_coefs == 5)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4];
    if (a_nr_coefs == 6)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] + n[5] * x[i - 5] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4] - d[5] * y[i - 5];
    if (a_nr_coefs == 7)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] + n[5] * x[i - 5] + n[6] * x[i - 6] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4] - d[5] * y[i - 5] - d[6] * y[i - 6];

    double* tmp = x;
    x = y;
    y = tmp;

    y[a_len - 1] = n[0] * x[a_len - 1];
    y[a_len - 2] = n[0] * x[a_len - 2] + n[1] * x[a_len - 1] - d[1] * y[a_len - 1];

    if (a_nr_coefs > 2)
        y[a_len - 3] = n[0] * x[a_len - 3] + n[1] * x[a_len - 2] + n[2] * x[a_len - 1] - d[1] * y[a_len - 2] - d[2] * y[a_len - 1] ;
    if (a_nr_coefs > 3)
        y[a_len - 4] = n[0] * x[a_len - 4] + n[1] * x[a_len - 3] + n[2] * x[a_len - 2] + n[3] * x[a_len - 1] - d[1] * y[a_len - 3] - d[2] * y[a_len - 2] - d[3] * y[a_len - 1];
    if (a_nr_coefs > 4)
        y[a_len - 5] = n[0] * x[a_len - 5] + n[1] * x[a_len - 4] + n[2] * x[a_len - 3] + n[3] * x[a_len - 2] + n[4] * x[a_len - 1] - d[1] * y[a_len - 4] - d[2] * y[a_len - 3] - d[3] * y[a_len - 2] - d[4] * y[a_len - 1] ;
    if (a_nr_coefs > 5)
        y[a_len - 6] = n[0] * x[a_len - 6] + n[1] * x[a_len - 5] + n[2] * x[a_len - 4] + n[3] * x[a_len - 3] + n[4] * x[a_len - 2] + n[5] * x[a_len - 1] - d[1] * y[a_len - 5] - d[2] * y[a_len - 4] - d[3] * y[a_len - 3] - d[4] * y[a_len - 2] - d[5] * y[a_len - 1] ;

    if (a_nr_coefs == 2)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] - d[1] * y[i + 1];
    if (a_nr_coefs == 3)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] - d[1] * y[i + 1] - d[2] * y[i + 2];
    if (a_nr_coefs == 4)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3];
    if (a_nr_coefs == 5)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3] - d[4] * y[i + 4];
    if (a_nr_coefs == 6)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] + n[5] * x[i + 5] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3] - d[4] * y[i + 4] - d[5] * y[i + 5];
    if (a_nr_coefs == 7)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] + n[5] * x[i + 5] + n[6] * x[i + 6] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3] - d[4] * y[i + 4] - d[5] * y[i + 5] - d[6] * y[i + 6];

    delete[] x;
}

void IIRFilterZeroPhaseO(double* x, double* o, unsigned int a_len, double* n, double* d, const unsigned int a_nr_coefs)
{
    double* y = new double [a_len];

    y[0] = n[0] * x[0];
    y[1] = n[0] * x[1] + n[1] * x[0] - d[1] * y[0];

    if (a_nr_coefs > 2)
        y[2] = n[0] * x[2] + n[1] * x[1] + n[2] * x[0] - d[1] * y[1] - d[2] * y[0];
    if (a_nr_coefs > 3)
        y[3] = n[0] * x[3] + n[1] * x[2] + n[2] * x[1] + n[3] * x[0] - d[1] * y[2] - d[2] * y[1] - d[3] * y[0];
    if (a_nr_coefs > 4)
        y[4] = n[0] * x[4] + n[1] * x[3] + n[2] * x[2] + n[3] * x[1] + n[4] * x[0] - d[1] * y[3] - d[2] * y[2] - d[3] * y[1] - d[4] * y[0] ;
    if (a_nr_coefs > 5)
        y[5] = n[0] * x[5] + n[1] * x[4] + n[2] * x[3] + n[3] * x[2] + n[4] * x[1] + n[5] * x[0] - d[1] * y[4] - d[2] * y[3] - d[3] * y[2] - d[4] * y[1] - d[5] * y[0];

    if (a_nr_coefs == 2)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] - d[1] * y[i - 1];
    if (a_nr_coefs == 3)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] - d[1] * y[i - 1] - d[2] * y[i - 2];
    if (a_nr_coefs == 4)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3];
    if (a_nr_coefs == 5)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4];
    if (a_nr_coefs == 6)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] + n[5] * x[i - 5] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4] - d[5] * y[i - 5];
    if (a_nr_coefs == 7)
        for (unsigned int i = a_nr_coefs - 1; i < a_len; ++i)
            y[i] = n[0] * x[i] + n[1] * x[i - 1] + n[2] * x[i - 2] + n[3] * x[i - 3] + n[4] * x[i - 4] + n[5] * x[i - 5] + n[6] * x[i - 6] - d[1] * y[i - 1] - d[2] * y[i - 2] - d[3] * y[i - 3] - d[4] * y[i - 4] - d[5] * y[i - 5] - d[6] * y[i - 6];

    x = y;

    o[a_len - 1] = n[0] * x[a_len - 1];
    o[a_len - 2] = n[0] * x[a_len - 2] + n[1] * x[a_len - 1] - d[1] * o[a_len - 1];

    if (a_nr_coefs > 2)
        o[a_len - 3] = n[0] * x[a_len - 3] + n[1] * x[a_len - 2] + n[2] * x[a_len - 1] - d[1] * o[a_len - 2] - d[2] * o[a_len - 1] ;
    if (a_nr_coefs > 3)
        o[a_len - 4] = n[0] * x[a_len - 4] + n[1] * x[a_len - 3] + n[2] * x[a_len - 2] + n[3] * x[a_len - 1] - d[1] * o[a_len - 3] - d[2] * o[a_len - 2] - d[3] * o[a_len - 1];
    if (a_nr_coefs > 4)
        o[a_len - 5] = n[0] * x[a_len - 5] + n[1] * x[a_len - 4] + n[2] * x[a_len - 3] + n[3] * x[a_len - 2] + n[4] * x[a_len - 1] - d[1] * o[a_len - 4] - d[2] * o[a_len - 3] - d[3] * o[a_len - 2] - d[4] * o[a_len - 1] ;
    if (a_nr_coefs > 5)
        o[a_len - 6] = n[0] * x[a_len - 6] + n[1] * x[a_len - 5] + n[2] * x[a_len - 4] + n[3] * x[a_len - 3] + n[4] * x[a_len - 2] + n[5] * x[a_len - 1] - d[1] * o[a_len - 5] - d[2] * o[a_len - 4] - d[3] * o[a_len - 3] - d[4] * o[a_len - 2] - d[5] * o[a_len - 1] ;

    if (a_nr_coefs == 2)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            o[i] = n[0] * x[i] + n[1] * x[i + 1] - d[1] * o[i + 1];
    if (a_nr_coefs == 3)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            o[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] - d[1] * o[i + 1] - d[2] * o[i + 2];
    if (a_nr_coefs == 4)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            o[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] - d[1] * o[i + 1] - d[2] * o[i + 2] - d[3] * o[i + 3];
    if (a_nr_coefs == 5)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            o[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] - d[1] * o[i + 1] - d[2] * o[i + 2] - d[3] * o[i + 3] - d[4] * o[i + 4];
    if (a_nr_coefs == 6)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            o[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] + n[5] * x[i + 5] - d[1] * o[i + 1] - d[2] * o[i + 2] - d[3] * o[i + 3] - d[4] * o[i + 4] - d[5] * o[i + 5];
    if (a_nr_coefs == 7)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            o[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] + n[5] * x[i + 5] + n[6] * x[i + 6] - d[1] * o[i + 1] - d[2] * o[i + 2] - d[3] * o[i + 3] - d[4] * o[i + 4] - d[5] * o[i + 5] - d[6] * o[i + 6];

    delete[] x;
}

void IIRFilterReverse(double* x, unsigned int a_len, double* n, double* d, int a_nr_coefs)
{
    double* y = new double [a_len];

    y[a_len - 1] = n[0] * x[a_len - 1];
    y[a_len - 2] = n[0] * x[a_len - 2] + n[1] * x[a_len - 1] - d[1] * y[a_len - 1];

    if (a_nr_coefs > 2)
        y[a_len - 3] = n[0] * x[a_len - 3] + n[1] * x[a_len - 2] + n[2] * x[a_len - 1] - d[1] * y[a_len - 2] - d[2] * y[a_len - 1] ;
    if (a_nr_coefs > 3)
        y[a_len - 4] = n[0] * x[a_len - 4] + n[1] * x[a_len - 3] + n[2] * x[a_len - 2] + n[3] * x[a_len - 1] - d[1] * y[a_len - 3] - d[2] * y[a_len - 2] - d[3] * y[a_len - 1];
    if (a_nr_coefs > 4)
        y[a_len - 5] = n[0] * x[a_len - 5] + n[1] * x[a_len - 4] + n[2] * x[a_len - 3] + n[3] * x[a_len - 2] + n[4] * x[a_len - 1] - d[1] * y[a_len - 4] - d[2] * y[a_len - 3] - d[3] * y[a_len - 2] - d[4] * y[a_len - 1] ;
    if (a_nr_coefs > 5)
        y[a_len - 6] = n[0] * x[a_len - 6] + n[1] * x[a_len - 5] + n[2] * x[a_len - 4] + n[3] * x[a_len - 3] + n[4] * x[a_len - 2] + n[5] * x[a_len - 1] - d[1] * y[a_len - 5] - d[2] * y[a_len - 4] - d[3] * y[a_len - 3] - d[4] * y[a_len - 2] - d[5] * y[a_len - 1] ;

    if (a_nr_coefs == 2)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] - d[1] * y[i + 1];
    if (a_nr_coefs == 3)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] - d[1] * y[i + 1] - d[2] * y[i + 2];
    if (a_nr_coefs == 4)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3];
    if (a_nr_coefs == 5)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3] - d[4] * y[i + 4];
    if (a_nr_coefs == 6)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] + n[5] * x[i + 5] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3] - d[4] * y[i + 4] - d[5] * y[i + 5];
    if (a_nr_coefs == 7)
        for (int i = a_len - a_nr_coefs; i > - 1; --i)
            y[i] = n[0] * x[i] + n[1] * x[i + 1] + n[2] * x[i + 2] + n[3] * x[i + 3] + n[4] * x[i + 4] + n[5] * x[i + 5] + n[6] * x[i + 6] - d[1] * y[i + 1] - d[2] * y[i + 2] - d[3] * y[i + 3] - d[4] * y[i + 4] - d[5] * y[i + 5] - d[6] * y[i + 6];

    memcpy(x, y, a_len * sizeof(double));
    delete[] y;
}

char* Filter(char* a_dataname, char* a_filter)
{
    char * l_result = nullptr;
    if (!a_dataname || !a_filter)
        l_result = MakeString(NewChar, "ERROR: Filter: Not enough argument!");
    else
    {
        CVariable* l_data = m_variable_list_ref->variablemap_find(a_dataname);
        if (!l_data)
            l_result = MakeString(NewChar, "ERROR: Filter: Can't find '", a_dataname, "' in variable list");
        CVariable* l_filter = m_variable_list_ref->variablemap_find(a_filter);
        double* l_denominator = nullptr;
        double* l_numerator = nullptr;
        if (!l_filter)
            l_result = MakeString(NewChar, "ERROR: Filter: Can't find '", a_filter, "' in variable list");
        else
        {
            l_denominator = l_filter->m_data[0];
            l_numerator = l_filter->m_data[1];
        }
        if (!l_result)
            for (unsigned int ch = 0; ch < l_data->m_total_samples.m_size; ++ch)
                IIRFilter(l_data->m_data[ch], l_data->m_widths.m_data[ch], l_numerator, l_denominator, l_filter->m_widths.m_data[0]);
    }
    return l_result;
}

void IIRFilterL(double* x, const unsigned int a_len, const double* n, double* d, const unsigned int a_nr_coefs)
{
    cout << "n[0] " << n[0] << endl;
    cout << "d[0] " << d[0] << endl;
    const double bcoeff[] = {n[4], n[3], n[2], n[1], n[0]};
    const double acoeff[] = {d[4], d[3], d[2], d[1], d[0]};
    //const double acoeff[] = {n[0], n[1], n[2], n[3], n[4]};
    //const double bcoeff[] = {d[0], d[1], d[2], d[3], d[4]};
    //const double* acoeff = n;
    //const double* bcoeff = d;
    static const double gain = 1.0;
    static const int NPOLE = 4;
    static const int NZERO = 4;

    double xv[] = { 0, 0, 0, 0, 0 };
    double yv[] = { 0, 0, 0, 0, 0 };

    auto doIIR = [&xv, &yv, acoeff, bcoeff](double value)
    {
        int i;
        double out = 0.0;
        for (i = 0; i < NZERO; i++)
        {
            xv[i] = xv[i + 1];
        }
        xv[NZERO] = value / gain;
        for (i = 0; i < NPOLE; i++)
        {
            yv[i] = yv[i + 1];
        }
        for (i = 0; i <= NZERO; i++)
        {
            out += xv[i] * bcoeff[i];
        }
        for (i = 0; i < NPOLE; i++)
        {
            out -= yv[i] * acoeff[i];
        }
        yv[NPOLE] = out;
        return out;
    };

    for (unsigned int i = 0; i < a_len; ++i)
        x[i] = doIIR(x[i]);
}

char* FilterL(char* a_dataname, char* a_filter)
{
    char * l_result = nullptr;
    if (!a_dataname || !a_filter)
        l_result = MakeString(NewChar, "ERROR: Filter: Not enough argument!");
    else
    {
        CVariable* l_data = m_variable_list_ref->variablemap_find(a_dataname);
        if (!l_data)
            l_result = MakeString(NewChar, "ERROR: Filter: Can't find '", a_dataname, "' in variable list");
        CVariable* l_filter = m_variable_list_ref->variablemap_find(a_filter);
        double* l_denominator = nullptr;
        double* l_numerator = nullptr;
        if (!l_filter)
            l_result = MakeString(NewChar, "ERROR: Filter: Can't find '", a_filter, "' in variable list");
        else
        {
            l_denominator = l_filter->m_data[0];
            l_numerator = l_filter->m_data[1];
        }
        if (!l_result)
            for (unsigned int ch = 0; ch < l_data->m_total_samples.m_size; ++ch)
                IIRFilterL(l_data->m_data[ch], l_data->m_widths.m_data[ch], l_numerator, l_denominator, l_filter->m_widths.m_data[0]);
    }
    return l_result;
}

char* FilterReverse(char* a_dataname, char* a_filter)
{
    char * l_result = nullptr;
    if (!a_dataname || !a_filter)
        l_result = MakeString(NewChar, "ERROR: FilterReverse: Not enough argument!");
    else
    {
        CVariable* l_data = m_variable_list_ref->variablemap_find(a_dataname);
        if (!l_data)
            l_result = MakeString(NewChar, "ERROR: FilterReverse: Can't find '", a_dataname, "' in variable list");
        CVariable* l_filter = m_variable_list_ref->variablemap_find(a_filter);
        double* l_denominator = nullptr;
        double* l_numerator = nullptr;
        if (!l_filter)
            l_result = MakeString(NewChar, "ERROR: FilterReverse: Can't find '", a_filter, "' in variable list");
        else
        {
            l_denominator = l_filter->m_data[0];
            l_numerator = l_filter->m_data[1];
        }
        if (!l_result)
            for (unsigned int ch = 0; ch < l_data->m_total_samples.m_size; ++ch)
                IIRFilterReverse(l_data->m_data[ch], l_data->m_widths.m_data[ch], l_numerator, l_denominator, l_filter->m_widths.m_data[0]);
    }
    return l_result;
}

void get_FIRFiltered(double* x, const unsigned int a_len, const double* n, const unsigned int a_nr_coefs)
{
    if (a_nr_coefs > 1)
    {
        double* y = new double[a_len];
        memset(y, 0, a_len * sizeof(double));
        x[0] = x[1];

        for (unsigned int i = 0; i < a_len; i++)
        {
            for (unsigned int j = 0; j < a_nr_coefs && j <= i; j++)
            {
                y[i] += n[j] * x[i - j];
            }
        }

        memcpy(x, y, a_len * sizeof(double));
        delete[] y;
    }
}

char* FIRFilter(char* a_dataname, char* a_filter)
{
    char * l_result = nullptr;
    if (!a_dataname || !a_filter)
        l_result = MakeString(NewChar, "ERROR: FIRFilter: Not enough argument!");
    else
    {
        CVariable* l_data = m_variable_list_ref->variablemap_find(a_dataname);
        if (!l_data)
            l_result = MakeString(NewChar, "ERROR: FIRFilter: Can't find '", a_dataname, "' in variable list");
        CVariable* l_filter = m_variable_list_ref->variablemap_find(a_filter);
        double* l_numerator = nullptr;
        if (!l_filter)
            l_result = MakeString(NewChar, "ERROR: FIRFilter: Can't find '", a_filter, "' in variable list");
        else
        {
            l_numerator = l_filter->m_data[0];
        }
        if (!l_result)
            for (unsigned int ch = 0; ch < l_data->m_total_samples.m_size; ++ch)
                get_FIRFiltered(l_data->m_data[ch], l_data->m_widths.m_data[ch], l_numerator, l_filter->m_widths.m_data[0]);
    }
    return l_result;
}

char* mfilter_rev_bp(char* a_param)
{
    double denominator[7];
    double numerator[7];
    ellf_design_filter(1, 2, 3, 1, 256, 1, 2, 3, denominator, numerator);
    double x [] = {0, 1, 2, 3, 7, 4, 3, 1, 5, 8, 4, 6, 5, 9};
    IIRFilter(x, 14, numerator, denominator, 6);
    IIRFilterReverse(x, 14, numerator, denominator, 6);
    return 0;
}

char* CreateSine(char* a_outdataname, char* n, char* a_samplerates, char* a_frequencies)
{
    UIntVec N;
    DoubleVec Frequencies;
    DoubleVec Samplerates;

    N.RebuildFrom(n);
    Frequencies.RebuildFrom(a_frequencies);
    Samplerates.RebuildFrom(a_samplerates);

    CVariable* output2 = NewCVariable();
    output2->Rebuild(N.m_size, N.m_data);

    for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
    {
        output2->m_sample_rates.m_data[i] = Samplerates.m_data[i];
        for (unsigned int j = 0; j < output2->m_widths.m_data[i]; ++j)
        {
            output2->m_data[i][j] = sin(j / output2->m_sample_rates.m_data[i] * 2.0 * M_PI * Frequencies.m_data[0]) * 100.0;
            for (unsigned int k = 1; k < Frequencies.m_size; ++k)
                output2->m_data[i][j] += sin(j / output2->m_sample_rates.m_data[i] * 2.0 * M_PI * Frequencies.m_data[k]) * 100.0;
        }
    }
    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(a_outdataname, output2);
    return 0;
}

char* CreateFromTo(char* a_outdataname, char* a_start, char* a_stop, char* a_step, char* a_samplerates)
{
    DoubleVec Samplerates;

    Samplerates.RebuildFrom(a_samplerates);

    CVariable* output2 = NewCVariable();
    double start = atoi(a_start);
    double stop = atoi(a_stop);
    double step = atoi(a_step);
    unsigned int sizes[] = {(unsigned int)((stop - start) / step)};
    output2->Rebuild(1, sizes);

    for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
    {
        output2->m_sample_rates.m_data[i] = Samplerates.m_data[i];
        double val = start;
        for (unsigned int j = 0; j < output2->m_widths.m_data[i]; ++j)
        {
            output2->m_data[i][j] = val;
            val += step;
        }
    }
    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(a_outdataname, output2);
    return 0;
}

char* HighpassButterbase6thOrderZerophase(char* a_invarname, char* a_lbound)
{
    DoubleVec LBound;
    LBound.RebuildFrom(a_lbound);
    if (!LBound.m_size)
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: while BandpassButterbase6thOrderZerophase '", a_invarname, "' not enugh arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        double denominator[7];
        double numerator[7];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
        {
            ellf_design_filter(1, 3, 3, 1, var->m_sample_rates.m_data[i], LBound.m_data[0], 0, 0, denominator, numerator);
            IIRFilter(var->m_data[i], var->m_widths.m_data[i], numerator, denominator, 6);
            IIRFilterReverse(var->m_data[i], var->m_widths.m_data[i], numerator, denominator, 6);
        }
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: can't find data in variablelist: '", a_invarname, "'");
}

char* BandpassButterbase6thOrderZerophase(char* a_invarname, char* a_lbound, char* a_hbound)
{
    DoubleVec LBound, HBound;
    LBound.RebuildFrom(a_lbound);
    HBound.RebuildFrom(a_hbound);
    if (!LBound.m_size || !HBound.m_size)
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: while BandpassButterbase6thOrderZerophase '", a_invarname, "' not enugh arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        double denominator[7];
        double numerator[7];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
        {
            ellf_design_filter(1, 2, 3, 1, var->m_sample_rates.m_data[i], LBound.m_data[0], HBound.m_data[0], 3, denominator, numerator);
            IIRFilter(var->m_data[i], var->m_widths.m_data[i], numerator, denominator, 6);
            IIRFilterReverse(var->m_data[i], var->m_widths.m_data[i], numerator, denominator, 6);
        }
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: can't find data in variablelist: '", a_invarname, "'");
}

bool Product(double* a_data1, double* a_data2, int size)
{
    if (!size || !a_data1 || !a_data2)
        return false;
    int cy = size / 16;
    int i = 0;
    for (int j = 0; j < cy; ++j)
    {
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
        a_data1[i] *= a_data2[i];
        ++i;
    }
    for (int j = i; j < size; ++j)
        a_data1[j] *= a_data2[j];
    return true;
}

double* Hanning(int n)
{
    double nm1 = 2.0 * M_PI / (n - 1);
    double* hanning = new double [n];
    for (int k = 0; k < n + 0; ++k)
        hanning[k - 0] = (0.5 - 0.5 * cos((double)k * nm1));
    return hanning;
}

void CreateCover(double* x, int a_datasize, double* kernel, int kernelsize, double a)
{
    double* y = new double [a_datasize];
    double opa = 1.0 / a;
    for (int n = 0; n < a_datasize; n++)
    {
        int ks = kernelsize;
        if (ks > n + 1)
            ks = n + 1;
        y[n] = kernel[0] * x[n];
        for (int j = 1; j < ks; ++j)
            y[n] += kernel[j] * x[n - j];
        y[n] *= opa;
    }
    memcpy(x, y, a_datasize * sizeof(double));
    delete[] y;
}

void RevCreateCover(double* x, int datasize, double* kernel, int kernelsize, double a)
{
    double* y = new double [datasize];
    double opa = 1.0 / a;
    for (int n = datasize - 1; n > -1; n--)
    {
        int ks = kernelsize;
        if (ks > datasize - n)
            ks = datasize - n;
        y[n] = kernel[0] * x[n];
        for ( int j = 1; j < ks; ++j)
            y[n] += kernel[j] * x[n + j];
        y[n] *= opa;
    }
    memcpy(x, y, datasize * sizeof(double));
    delete[] y;
}

void CreateCoverCreateCover(double* x, int datasize, double* kernel, int kernelsize, double a)
{
    double* y = new double [datasize];
    double opa = 1.0 / a;
    for (int n = 0; n < datasize; n++)
    {
        int ks = kernelsize;
        if (ks > n + 1)
            ks = n + 1;
        y[n] = kernel[0] * x[n];
        for (int j = 1; j < ks; ++j)
            y[n] += kernel[j] * x[n - j];
        y[n] *= opa;
    }
    for (int n = datasize - 1; n > -1; n--)
    {
        int ks = kernelsize;
        if (ks > datasize - n)
            ks = datasize - n;
        x[n] = kernel[0] * y[n];
        for ( int j = 1; j < ks; ++j)
            x[n] += kernel[j] * y[n + j];
        x[n] *= opa;
    }
    delete[] y;
}

void RealCover(double* x, int size, double fs = 0)
{
    if (!size)
        return;
    double lastmax = x[0];
    int lastmaxx = 0;
    for (int i = 0; i < size - 2; ++i)
        if (x[i + 1] > x[i] && x[i + 2] < x[i + 1])
        {
            double slope = (x [i + 1] - lastmax ) / (i + 1 - lastmaxx);
            for (int j = lastmaxx; j < i + 1; ++j)
                x[j] = lastmax + slope * (j - lastmaxx);
            ++i;
            lastmax = x [i];
            lastmaxx = i;
        }
}

void CreateCover(double* x, int size, double fs)
{
    double* kernel = Hanning(int(fs / 12.0));
    double ac = fs / 29.0;
    CreateCoverCreateCover(x, size, kernel, int(fs / 12.0), ac);
    delete[] kernel;
}

char* Cover1(char* a_invarname)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            //CreateCover(var->m_data[i], var->m_widths.m_data[i], var->m_sample_rates.m_data[i]);
            RealCover(var->m_data[i], var->m_widths.m_data[i], var->m_sample_rates.m_data[i]);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: Cover1: can't find data in variablelist: '", a_invarname, "'");
    return 0;
}

double* filtfilt(double* x, unsigned int datasize, double* b, double* a)
{
    double* z = new double [datasize];
    z[0] = b[0] * x[0];
    z[1] = b[0] * x[1] + b[1] * x[0] - a[1] * z[0];
    z[2] = b[0] * x[2] + b[1] * x[1] + b[2] * x[0] - a[1] * z[1] - a[2] * z[0];
    z[3] = b[0] * x[3] + b[1] * x[2] + b[2] * x[1] + b[3] * x[0] - a[1] * z[2] - a[2] * z[1] - a[3] * z[0];
    z[4] = b[0] * x[4] + b[1] * x[3] + b[2] * x[2] + b[3] * x[1] + b[4] * x[0] - a[1] * z[3] - a[2] * z[2] - a[3] * z[1] - a[4] * z[0];
    z[5] = b[0] * x[5] + b[1] * x[4] + b[2] * x[3] + b[3] * x[2] + b[4] * x[1] + b[5] * x[0] - a[1] * z[4] - a[2] * z[3] - a[3] * z[2] - a[4] * z[1] - a[5] * z[0];
    for (unsigned int n = 6; n < datasize; n++)
        z[n] = b[0] * x[n] + b[1] * x[n - 1] + b[2] * x[n - 2] + b[3] * x[n - 3] + b[4] * x[n - 4] + b[5] * x[n - 5] + b[6] * x[n - 6] - a[1] * z[n - 1] - a[2] * z[n - 2] - a[3] * z[n - 3] - a[4] * z[n - 4] - a[5] * z[n - 5] - a[6] * z[n - 6];

    double* y = new double [datasize];
    y[datasize - 1] = b[0] * z[datasize - 1];
    y[datasize - 2] = b[0] * z[datasize - 2] + b[1] * z[datasize - 1] - a[1] * y[datasize - 1];
    y[datasize - 3] = b[0] * z[datasize - 3] + b[1] * z[datasize - 2] + b[2] * z[datasize - 1] - a[1] * y[datasize - 2] - a[2] * y[datasize - 1];
    y[datasize - 4] = b[0] * z[datasize - 4] + b[1] * z[datasize - 3] + b[2] * z[datasize - 2] + b[3] * z[datasize - 1] - a[1] * y[datasize - 3] - a[2] * y[datasize - 2] - a[3] * y[datasize - 1];
    y[datasize - 5] = b[0] * z[datasize - 5] + b[1] * z[datasize - 4] + b[2] * z[datasize - 3] + b[3] * z[datasize - 2] + b[4] * z[datasize - 1] - a[1] * y[datasize - 4] - a[2] * y[datasize - 3] - a[3] * y[datasize - 2] - a[4] * y[datasize - 1];
    y[datasize - 6] = b[0] * z[datasize - 6] + b[1] * z[datasize - 5] + b[2] * z[datasize - 4] + b[3] * z[datasize - 3] + b[4] * z[datasize - 2] + b[5] * z[datasize - 1] - a[1] * y[datasize - 5] - a[2] * y[datasize - 4] - a[3] * y[datasize - 3] - a[4] * y[datasize - 2] - a[5] * y[datasize - 1];
    for (int n = datasize - 7; n > -1; n--)
        y[n] = b[0] * z[n] + b[1] * z[n + 1] + b[2] * z[n + 2] + b[3] * z[n + 3] + b[4] * z[n + 4] + b[5] * z[n + 5] + b[6] * z[n + 6] - a[1] * y[n + 1] - a[2] * y[n + 2] - a[3] * y[n + 3] - a[4] * y[n + 4] - a[5] * y[n + 5] - a[6] * y[n + 6];
    delete[] z;
    return y;
}

//char* BandpassButterbase6thOrderZerophase(char* a_invarname, char* lbound, char* hbound)
//{
//    DoubleVec LBound, HBound;
//    LBound.RebuildFrom(lbound);
//    HBound.RebuildFrom(hbound);
//    if (!LBound.m_size||!HBound.m_size)
//        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: while BandpassButterbase6thOrderZerophase '",a_invarname, "' not enugh arguments!");
//    if (m_variable_list_ref->variablemap_find(a_invarname))
//    {
//        double denominator[7];
//        double numerator[7];
//        for (unsigned int i=0;i<var->m_total_samples.m_size;++i)
//        {
//            ellf_design_filter(1,2,3, 1,var->m_sample_rates.m_data[i],LBound.m_data[0],HBound.m_data[0],3,denominator,numerator);
//            filter3    (var->m_data[i], var->m_widths.m_data[i],numerator,denominator,6);
//            revfilter3 (var->m_data[i], var->m_widths.m_data[i],numerator,denominator,6);
//        }
//        return 0;
//    }
//    else
//        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: can't find data in variablelist: '",a_invarname, "'");
//}

char* BandpassButterbase6thOrderZerophaseC(char* a_invarname, char* a_varnametostore, char* lbound, char* hbound)
{
    DoubleVec LBound, HBound;
    LBound.RebuildFrom(lbound);
    HBound.RebuildFrom(hbound);
    if (!LBound.m_size || !HBound.m_size)
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: while BandpassButterbase6thOrderZerophaseC '", a_invarname, "' not enugh arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[var->m_total_samples.m_size];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            output2sizes[i] = var->m_widths.m_data[i];
        output2->Rebuild(var->m_total_samples.m_size, output2sizes);
        var->SCopyTo(output2);
        double denominator[7];
        double numerator[7];
        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
            if (output2->m_widths.m_data[i])
            {
                ellf_design_filter(1, 2, 3, 1, var->m_sample_rates.m_data[i], LBound.m_data[0] + 0.1, HBound.m_data[0] - 0.1, 3, denominator, numerator);
                IIRFilterZeroPhaseO(var->m_data[i], output2->m_data[i], var->m_widths.m_data[i], numerator, denominator, 7);
            }
        strcpy(output2->m_varname, a_varnametostore);
        m_variable_list_ref->Insert(a_varnametostore, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophaseC: can't find data in variablelist: '", a_invarname, "'");
}

char* SQRBandpassButterbase6thOrderZerophaseC(char* a_invarname, char* a_varnametostore, char* lbound, char* hbound)
{
    DoubleVec LBound, HBound;
    LBound.RebuildFrom(lbound);
    HBound.RebuildFrom(hbound);
    if (!LBound.m_size || !HBound.m_size)
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: while BandpassButterbase6thOrderZerophaseC '", a_invarname, "' not enugh arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[var->m_total_samples.m_size];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            output2sizes[i] = var->m_widths.m_data[i];
        output2->Rebuild(var->m_total_samples.m_size, output2sizes);
        var->SCopyTo(output2);
        double denominator[7];
        double numerator[7];
        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
            if (output2->m_widths.m_data[i])
            {
                ellf_design_filter(1, 2, 3, 1, var->m_sample_rates.m_data[i], LBound.m_data[0] + 0.1, HBound.m_data[0] - 0.1, 3, denominator, numerator);
                IIRFilterZeroPhaseO(var->m_data[i], output2->m_data[i], var->m_widths.m_data[i], numerator, denominator, 7);
            }
        strcpy(output2->m_varname, a_varnametostore);
        m_variable_list_ref->Insert(a_varnametostore, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophaseC: can't find data in variablelist: '", a_invarname, "'");
}

char* ABSBandpassButterbase6thOrderZerophaseC(char* a_invarname, char* a_varnametostore, char* lbound, char* hbound)
{
    DoubleVec LBound, HBound;
    LBound.RebuildFrom(lbound);
    HBound.RebuildFrom(hbound);
    if (!LBound.m_size || !HBound.m_size)
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophase: while BandpassButterbase6thOrderZerophaseC '", a_invarname, "' not enugh arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[var->m_total_samples.m_size];
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            output2sizes[i] = var->m_widths.m_data[i];
        output2->Rebuild(var->m_total_samples.m_size, output2sizes);
        var->SCopyTo(output2);
        double denominator[7];
        double numerator[7];
        for (unsigned int i = 0; i < output2->m_total_samples.m_size; ++i)
            if (output2->m_widths.m_data[i])
            {
                ellf_design_filter(1, 2, 3, 1, var->m_sample_rates.m_data[i], LBound.m_data[0] + 0.0, HBound.m_data[0] - 0.0, 3, denominator, numerator);
                double* data = filtfilt(var->m_data[i], var->m_widths.m_data[i], numerator, denominator);
                for (unsigned int j = 0; j < output2->m_widths.m_data[i]; ++j)
                    output2->m_data[i][j] = __builtin_fabsf (data [j]);
                delete[] data;
            }
        strcpy(output2->m_varname, a_varnametostore);
        m_variable_list_ref->Insert(a_varnametostore, output2);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: BandpassButterbase6thOrderZerophaseC: can't find data in variablelist: '", a_invarname, "'");
}

char* GetVal(char* a_invarname, char* a_lbound, char* a_hbound)
{
    IntVec LBound, HBound;
    LBound.RebuildFrom(a_lbound);
    HBound.RebuildFrom(a_hbound);
    if (!LBound.m_size || !HBound.m_size)
        return MakeString(NewChar, "ERROR: GetValue: while GetValue from '", a_invarname, "' not enugh arguments!");
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        DoubleVec dv1;
        dv1.AddElement(var->m_data[LBound.m_data[0]][HBound.m_data[0]]);
        char* str1 = dv1.ToString(" ");
        if (str1)
        {
            char* str2 = MakeString(NewChar, "RESULT: ", str1);
            delete[] str1;
            return str2;
        }
        else
            return MakeString(NewChar, "ERROR: GetValue: fatal terror. out of memory?");
    }
    else
        return MakeString(NewChar, "ERROR: GetValue: can't find data in variablelist: '", a_invarname, "'");
}

void findmin(double* a_inp, int size, double* a_val, int*a_indx)
{
    if (!size)
        return;
    *a_val = MAXINT;
    for (int i = 0; i < size; ++i)
        if (*a_val > a_inp[i])
        {
            *a_val = a_inp[i];
            *a_indx = i;
        }
}

void findmax(double* a_inp, int a_size, double* a_val, int* a_indx)
{
    if (!a_size)
        return;
    *a_val = MININT;
    for (int i = 0; i < a_size; ++i)
        if (*a_val < a_inp[i])
        {
            *a_val = a_inp[i];
            *a_indx = i;
        }
}

double findmin(double* a_inp, int a_size)
{
    if (!a_size)
        return 0;
    double val = MAXINT;
    for (int i = 0; i < a_size; ++i)
        if (val > a_inp[i])
            val = a_inp[i];
    return val;
}

double findmax(double* a_inp, int a_size)
{
    if (!a_size)
        return 0;
    double val = MININT;
    for (int i = 0; i < a_size; ++i)
        if (val < a_inp[i])
            val = a_inp[i];
    return val;
}

int get_precision_for_val(double val, int nr_characters = 8)
{
    int width = (val <= 0) ? 1 : floor(log10(fabs(val))) + 1;
    if (width < 0)
        width = 0;
    return (width > nr_characters) ? 0 : nr_characters - width;
}

char* GetMax(char* a_invarname)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        std::ostringstream oss;
        for (unsigned int ch = 0; ch < var->m_total_samples.m_size; ch++)
        {
            double val = findmax(var->m_data[ch], var->m_widths.m_data[ch]);
            oss << std::fixed << std::setprecision(get_precision_for_val(val)) << val << " ";
        }
        std::string result = oss.str();
        return MakeString(NewChar, "RESULT: ", result.c_str());
    }
    else
        return MakeString(NewChar, "ERROR: GetMax: can't find data in variablelist: '", a_invarname, "'");
}

char* GetMin(char* a_invarname)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        std::ostringstream oss;
        for (unsigned int ch = 0; ch < var->m_total_samples.m_size; ch++)
        {
            double val = findmin(var->m_data[ch], var->m_widths.m_data[ch]);
            oss << std::fixed << std::setprecision(get_precision_for_val(val)) << val << " ";
        }
        std::string result = oss.str();
        return MakeString(NewChar, "RESULT: ", result.c_str());
    }
    else
        return MakeString(NewChar, "ERROR: GetMin: can't find data in variablelist: '", a_invarname, "'");
}

char* swindxdetect(char* a_invarname, char* a_outdataname, char* a_spswkrits)
{
    if (!a_invarname || !a_outdataname || !a_spswkrits)
        return MakeString(NewChar, "ERROR: swindxdetect");

    CVariable* invar = m_variable_list_ref->variablemap_find(a_invarname);
    CVariable* krits = m_variable_list_ref->variablemap_find(a_spswkrits);
    if (!invar || !krits)
        return MakeString(NewChar, "ERROR: swindxdetect");

    double lb = 0.3;
    double hb = 1.5;
    double seconds = 6;

    int swwscount[invar->m_total_samples.m_size];
    int indxx = 0;
    double swindxs[10000][2];

    int swwscount2[invar->m_total_samples.m_size];
    int indxx2 = 0;
    double swindxs2[10000][2];

    for (unsigned int ch = 0; ch < invar->m_total_samples.m_size; ch++)
    {
        swwscount[ch] = 0;
        swwscount2[ch] = 0;
        double fs = invar->m_sample_rates.m_data[ch];
        int interv = (int)(fs * seconds);
        int max1 = (int)(invar->m_widths.m_data[ch] - floor(fs / 2) - 5 - floor(interv / 2)) - 5;

        double denominator[7];
        double numerator[7];
        ellf_design_filter(1, 2, 3, 1, invar->m_sample_rates.m_data[ch], lb, hb, 3, denominator, numerator);
        double* a = filtfilt(invar->m_data[ch], invar->m_widths.m_data[ch], numerator, denominator);
        int i = (int)floor(interv / 2);
        double krit  = krits->m_data[2][ch] * -0.50;
        double krit2 = krits->m_data[2][ch] *  0.50;
        double opsps = 1.0 / invar->m_sample_rates.m_data[ch];
        while (i < max1)
        {
            i += 5;
            if (a[i] < krit)
            {
                int maxx;
                double maxa;
                findmin(a + i, (int)floor(fs * 0.5), &maxa, &maxx);
                maxx += i;
                i = i + (int)fs;
                swindxs[indxx][0] = ch;
                swindxs[indxx++][1] = (double)maxx * opsps;
                swwscount[ch]++;
            }
        }
        i = (int)floor(interv / 2);
        while (i < max1)
        {
            i += 5;
            if (a[i] > krit2)
            {
                int maxx;
                double maxa;
                findmax(a + i, (int)floor(fs * 0.5), &maxa, &maxx);
                maxx += i;
                i = i + (int)fs;
                swindxs2[indxx2][0] = ch;
                swindxs2[indxx2++][1] = (double)maxx * opsps;
                swwscount2[ch]++;
            }
        }
        delete[]a;
    }
    CVariable* output2 = NewCVariable();
    unsigned int output2sizes[4];
    output2sizes[0] = indxx;
    output2sizes[1] = indxx;
    output2sizes[2] = indxx2;
    output2sizes[3] = indxx2;
    output2->Rebuild(4, output2sizes);
    for (unsigned int i = 0; i < output2->m_widths.m_data[0]; ++i)
    {
        output2->m_data[0][i] = swindxs[i][0];
        output2->m_data[1][i] = swindxs[i][1];
    }
    for (unsigned int i = 0; i < output2->m_widths.m_data[2]; ++i)
    {
        output2->m_data[2][i] = swindxs2[i][0];
        output2->m_data[3][i] = swindxs2[i][1];
    }

    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(a_outdataname, output2);

    return 0;
}

double mean(double * a_data, int a_size)
{
    if (!a_data || !a_size)
        return 0;
    double result = 0;
    int cy = a_size / 16;
    int i = 0;
    for (int j = 0; j < cy; ++j)
    {
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
        result += a_data[i];
        ++i;
    }
    for (int j = i; j < a_size; ++j)
        result += a_data[j];
    result /= (double)a_size;
    return result;
}

bool SQR(double* a_data, int a_size)
{
    if (!a_data || !a_size)
        return false;
    int cy = a_size / 16;
    int i = 0;
    for (int j = 0; j < cy; ++j)
    {
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
        a_data[i] *= a_data[i];
        ++i;
    }
    for (int j = i; j < a_size; ++j)
        a_data[j] *= a_data[j];
    return true;
}

bool SQRT(double* a_data, int a_size)
{
    if (!a_data || !a_size)
        return false;
    int cy = a_size / 16;
    int i = 0;
    for (int j = 0; j < cy; ++j)
    {
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
        a_data[i] = __builtin_sqrtf(a_data[i]);
        ++i;
    }
    for (int j = i; j < a_size; ++j)
        a_data[j] = __builtin_sqrtf(a_data[j]);
    return true;
}

char* SQRT_Inplace(char* a_invarname)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            SQRT(var->m_data[i], var->m_widths.m_data[i]);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: SQRT_Inplace: can't find data in variablelist: '", a_invarname, "'");
}

char* SQR_Inplace(char* a_invarname)
{
    if (CVariable* var = m_variable_list_ref->variablemap_find(a_invarname))
    {
        for (unsigned int i = 0; i < var->m_total_samples.m_size; ++i)
            SQR(var->m_data[i], var->m_widths.m_data[i]);
        return 0;
    }
    else
        return MakeString(NewChar, "ERROR: SQR_Inplace: can't find data in variablelist: '", a_invarname, "'");
}

char* orsocounter(char* a_invarname, char* a_spswkrits, char* a_spindx, char* a_spcountandswrel)
{
    if (!a_invarname || !a_spswkrits || !a_spindx)
        return MakeString(NewChar, "ERROR: orsocounter");

    IntVec sindleindx;
    sindleindx.RebuildFrom(a_spindx);

    CVariable* invar = m_variable_list_ref->variablemap_find(a_invarname);
    CVariable* krits = m_variable_list_ref->variablemap_find(a_spswkrits);
    if (!invar || !krits || !sindleindx.m_size)
        return MakeString(NewChar, "ERROR: orsocounter");

    CVariable* spcountandswreldat = m_variable_list_ref->variablemap_find(a_spcountandswrel);
    if (!spcountandswreldat)
    {
        spcountandswreldat = NewCVariable();
        unsigned int spcountandswreldatsizes[14];
        for (unsigned int i = 0; i < 14; ++i)
            spcountandswreldatsizes[i] = invar->m_total_samples.m_size;
        spcountandswreldat->Rebuild(14, spcountandswreldatsizes);
        strcpy(spcountandswreldat->m_varname, a_spcountandswrel);
        m_variable_list_ref->Insert(a_spcountandswrel, spcountandswreldat);
    }
    CVariable spls, avgm;

    unsigned int splssizes[invar->m_total_samples.m_size];
    for (unsigned int i = 0; i < invar->m_total_samples.m_size; ++i)
        splssizes[i] = (int) (floor(invar->m_widths.m_data[i] / (invar->m_sample_rates.m_data[i] / 2.0)));
    spls.Rebuild(invar->m_total_samples.m_size, splssizes);
    avgm.Rebuild(invar->m_total_samples.m_size, splssizes);

    for (unsigned int ch = 0; ch < invar->m_total_samples.m_size; ch++)
    {
        double fsp2 = invar->m_sample_rates.m_data[ch] / 2.0;
        double durinmin =  invar->m_widths.m_data [ch] / invar->m_sample_rates.m_data[ch] / 60.0;
        double kriter = krits->m_data[sindleindx.m_data[0]][ch];

        for (unsigned int i = 0; i < spls.m_widths.m_data[ch]; ++i)
        {
            avgm.m_data[ch][i] = __builtin_sqrtf(mean(invar->m_data[ch] + (int)(i * fsp2), (int)fsp2));
            if (avgm.m_data[ch][i] > kriter)
            {
                spls.m_data[ch][i] = 1;
            }
        }

        int scount = 0;
        for (unsigned int i = 0; i < spls.m_widths.m_data[ch] - 1; ++i)
        {
            if ((spls.m_data[ch][i] - spls.m_data[ch][i + 1]) > 0)
            {
                scount++;
                invar->m_data[ch][(int)(i * fsp2)] = -300;
            }
        }
        spcountandswreldat->m_data[sindleindx.m_data[0]][ch] = scount / durinmin;
    }
    return 0;
}

char* Orsocounter2(char* a_invarname, char* a_spswkrits, char* a_spindx, char* a_spcountandswrel)
{
    if (!a_invarname || !a_spswkrits || !a_spindx)
        return MakeString(NewChar, "ERROR: orsocounter");

    IntVec sindleindx;
    sindleindx.RebuildFrom(a_spindx);

    CVariable* invar = m_variable_list_ref->variablemap_find(a_invarname);
    CVariable* krits = m_variable_list_ref->variablemap_find(a_spswkrits);
    if (!invar || !krits || !sindleindx.m_size)
        return MakeString(NewChar, "ERROR: orsocounter");

    CVariable* spcountandswreldat = m_variable_list_ref->variablemap_find(a_spcountandswrel);
    if (!spcountandswreldat)
    {
        spcountandswreldat = NewCVariable();
        int spcountandswreldatsize = 14;
        unsigned int spcountandswreldatsizes[spcountandswreldatsize];
        for (int i = 0; i < spcountandswreldatsize; ++i)
            spcountandswreldatsizes[i] = invar->m_total_samples.m_size;
        spcountandswreldat->Rebuild(spcountandswreldatsize, spcountandswreldatsizes);
        strcpy(spcountandswreldat->m_varname, a_spcountandswrel);
        m_variable_list_ref->Insert(a_spcountandswrel, spcountandswreldat);
    }
    CVariable spls, avgm;

    unsigned int splssizes[invar->m_total_samples.m_size];
    for (unsigned int i = 0; i < invar->m_total_samples.m_size; ++i)
        splssizes[i] = (int) (floor(invar->m_widths.m_data[i] / (invar->m_sample_rates.m_data[i] / 2.0)));
    spls.Rebuild(invar->m_total_samples.m_size, splssizes);
    avgm.Rebuild(invar->m_total_samples.m_size, splssizes);

    for (unsigned int ch = 0; ch < invar->m_total_samples.m_size; ch++)
    {
        double fsp2 = invar->m_sample_rates.m_data[ch] / 2.0;
        double durinmin =  invar->m_widths.m_data [ch] / invar->m_sample_rates.m_data[ch] / 60.0;
        double kriter = krits->m_data[sindleindx.m_data[0]][ch];

        for (unsigned int i = 0; i < spls.m_widths.m_data[ch]; ++i)
        {
            avgm.m_data[ch][i] = mean(invar->m_data[ch] + (int)(i * fsp2), (int)fsp2);
            if (avgm.m_data[ch][i] > kriter)
            {
                spls.m_data[ch][i] = 1;
            }
        }

        int scount = 0;
        for (unsigned int i = 0; i < spls.m_widths.m_data[ch] - 1; ++i)
        {
            if ((spls.m_data[ch][i] - spls.m_data[ch][i + 1]) < 0)
                scount++;
        }
        spcountandswreldat->m_data[sindleindx.m_data[0] + 8][ch] = scount / durinmin;
    }
    return 0;
}

char* SpindleCount(char* a_invarname, char* a_spswkrits, char* a_spindx, char* a_spcountandswrel)
{
    if (!a_invarname || !a_spswkrits || !a_spindx)
        return MakeString(NewChar, "ERROR: SpindleCount1");

    IntVec sindleindx;
    sindleindx.RebuildFrom(a_spindx);

    CVariable* invar = m_variable_list_ref->variablemap_find(a_invarname);
    CVariable* krits = m_variable_list_ref->variablemap_find(a_spswkrits);
    CVariable* spcountandswreldat = m_variable_list_ref->variablemap_find(a_spcountandswrel);
    if (!invar || !krits || !sindleindx.m_size || !spcountandswreldat)
        return MakeString(NewChar, "ERROR: SpindleCount2");

    for (unsigned int ch = 0; ch < invar->m_total_samples.m_size; ch++)
    {
        double durinmin =  invar->m_widths.m_data [ch] / invar->m_sample_rates.m_data[ch] / 60.0;
        double kriter = krits->m_data[sindleindx.m_data[0]][ch] * 2.0; //1.5
        int scount = 0;
        int isamples = (int)(invar->m_sample_rates.m_data[ch] * 0.50);
        int neres = (int)(invar->m_sample_rates.m_data[ch] / 10.0);
        for (unsigned int i = neres; i < invar->m_widths.m_data[ch] - isamples; ++i)
        {
            if ((invar->m_data[ch][i] >= kriter) && (invar->m_data[ch][i] > invar->m_data[ch][i - neres]))
            {
                scount++;
                i += (int)(isamples * 2.0); //+maxindx;
            }
        }
        spcountandswreldat->m_data[sindleindx.m_data[0] + 8 + 4][ch] = scount / durinmin;
    }
    return 0;
}

//-------------------------------A használt változók:---------------------------------------------------
//m_data                                 -- A leszûrt es búrkolt EEG görbe egy csatornája
//i = 0                                -- Adatpont léptetõ lenullázva, innen kezdünk lépdelni az adatban
//kriter   = kriter * 2                -- A kritbõl kiolvasott érték * 2 (vagz 1.5, stb...)
//S1sec    = m_sample_rates[ch]           -- 1 sec-nyi sample érték  (249, 256 stb...)
//S01sec   = s1sec/10                  -- 0.1 sec-nyi sample érték
//SpindleCount = 0                     -- Orso-számláló lenullázva
//-------------------Tulajdonképpeni algoritmus, keressük a csúcsot-------------------------------------
//if(m_data[i]>kriter)                   -- Ha az adat adott i pontjában nagyobb mint a kritérium,
//{
//    maxindx = i                      -- akkor megjegyezzük ennek az adatpontnak a helyét
//    maxval  = m_data[i]                -- és az amplitúdóját
//    j=0
//    while(1=1)                       -- és amíg a világ világ (ameddig igaz hogy 1=1)
//    {
//        j = j + 1                    -- növeljük a j - t
//        if (maxval<m_data[i+j])         -- keressük a következõ pontot ahol az adatpont nagyobb mint
//        {                                  amit találtunk, tehát megyunk felfelé a felszálló ágon,
//            maxindx = i + j          -- megjegyezzük ennek az adatpontnak a helyét
//            maxval  = m_data[i+j]      -- és az amplitúdóját
//        }
//        if (m_data[i+j]-m_data[i+j+S01sec]>=kriter/10)-- egészen addig megyünk fel, ameddig el nem kezd
//            goto ex1                                leszállni. Mégpedig olyan mértékben hogy az adott
//    }                                               ponthoz viszonyított 0.1 sec-ra (S01sec) levo
//ex1:                                                kovetkezõ adat legalább krit/10 - el kisebb legyen
//    m_data[maxindx]=-5   -- ha meglett a csúcs, teszünk egy markert,
//    SpindleCount++     -- növeljük az orsó-számlálót
//    i=maxindx          -- ugrunk i-vel az adott csúcs poziciojára hogy legközelebb onnan folytassuk.
//}                      -- És ezt az if (m_data[i]>kriter)-et ismételjük míg tart az adat (i<m_data_size).
//------------------------------------------------------------------------------------------------------

char* IProd(char* a_invarname, char* a_value)
{
    char* l_error = nullptr;
    CVariable* l_invar = nullptr;
    DoubleVec l_params;
    if (!a_invarname || !a_value)
        l_error = MakeString(NewChar, "ERROR: IProd: not enough arguments, 'invarname' or 'value' is missing!");
    if(!l_error)
    {
        l_params.AddElement(a_value);
        l_invar = m_variable_list_ref->variablemap_find(a_invarname);
    }
    if (!l_error && !l_invar)
        l_error = MakeString(NewChar, "ERROR: IProd: can't find '", a_invarname, "' in variablelist");

    if (!l_error)
        for (unsigned int ch = 0; ch < l_invar->m_total_samples.m_size; ch++)
            for (unsigned int i = 0; i < l_invar->m_widths.m_data[ch]; ++i)
                l_invar->m_data[ch][i] *= l_params.m_data[0];

    return l_error;
}

char* IDiv(char* a_invarname, char* a_value)
{
    char* l_error = nullptr;
    CVariable* l_invar = nullptr;
    DoubleVec l_params;
    if (!a_invarname || !a_value)
        l_error = MakeString(NewChar, "ERROR: IDiv: not enough arguments, 'invarname' or 'value' is missing!");
    if(!l_error)
    {
        l_params.AddElement(a_value);
        l_invar = m_variable_list_ref->variablemap_find(a_invarname);
    }
    if (!l_error && !l_invar)
        l_error = MakeString(NewChar, "ERROR: IDiv: can't find '", a_invarname, "' in variablelist");

    if (!l_error)
        for (unsigned int ch = 0; ch < l_invar->m_total_samples.m_size; ch++)
            for (unsigned int i = 0; i < l_invar->m_widths.m_data[ch]; ++i)
                l_invar->m_data[ch][i] /= l_params.m_data[0];

    return l_error;
}

char* IAdd(char* a_invarname, char* a_value)
{
    char* l_error = nullptr;
    CVariable* l_invar = nullptr;
    DoubleVec l_params;
    if (!a_invarname || !a_value)
        l_error = MakeString(NewChar, "ERROR: IAdd: not enough arguments, 'invarname' or 'value' is missing!");
    if(!l_error)
    {
        l_params.AddElement(a_value);
        l_invar = m_variable_list_ref->variablemap_find(a_invarname);
    }
    if (!l_error && !l_invar)
        l_error = MakeString(NewChar, "ERROR: IAdd: can't find '", a_invarname, "' in variablelist");

    if (!l_error)
        for (unsigned int ch = 0; ch < l_invar->m_total_samples.m_size; ch++)
            for (unsigned int i = 0; i < l_invar->m_widths.m_data[ch]; ++i)
                l_invar->m_data[ch][i] += l_params.m_data[0];

    return l_error;
}

char* DotDiff(char* a_outvar, char* a_out_ch, char* a_lhs, char* a_lhs_ch, char* a_rhs, char* a_rhs_ch)
{
    if (!a_outvar || !a_lhs || !a_rhs || !a_out_ch || !a_lhs_ch || !a_rhs_ch)
        return MakeString(NewChar, "ERROR: DotDiff: not enough arguments");
    CVariable* lhs = m_variable_list_ref->variablemap_find(a_lhs);
    CVariable* rhs = m_variable_list_ref->variablemap_find(a_rhs);
    CVariable* output = m_variable_list_ref->variablemap_find(a_outvar);
    if (!lhs || !rhs || !output)
        return MakeString(NewChar, "ERROR: DotDiff: a viariable does not exists");

    UIntVec out_ch, lhs_ch, rhs_ch;
    out_ch.RebuildFrom(a_out_ch);
    lhs_ch.RebuildFrom(a_lhs_ch);
    rhs_ch.RebuildFrom(a_rhs_ch);

    cout << "lhs size:" << lhs->m_widths.m_size << "\n";
    cout << "rhs size:" << rhs->m_widths.m_size << "\n";
    cout << "output size:" << output->m_widths.m_size << "\n";
    cout << "---------------------------\n";
    cout << "lhs_ch size: " << lhs_ch.m_size << "\n";
    cout << "rhs_ch size: " << rhs_ch.m_size << "\n";
    cout << "out_ch size: " << out_ch.m_size << "\n";


    if (lhs_ch.m_size != rhs_ch.m_size || rhs_ch.m_size != out_ch.m_size)
    {
        return MakeString(NewChar, "ERROR: DotDiff: number of channel indexes does not match");
    }

    UIntVec new_sizes;
    new_sizes.Rebuild(output->m_widths.m_size);
    for (unsigned int i = 0; i < output->m_widths.m_size; ++i)
        new_sizes.m_data[i] = output->m_widths.m_data[i];

    for (unsigned int i = 0; i < out_ch.m_size; ++i)
        new_sizes.m_data[out_ch.m_data[i]] = lhs->m_widths.m_data[lhs_ch.m_data[i]];
    output->RebuildPreserve(output->m_total_samples.m_size, new_sizes.m_data);
    for (unsigned int i = 0; i < out_ch.m_size; ++i)
    {
        unsigned int outch = out_ch.m_data[i];
        unsigned int lhsch = lhs_ch.m_data[i];
        unsigned int rhsch = rhs_ch.m_data[i];
        for (unsigned int j = 0; j < output->m_widths.m_data[outch]; ++j)
            output->m_data[outch][j] = lhs->m_data[lhsch][j] - rhs->m_data[rhsch][j];
    }
    return 0;
}

char* DerivReal(char* a_outvarname, char* a_invarname)
{
    if (!a_invarname || !a_outvarname)
        return MakeString(NewChar, "ERROR: DerivReal");
    CVariable* invar = m_variable_list_ref->variablemap_find(a_invarname);
    if (!invar)
        return MakeString(NewChar, "ERROR: DerivReal");

    CVariable* output2 = NewCVariable();
    unsigned int output2sizes[invar->m_total_samples.m_size];
    for (unsigned int i = 0; i < invar->m_total_samples.m_size; ++i)
        if (invar->m_widths.m_data[i])
            output2sizes[i] = invar->m_widths.m_data[i] - 1;
        else
            output2sizes[i] = 0;
    output2->Rebuild(invar->m_total_samples.m_size, output2sizes);
    invar->SCopyTo(output2);
    strcpy(output2->m_varname, a_outvarname);
    m_variable_list_ref->Insert(a_outvarname, output2);
    for (unsigned int ch = 0; ch < invar->m_total_samples.m_size; ch++)
        if (output2->m_widths.m_data[ch])
            for (unsigned int i = 0; i < output2->m_widths.m_data[ch]; ++i)
                output2->m_data[ch][i] = invar->m_data[ch][i + 1] - invar->m_data[ch][i];
    return 0;
}

char* DotProdInplace(char* a_lhs, char* a_rhs)
{
    if (!a_lhs || !a_rhs)
        return MakeString(NewChar, "ERROR: DotProdInplace: not enough arguments");
    CVariable* lhs = m_variable_list_ref->variablemap_find(a_lhs);
    CVariable* rhs = m_variable_list_ref->variablemap_find(a_rhs);
    if (!lhs || !rhs)
        return MakeString(NewChar, "ERROR: DotProdInplace: a viariable does not exists");

    if (lhs->m_total_samples.m_size != rhs->m_total_samples.m_size)
        cout << "DotProdInplace: number of channel indexes does not match" << endl;

    unsigned int nr_of_channels = lhs->m_total_samples.m_size;
    if (nr_of_channels > rhs->m_total_samples.m_size)
        nr_of_channels = rhs->m_total_samples.m_size;

    for (unsigned int i = 0; i < nr_of_channels; ++i)
    {
        if (lhs->m_total_samples.m_data[i] != rhs->m_total_samples.m_data[i])
            cout << "DotProdInplace: lengths does not match" << endl;
        unsigned int len = lhs->m_total_samples.m_data[i];
        if (len > rhs->m_total_samples.m_data[i])
            len = rhs->m_total_samples.m_data[i];
        cout << lhs->m_total_samples.m_data[i] << endl;
        cout << rhs->m_total_samples.m_data[i] << endl;

        for (unsigned int j = 0; j < len; ++j)
            lhs->m_data[i][j] = lhs->m_data[i][j] / ((double)(len - j + 100));
        //lhs->m_data[i][j] = lhs->m_data[i][j] / rhs->m_data[i][j];
    }
    return 0;
}

char* Substract(char* a_var1, char* a_var2)
{
    if (!a_var1 || !a_var2)
        return MakeString(NewChar, "ERROR: Substract: Not enough arguments.");
    CVariable* l_var1 = m_variable_list_ref->variablemap_find(a_var1);
    CVariable* l_var2 = m_variable_list_ref->variablemap_find(a_var2);
    if (!l_var1 || !l_var2)
        return MakeString(NewChar, "ERROR: Substract: variable does not exists.");

    unsigned int l_var1_channels = l_var1->m_total_samples.m_size;
    unsigned int l_var2_channels = l_var2->m_total_samples.m_size;
    if (l_var1_channels != l_var2_channels && l_var2_channels != 1)
        return MakeString(NewChar, "ERROR: Substract: variable channel numbers should either match, or the subtrahend's should be 1.");

    for (unsigned int i = 0; i < l_var1_channels; ++i)
    {
        unsigned int l_var2_channel_index = 0;
        if (l_var1_channels == l_var2_channels)
            l_var2_channel_index = i;
        unsigned int l_var1_samples = l_var1->m_total_samples.m_data[i];
        unsigned int l_var2_samples = l_var2->m_total_samples.m_data[l_var2_channel_index];
        if (l_var1_samples != l_var2_samples)
            return MakeString(NewChar, "ERROR: Substract: variable sample numbers should match.");
        for (unsigned int j = 0; j < l_var1_samples; ++j)
            l_var1->m_data[i][j] -= l_var2->m_data[l_var2_channel_index][j];
    }
    return 0;
}

char* Add(char* a_var1, char* a_var2)
{
    if (!a_var1 || !a_var2)
        return MakeString(NewChar, "ERROR: Add: Not enough arguments.");

    CVariable* l_var1 = m_variable_list_ref->variablemap_find(a_var1);
    CVariable* l_var2 = m_variable_list_ref->variablemap_find(a_var2);
    if (!l_var1 || !l_var2)
        return MakeString(NewChar, "ERROR: Add: variable does not exists.");

    unsigned int l_var1_channels = l_var1->m_total_samples.m_size;
    unsigned int l_var2_channels = l_var2->m_total_samples.m_size;
    if (l_var1_channels != l_var2_channels && l_var2_channels != 1)
        return MakeString(NewChar, "ERROR: Add: variable channel numbers should either match, or the subtrahend's should be 1.");

    for (unsigned int i = 0; i < l_var1_channels; ++i)
    {
        unsigned int l_var2_channel_index = 0;
        if (l_var1_channels == l_var2_channels)
            l_var2_channel_index = i;

        unsigned int l_var1_samples = l_var1->m_total_samples.m_data[i];
        unsigned int l_var2_samples = l_var2->m_total_samples.m_data[l_var2_channel_index];
        l_var1_samples = l_var1_samples < l_var2_samples ? l_var1_samples : l_var2_samples;

        for (unsigned int j = 0; j < l_var1_samples; ++j)
            l_var1->m_data[i][j] += l_var2->m_data[l_var2_channel_index][j];
    }
    return 0;
}

char* Div(char* a_var1, char* a_var2)
{
    if (!a_var1 || !a_var2)
        return MakeString(NewChar, "ERROR: Div: Not enough arguments.");

    CVariable* l_var1 = m_variable_list_ref->variablemap_find(a_var1);
    CVariable* l_var2 = m_variable_list_ref->variablemap_find(a_var2);
    if (!l_var1 || !l_var2)
        return MakeString(NewChar, "ERROR: Div: variable does not exists.");

    unsigned int l_var1_channels = l_var1->m_total_samples.m_size;
    unsigned int l_var2_channels = l_var2->m_total_samples.m_size;
    if (l_var1_channels != l_var2_channels && l_var2_channels != 1)
        return MakeString(NewChar, "ERROR: Div: variable channel numbers should either match, or the subtrahend's should be 1.");

    for (unsigned int i = 0; i < l_var1_channels; ++i)
    {
        unsigned int l_var2_channel_index = 0;
        if (l_var1_channels == l_var2_channels)
            l_var2_channel_index = i;

        unsigned int l_var1_samples = l_var1->m_total_samples.m_data[i];
        unsigned int l_var2_samples = l_var2->m_total_samples.m_data[l_var2_channel_index];
        l_var1_samples = l_var1_samples < l_var2_samples ? l_var1_samples : l_var2_samples;

        for (unsigned int j = 0; j < l_var1_samples; ++j)
            l_var1->m_data[i][j] /= l_var2->m_data[l_var2_channel_index][j];
    }
    return 0;
}

char* Prod(char* a_var1, char* a_var2)
{
    if (!a_var1 || !a_var2)
        return MakeString(NewChar, "ERROR: Div: Not enough arguments.");

    CVariable* l_var1 = m_variable_list_ref->variablemap_find(a_var1);
    CVariable* l_var2 = m_variable_list_ref->variablemap_find(a_var2);
    if (!l_var1 || !l_var2)
        return MakeString(NewChar, "ERROR: Div: variable does not exists.");

    unsigned int l_var1_channels = l_var1->m_total_samples.m_size;
    unsigned int l_var2_channels = l_var2->m_total_samples.m_size;
    if (l_var1_channels != l_var2_channels && l_var2_channels != 1)
        return MakeString(NewChar, "ERROR: Div: variable channel numbers should either match, or the subtrahend's should be 1.");

    for (unsigned int i = 0; i < l_var1_channels; ++i)
    {
        unsigned int l_var2_channel_index = 0;
        if (l_var1_channels == l_var2_channels)
            l_var2_channel_index = i;

        unsigned int l_var1_samples = l_var1->m_total_samples.m_data[i];
        unsigned int l_var2_samples = l_var2->m_total_samples.m_data[l_var2_channel_index];
        l_var1_samples = l_var1_samples < l_var2_samples ? l_var1_samples : l_var2_samples;

        for (unsigned int j = 0; j < l_var1_samples; ++j)
            l_var1->m_data[i][j] *= l_var2->m_data[l_var2_channel_index][j];
    }
    return 0;
}

char* GetXVals(char* a_outvarname, char* a_invarname, char* a_condition, char* a_value1, char* a_value2)
{
    if (!a_invarname || !a_outvarname)
        return MakeString(NewChar, "ERROR: GetXVals");

    DoubleVec params;
    params.AddElement(a_value1);
    params.AddElement(a_value2);

    CVariable* invar = m_variable_list_ref->variablemap_find(a_invarname);
    if (!invar)
        return MakeString(NewChar, "ERROR: GetXVals: can't find data in variablelist: '", a_invarname, "'");

    if (!strcmp(a_condition, ">"))
    {
        int count = 0;
        for (unsigned int i = 0; i < invar->m_widths.m_data[0]; ++i)
            if (invar->m_data[0][i] > params.m_data[0])
                count++;
        CVariable* output2 = NewCVariable();
        unsigned int output2sizes[1];
        output2sizes[0] = count;
        output2->Rebuild(1, output2sizes);
        invar->SCopyTo(output2);

        strcpy(output2->m_horizontal_units, "Sample");
        strcpy(output2->m_vertical_units.m_data[0].s, invar->m_horizontal_units);
        strcpy(output2->m_varname, a_outvarname);
        output2->m_sample_rates.m_data[0] = 1;
        m_variable_list_ref->Insert(a_outvarname, output2);
        count = 0;
        for (unsigned int i = 0; i < invar->m_widths.m_data[0]; ++i)
            if (invar->m_data[0][i] > params.m_data[0])
                output2->m_data[0][count++] = i / invar->m_sample_rates.m_data[0];
        return 0;
    }
    return MakeString(NewChar, "ERROR: GetXVals: Unknown condition: ", a_condition);
}

char* SpikeDetect(char* a_outvarname, char* a_invarname, char* a_kriter, char* a_spikelen)
{
    if (!a_invarname || !a_outvarname)
        return MakeString(NewChar, "ERROR: SpikeDetect");

    DoubleVec params;
    params.AddElement(1);
    params.AddElement(a_kriter);
    params.AddElement(a_spikelen);

    CVariable* invar = m_variable_list_ref->variablemap_find(a_invarname);
    if (!invar)
        return MakeString(NewChar, "ERROR: SpikeDetect");

    CVariable* output2 = NewCVariable();
    output2->Rebuild(invar->m_total_samples.m_size, invar->m_total_samples.m_data);
    invar->SCopyTo(output2);
    strcpy(output2->m_varname, a_outvarname);
    m_variable_list_ref->Insert(a_outvarname, output2);

    for (unsigned int ch = 0; ch < invar->m_total_samples.m_size; ch++)
    {
        double kriter = params.m_data[1] * params.m_data[0];
        int scount = 0;
        int isamples = (int)(invar->m_sample_rates.m_data[ch] * params.m_data[2]);
        int neres = (int)(invar->m_sample_rates.m_data[ch] * params.m_data[2] / 3.0);
        for (unsigned int i = neres; i < invar->m_widths.m_data[ch] - isamples; ++i)
        {
            if ((invar->m_data[ch][i] >= kriter) && (invar->m_data[ch][i] > invar->m_data[ch][i - neres]))
            {
                int maxindx = i;
                double maxval = MININT;
                int j = 0;

                for (int k = 0; k < isamples; ++k)
                {
                    if (invar->m_data[ch][i + k] < kriter)
                        goto nospindle;
                }

                while(i + j + neres < invar->m_widths.m_data[ch])
                {
                    if (maxval < invar->m_data[ch][i + j])
                    {
                        maxval = invar->m_data[ch][i + j];
                        maxindx = j;
                    }
                    if (invar->m_data[ch][i + j] - invar->m_data[ch][i + j + neres] > kriter / 10)
                        goto spindle;
                    ++j;
                }
spindle:
                output2->m_data[ch][i + maxindx] = 1000;
                scount++;
                i += (int)(isamples + j);
nospindle:
                i = i;
            }
        }
    }
    return 0;
}

char* LoadKrit(char* a_outdataname, char* a_critfilename)
{
    CVariable* output2 = NewCVariable();
    unsigned int output2sizes[4];

    byte* buff = LoadBuffer(a_critfilename);
    DoubleVec dv1;
    dv1.RebuildFrom((char*)buff);

    output2sizes[0] = 10;
    output2sizes[1] = 10;
    output2sizes[2] = 10;
    output2sizes[3] = 10;
    output2->Rebuild(4, output2sizes);

    for (unsigned int i = 0; i < 10; ++i)
    {
        output2->m_data[0][i] = dv1.m_data[i + 3];
        output2->m_data[1][i] = dv1.m_data[i + 3 + 13];
        output2->m_data[2][i] = dv1.m_data[i + 3 + 26];
    }
    output2->m_data[3][0] = dv1.m_data[13 * 3];
    output2->m_data[3][1] = dv1.m_data[13 * 3 + 1];
    output2->m_data[3][2] = dv1.m_data[13 * 3 + 2];
    output2->m_data[3][3] = dv1.m_data[13 * 3 + 3];

    strcpy(output2->m_varname, a_outdataname);
    m_variable_list_ref->Insert(a_outdataname, output2);
    return 0;
}

CVariable* oszamlesf(char* a_spavgpos, CVariable* a_szurlet1, CVariable* a_slowws, int a_swindx)
{
    int seconds = 6;

    double fs = a_szurlet1->m_sample_rates.m_data[0];
    int interv = (int)(fs * seconds);

    CVariable* spavgposdat = NewCVariable();
    unsigned int spavgposdatsizes[a_szurlet1->m_total_samples.m_size];
    for (unsigned int i = 0; i < a_szurlet1->m_total_samples.m_size; ++i)
        spavgposdatsizes[i] = interv;
    spavgposdat->Rebuild(a_szurlet1->m_total_samples.m_size, spavgposdatsizes);

    int swcount = a_slowws->m_widths.m_data[a_swindx];
    CMatrix <double> ints;
    ints.Rebuild(swcount, interv);

    int itemnr = 0;
    int lastswchindx = (int) a_slowws->m_data[a_swindx][0];

    for (int indxx = 0; indxx < swcount; indxx++)
    {
        int i = (int) ( a_slowws->m_data[a_swindx + 1][indxx] * a_szurlet1->m_sample_rates.m_data[lastswchindx] );
        if (lastswchindx != (int) a_slowws->m_data[a_swindx][indxx])
        {
            for (int m = 0; m < itemnr; m++)
                for (int k = 0; k < interv; ++k)
                    spavgposdat->m_data[lastswchindx][k] += ints.m_data[m][k];
            if (itemnr)
                for (int k = 0; k < interv; ++k)
                    spavgposdat->m_data[lastswchindx][k] /= (double)itemnr;
            itemnr = 0;
        }
        if ((((unsigned int)(i - floor(interv * 0.5) + 1)) >= 0) && ((unsigned int)(i - floor(interv * 0.5) + 1) + interv) < a_szurlet1->m_widths.m_data[(int)a_slowws->m_data[a_swindx][indxx]])
        {
            memcpy(ints.m_data[itemnr], a_szurlet1->m_data[(int)a_slowws->m_data[a_swindx][indxx]] + (int)(i - floor(interv * 0.5) + 1), interv * sizeof(double));
            lastswchindx = (int) a_slowws->m_data[a_swindx][indxx];
            itemnr++;
        }
    }
    for (int m = 0; m < itemnr; m++)
        for (int k = 0; k < interv; ++k)
            spavgposdat->m_data[lastswchindx][k] += ints.m_data[m][k];
    if (itemnr)
        for (int k = 0; k < interv; ++k)
            spavgposdat->m_data[lastswchindx][k] /= (double)itemnr;

    strcpy(spavgposdat->m_varname, a_spavgpos);
    return spavgposdat;
}

char* oszamlesf(char* a_covereddata, char* a_spavgpos, char* a_spavgneg, char* a_slowwaves)
{
    if (!a_spavgpos || !a_spavgneg || !a_slowwaves)
        return MakeString(NewChar, "ERROR: oszamlesf");

    CVariable* szurlet1 = m_variable_list_ref->variablemap_find(a_covereddata);
    CVariable* slowws = m_variable_list_ref->variablemap_find(a_slowwaves);
    if (!szurlet1 || !slowws)
        return MakeString(NewChar, "ERROR: oszamlesf");

    CVariable* spavgposdat = oszamlesf (a_spavgpos, szurlet1, slowws, 0);
    m_variable_list_ref->Insert(spavgposdat->m_varname, spavgposdat);
    CVariable* spavgnegdat = oszamlesf (a_spavgneg, szurlet1, slowws, 2);
    m_variable_list_ref->Insert(spavgnegdat->m_varname, spavgnegdat);

    return 0;
}

char* mutato(char* a_spavgpos1, char* a_spavgneg1, char* a_spavgpos2, char* a_spavgneg2, char* a_spcountandswrel)
{
    if (!a_spavgpos1 || !a_spavgneg2 || !a_spcountandswrel || !a_spavgpos2 || !a_spavgneg2)
        return MakeString(NewChar, "ERROR: mutato");

    CVariable* spavgpos1dat = m_variable_list_ref->variablemap_find(a_spavgpos1);
    CVariable* spavgneg1dat = m_variable_list_ref->variablemap_find(a_spavgneg1);
    CVariable* spavgpos2dat = m_variable_list_ref->variablemap_find(a_spavgpos2);
    CVariable* spavgneg2dat = m_variable_list_ref->variablemap_find(a_spavgneg2);
    CVariable* spcountandswreldat = m_variable_list_ref->variablemap_find(a_spcountandswrel);
    if (!spavgpos1dat || !spavgneg1dat || !spavgpos2dat || !spavgneg2dat || !spcountandswreldat)
        return MakeString(NewChar, "ERROR: mutato");

    for (unsigned int ch = 0; ch < spavgpos1dat->m_total_samples.m_size; ch++)
    {
        spcountandswreldat->m_data[2][ch] = findmax(spavgpos1dat->m_data[ch], spavgpos1dat->m_widths.m_data[ch]) / findmin(spavgpos1dat->m_data[ch], spavgpos1dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[3][ch] = findmax(spavgneg1dat->m_data[ch], spavgneg1dat->m_widths.m_data[ch]) / findmin(spavgneg1dat->m_data[ch], spavgneg1dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[4][ch] = findmax(spavgpos2dat->m_data[ch], spavgpos2dat->m_widths.m_data[ch]) / findmin(spavgpos2dat->m_data[ch], spavgpos2dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[5][ch] = findmax(spavgneg2dat->m_data[ch], spavgneg2dat->m_widths.m_data[ch]) / findmin(spavgneg2dat->m_data[ch], spavgneg2dat->m_widths.m_data[ch]);
    }
    for (unsigned int ch = 0; ch < spavgpos1dat->m_total_samples.m_size; ch++)
    {
        spcountandswreldat->m_data[6][ch] = (spcountandswreldat->m_data[2][ch] + spcountandswreldat->m_data[3][ch]) / 2.0;
        spcountandswreldat->m_data[7][ch] = (spcountandswreldat->m_data[4][ch] + spcountandswreldat->m_data[5][ch]) / 2.0;
    }
    Call ("CreateSine (outdataname, 10000 10000, 1000 1000, 100 30 10); DisplayData(outdataname);");
    return 0;
}

char* Mutato2(char* a_spavgpos1, char* a_spavgneg1, char* a_spavgpos2, char* a_spavgneg2, char* a_spcountandswrel)
{
    if (!a_spavgpos1 || !a_spavgneg2 || !a_spcountandswrel || !a_spavgpos2 || !a_spavgneg2)
        return MakeString(NewChar, "ERROR: mutato");

    CVariable* spavgpos1dat = m_variable_list_ref->variablemap_find(a_spavgpos1);
    CVariable* spavgneg1dat = m_variable_list_ref->variablemap_find(a_spavgneg1);
    CVariable* spavgpos2dat = m_variable_list_ref->variablemap_find(a_spavgpos2);
    CVariable* spavgneg2dat = m_variable_list_ref->variablemap_find(a_spavgneg2);
    CVariable* spcountandswreldat = m_variable_list_ref->variablemap_find(a_spcountandswrel);
    if (!spavgpos1dat || !spavgneg1dat || !spavgpos2dat || !spavgneg2dat || !spcountandswreldat)
        return MakeString(NewChar, "ERROR: mutato");

    for (unsigned int ch = 0; ch < spavgpos1dat->m_total_samples.m_size; ch++)
    {
        spcountandswreldat->m_data[2][ch] = findmax(spavgpos1dat->m_data[ch], spavgpos1dat->m_widths.m_data[ch]) / findmin(spavgpos1dat->m_data[ch], spavgpos1dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[3][ch] = findmax(spavgneg1dat->m_data[ch], spavgneg1dat->m_widths.m_data[ch]) / findmin(spavgneg1dat->m_data[ch], spavgneg1dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[4][ch] = findmax(spavgpos2dat->m_data[ch], spavgpos2dat->m_widths.m_data[ch]) / findmin(spavgpos2dat->m_data[ch], spavgpos2dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[5][ch] = findmax(spavgneg2dat->m_data[ch], spavgneg2dat->m_widths.m_data[ch]) / findmin(spavgneg2dat->m_data[ch], spavgneg2dat->m_widths.m_data[ch]);
    }
    for (unsigned int ch = 0; ch < spavgpos1dat->m_total_samples.m_size; ch++)
    {
        spcountandswreldat->m_data[10][ch] = (spcountandswreldat->m_data[2][ch] + spcountandswreldat->m_data[3][ch]) / 2.0;
        spcountandswreldat->m_data[11][ch] = (spcountandswreldat->m_data[4][ch] + spcountandswreldat->m_data[5][ch]) / 2.0;
    }
    for (unsigned int ch = 0; ch < spavgpos1dat->m_total_samples.m_size; ch++)
    {
        spcountandswreldat->m_data[2][ch] = 0; //findmax(spavgpos1dat->m_data[ch],spavgpos1dat->m_widths.m_data[ch])/findmin(spavgpos1dat->m_data[ch],spavgpos1dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[3][ch] = 0; //findmax(spavgneg1dat->m_data[ch],spavgneg1dat->m_widths.m_data[ch])/findmin(spavgneg1dat->m_data[ch],spavgneg1dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[4][ch] = 0; //findmax(spavgpos2dat->m_data[ch],spavgpos2dat->m_widths.m_data[ch])/findmin(spavgpos2dat->m_data[ch],spavgpos2dat->m_widths.m_data[ch]);
        spcountandswreldat->m_data[5][ch] = 0; //findmax(spavgneg2dat->m_data[ch],spavgneg2dat->m_widths.m_data[ch])/findmin(spavgneg2dat->m_data[ch],spavgneg2dat->m_widths.m_data[ch]);
    }
    return 0;
}

char* Cout(char* a_param1, char* a_param2, char* a_param3, char* a_param4, char* a_param5, char* a_param6, char* a_param7, char* a_param8, char* a_param9, char* a_param10)
{
    if (a_param1)
        cout << a_param1 << "  ";
    if (a_param2)
        cout << a_param2 << "  ";
    if (a_param3)
        cout << a_param3 << "  ";
    if (a_param4)
        cout << a_param4 << "  ";
    if (a_param5)
        cout << a_param5 << "  ";
    if (a_param6)
        cout << a_param6 << "  ";
    if (a_param7)
        cout << a_param7 << "  ";
    if (a_param8)
        cout << a_param8 << "  ";
    if (a_param9)
        cout << a_param9 << "  ";
    if (a_param10)
        cout << a_param10 << "  ";
    cout << endl;
    return 0;
}

char* Transpose(char* a_dst_var_name, char* a_src_var_name, char* a_src_start_x, char* a_src_start_y, char* a_src_stop_x, char* a_src_stop_y)
{
    CVariable* l_src_var = m_variable_list_ref->variablemap_find(a_src_var_name);
    if (!l_src_var)
        return MakeString(NewChar, "ERROR: Transpose: can't find data in variablelist: '", a_src_var_name, "'");
    IntVec coords;
    coords.AddElement(a_src_start_x);
    coords.AddElement(a_src_start_y);
    coords.AddElement(a_src_stop_x);
    coords.AddElement(a_src_stop_y);
    if (coords.m_size && coords.m_size < 4) /** In case coordinate parameters are used, check whether enough coordinates are supplied or not. */
        return MakeString(NewChar, "ERROR: Transpose: Wrong number of arguments: not enough source coordinates!");
    if (!l_src_var->m_total_samples.m_size)
        return 0;
    else if (!l_src_var->m_total_samples.m_data[0])
        return 0;
    if (!coords.m_size)
    {
        coords.AddElement(0);
        coords.AddElement(0);
        coords.AddElement(l_src_var->m_total_samples.m_size);
        coords.AddElement(l_src_var->m_total_samples.m_data[0]);
    }
    else
    {
        int l_start_x = coords.m_data[0];
        int l_start_y = coords.m_data[1];
        unsigned l_stop_x = coords.m_data[2];
        unsigned l_stop_y = coords.m_data[3];
        if (l_start_x < 0 || l_start_y < 0)
            return MakeString(NewChar, "ERROR: Transpose: negative start coordinates not applicable");
        if (l_stop_x > l_src_var->m_total_samples.m_size)
            return MakeString(NewChar, "ERROR: Transpose: stop x coordinate is out of the range");
        if (l_stop_y > l_src_var->m_total_samples.m_data[0])
            return MakeString(NewChar, "ERROR: Transpose: stop y coordinate is out of the range");
    }

    CVariable* l_dst_var = NewCVariable();
    UIntVec N;
    N.Rebuild(coords.m_data[3] - coords.m_data[1]);
    for (unsigned int i = 0; i < N.m_size; ++i)
        N.m_data[i] = coords.m_data[2] - coords.m_data[0];

    l_dst_var->Rebuild(coords.m_data[3] - coords.m_data[1], N.m_data);

    for (unsigned int i = 0; i < l_dst_var->m_total_samples.m_size; ++i)
    {
        for (unsigned int j = 0; j < l_dst_var->m_widths.m_data[0]; ++j)
            l_dst_var->m_data[i][j] = l_src_var->m_data[j + coords.m_data[0]][i + coords.m_data[1]];
    }

    strcpy(l_dst_var->m_varname, a_dst_var_name);
    m_variable_list_ref->Insert(l_dst_var->m_varname, l_dst_var);
    return 0;
}

extern "C"
{
    char* __declspec (dllexport) Procedure(int procindx, char* a_statement_body, char* a_param1, char* a_param2, char* a_param3, char* a_param4, char* a_param5, char* a_param6, char* a_param7, char* a_param8, char* a_param9, char* a_param10, char* a_param11, char* a_param12)
    {
        switch (procindx)
        {
        case 0:
            return mfilter_rev_bp(a_param1);
        case 1:
            return CreateSine(a_param1, a_param2, a_param3, a_param4);
        case 2:
            return BandpassButterbase6thOrderZerophase(a_param1, a_param2, a_param3);
        case 3:
            return Cover1(a_param1);
        case 4:
            return swindxdetect(a_param1, a_param2, a_param3);
        case 5:
            return LoadKrit(a_param1, a_param2);
        case 6:
            return orsocounter(a_param1, a_param2, a_param3, a_param4);
        case 7:
            return GetVal(a_param1, a_param2, a_param3);
        case 8:
            return BandpassButterbase6thOrderZerophaseC(a_param1, a_param2, a_param3, a_param4);
        case 9:
            return SQRBandpassButterbase6thOrderZerophaseC(a_param1, a_param2, a_param3, a_param4);
        case 10:
            return SQRT_Inplace(a_param1);
        case 11:
            return SQR_Inplace(a_param1);
        case 12:
            return oszamlesf(a_param1, a_param2, a_param3, a_param4);
        case 13:
            return mutato(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 14:
            return Orsocounter2(a_param1, a_param2, a_param3, a_param4);
        case 15:
            return ABSBandpassButterbase6thOrderZerophaseC(a_param1, a_param2, a_param3, a_param4);
        case 16:
            return Mutato2(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 17:
            return SpindleCount(a_param1, a_param2, a_param3, a_param4);
        case 18:
            return CreateFilter(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6);
        case 19:
            return Filter(a_param1, a_param2);
        case 20:
            return Transpose(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6);
        case 21:
            return SpikeDetect(a_param1, a_param2, a_param3, a_param4);
        case 22:
            return IProd(a_param1, a_param2);
        case 23:
            return HighpassButterbase6thOrderZerophase(a_param1, a_param2);
        case 24:
            return GetXVals(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 25:
            return DerivReal(a_param1, a_param2);
        case 26:
            return Substract(a_param1, a_param2);
        case 27:
            return Add(a_param1, a_param2);
        case 28:
            return FilterReverse(a_param1, a_param2);
        case 29:
            return DotDiff(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6);
        case 30:
            return IAdd(a_param1, a_param2);
        case 31:
            return CreateFromTo(a_param1, a_param2, a_param3, a_param4, a_param5);
        case 32:
            return DotProdInplace(a_param1, a_param2);
        case 33:
            return FIRFilter(a_param1, a_param2);
        case 34:
            return FilterL(a_param1, a_param2);
        case 35:
            return IDiv(a_param1, a_param2);
        case 36:
            return Div(a_param1, a_param2);
        case 37:
            return Prod(a_param1, a_param2);
        case 38:
            return GetMin(a_param1);
        case 39:
            return GetMax(a_param1);
        case 40:
            return Cout(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param6, a_param6, a_param6, a_param7);
        }
        return 0;
    }

    void __declspec (dllexport) InitLib(CSignalCodec_List* a_datalist, CVariable_List* a_variablelist, NewCVariable_Proc a_newCVariable, NewChar_Proc a_newChar, Call_Proc a_inpCall, FFT_PROC a_inpFFT, RFFT_PROC a_inpRFFT, FFTEXEC_PROC a_inpFFTEXEC, FFTDESTROY_PROC a_inpFFTDESTROY)
    {
        m_variable_list_ref = a_variablelist;
        m_data_list_ref     = a_datalist;
        NewChar             = a_newChar;
        NewCVariable        = a_newCVariable;
        Call                = a_inpCall;
    }

    int __declspec (dllexport) GetProcedureList(TFunctionLibrary* a_functionlibrary_reference)
    {
        CStringVec FunctionList;
        FunctionList.AddElement("mfilter_rev_bp(indataname'the input data')");
        FunctionList.AddElement("CreateSine(outdataname, nrsamples, samplerates, frequencies)");
        FunctionList.AddElement("BandpassButterbase6thOrderZerophase(invarname, lbound, hbound)");
        FunctionList.AddElement("Cover1(invarname)");
        FunctionList.AddElement("swindxdetect(invarname, outdataname, spswkrits)");
        FunctionList.AddElement("LoadKrit(outdataname, critfilename)");
        FunctionList.AddElement("orsocounter(invarname, spswkrits , spindx, spcountandswrel)");
        FunctionList.AddElement("GetVal(invarname, X , Y)");
        FunctionList.AddElement("BandpassButterbase6thOrderZerophaseC(invarname, varnametostore, lbound, hbound)");
        FunctionList.AddElement("SQRBandpassButterbase6thOrderZerophaseC(invarname, varnametostore, lbound, hbound)");
        FunctionList.AddElement("SQRT_Inplace(invarname)");
        FunctionList.AddElement("SQR_Inplace(invarname)");
        FunctionList.AddElement("oszamlesf(covereddata, spavgpos, spavgneg, slowwaves)");
        FunctionList.AddElement("mutato(spavgpos1, spavgneg1, spavgpos2, spavgneg2, spcountandswrel)");
        FunctionList.AddElement("Orsocounter2(invarname, spswkrits, spindx, spcountandswrel)");
        FunctionList.AddElement("ABSBandpassButterbase6thOrderZerophaseC(invarname, varnametostore, lbound, hbound)");
        FunctionList.AddElement("Mutato2(spavgpos1, spavgneg1, spavgpos2, spavgneg2, spcountandswrel)");
        FunctionList.AddElement("SpindleCount(invarname, spswkrits, spindx, spcountandswrel)");
        FunctionList.AddElement("CreateFilter(outdataname, shape_kind, order_samplingrate, edges, iripple, iellipstopbandedge)");
        FunctionList.AddElement("Filter(dataname, filter)");
        FunctionList.AddElement("Transpose(destination, source, sourcestartx, sourcestarty, sourcestopx, sourcestopy)");
        FunctionList.AddElement("SpikeDetect(outvarname, invarname, kriter, spikelen)");
        FunctionList.AddElement("IProd(invarname, value)");
        FunctionList.AddElement("HighpassButterbase6thOrderZerophase(invarname, lbound)");
        FunctionList.AddElement("GetXVals(outvarname, invarname, condition, value1, value2)");
        FunctionList.AddElement("DerivReal(outvarname, invarname)");
        FunctionList.AddElement("Substract(var1, var2)");
        FunctionList.AddElement("Add(var1, var2)");
        FunctionList.AddElement("FilterReverse(dataname, filter)");
        FunctionList.AddElement("DotDiff(a_outvar, a_out_ch, a_lhs, a_lhs_ch, a_rhs, a_rhs_ch)");
        FunctionList.AddElement("IAdd(invarname, value)");
        FunctionList.AddElement("CreateFromTo(a_outdataname, a_start, a_stop, a_step, a_samplerates)");
        FunctionList.AddElement("DotProdInplace(a_lhs, a_rhs)");
        FunctionList.AddElement("FIRFilter(indataname'the input data', filter_name'FIR filter coefficient')");
        FunctionList.AddElement("FilterL(dataname, filter)");
        FunctionList.AddElement("IDiv(invarname, value)");
        FunctionList.AddElement("Div(var1, var2)");
        FunctionList.AddElement("Prod(var1, var2)");
        FunctionList.AddElement("GetMin(invarname)");
        FunctionList.AddElement("GetMax(invarname)");
        FunctionList.AddElement("Cout(a_param1, a_param2, a_param3, a_param4, a_param5, a_param6, a_param6, a_param6, a_param6, a_param7)");
        a_functionlibrary_reference->ParseFunctionList(&FunctionList);
        return FunctionList.m_size;
    }

    bool __declspec (dllexport) CopyrightInfo (CStringMx* a_copyrightinfo)
    {
        a_copyrightinfo->RebuildPreserve(a_copyrightinfo->m_size + 1);
        a_copyrightinfo->m_data[a_copyrightinfo->m_size - 1]->AddElement("This library is.");
        return 0;
    }

    bool __stdcall DllMain(int a_hInst, int a_reason, int a_reserved)
    {
        return true;
    }
}
