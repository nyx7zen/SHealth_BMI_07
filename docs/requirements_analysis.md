# SHealth BMI — 요구사항 분석 (C++ 구현·테스트 관점)

| 항목 | 내용 |
|------|------|
| 프로젝트 | SHealth_BMI_07 |
| 작성일 | 2026-05-19 |
| 기준 | README.md, tasks/0.setting.md, 레거시 SHealth.cpp |
| 목적 | 도메인 규칙·경계값·Google Test 시나리오를 C++ QA 관점에서 고정 |

---

## 1. 처리 단계·분류별 비즈니스 규칙

### 1.1 파이프라인 단계

| 구분 | 스펙(README) | 레거시 구현 | C++ 구현 시 유의 |
|------|--------------|-------------|------------------|
| CSV 파싱 | `id,age,weight,height` 헤더 1행 스킵 후 데이터 로드 | `SHealth.cpp:15-25` 헤더 스킵, `tokens[1..3]`만 사용 | `tokens.size() < 4` 미검증; `stoi`/`stod` 예외 미처리 |
| 연령대 `[a,a+10)` | 20·30·40·50·60·70대, `a ≤ age < a+10` | `ages[i] >= a && ages[i] < a+10` (29-38행, 62-63행) | 19세·71세 이상은 **집계 제외** |
| weight=0 보정 | 동일 연령대 유효 체중(weight≠0) 평균 대입 | 2-pass: 합산 후 `weights[i]=sum/ageCount` (28-47행) | `ageCount==0` 시 `sum/ageCount` **UB** |
| BMI 계산 | `BMI = kg / (m)²`, `m = cm/100` | `weights[i]/((heights[i]/100)²)` (51-52행) | `height==0` 시 0 나눗셈 |
| 저체중 | BMI ≤ 18.5, type **100** | `bmis[i] <= 18.5` (65행) | 경계 18.5 포함 ✓ |
| 정상체중 | 18.5 < BMI < 23, type **200** | `> 18.5 && < 23` (67행) | 23.0은 정상 아님 ✓ |
| 과체중 | 23 ≤ BMI < 25, type **300** | `>= 23 && < 25` (69행) | 23.0 과체중 ✓ |
| 비만 | BMI ≥ 25, type **400** | `bmis[i] > 25` (71행) | **BMI=25 미집계** (스펙 불일치) |
| 비율 산출 | 연령대별 4분류 인원 % , 합 ≈ 100% | `count*100/sum` (76-105행) | `sum==0` 시 0 나눗셈 |
| API 조회 | `getBmiRatio(ageClass, type)` | 24분기 if-else (111-136행) | 잘못된 ageClass/type → `0.0` |

### 1.2 BMI 4분류 요약 (테스트 type 코드)

| 분류 | README 조건 | type | 레거시와 차이 |
|------|-------------|------|---------------|
| 저체중 | BMI ≤ 18.5 | 100 | 일치 |
| 정상체중 | 18.5 < BMI < 23 | 200 | 일치 |
| 과체중 | 23 ≤ BMI < 25 | 300 | 일치 |
| 비만 | BMI ≥ 25 | 400 | **`> 25`만 비만** → 25.0 누락 |

### 1.3 공개 API (변경 시 테스트·main 동기화)

```cpp
int calculateBmi(const std::string& filename);  // 처리 건수 반환
double getBmiRatio(int ageClass, int type);     // ageClass: 20|30|40|50|60|70
```

---

## 2. 수치·문자열 처리 시 주의점

1. **`split(line, ',')`** (`SHealth.cpp:139-147`) — 빈 줄이면 `tokens`가 비어 있지 않을 수 있음(빈 문자열 1개). 레거시는 `tokens.empty()` 시 루프 `break` (18-19행). 컬럼 4개 미만이면 `stoi`/`stod` 범위 밖 접근 위험.

