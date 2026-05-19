#include "SHealth.h"
#include <cstdio>

int main() {
    SHealth shealth;
    shealth.calculateBmi("shealth.dat");

    printf("20 - underweight = %f, normal = %f, overweight = %f, obesity = %f\n",
           shealth.getBmiRatio(20, 100), shealth.getBmiRatio(20, 200),
           shealth.getBmiRatio(20, 300), shealth.getBmiRatio(20, 400));
    printf("30 - underweight = %f, normal = %f, overweight = %f, obesity = %f\n",
           shealth.getBmiRatio(30, 100), shealth.getBmiRatio(30, 200),
           shealth.getBmiRatio(30, 300), shealth.getBmiRatio(30, 400));
    printf("40 - underweight = %f, normal = %f, overweight = %f, obesity = %f\n",
           shealth.getBmiRatio(40, 100), shealth.getBmiRatio(40, 200),
           shealth.getBmiRatio(40, 300), shealth.getBmiRatio(40, 400));
    printf("50 - underweight = %f, normal = %f, overweight = %f, obesity = %f\n",
           shealth.getBmiRatio(50, 100), shealth.getBmiRatio(50, 200),
           shealth.getBmiRatio(50, 300), shealth.getBmiRatio(50, 400));
    printf("60 - underweight = %f, normal = %f, overweight = %f, obesity = %f\n",
           shealth.getBmiRatio(60, 100), shealth.getBmiRatio(60, 200),
           shealth.getBmiRatio(60, 300), shealth.getBmiRatio(60, 400));
    printf("70 - underweight = %f, normal = %f, overweight = %f, obesity = %f\n",
           shealth.getBmiRatio(70, 100), shealth.getBmiRatio(70, 200),
           shealth.getBmiRatio(70, 300), shealth.getBmiRatio(70, 400));

    return 0;
}
