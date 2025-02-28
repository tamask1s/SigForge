#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Distances.h"

double PCorrelator::Distance(const double* vec, const double* vec2, unsigned int aW, DistanceMetric distance_metric)
{
    return DistanceC(vec, 0, vec2, aW, distance_metric);
}

double PCorrelator::DistanceC(const double* vec, unsigned int aIX, const double* vec2, unsigned int aW, DistanceMetric distance_metric)
{
    double distance = 0.0f;
    double n1 = 0.0f, n2 = 0.0f;

    switch (distance_metric)
    {
    default:
    case EUCL: /// Euclidian
        for (unsigned int i = 0; i < aW; ++i)
            distance += (vec[i + aIX] - vec2[i]) * (vec[i + aIX] - vec2[i]);
        return sqrt(distance);

    case SOSD: /// Sum of squared distances
        for (unsigned int i = 0; i < aW; ++i)
            distance += (vec[i + aIX] - vec2[i]) * (vec[i + aIX] - vec2[i]);
        return distance;

    case TXCB: /// Manhattan / Taxicab / CityBlock
        for (unsigned int i = 0; i < aW; ++i)
            distance += fabs(vec[i + aIX] - vec2[i]);
        return distance;

    case CONV: /// Conv
        for (unsigned int i = 0; i < aW; ++i)
            distance += vec[i + aIX] * vec2[i];
        return distance;

    case ANGL: /// Angle between vectors
        for (unsigned int i = 0; i < aW; ++i)
        {
            distance += vec[i + aIX] * vec2[i];
            n1 += vec[i + aIX] * vec[i + aIX];
            n2 += vec2[i] * vec2[i];
        }
        if (n1 == 0.0 || n2 == 0.0)
            return 0;
        //return acos(distance / (sqrt(n1)*sqrt(n2)));
        return /*acos*/(distance / (sqrt(n1 * n2)));

    case PCC: /// Pearson correlation
    {
        double sumX = 0;
        double sumX2 = 0;
        double sumY = 0;
        double sumY2 = 0;
        double sumXY = 0;
        int n = aW;
        for (unsigned int i = 0; i < aW; i += 1)
        {
            double x = vec[i + aIX];
            double y = vec2[i];
            sumX += x;
            sumX2 += x * x;
            sumY += y;
            sumY2 += y * y;
            sumXY += x * y;
        }
        int n2 = n * n;
        double num = sumXY / n - sumX * sumY / n2;
        double denum = sqrt((sumX2 / n - sumX * sumX / n2) * (sumY2 / n - sumY * sumY / n2));
        if (!denum)
            return 0;
        return num / denum;
    }
    }
}

double PCorrelator::DistanceCM(const double* vec, unsigned int aIX, const double* vec2, unsigned int aW, const unsigned char* aMask, DistanceMetric distance_metric)
{
    double distance = 0.0f;
    double n1 = 0.0f, n2 = 0.0f;

    switch (distance_metric)
    {
    default:
    case EUCL: /// Euclidian
        for (unsigned int i = 0; i < aW; ++i)
            distance += aMask[i] ? ((vec[i + aIX] - vec2[i]) * (vec[i + aIX] - vec2[i])) : 0;
        return sqrt(distance);

    case SOSD: /// Sum of squared distances
        for (unsigned int i = 0; i < aW; ++i)
            distance += aMask[i] ? ((vec[i + aIX] - vec2[i]) * (vec[i + aIX] - vec2[i])) : 0;
        return distance;

    case TXCB: /// Manhattan / Taxicab / CityBlock
        for (unsigned int i = 0; i < aW; ++i)
            distance += aMask[i] ? fabs(vec[i + aIX] - vec2[i]) : 0;
        return distance;

    case ANGL: /// Angle between vectors
        for (unsigned int i = 0; i < aW; ++i)
        {
            distance += aMask[i] ? (vec[i + aIX] * vec2[i]) : 0;
            n1 += aMask[i] ? (vec[i + aIX] * vec[i + aIX]) : 0;
            n2 += aMask[i] ? (vec2[i] * vec2[i]) : 0;
        }
        if (n1 == 0.0 || n2 == 0.0)
            return 0;
        //return acos(distance / (sqrt(n1)*sqrt(n2)));
        return /*acos*/(distance / (sqrt(n1 * n2)));

    case PCC: /// Pearson correlation
    {
        double sumX = 0;
        double sumX2 = 0;
        double sumY = 0;
        double sumY2 = 0;
        double sumXY = 0;
        int n = aW;
        for (unsigned int i = 0; i < aW; i += 1)
        {
            double x = aMask[i] ? vec[i + aIX] : 0;
            double y = aMask[i] ? vec2[i] : 0;
            sumX += x;
            sumX2 += x * x;
            sumY += y;
            sumY2 += y * y;
            sumXY += x * y;
        }
        int n2 = n * n;
        double num = sumXY / n - sumX * sumY / n2;
        double denum = sqrt((sumX2 / n - sumX * sumX / n2) * (sumY2 / n - sumY * sumY / n2));
        if (!denum)
            return 0;
        return num / denum;
    }
    }
}

