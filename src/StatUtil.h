#ifndef _STAT_UTIL_H_
#define _STAT_UTIL_H_

template <typename T>
T StatSum(vector<T>& values)
{
    T sum = 0;
    for (unsigned int i=0; i<values.size(); i++)
        sum += values[i];
    return sum;
}

template <typename T>
double StatAvg(vector<T>& values)
{
    return StatSum(values) / ((double) values.size());
}

template <typename T>
T StatMax(vector<T>& values)
{
    if (values.size() == 0)
        return 0;

    T max = values[0];
    for (unsigned int i=1; i<values.size(); i++) {
        if (max < values[i])
            max = values[i];
    }

    return max;
}

template <typename T>
T StatMin(vector<T>& values)
{
    if (values.size() == 0)
        return 0;

    T min = values[0];
    for (unsigned int i=1; i<values.size(); i++) {
        if (min > values[i])
            min = values[i];
    }

    return min;
}

template <typename T>
double StatStddev(vector<T>& values)
{
    if (values.size() <= 1)
        return 0;

    double avg = StatAvg(values);
    double sum_sqr = 0;
    for (unsigned int i=0; i<values.size(); i++)
        sum_sqr += (avg - values[i]) * (avg - values[i]);
    double variance = sum_sqr/((double) (values.size()-1));
    double stddev = sqrt(variance);

    return stddev;
}

// R. Jain, D.M. Chiu and W. Hawe 
// Quantitative Measure of Fairness and Discrimination for Resource Allocation in Shared Systems 1994
// fairness = (Sigma x)^2 / (n * Sigma (x^2))
// The result ranges from 1/n (worst case) to 1 (best case).
template <typename T>
double StatFairness(vector<T> & values)
{
    T sum = StatSum(values);
    T sum_sqr = 0;

    for (unsigned int i=0; i<values.size(); i++)
        sum_sqr += values[i] * values[i];

    double fairness = (sum * sum) / (((double) values.size()) * sum_sqr);

    return fairness;
}

#endif // #ifndef _STAT_UTIL_H_
