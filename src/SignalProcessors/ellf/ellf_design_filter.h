
int ellf_design_filter(int filterkind, int filtershape, int filterorder, double filterripple, double samplingrate, double edge1, double edge2, double ellipstopbandedge, double* filterdenominator, double* filternumerator);

//    Filter kind: 1 Butterworth
//                 2 Chebyshev
//                 3 Elliptic
//    Filter shape: 1 low pass
//                  2 band pass
//                  3 high pass
//                  4 band stop
//    Ripple: for Chebyshev and Elliptic filters
//    Ellipstopbandedge: for Elliptic filters

