# SHealth BMI — UnitTest TC 종합 결과 보고서

| 항목 | 내용 |
|------|------|
| 프로젝트 | SHealth_BMI_07 |
| 작성일 | 2026-05-20 |
| 브랜치 | `tc` |
| 대상 | README Activity 3 — UnitTest 작성 (4항목) |
| 테스트 파일 | `src/test/cpp/SHealthBMITest.cpp` |
| 참조 보고서 | `Report/4`~`7` (BMI 계산·Age 보정·4분류·예외) |

---

## 1. 종합 결과

| README Activity 3 항목 | TC 수 | 신규 | 최종 검증 시점 | 결과 |
|------------------------|-------|------|----------------|------|
| BMI 계산 로직 TC | 3 | +2 | 15/15 Passed | ✅ |
| Age 평균치 보정 로직 TC | 5 | +3 | 18/18 Passed | ✅ |
| 정상/저체중/과체중/비만 분류 TC | 9 | +3 | 21/21 Passed | ✅ |
| 예외상황 TC | 6* | +1 | 22/22 Passed | ✅ |

\* `ImputeWeight_AllZeroInBand_NoDivisionByZero`는 Age 보정·예외(0 나눗셈) 양쪽에 해당.

**최종: 22개 `TEST_F` — 100% Passed (0 failed)**

```text
100% tests passed, 0 tests failed out of 22
```

### 검증 명령

```powershell
cd build
cmake --build .
ctest --output-on-failure
```

---

## 2. README Activity 3 완료 현황

```markdown
3. UnitTest 작성 (1시간)
- [x] BMI 계산 로직 TC
- [x] Age 평균치 보정 로직 TC
- [x] 정상/저체중/과체중/비만 분류 TC
- [x] 예외상황 TC
```

---

## 3. 카테고리별 TC 상세

### 3.1 BMI 계산 로직 TC

> 출처: `Report/4.SHealth_BMI_단위테스트.md`

| TEST_F | Given | Then | 상태 |
|--------|-------|------|------|
| `CalculateBmi_MultipleRows_ReturnsCorrectCount` | CSV 3행 | `calculateBmi` → 3 | ✅ |
| `CalculateBmi_StandardWeightHeight_ReturnsExpectedBmi` | 70kg, 170cm | BMI≈24.22 → 20대 과체중(300) 100% | ✅ |
| `CalculateBmi_HeightInMeters_ConvertsFromCm` | 80kg, 180cm | cm→m 변환, 과체중(300) 100% | ✅ |

**검증 공식**

```
BMI = weight(kg) / (height(m))²
height(m) = height(cm) / 100.0
```

| 케이스 | 계산 | 기대 분류 |
|--------|------|-----------|
| 70kg / 170cm | 70 / 1.7² ≈ 24.22 | 과체중 (300) |
| 80kg / 180cm | 80 / 1.8² ≈ 24.69 | 과체중 (300) |

**설계:** `computeBmi`는 private → `calculateBmi` 건수 + `getBmiRatio`로 간접 검증.

---

### 3.2 Age 평균치 보정 로직 TC

> 출처: `Report/5.SHealth_BMI_Age평균치보정.md`

| TEST_F | Given | Then | 상태 |
|--------|-------|------|------|
| `ImputeWeight_OneZeroInAgeBand_UsesPeerAverage` | 20대 60kg + 0kg | 동 연령대 평균 보정, 20대 합 100% | ✅ |
| `ImputeWeight_AllZeroInBand_NoDivisionByZero` | 20대 전원 0kg | 0 나눗셈·크래시 없음 | ✅ |
| `ImputeWeight_ZeroOutsideBand_Unchanged` | 20대 60kg, 30대 0kg | 20대 정상 100%, 30대 저체중 100% | ✅ |
| `ImputeWeight_TwoBands_IndependentAverages` | 20·30대 각 0kg 1건 | 연령대별 독립 보정 | ✅ |
| `ImputeWeight_NoZero_UnchangedWeights` | 20대 모두 양수 체중 | 저체중·과체중 각 50% | ✅ |

