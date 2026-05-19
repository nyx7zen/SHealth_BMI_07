#include "SHealth.h"
#include <fstream>
#include <iostream>
#include <sstream>

SHealth::BmiCategory SHealth::classifyBmi(double bmi) const {
    if (bmi <= kBmiUnderweightMax) {
        return BmiCategory::Underweight;
    }
    if (bmi < kBmiNormalMax) {
        return BmiCategory::Normal;
    }
    if (bmi < kBmiOverweightMax) {
        return BmiCategory::Overweight;
    }
    return BmiCategory::Obesity;
}

bool SHealth::isInAgeBand(int age, AgeBand band) {
    const int bandStart = static_cast<int>(band);
    return age >= bandStart && age < bandStart + kAgeBandWidth;
}

int SHealth::ageBandToIndex(AgeBand band) {
    return (static_cast<int>(band) - kMinAgeBandStart) / kAgeBandWidth;
}

int SHealth::ageClassToIndex(int ageClass) {
    if (ageClass < kMinAgeBandStart || ageClass > kMaxAgeBandStart) {
        return -1;
    }
    if ((ageClass - kMinAgeBandStart) % kAgeBandWidth != 0) {
        return -1;
    }
    return ageBandToIndex(static_cast<AgeBand>(ageClass));
}

int SHealth::typeCodeToCategoryIndex(int type) {
    switch (type) {
        case kTypeUnderweight:
            return 0;
        case kTypeNormal:
            return 1;
        case kTypeOverweight:
            return 2;
        case kTypeObesity:
            return 3;
        default:
            return -1;
    }
}

void SHealth::forEachAgeBand(const std::function<void(AgeBand)>& fn) {
    for (int start = kMinAgeBandStart; start <= kMaxAgeBandStart; start += kAgeBandWidth) {
        fn(static_cast<AgeBand>(start));
    }
}

double SHealth::computeBmi(double weightKg, double heightCm) const {
    const double heightM = heightCm / kCmPerMeter;
    return weightKg / (heightM * heightM);
}

void SHealth::loadRecordsFromCsv(const std::string& filename) {
    recordCount_ = 0;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        const std::vector<std::string> tokens = split(line, kCsvDelimiter);
        if (tokens.empty()) {
            break;
        }
        if (recordCount_ >= static_cast<int>(kMaxRecordCount)) {
            break;
        }
        ages_[recordCount_] = std::stoi(tokens[1]);
        weights_[recordCount_] = std::stod(tokens[2]);
        heights_[recordCount_] = std::stod(tokens[3]);
        recordCount_++;
    }
}

void SHealth::imputeMissingWeightsForBand(AgeBand band) {
    double weightSum = 0.0;
    int validWeightCount = 0;
    for (int i = 0; i < recordCount_; i++) {
        if (!isInAgeBand(ages_[i], band)) {
            continue;
        }
        if (weights_[i] == kMissingWeight) {
            continue;
        }
        weightSum += weights_[i];
        validWeightCount++;
    }
    if (validWeightCount == 0) {
        return;
    }
    const double averageWeight = weightSum / validWeightCount;
    for (int i = 0; i < recordCount_; i++) {
        if (isInAgeBand(ages_[i], band) && weights_[i] == kMissingWeight) {
            weights_[i] = averageWeight;
        }
    }
}

void SHealth::imputeMissingWeightsByAgeBand() {
    forEachAgeBand([this](AgeBand band) { imputeMissingWeightsForBand(band); });
}

void SHealth::computeAllBmis() {
    for (int i = 0; i < recordCount_; i++) {
        bmis_[i] = computeBmi(weights_[i], heights_[i]);
    }
}

void SHealth::aggregateRatiosForBand(AgeBand band) {
    std::array<int, kBmiCategoryCount> categoryCounts{};
    int memberCount = 0;

    for (int i = 0; i < recordCount_; i++) {
        if (!isInAgeBand(ages_[i], band)) {
            continue;
        }
        memberCount++;
        const int categoryIndex = static_cast<int>(classifyBmi(bmis_[i]));
        categoryCounts[categoryIndex]++;
    }

    const int bandIndex = ageBandToIndex(band);
    if (memberCount == 0) {
        bmiRatios_[bandIndex] = {};
        return;
    }

    for (int categoryIndex = 0; categoryIndex < kBmiCategoryCount; categoryIndex++) {
        bmiRatios_[bandIndex][categoryIndex] =
            static_cast<double>(categoryCounts[categoryIndex]) * kPercentFactor / memberCount;
    }
}

void SHealth::aggregateRatiosByAgeBand() {
    forEachAgeBand([this](AgeBand band) { aggregateRatiosForBand(band); });
}

int SHealth::calculateBmi(const std::string& filename) {
    bmiRatios_ = {};
    loadRecordsFromCsv(filename);
    imputeMissingWeightsByAgeBand();
    computeAllBmis();
    aggregateRatiosByAgeBand();
    return recordCount_;
}

double SHealth::getBmiRatio(int ageClass, int type) {
    const int bandIndex = ageClassToIndex(ageClass);
    const int categoryIndex = typeCodeToCategoryIndex(type);
    if (bandIndex < 0 || categoryIndex < 0) {
        return 0.0;
    }
    return bmiRatios_[bandIndex][categoryIndex];
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
