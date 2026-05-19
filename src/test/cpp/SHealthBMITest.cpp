#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include "SHealth.h"

class SHealthBMITest : public ::testing::Test {
protected:
    std::string tempCsvPath_;

    void writeCsv(const std::string& body) {
        tempCsvPath_ = "test_shealth_temp.csv";
        std::ofstream out(tempCsvPath_);
        out << "id,age,weight,height\n";
        out << body;
    }

    void TearDown() override {
        if (!tempCsvPath_.empty()) {
            std::remove(tempCsvPath_.c_str());
        }
    }
};

TEST_F(SHealthBMITest, CalculateBmi_FileNotFound_ReturnsZero) {
    // Given: 존재하지 않는 파일
    SHealth shealth;
    // When
    const int count = shealth.calculateBmi("nonexistent_file_12345.csv");
    // Then
    EXPECT_EQ(count, 0);
}

TEST_F(SHealthBMITest, CalculateBmi_MultipleRows_ReturnsCorrectCount) {
    // Given: 데이터 3행
    writeCsv("1,25,70,170\n2,35,80,180\n3,45,90,175\n");
    SHealth shealth;
    // When
    const int count = shealth.calculateBmi(tempCsvPath_);
    // Then
    EXPECT_EQ(count, 3);
}

TEST_F(SHealthBMITest, ClassifyBmi_At25_IsObesity) {
    // Given: BMI = 72.25 / (1.7^2) = 25.0, 20대
    writeCsv("1,25,72.25,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    // When / Then: 비만(type 400) 100%
    EXPECT_NEAR(shealth.getBmiRatio(20, 400), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 100), 0.0, 0.01);
}

TEST_F(SHealthBMITest, ClassifyBmi_At18_5_IsUnderweight) {
    // Given: BMI < 18.5 (저체중 구간)
    writeCsv("1,25,50,170\n");  // 50 / 2.89 ≈ 17.3
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    EXPECT_NEAR(shealth.getBmiRatio(20, 100), 100.0, 0.01);
}

TEST_F(SHealthBMITest, ClassifyBmi_At23_IsOverweight) {
    // Given: BMI = 23.0
    writeCsv("1,25,66.47,170\n");  // 66.47 / 2.89 ≈ 23.0
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    EXPECT_NEAR(shealth.getBmiRatio(20, 300), 100.0, 0.01);
}

TEST_F(SHealthBMITest, ClassifyBmi_JustBelow25_IsOverweight) {
    // Given: BMI ≈ 24.99
    writeCsv("1,25,72.22,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    EXPECT_NEAR(shealth.getBmiRatio(20, 300), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 400), 0.0, 0.01);
}

TEST_F(SHealthBMITest, ImputeWeight_OneZeroInAgeBand_UsesPeerAverage) {
    // Given: 20대 60kg 1명, 0kg 1명 → 평균 60kg 보정
    writeCsv("1,25,60,170\n2,27,0,165\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    // Then: 두 명 모두 유효 BMI → 20대 합 100%
    const double sum = shealth.getBmiRatio(20, 100) + shealth.getBmiRatio(20, 200) +
                       shealth.getBmiRatio(20, 300) + shealth.getBmiRatio(20, 400);
    EXPECT_NEAR(sum, 100.0, 0.01);
}

TEST_F(SHealthBMITest, ImputeWeight_AllZeroInBand_NoDivisionByZero) {
    // Given: 20대 전원 weight 0 → 보정 스킵, BMI=0은 저체중으로 집계
    writeCsv("1,25,0,170\n2,27,0,165\n");
    SHealth shealth;
    // When / Then: 크래시 없이 완료, 비율 합은 정의됨
    EXPECT_EQ(shealth.calculateBmi(tempCsvPath_), 2);
    const double sum = shealth.getBmiRatio(20, 100) + shealth.getBmiRatio(20, 200) +
                       shealth.getBmiRatio(20, 300) + shealth.getBmiRatio(20, 400);
    EXPECT_NEAR(sum, 100.0, 0.01);
}

TEST_F(SHealthBMITest, GetBmiRatio_FourTypes_SumNear100) {
    // Given: 20대 4분류 각 1명
    writeCsv("1,25,50,170\n2,26,60,170\n3,27,70,170\n4,28,90,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    const double sum = shealth.getBmiRatio(20, 100) + shealth.getBmiRatio(20, 200) +
                       shealth.getBmiRatio(20, 300) + shealth.getBmiRatio(20, 400);
    EXPECT_NEAR(sum, 100.0, 0.01);
}

TEST_F(SHealthBMITest, GetBmiRatio_InvalidType_ReturnsZero) {
    writeCsv("1,25,70,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    EXPECT_DOUBLE_EQ(shealth.getBmiRatio(20, 999), 0.0);
}

TEST_F(SHealthBMITest, GetBmiRatio_InvalidAgeClass_ReturnsZero) {
    writeCsv("1,25,70,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    EXPECT_DOUBLE_EQ(shealth.getBmiRatio(15, 100), 0.0);
}

TEST_F(SHealthBMITest, ClassifyBmi_Age19_ExcludedFrom20Band) {
    // Given: 19세는 20대 집계 제외
    writeCsv("1,19,72.25,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    EXPECT_NEAR(shealth.getBmiRatio(20, 400), 0.0, 0.01);
}

TEST_F(SHealthBMITest, CalculateBmi_HeaderOnly_ReturnsZero) {
    writeCsv("");
    SHealth shealth;
    EXPECT_EQ(shealth.calculateBmi(tempCsvPath_), 0);
}
