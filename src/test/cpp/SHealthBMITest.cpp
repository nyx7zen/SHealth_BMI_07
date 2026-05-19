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

TEST_F(SHealthBMITest, CalculateBmi_StandardWeightHeight_ReturnsExpectedBmi) {
    // Given: 70kg, 170cm → BMI = 70 / (1.7²) ≈ 24.2215 (과체중)
    writeCsv("1,25,70,170\n");
    SHealth shealth;
    // When
    const int count = shealth.calculateBmi(tempCsvPath_);
    // Then: 처리 1건, BMI≈24.22 → 20대 과체중(type 300) 100%
    EXPECT_EQ(count, 1);
    EXPECT_NEAR(shealth.getBmiRatio(20, 300), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 400), 0.0, 0.01);
}

TEST_F(SHealthBMITest, CalculateBmi_HeightInMeters_ConvertsFromCm) {
    // Given: 80kg, 180cm → BMI = 80 / (1.8²) ≈ 24.69 (cm→m 변환 필수)
    writeCsv("1,25,80,180\n");
    SHealth shealth;
    // When
    const int count = shealth.calculateBmi(tempCsvPath_);
    // Then: cm를 m로 변환하지 않으면 BMI≈0 → 저체중; 정상 변환 시 과체중
    EXPECT_EQ(count, 1);
    EXPECT_NEAR(shealth.getBmiRatio(20, 300), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 100), 0.0, 0.01);
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

TEST_F(SHealthBMITest, ClassifyBmi_JustAbove18_5_IsNormal) {
    // Given: BMI = 18.51 (> 18.5, 저체중 경계 직후) — 53.51kg / 170cm
    writeCsv("1,25,53.51,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    // Then: 정상(type 200) 100%, 저체중(100) 0%
    EXPECT_NEAR(shealth.getBmiRatio(20, 200), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 100), 0.0, 0.01);
}

TEST_F(SHealthBMITest, ClassifyBmi_Between18_5And23_IsNormal) {
    // Given: BMI = 21.0 (18.5 < BMI < 23 정상 구간) — 60.69kg / 170cm
    writeCsv("1,25,60.69,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    EXPECT_NEAR(shealth.getBmiRatio(20, 200), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 300), 0.0, 0.01);
}

TEST_F(SHealthBMITest, ClassifyBmi_Age30_In30Band) {
    // Given: 30세 BMI=25(비만) — 30대 [30,40) 집계
    writeCsv("1,30,72.25,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    // Then: 30대 비만(400) 100%, 20대는 0%
    EXPECT_NEAR(shealth.getBmiRatio(30, 400), 100.0, 0.01);
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

TEST_F(SHealthBMITest, ImputeWeight_ZeroOutsideBand_Unchanged) {
    // Given: 20대 60kg 1명, 30대 0kg 1명(동 연령대 유효 체중 없음) → 30대는 20대 평균 미적용
    writeCsv("1,25,60,170\n2,35,0,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    // Then: 20대는 60kg 기준 정상(200) 100%, 30대는 보정 없이 BMI=0 → 저체중 100%
    EXPECT_NEAR(shealth.getBmiRatio(20, 200), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(30, 100), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 100), 0.0, 0.01);
}

TEST_F(SHealthBMITest, ImputeWeight_TwoBands_IndependentAverages) {
    // Given: 20대 50kg+0kg(→50kg), 30대 80kg+0kg(→80kg) — 연령대별 독립 평균 보정
    writeCsv("1,25,50,170\n2,27,0,170\n3,35,80,170\n4,37,0,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    // Then: 50kg→저체중(100), 80kg→비만(400), 각 연령대 100%
    EXPECT_NEAR(shealth.getBmiRatio(20, 100), 100.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(30, 400), 100.0, 0.01);
}

TEST_F(SHealthBMITest, ImputeWeight_NoZero_UnchangedWeights) {
    // Given: 20대 모두 양수 체중 — 보정 없이 원값 유지
    writeCsv("1,25,50,170\n2,26,70,170\n");
    SHealth shealth;
    shealth.calculateBmi(tempCsvPath_);
    // Then: 저체중·과체중 각 50%
    EXPECT_NEAR(shealth.getBmiRatio(20, 100), 50.0, 0.01);
    EXPECT_NEAR(shealth.getBmiRatio(20, 300), 50.0, 0.01);
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
