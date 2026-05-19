#include "SHealth.h"
#include <fstream>
#include <iostream>
#include <sstream>

int SHealth::calculateBmi(const std::string& filename) {
    count = 0;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 0;
    }

    std::string line;
    std::getline(file, line); // 첫번째 줄 읽기 (헤더)
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = split(line, ',');
        if (tokens.empty()) {
            break;
        }
        ages[count] = std::stoi(tokens[1]);
        weights[count] = std::stod(tokens[2]);
        heights[count] = std::stod(tokens[3]);
        count++;
    }
    file.close();

    // 데이터 수집 중 누락된 체중에 나이대의 평균 체중을 적용
    for (int a = 20; a <= 70; a += 10) {
        double sum = 0;
        int ageCount = 0;
        for (int i = 0; i < count; i++) {
            if (ages[i] >= a && ages[i] < a + 10) {
                if (weights[i] == 0.0) {
                    continue;
                }
                sum += weights[i];
                ageCount++;
            }
        }
        for (int i = 0; i < count; i++) {
            if (ages[i] >= a && ages[i] < a + 10) {
                if (weights[i] == 0.0) {
                    weights[i] = sum / ageCount;
                }
            }
        }
    }

    // BMI 계산하기
    for (int i = 0; i < count; i++) {
        bmis[i] = weights[i] / ((heights[i] / 100.0) * (heights[i] / 100.0));
    }

    // 나이대의 BMI기준 저체중, 정상체중, 과체중, 비만 비율 계산
    for (int a = 20; a <= 70; a += 10) {
        int underweight = 0;
        int normalweight = 0;
        int overweight = 0;
        int obesity = 0;
        int sum = 0;
        for (int i = 0; i < count; i++) {
            if (ages[i] >= a && ages[i] < a + 10) {
                sum++;
                if (bmis[i] <= 18.5) {
                    underweight++;
                } else if (bmis[i] > 18.5 && bmis[i] < 23) {
                    normalweight++;
                } else if (bmis[i] >= 23 && bmis[i] < 25) {
                    overweight++;
                } else if (bmis[i] > 25) {
                    obesity++;
                }
            }
        }
        if (a == 20) {
            underweight20 = (double)underweight * 100 / sum;
            normalweight20 = (double)normalweight * 100 / sum;
            overweight20 = (double)overweight * 100 / sum;
            obesity20 = (double)obesity * 100 / sum;
        } else if (a == 30) {
            underweight30 = (double)underweight * 100 / sum;
            normalweight30 = (double)normalweight * 100 / sum;
            overweight30 = (double)overweight * 100 / sum;
            obesity30 = (double)obesity * 100 / sum;
        } else if (a == 40) {
            underweight40 = (double)underweight * 100 / sum;
            normalweight40 = (double)normalweight * 100 / sum;
            overweight40 = (double)overweight * 100 / sum;
            obesity40 = (double)obesity * 100 / sum;
        } else if (a == 50) {
            underweight50 = (double)underweight * 100 / sum;
            normalweight50 = (double)normalweight * 100 / sum;
            overweight50 = (double)overweight * 100 / sum;
            obesity50 = (double)obesity * 100 / sum;
        } else if (a == 60) {
            underweight60 = (double)underweight * 100 / sum;
            normalweight60 = (double)normalweight * 100 / sum;
            overweight60 = (double)overweight * 100 / sum;
            obesity60 = (double)obesity * 100 / sum;
        } else if (a == 70) {
            underweight70 = (double)underweight * 100 / sum;
            normalweight70 = (double)normalweight * 100 / sum;
            overweight70 = (double)overweight * 100 / sum;
            obesity70 = (double)obesity * 100 / sum;
        }
    }
    return count;
}

double SHealth::getBmiRatio(int ageClass, int type) {
    if (ageClass == 20 && type == 100) return underweight20;
    else if (ageClass == 20 && type == 200) return normalweight20;
    else if (ageClass == 20 && type == 300) return overweight20;
    else if (ageClass == 20 && type == 400) return obesity20;
    else if (ageClass == 30 && type == 100) return underweight30;
    else if (ageClass == 30 && type == 200) return normalweight30;
    else if (ageClass == 30 && type == 300) return overweight30;
    else if (ageClass == 30 && type == 400) return obesity30;
    else if (ageClass == 40 && type == 100) return underweight40;
    else if (ageClass == 40 && type == 200) return normalweight40;
    else if (ageClass == 40 && type == 300) return overweight40;
    else if (ageClass == 40 && type == 400) return obesity40;
    else if (ageClass == 50 && type == 100) return underweight50;
    else if (ageClass == 50 && type == 200) return normalweight50;
    else if (ageClass == 50 && type == 300) return overweight50;
    else if (ageClass == 50 && type == 400) return obesity50;
    else if (ageClass == 60 && type == 100) return underweight60;
    else if (ageClass == 60 && type == 200) return normalweight60;
    else if (ageClass == 60 && type == 300) return overweight60;
    else if (ageClass == 60 && type == 400) return obesity60;
    else if (ageClass == 70 && type == 100) return underweight70;
    else if (ageClass == 70 && type == 200) return normalweight70;
    else if (ageClass == 70 && type == 300) return overweight70;
    else if (ageClass == 70 && type == 400) return obesity70;
    return 0.0;
}

std::vector<std::string> SHealth::split(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