2. **`std::stoi` / `std::stod`** (21-23행) — 잘못된 CSV는 `std::invalid_argument`, `std::out_of_range` 발생. 테스트 fixture는 **유효 숫자만** 쓰거나 try-catch/검증 레이어 추가.

3. **부동소수점 동등 비교** — `weight == 0.0` (34, 43행). BMI 경계(18.5, 23, 25)는 `EXPECT_NEAR(bmi, 25.0, 1e-4)` 등 ε 비교 권장.

4. **연령 구간** — 반개구간 `[a, a+10)`: 20대 = 20≤age<30. age=19, 71은 어느 연령대 통계에도 포함되지 않음(의도 확인용 TC).

5. **파일 I/O** — 파일 없음 시 stderr 출력 후 `return 0` (9-11행). 테스트는 임시 경로·존재하지 않는 파일로 검증.

6. **고정 배열** — `ages[10000]` 등 (`SHealth.h:13-16`). `count` 증가 시 버퍼 오버플로. 대용량 CSV 통합 테스트 시 상한 검증.

7. **공개 API 시그니처** — 리팩토링·기능 추가 시 `calculateBmi`/`getBmiRatio` 시그니처는 README 실습 범위에서 유지; 신규 기능은 **새 메서드** 권장.

8. **`id` 컬럼** — 파싱하지 않음(`tokens[0]` 미사용). 향후 사용자 목록 API 시 id 필요.

---

## 3. 예외·경계값 조건

### 3.1 경계·예외 표

| 영역 | 경계·예외 | 기대 동작(스펙 합의안) | 레거시 리스크 |
|------|-----------|-------------------------|---------------|
| BMI | 18.5 | 저체중 | ✓ |
| BMI | 18.5+ε | 정상 | ✓ |
| BMI | 23.0 | 과체중 | ✓ |
| BMI | 25.0 | **비만** | **미집계** (`> 25`만, 71행) |
| BMI | 25.0−ε | 과체중 | ✓ |
| 연령 | 19 | 집계 제외 | 구간 밖 |
| 연령 | 20, 29 | 20대 | 하한·상한-1 |
| 연령 | 30 | 30대 | 구간 경계 |
| 연령 | 69, 70 | 60대 / 70대 | 70≤age<80 |
| 연령 | 71 | 집계 제외 | 구간 밖 |
| 체중 | 0 | 동일 연령대 평균 보정 | `ageCount=0` 시 나눗셈 |
| 체중 | 양수 | 그대로 사용 | — |
| 키 | 0 | (신규) 연령대 평균 보정 후 BMI | 현재 **0 나눗셈** |
| 키 | 양수(cm) | m=cm/100 변환 | — |
| 파일 | 없음 | 0 반환, 오류 메시지 | ✓ |
| 파일 | 헤더만 | count=0 | 비율 0/0 |
| 파일 | 빈 데이터 줄 | `break`로 중단 | 부분 로드 |
| 배열 | count>10000 | 거부 또는 컨테이너 | 오버플로 |
| 비율 | 연령대 인원 sum=0 | 0% 또는 예외 정의 | **미정의** (77행 등) |

### 3.2 스펙 vs 레거시 — BMI=25 (중요)

| 구분 | README | `SHealth.cpp:71` |
|------|--------|------------------|
| 비만 | BMI **≥** 25 | `bmis[i] **>** 25` |
| BMI=25.0 | 비만 | **어느 분류에도 미포함** |

**TDD 권장:** `ClassifyBmi_At25_IsObesity` 테스트를 스펙 기준으로 먼저 작성(Red) → 구현을 `>= 25`로 수정(Green).

### 3.3 weight=0 보정 — ageCount=0

연령대 내 모든 레코드가 `weight==0`이면 `ageCount==0`, `weights[i] = sum / ageCount` (44행)에서 **0으로 나눗셈**. 스펙 합의안: 보정 스킵·0 유지·예외 중 하나를 테스트로 고정.