**도메인 규칙**

- `weight == 0` → 같은 연령대 `[a, a+10)` 유효 체중 평균으로 보정
- 동 연령대 유효 체중 없음 → 보정 스킵
- 타 연령대는 보정에 영향 없음

---

### 3.3 정상/저체중/과체중/비만 분류 TC

> 출처: `Report/6.SHealth_BMI_4분류.md`

| BMI 구간 | 분류 | type |
|----------|------|------|
| ≤ 18.5 | 저체중 | 100 |
| 18.5 < BMI < 23 | 정상 | 200 |
| 23 ≤ BMI < 25 | 과체중 | 300 |
| ≥ 25 | 비만 | 400 |

| TEST_F | Given | Then | 상태 |
|--------|-------|------|------|
| `ClassifyBmi_At18_5_IsUnderweight` | BMI ≤ 18.5 | 저체중(100) 100% | ✅ |
| `ClassifyBmi_JustAbove18_5_IsNormal` | BMI 18.51 | 정상(200) 100% | ✅ |
| `ClassifyBmi_Between18_5And23_IsNormal` | BMI 21.0 | 정상(200) 100% | ✅ |
| `ClassifyBmi_At23_IsOverweight` | BMI = 23.0 | 과체중(300) 100% | ✅ |
| `ClassifyBmi_JustBelow25_IsOverweight` | BMI ≈ 24.99 | 과체중(300) 100% | ✅ |
| `ClassifyBmi_At25_IsObesity` | BMI = 25.0 | 비만(400) 100% | ✅ |
| `GetBmiRatio_FourTypes_SumNear100` | 20대 4분류 각 1명 | type 합 ≈ 100% | ✅ |
| `ClassifyBmi_Age19_ExcludedFrom20Band` | 19세 | 20대 집계 제외 | ✅ |
| `ClassifyBmi_Age30_In30Band` | 30세 BMI=25 | 30대 비만 100% | ✅ |

**경계값 검증**

| 경계 | 기대 분류 | 테스트 |
|------|-----------|--------|
| 18.5 | 저체중 | `ClassifyBmi_At18_5_IsUnderweight` |
| 18.51 | 정상 | `ClassifyBmi_JustAbove18_5_IsNormal` |
| 21 | 정상 | `ClassifyBmi_Between18_5And23_IsNormal` |
| 23 | 과체중 | `ClassifyBmi_At23_IsOverweight` |
| 24.99 | 과체중 | `ClassifyBmi_JustBelow25_IsOverweight` |
| 25 | 비만 | `ClassifyBmi_At25_IsObesity` |

---

### 3.4 예외상황 TC

> 출처: `Report/7.SHealth_BMI_예외상황.md`

| TEST_F | Given | Then | 상태 |
|--------|-------|------|------|
| `CalculateBmi_FileNotFound_ReturnsZero` | 존재하지 않는 파일 | count=0 | ✅ |
| `CalculateBmi_HeaderOnly_ReturnsZero` | 헤더만 CSV | count=0 | ✅ |
| `GetBmiRatio_InvalidType_ReturnsZero` | type=999 | 0.0 | ✅ |
| `GetBmiRatio_InvalidAgeClass_ReturnsZero` | ageClass=15 | 0.0 | ✅ |
| `ImputeWeight_AllZeroInBand_NoDivisionByZero` | 동 대 전원 weight 0 | 크래시 없음 | ✅ |
| `CalculateBmi_EmptyLine_StopsOrSkips` | 유효행→빈줄→유효행 | 빈 줄에서 중단, count=1 | ✅ |

**빈 줄 정책:** `tokens.empty()` 시 `loadRecordsFromCsv`에서 즉시 `break`.

---

## 4. 전체 TEST_F 인벤토리 (22건)