void PCorrelator::DistanceMap(double* corr, const double* small, int smallw, const double* big, int bigw, DistanceMetric distance_metric, int* a_maxindx, int* a_minindx, double* a_maxval, double* a_minval)
{
    double maxval = -1000000;
    double minval = 1000000;
    int minindx = 0;
    int maxindx = 0;
    for (int i = 0; i < bigw; ++i)
        ((double*)corr)[i] = 0;

    //memset(corr, 0, bigw * sizeof(double));
    //memset((double*)small, 0, smallw * sizeof(double));

    //  for (int i = 0; i < smallw; ++i)
//        ((double*)small)[i] = 0;



//    ((double*)small)[smallw / 2 - 86] = 1;
//    ((double*)small)[smallw / 2 - 87] = 1;
//    ((double*)small)[smallw / 2 - 88] = 1;
//    ((double*)small)[smallw / 2 - 89] = 1;
//    ((double*)small)[smallw / 2 - 90] = 1;
//    ((double*)small)[smallw / 2 - 91] = 1;
//    ((double*)small)[smallw / 2 - 92] = 1;
//
//    ((double*)small)[smallw / 2 - 59] = -1;
//    ((double*)small)[smallw / 2 - 60] = -1;
//    ((double*)small)[smallw / 2 - 61] = -1;
//    ((double*)small)[smallw / 2 - 62] = -1;
//    ((double*)small)[smallw / 2 - 63] = -1;
//    ((double*)small)[smallw / 2 - 64] = -1;
//    ((double*)small)[smallw / 2 - 65] = -1;
    int r = smallw / 2;
    for (int i = 0; i < bigw - smallw + 1; ++i)
    {
        corr[i + r] = PCorrelator::DistanceC(big, i, small, smallw, distance_metric);
        //corr[i + smallw / 2] = big[i];
        if ((a_maxval || a_maxindx) && (maxval < corr[i]))
        {
            maxval = corr[i];
            maxindx = i;
        }
        if ((a_minval || a_minindx) && (minval > corr[i]))
        {
            minval = corr[i];
            minindx = i;
        }
    }
    if (a_maxindx)
        *a_maxindx = maxindx;
    if (a_minindx)
        *a_minindx = minindx;
    if (a_maxval)
        *a_maxval = maxval;
    if (a_minval)
        *a_minval = minval;
}

void PCorrelator::DistanceMapM(double* corr, const double* small, int smallw, const double* big, int bigw, const unsigned char* aMask, DistanceMetric distance_metric, int* a_maxindx, int* a_minindx, double* a_maxval, double* a_minval)
{
    double maxval = -1000000;
    double minval = 1000000;
    int minindx = 0;
    int maxindx = 0;
    for (int i = 0; i < bigw - smallw + 1; ++i)
    {
        corr[i] = PCorrelator::DistanceCM(big, i, small, smallw, aMask, distance_metric);
        if ((a_maxval || a_maxindx) && (maxval < corr[i]))
        {
            maxval = corr[i];
            maxindx = i;
        }
        if ((a_minval || a_minindx) && (minval > corr[i]))
        {
            minval = corr[i];
            minindx = i;
        }
    }
    if (a_maxindx)
        *a_maxindx = maxindx;
    if (a_minindx)
        *a_minindx = minindx;
    if (a_maxval)
        *a_maxval = maxval;
    if (a_minval)
        *a_minval = minval;
}

double PSignalMetrics::mean(const double* a_vec, unsigned int a_len)
{
    return 0;
}

double PSignalMetrics::energy(const double* a_vec, unsigned int a_len, bool a_normalized)
{
    double result = 0;
    for (unsigned int i = 0; i < a_len; ++i)
        result += a_vec[i] * a_vec[i];
    return a_normalized ? (result / a_len) : result;
}

double PSignalMetrics::lp_norm(const double* a_vec, unsigned int a_len, bool a_normalized, DistanceMetric distance_metric)
{
    double result = 0;
    switch (distance_metric)
    {
    default:
    case EUCL: /// Euclidian
        for (unsigned int i = 0; i < a_len; ++i)
            result += a_vec[i] * a_vec[i];
        return a_normalized ? sqrt(result / a_len) : sqrt(result);

    case TXCB: /// Manhattan / Taxicab / CityBlock
        for (unsigned int i = 0; i < a_len; ++i)
            result += fabs(a_vec[i]);
        return a_normalized ? (result / a_len) : result;
    }
}

double PKernels::gaussian_pdf(double x, double m, double s)
{
    static const double inv_sqrt_2pi = 0.3989422804014327;
    double a = (x - m) / s;
    return inv_sqrt_2pi / s * exp(-0.5f * a * a);
}

/** mu: shift; a_sr: radius */
void PKernels::gauss_1d_pos(double* g, unsigned int gw, double sigma, double a_mu, double a_sr)
{
    static const double inv_sqrt_2pi = 0.3989422804014327;
    double inv_sqrt_2pi_div_sigma = inv_sqrt_2pi / sigma;
    double inv_sigma_times_sr = 1.0 / (sigma * a_sr);
    for (unsigned int x = 0; x < gw; x++)
    {
        double exc = (x - a_mu - gw * 0.5) * inv_sigma_times_sr;
        g[x] = inv_sqrt_2pi_div_sigma * exp(-0.5f * exc * exc);
    }

}
