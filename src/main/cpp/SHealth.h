#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

class SHealth {
public:
    int calculateBmi(const std::string& filename);
    double getBmiRatio(int ageClass, int type);

private:
    static constexpr double kBmiUnderweightMax = 18.5;
    static constexpr double kBmiNormalMax = 23.0;
    static constexpr double kBmiOverweightMax = 25.0;
    static constexpr double kCmPerMeter = 100.0;
    static constexpr double kPercentFactor = 100.0;
    static constexpr double kMissingWeight = 0.0;
    static constexpr double kMissingHeight = 0.0;
    static constexpr int kAgeBandWidth = 10;
    static constexpr int kMinAgeBandStart = 20;
    static constexpr int kMaxAgeBandStart = 70;
    static constexpr int kAgeBandCount = 6;
    static constexpr int kBmiCategoryCount = 4;
    static constexpr int kTypeUnderweight = 100;
    static constexpr int kTypeNormal = 200;
    static constexpr int kTypeOverweight = 300;
    static constexpr int kTypeObesity = 400;
    static constexpr std::size_t kMaxRecordCount = 10000;
    static constexpr char kCsvDelimiter = ',';

    enum class BmiCategory { Underweight, Normal, Overweight, Obesity };
    enum class AgeBand {
        Twenties = 20,
        Thirties = 30,
        Forties = 40,
        Fifties = 50,
        Sixties = 60,
        Seventies = 70
    };

    int recordCount_ = 0;
    int ages_[kMaxRecordCount];
    double heights_[kMaxRecordCount];
    double weights_[kMaxRecordCount];
    double bmis_[kMaxRecordCount];
    std::array<std::array<double, kBmiCategoryCount>, kAgeBandCount> bmiRatios_{};

    BmiCategory classifyBmi(double bmi) const;
    static bool isInAgeBand(int age, AgeBand band);
    static int ageBandToIndex(AgeBand band);
    static int ageClassToIndex(int ageClass);
    static int typeCodeToCategoryIndex(int type);
    void forEachAgeBand(const std::function<void(AgeBand)>& fn);
    void imputeMissingWeightsForBand(AgeBand band);
    void imputeMissingHeightsForBand(AgeBand band);
    void aggregateRatiosForBand(AgeBand band);
    double computeBmi(double weightKg, double heightCm) const;

    void loadRecordsFromCsv(const std::string& filename);
    void imputeMissingWeightsByAgeBand();
    void imputeMissingHeightsByAgeBand();
    void computeAllBmis();
    void aggregateRatiosByAgeBand();

    std::vector<std::string> split(const std::string& line, char delimiter);
};
