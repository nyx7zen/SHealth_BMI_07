#pragma once

#include <string>
#include <vector>

class SHealth {
public:
    int calculateBmi(const std::string& filename);
    double getBmiRatio(int ageClass, int type);

private:
    int count = 0;
    int ages[10000];
    double heights[10000];
    double weights[10000];
    double bmis[10000];

    double underweight20 = 0, underweight30 = 0, underweight40 = 0;
    double underweight50 = 0, underweight60 = 0, underweight70 = 0;
    double normalweight20 = 0, normalweight30 = 0, normalweight40 = 0;
    double normalweight50 = 0, normalweight60 = 0, normalweight70 = 0;
    double overweight20 = 0, overweight30 = 0, overweight40 = 0;
    double overweight50 = 0, overweight60 = 0, overweight70 = 0;
    double obesity20 = 0, obesity30 = 0, obesity40 = 0;
    double obesity50 = 0, obesity60 = 0, obesity70 = 0;

    std::vector<std::string> split(const std::string& line, char delimiter);
};
