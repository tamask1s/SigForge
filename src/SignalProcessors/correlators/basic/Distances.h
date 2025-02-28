#ifndef __DISTANCES_H_
#define __DISTANCES_H_

/**
    Functions of this processor class returns either some kind of difference, or the correlation between two vectors.
    In case of PCC returns the Pearson correlation, ranging from -1 to 1.
    In other cases a bigger value means a bigger distance between the 2 vectors.
    Vectors are not normalized during normal distance calculations (EUCL, SOSD, TXCB).

    Example which prints a correlation of '0.98198050606196563':
    double vec1[] = {1, 2, 3}, vec2[] = {1, 5, 7};
    cout.precision(17);
    cout << PCorrelator::Distance(vec1, vec2, 3, PCorrelator::PCC) << endl;

    ref: https://stackoverflow.com/questions/3949226/calculating-pearson-correlation-and-significance-in-python
**/

enum DistanceMetric
{
    EUCL = 0, /// Euclidian distance
    SOSD,     /// Sum of squared distances
    TXCB,     /// Manhattan / Taxicab / Cityblock
    ANGL,     /// Angle between vectors
    PCC,      /// Pearson correlation
    CONV      /// Convolution
};

struct PCorrelator
{
    /** Cross-correlation of vec and vec2 */
    static double Distance(const double* vec, const double* vec2, unsigned int aW, DistanceMetric distance_metric = PCC);
    /** Cross-correlation of vec and vec2, vec starting from coordinates aX */
    static double DistanceC(const double* vec, unsigned int aX, const double* vec2, unsigned int aW, DistanceMetric distance_metric = PCC);
    /** Masked correlation */
    static double DistanceCM(const double* vec, unsigned int aX, const double* vec2, unsigned int aW, const unsigned char* aMask, DistanceMetric distance_metric = PCC);
    /** CorrelationMap */
    static void DistanceMap(double* corr, const double* small, int smallw,  const double* big, int bigw, DistanceMetric distance_metric = PCC, int* maxindx = 0, int* minindx = 0, double* maxval = 0, double* minval = 0);
    /** Masked CorrelationMap */
    static void DistanceMapM(double* corr, const double* small, int smallw, const double* big, int bigw, const unsigned char* aMask, DistanceMetric distance_metric = PCC, int* maxindx = 0, int* minindx = 0, double* maxval = 0, double* minval = 0);
};

/** ref: https://www.dsprelated.com/freebooks/mdft/Signal_Metrics.html */
struct PSignalMetrics
{
    static double mean(const double* a_vec, unsigned int a_len);
    static double energy(const double* a_vec, unsigned int a_len, bool a_normalized = true);
    /** accepts EUCL(L2_norm) or TXCB(L1_norm). The normalized EUCL L2_norm results in RMS (Root mean square) */
    static double lp_norm(const double* a_vec, unsigned int a_len, bool a_normalized = true, DistanceMetric distance_metric = EUCL);
};

struct PKernels
{
    static double gaussian_pdf(double x, double m, double s);
    static void gauss_1d_pos(double* g, unsigned int gw, double sigma, double mu, double sr);
};

#endif // __DISTANCES_H_