---

## 4. README 4단계 신규·확장 요구사항 명세

### 4.1 SRP 리팩토링

| 항목 | 내용 |
|------|------|
| 입력 | CSV 경로 |
| 출력 | 내부 상태(보정된 weight/height, bmis, 연령대 통계) |
| 분리 제안 | `loadFromFile` / `imputeMissingWeights` / `imputeMissingHeights`(신규) / `computeAllBmis` / `aggregateByAgeBand` |
| 기존 API | `calculateBmi`는 위 단계 오케스트레이션으로 유지 가능 |
| 테스트 | 각 단계 단위 테스트 + 통합 1건 |

### 4.2 특정 연령대 BMI 분포 비율

| 항목 | 내용 |
|------|------|
| 관계 | 기존 `getBmiRatio(ageClass, type)`와 **동일 도메인** — 중복 가능 |
| 구현 | 리팩토링 후 `map<AgeBand, map<Category, double>>` 등으로 단일 소스 |
| 테스트 | 20대 4 type 합 ≈ 100% |

### 4.3 height==0 평균 보정 (신규, weight=0 대칭)

| 항목 | 내용 |
|------|------|
| 조건 | `height == 0.0` |
| 규칙 | 동일 연령대 `[a,a+10)` 내 `height ≠ 0` 레코드의 **평균 키(cm)** 대입 |
| 순서 | weight 보정 → height 보정 → BMI 계산 (의존 순서 고정) |
| 예외 | 유효 height 0건(`heightCount==0`) — weight와 동일하게 테스트로 합의 |
| 테스트 | 20대만 height 0 1건 + 유효 2건 → 평균 대입 후 BMI 기대값 |

### 4.4 정상 BMI 사용자 목록 조회

| 항목 | 내용 |
|------|------|
| 필터 | 18.5 < BMI < 23 (README 정상체중) |
| 반환 | `std::vector<int>` id 목록 또는 `(id, bmi)` — **신규 메서드** 권장 |
| 테스트 | fixture 3명(저체중/정상/비만) → 정상 id만 |

### 4.5 전체 사용자 대비 BMI 범주 비율

| 항목 | 내용 |
|------|------|
| 범위 | 연령대 무관, 전체 `count` 기준 4분류 % |
| 관계 | `getBmiRatio`는 연령대별; 본 기능은 **전체 집계** — 별도 API |
| 테스트 | 4건 알려진 분류 → 각 비율 25% |

---

## 5. Google Test 시나리오 목록 (TDD 순서)

> 네이밍: `TEST_F(SHealthBMITest, MethodName_State_ExpectedResult)`  
> fixture: `SetUp`에서 임시 CSV 생성 권장

### 5.1 BMI 계산 (1–5)

1. **CalculateBmi_StandardWeightHeight_ReturnsExpectedBmi** — G:70kg/170cm W:단위 BMI 계산 W:≈24.2215  
2. **CalculateBmi_HeightInMeters_ConvertsFromCm** — G:height 180cm W:BMI W:weight/(1.8²)  
3. **CalculateBmi_UnderweightSample_BmiAtMost18_5** — G:저체중 샘플 W:BMI W:≤18.5  
4. **CalculateBmi_ObesitySample_BmiAtLeast25** — G:비만 샘플 W:BMI W:≥25  
5. **CalculateBmi_MultipleRows_ReturnsCorrectCount** — G:CSV 3행 W:calculateBmi W:returns 3  

### 5.2 체중 0 보정 (6–10)

6. **ImputeWeight_OneZeroInAgeBand_UsesPeerAverage** — G:20대 60kg, 0kg W:보정 W:60kg  
7. **ImputeWeight_AllZeroInBand_NoDivisionByZero** — G:20대 전원 weight 0 W:보정 W:ageCount=0 정책  
8. **ImputeWeight_ZeroOutsideBand_Unchanged** — G:30대만 0 W:20대 평균 미적용  
9. **ImputeWeight_NoZero_UnchangedWeights** — G:모두 양수 W:원값 유지  
10. **ImputeWeight_TwoBands_IndependentAverages** — G:20대·30대 각 0 1건 W:대별 다른 평균  

