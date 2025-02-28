#include <math.h>
#include <algorithm>
#include "CMatMatrix.hpp"

double ft_mean(double *a_data, unsigned int a_nr_samples)
{
    double v_mean = 0;
    for (unsigned int i = 0; i < a_nr_samples; ++i)
    {
        v_mean += (a_data[i] / a_nr_samples);
    }
    return (v_mean);
}

double ft_mean_flaged(double *a_data, double *a_flag, unsigned int a_nr_samples, unsigned int a_nr_flags)
{
    double v_mean = 0;
    for (unsigned int i = 0; i < a_nr_flags; ++i)
    {
        v_mean += (a_data[int(a_flag[i])] / a_nr_flags);
    }
    return (v_mean);
}

double ft_mean_sc(double *a_data, unsigned int a_nr_samples, int perci, int percf)
{
    if (perci + percf >= 90)
        return (ft_mean(a_data, a_nr_samples));
    double a_copy[a_nr_samples];
    memcpy(a_copy, a_data, a_nr_samples * sizeof(double));
    std::sort(a_copy, a_copy + a_nr_samples);
    unsigned int start = std::floor(perci * a_nr_samples / 100.0);
    unsigned int stop = a_nr_samples - std::floor(percf * a_nr_samples / 100.0);
    if (stop <= start)
        return (ft_mean(a_data, a_nr_samples));
    else
        return (ft_mean(a_copy + start, stop - start));
}

double ft_median(double *a_data, unsigned int a_nr_samples)
{
    if (a_nr_samples != 0)
    {
        double arr[a_nr_samples];
        memcpy(arr, a_data, a_nr_samples * sizeof(double));
        std::sort(arr, arr + a_nr_samples);
        unsigned int median = a_nr_samples / 2;
        return (arr[median]);
    }
    else
        return (-1.0);
}

double ft_max(double *a_data, unsigned int a_nr_samples)
{
    if (a_nr_samples != 0)
    {
        unsigned int first = a_nr_samples * 0.1;
        double v_max = a_data[first];
        for (unsigned int i = first + 1; i < a_nr_samples - first; ++i)
        {
            if (a_data[i] > v_max)
                v_max = a_data[i];
        }
        return (v_max);
    }
    else
        return (-1.0);
}

double ft_min(double *a_data, unsigned int a_nr_samples)
{
    if (a_nr_samples != 0)
    {
        unsigned int first = a_nr_samples * 0.1;
        double v_min = a_data[first];
        for (unsigned int i = first + 1; i < a_nr_samples - first; ++i)
        {
            if (a_data[i] < v_min)
                v_min = a_data[i];
        }
        return (v_min);
    }
    else
        return (-1.0);
}

int ft_max_h(double *a_data, unsigned int a_nr_samples)
{
    if (a_nr_samples != 0)
    {
        unsigned int first = a_nr_samples * 0.1;
        double v_max = a_data[first];
        int h_max = first;
        for (unsigned int i = first + 1; i < a_nr_samples - first; ++i)
        {
            if (a_data[i] > v_max)
            {
                v_max = a_data[i];
                h_max = i;
            }
        }
        return (h_max);
    }
    else
        return (-1.0);
}

double ft_std(double *a_data, unsigned int a_nr_samples)
{
    double meanCh = ft_mean(a_data, a_nr_samples);
    double stdCh = 0;
    for (unsigned int i = 0; i < a_nr_samples; ++i)
    {
        stdCh += ((a_data[i] - meanCh) * (a_data[i] - meanCh)) / a_nr_samples;
    }
    stdCh = sqrt(stdCh);
    return (stdCh);
}

CMatMatrix ft_cov(CMatMatrix &a_signal)
{
    double nr_samples = a_signal.nr_col;
    CMatMatrix cm_covariance = a_signal * a_signal.Transpose();
    cm_covariance = cm_covariance * (1.0 / nr_samples);
    return (cm_covariance);
}

int GetSmallestPowOfTwo(int a_val)
{
    int l_val = 1;
    while(l_val < a_val)
        l_val <<= 1;
    return l_val;
}

double ft_max_sc(double *a_data, unsigned int a_nr_samples, int percf)
{
    int max_sc = int(a_nr_samples) - (int(a_nr_samples) * percf / 100) - 1;
    if (max_sc < 0)
        return (ft_max(a_data, a_nr_samples));
    if (a_nr_samples != 0)
    {
        double *arr = new double[a_nr_samples];
        memcpy(arr, a_data, a_nr_samples * sizeof(double));
        std::sort(arr, arr + a_nr_samples);
        double ret = arr[max_sc];
        delete[] arr;
        return (ret);
    }
    else
        return (-1.0);
}

double ft_min_sc(double *a_data, unsigned int a_nr_samples, int perci)
{
    unsigned int min_sc = (a_nr_samples * perci) / 100;
    if (min_sc >= a_nr_samples)
        return (ft_min(a_data, a_nr_samples));
    if (a_nr_samples != 0)
    {
        double *arr = new double[a_nr_samples];
        memcpy(arr, a_data, a_nr_samples * sizeof(double));
        std::sort(arr, arr + a_nr_samples);
        double ret = arr[min_sc];
        delete[] arr;
        return (ret);
    }
    else
        return (-1.0);
}

void ft_hist(double *a_data, unsigned int a_nr_samples, int nbins, double *counter, double *centers)
{
    if (a_nr_samples > 1 && nbins > 1)
    {
        double arr[a_nr_samples];
        memcpy(arr, a_data, a_nr_samples * sizeof(double));
        std::sort(arr, arr + a_nr_samples);
        double w_bin = (arr[a_nr_samples - 1] - arr[0]) / nbins;
        centers[0] = arr[0] + w_bin / 2;
        for (int i = 1; i < nbins; i++)
        {
            centers[i] = centers[i - 1] + w_bin;
        }
        int v = 0;
        counter[v] = 0;
        for (unsigned int j = 0; j < a_nr_samples; j++)
        {
          if (a_data[j] < arr[0] + (v + 1) * w_bin)
            counter[v]++;
          else if (v == nbins - 1 && a_data[j] == arr[0] + (v + 1) * w_bin)
            counter[v]++;
          else
          {
            v++;
            counter[v] = 0;
            j--;
          }
        }
    }
}

int ft_sign(double value)
{
    if (value < 0.0)
        return -1;
    if (value > 0.0)
        return 1;
    return (0);
}