| # | TEST_F | README 분류 |
|---|--------|---------------|
| 1 | `CalculateBmi_FileNotFound_ReturnsZero` | 예외 |
| 2 | `CalculateBmi_MultipleRows_ReturnsCorrectCount` | BMI 계산 |
| 3 | `CalculateBmi_StandardWeightHeight_ReturnsExpectedBmi` | BMI 계산 |
| 4 | `CalculateBmi_HeightInMeters_ConvertsFromCm` | BMI 계산 |
| 5 | `ClassifyBmi_At25_IsObesity` | 분류 |
| 6 | `ClassifyBmi_At18_5_IsUnderweight` | 분류 |
| 7 | `ClassifyBmi_At23_IsOverweight` | 분류 |
| 8 | `ClassifyBmi_JustBelow25_IsOverweight` | 분류 |
| 9 | `ClassifyBmi_JustAbove18_5_IsNormal` | 분류 |
| 10 | `ClassifyBmi_Between18_5And23_IsNormal` | 분류 |
| 11 | `ClassifyBmi_Age30_In30Band` | 분류 |
| 12 | `ImputeWeight_OneZeroInAgeBand_UsesPeerAverage` | Age 보정 |
| 13 | `ImputeWeight_AllZeroInBand_NoDivisionByZero` | Age 보정 / 예외 |
| 14 | `ImputeWeight_ZeroOutsideBand_Unchanged` | Age 보정 |
| 15 | `ImputeWeight_TwoBands_IndependentAverages` | Age 보정 |
| 16 | `ImputeWeight_NoZero_UnchangedWeights` | Age 보정 |
| 17 | `GetBmiRatio_FourTypes_SumNear100` | 분류 |
| 18 | `GetBmiRatio_InvalidType_ReturnsZero` | 예외 |
| 19 | `GetBmiRatio_InvalidAgeClass_ReturnsZero` | 예외 |
| 20 | `ClassifyBmi_Age19_ExcludedFrom20Band` | 분류 |
| 21 | `CalculateBmi_HeaderOnly_ReturnsZero` | 예외 |
| 22 | `CalculateBmi_EmptyLine_StopsOrSkips` | 예외 |

---

## 5. 공통 제약·설계

| 항목 | 내용 |
|------|------|
| 공개 API | `calculateBmi`, `getBmiRatio` 시그니처 유지 |
| Fixture | `writeCsv` + 임시 CSV, `TearDown`에서 삭제 |
| 데이터 | `shealth.dat` 전체 의존 금지 |
| 허용 오차 | `EXPECT_NEAR(..., 0.01)` |
| 프레임워크 | Google Test (`ctest`) |

---

## 6. Git 커밋 이력 (TC 단위)

| 커밋 메시지 | 내용 |
|-------------|------|
| `test: BMI 계산 로직 TC 추가` | BMI 계산 TC 2건 |
| `test: Age 평균치 보정 로직 TC 추가` | Age 보정 TC 3건 |
| `test: 정상/저체중/과체중/비만 분류 TC` | 분류 TC 3건 |
| `test: 예외상황 TC` | 예외 TC 1건 |

---

## 7. 산출물 참조

| 구분 | 경로 |
|------|------|
| 종합 보고서 (본 문서) | `docs/test_report.md` |
| BMI 계산 | `Report/4.SHealth_BMI_단위테스트.md` |
| Age 보정 | `Report/5.SHealth_BMI_Age평균치보정.md` |
| 4분류 | `Report/6.SHealth_BMI_4분류.md` |
| 예외 | `Report/7.SHealth_BMI_예외상황.md` |
| 테스트 소스 | `src/test/cpp/SHealthBMITest.cpp` |

---

## 8. 결론

README Activity 3의 **4가지 UnitTest 항목**을 모두 구현·검증했으며, 최종 **22개 테스트가 Green** 상태이다. BMI 계산·연령대 보정·4분류 경계·예외 입력을 공개 API 기준으로 회귀 가능하게 고정했다.

---

*작성: Report 4~7 종합 · 검증일 2026-05-20*