### 5.3 분류·경계 (11–18)

11. **ClassifyBmi_At18_5_IsUnderweight** — G:BMI=18.5 W:분류 W:저체중(type 100)  
12. **ClassifyBmi_JustAbove18_5_IsNormal** — G:BMI=18.5+ε W:정상(200)  
13. **ClassifyBmi_At23_IsOverweight** — G:BMI=23 W:과체중(300)  
14. **ClassifyBmi_At25_IsObesity** — G:BMI=25 W:비만(400) — **레거시 Red 예상**  
15. **ClassifyBmi_JustBelow25_IsOverweight** — G:BMI=24.99 W:과체중  
16. **ClassifyBmi_Between18_5And23_IsNormal** — G:BMI=21 W:정상  
17. **ClassifyBmi_Age19_ExcludedFrom20Band** — G:age 19 W:20대 sum W:미포함  
18. **ClassifyBmi_Age30_In30Band** — G:age 30 W:30대 집계 W:포함  

### 5.4 비율·API (19–22)

19. **GetBmiRatio_20Underweight_MatchesAggregatedPercent** — G:20대 2명 저체중 W:getBmiRatio(20,100) W:100%  
20. **GetBmiRatio_FourTypes_SumNear100** — G:20대 혼합 4분류 W:4 type 합 W:≈100%  
21. **GetBmiRatio_InvalidType_ReturnsZero** — G:type 999 W:0.0  
22. **GetBmiRatio_InvalidAgeClass_ReturnsZero** — G:ageClass 15 W:0.0  

### 5.5 예외·파일 (23–25)

23. **CalculateBmi_FileNotFound_ReturnsZero** — G:없는 경로 W:0  
24. **CalculateBmi_HeaderOnly_ReturnsZero** — G:헤더만 W:count 0  
25. **CalculateBmi_EmptyLine_StopsOrSkips** — G:데이터 후 빈 줄 W:정책에 맞는 count  

### 5.6 신규 기능 (26–30, 4단계)

26. **ImputeHeight_ZeroInBand_UsesAverageHeight** — G:height 0 + 유효 peer W:평균 cm  
27. **GetNormalBmiUsers_Mixed_ReturnsOnlyNormalIds** — G:4분류 혼합 W:id 목록 W:정상만  
28. **GetOverallBmiRatio_AllObese_Returns100PercentObesity** — G:전원 BMI≥25 W:비만 100%  
29. **CalculateBmi_HeightZeroBeforeImpute_DivisionRisk** — G:height 0, 미보정 W:Red→Green  
30. **Refactor_CalculateBmi_StillPassesIntegration** — G:shealth.dat 소량 fixture W:회귀 통과  

---

## 6. TDD 구현 순서 제안

1. BMI 순수 계산(헬퍼 추출) → 2. CSV 로드 → 3. weight 보정 → 4. 분류 경계(Red: BMI=25) → 5. getBmiRatio → 6. 예외·파일 → 7. height 보정·신규 API  

---

## 7. 결론

- **진실의 원천:** README + 본 문서의 경계 표; 레거시 `SHealth.cpp`는 BMI=25·0 나눗셈에서 스펙과 불일치.  
- **최우선 테스트:** 14번(`ClassifyBmi_At25_IsObesity`), 6–7번(보정), 11–15번(경계).  
- **신규 기능:** height=0 보정은 weight=0과 대칭 패턴으로 명세·테스트 후 구현.

---

*근거 코드: `SHealth.cpp` 65-73행(분류), 28-47행(보정), 9-11행(파일), `SHealthBMITest.cpp` 현재 `FAIL()` only.*
