# SHealth BMI — 단계별 리팩토링 계획

| 항목 | 내용 |
|------|------|
| 프로젝트 | SHealth_BMI_07 |
| 작성일 | 2026-05-19 |
| 대상 | `calculateBmi()`, `getBmiRatio()` 및 연관 private 멤버 |
| 근거 | `docs/code_quality_report.md`, `docs/requirements_analysis.md`, `tasks/0.setting.md`, `README.md` |
| 범위 | **계획 문서만** — 소스 수정은 단계별 사용자 요청 시 수행 |

---

## 제약·전제 요약

| 제약 | 내용 |
|------|------|
| 공개 API | `int calculateBmi(const std::string& filename)`, `double getBmiRatio(int ageClass, int type)` — 시그니처·반환 의미·호출 규약 **변경 금지** (`SHealthBMI.cpp`·테스트 동기화 필요 시 본문에만 명시) |
| 도메인 규칙 | BMI 임계값 **18.5 / 23 / 25**; 연령대 `[20,30,…,70)` 구간 `[a,a+10)`; type **100/200/300/400**; CSV `id,age,weight,height` **포맷 변경 금지** |
| 진행 방식 | 각 커밋마다 **Green에서만** 다음 단계 — `cmake --build` && `ctest` 통과 후 진행; Red 상태에서 리팩토링 확대 금지 |
| 금지 | 전역 변수 추가; premature abstraction(과도한 템플릿·`variant` 남용); 한 커밋에 테스트+테이블화+SRP 혼합 |
| God Method | README 4단계(height 보정·정상 목록·전체 비율)는 **`calculateBmi`에 누적하지 않음** — Phase D 신규 public 메서드로 분리 |

**권장 순서:** 테스트 스위트(Green) → 스펙 버그 수정(BMI=25) → 상수화 → 함수 추출 → 테이블화 → SRP·신규 API

**품질 보고서 우선순위 매핑:** 표의 `CQ#` 열 = `docs/code_quality_report.md` Top 5 (1=테스트·0나눗셈·BMI=25, 2=God Method·classifyBmi·상수화, 3=테이블·parallel array, 4=height 보정·예외, 5=README 4단계·split 분리)

---

## 단계별 로드맵

| 단계 | 커밋 목표 | 주요 변경 | 선행/추가 테스트 | 검증 명령 | 완료 기준 | CQ# | 커밋 메시지 예시 |
|------|-----------|-----------|------------------|-----------|-----------|-----|------------------|
| 0 | 테스트 기반 구축 | `SHealthBMITest.cpp` `FAIL()` 제거; fixture·임시 CSV 헬퍼; BMI 계산 1~2 TC, 파일 없음 TC | `CalculateBmi_StandardWeightHeight_ReturnsExpectedBmi`, `CalculateBmi_FileNotFound_ReturnsZero` | `cmake --build . && ctest --output-on-failure` | `ctest` Green, `FAIL()` 없음 | 1 | `test: add SHealth fixture and initial BMI/file tests` |
| 1 | 로드·건수 검증 | `CalculateBmi_MultipleRows_ReturnsCorrectCount`, `CalculateBmi_HeaderOnly_ReturnsZero` | 위 + #5, #24 (`requirements_analysis` §5.1) | 동일 | CSV 3행→3, 헤더만→0 | 1 | `test: cover CSV row count and header-only file` |
| 2 | 스펙 Red→Green: BMI=25 | L71 `> 25` → `>= 25` (또는 `else`); **동작 변경 전용 커밋** | `ClassifyBmi_At25_IsObesity`, `ClassifyBmi_JustBelow25_IsOverweight` | 동일 | BMI=25가 type 400·20대 비율에 반영 | 1 | `fix: classify BMI 25.0 as obesity per README` |
| 3 | 경계값 TC 확장 | 18.5/23/19·30 연령 구간 TC 추가 (구현 변경 없음) | `ClassifyBmi_At18_5_IsUnderweight`, `At23_IsOverweight`, `Age19_ExcludedFrom20Band`, `Age30_In30Band` | 동일 | 분류·연령 경계 Green | 1 | `test: add BMI and age-band boundary cases` |
| 4 | `classifyBmi` 추출 | private `BmiCategory classifyBmi(double)` — 경계 **한 곳**; 집계 루프에서 호출 | 기존 경계 TC 유지 (동작 동일) | 동일 | L65–73 분기 1곳으로 수렴 | 2 | `refactor: extract classifyBmi for single classification path` |
| 5 | 매직 넘버 상수화 | `constexpr` BMI 임계값, `kCmPerMeter`, `kPercentFactor`, 연령 step; **동작 변경 없음** | 기존 TC 전부 | 동일 | 리터럴 18.5/23/25/100.0 등 named constant | 2 | `refactor: replace BMI and unit magic numbers with constexpr` |
| 6 | 0 나눗셈 방어 | `ageCount==0` 보정 스킵(L44); `sum==0` 비율 0%(L77–105) | `ImputeWeight_AllZeroInBand_NoDivisionByZero`, 연령대 sum=0 시 비율 TC | 동일 | 보정·집계에서 UB 없음 | 1 | `fix: guard division when ageCount or band sum is zero` |
| 7 | 연령대 필터 중복 제거 | `forAgeBand(int a, Fn)` 또는 `isInAgeBand(age,a)` — 보정·집계 **2곳**만 (저장 분기는 아직) | `ImputeWeight_OneZeroInAgeBand_UsesPeerAverage`, `TwoBands_IndependentAverages` | 동일 | `[a,a+10)` 조건 문자열 1곳 | 3 | `refactor: deduplicate age-band filter in impute and aggregate` |
| 8 | 집계 저장 테이블화 | 24개 `double` 멤버 → `std::array<std::array<double,4>,6>`; `a==20…70` 6분기 제거 | `GetBmiRatio_20Underweight_MatchesAggregatedPercent`, `FourTypes_SumNear100` | 동일 | `SHealth.h` 멤버 수 대폭 감소, 집계·저장 1루프 | 3 | `refactor: store age-band ratios in 6x4 table` |
| 9 | `getBmiRatio` 단순화 | 24-way if-else → 인덱스 조회 1~3줄; 잘못된 키→`0.0` 유지 | `GetBmiRatio_InvalidType_ReturnsZero`, `InvalidAgeClass_ReturnsZero` | 동일 | L111–136 사다리 제거 | 3 | `refactor: replace getBmiRatio ladder with table lookup` |
| 10 | `calculateBmi` SRP 1차 | private `loadRecordsFromCsv`, `imputeMissingWeightsByAgeBand`, `computeAllBmis`, `aggregateRatiosByAgeBand`; public은 오케스트레이션만 | `Refactor_CalculateBmi_StillPassesIntegration` (소량 fixture) | 동일 | `calculateBmi` ~15줄 수준 orchestration | 2 | `refactor: split calculateBmi into pipeline private methods` |
| 11 | `enum class` 도입 | `BmiCategory`, `AgeBand`; type 100~400·연령 20~70을 enum↔int 변환 헬퍼 | 기존 `getBmiRatio(int,int)` 호출 TC | 동일 | 매직 type·age 리터럴 제거 | 2 | `refactor: add enum class for BMI category and age band` |
| 12 | parallel array → `vector` | `HealthRecord` 또는 `vector` 4튜플; `count`·`[10000]` 제거; **동작·TC 동일** | 전체 회귀 + `count>10000` 정책 TC(선택) | 동일 | 오버플로 위험 제거 | 3 | `refactor: replace fixed arrays with vector of health records` |
| 13 | Phase D: height 보정 | `imputeMissingHeightsByAgeBand` (weight 보정 대칭); **BMI 루프 전** | `ImputeHeight_ZeroInBand_UsesAverageHeight`, `CalculateBmi_HeightZeroBeforeImpute` | 동일 | height=0 시 0 나눗셈 없음 | 4 | `feat: impute missing height with age-band average` |
| 14 | Phase D: 신규 API | `getNormalBmiUsers()`, `getOverallCategoryRatio` 등 — **God Method 미누적** | `GetNormalBmiUsers_Mixed_…`, `GetOverallBmiRatio_AllObese_…` | 동일 | README 4단계 3·4항목 Green | 5 | `feat: add APIs for normal BMI users and overall ratios` |
| 15 | 통합·문서화 | `shealth.dat` 소량 통합 TC; `docs/`·주석 정리; 선택 `CsvReader` 분리 | #30 `Refactor_CalculateBmi_StillPassesIntegration` | 동일 | 실습 Before/After 정리 가능 | 5 | `docs: finalize refactor notes and integration test` |

> **Windows:** `build` 디렉터리에서 동일. MSVC 다중 구성 시 `ctest -C Debug --output-on-failure` 필요할 수 있음.

---

## Phase A — 테스트·스펙 고정

**목표:** Red→Green으로 README를 진실의 원천으로 고정. 레거시 L71 `> 25` vs 스펙 `≥ 25` 불일치 해소.

### 체크리스트

- [ ] `SHealthBMITest.cpp` L4–6 `FAIL()` 제거
- [ ] `TEST_F(SHealthBMITest, …)` + SetUp 임시 CSV (`std::filesystem::temp_directory_path()` 등)
- [ ] BMI 계산: `CalculateBmi_StandardWeightHeight_ReturnsExpectedBmi` (EXPECT_NEAR, ε=1e-4)
- [ ] 파일: `CalculateBmi_FileNotFound_ReturnsZero`
- [ ] **Red→Green:** `ClassifyBmi_At25_IsObesity` — fixture에 BMI=25.0 1명 → `getBmiRatio(20,400)` 또는 집계 간접 검증
- [ ] 경계: 18.5, 23, 24.99 (`requirements_analysis` §5.3 #11–15)
- [ ] 보정: `ImputeWeight_OneZeroInAgeBand_UsesPeerAverage`, `ImputeWeight_AllZeroInBand_NoDivisionByZero` (#6–7)
- [ ] `ctest` 전체 Green 후 Phase B 진입

### `ctest`로 확인할 테스트 이름 (Phase A 최소)

| # | 테스트 이름 |
|---|-------------|
| 1 | `CalculateBmi_StandardWeightHeight_ReturnsExpectedBmi` |
| 2 | `CalculateBmi_FileNotFound_ReturnsZero` |
| 3 | `ClassifyBmi_At25_IsObesity` |
| 4 | `ClassifyBmi_At18_5_IsUnderweight` |
| 5 | `ClassifyBmi_At23_IsOverweight` |
| 6 | `ImputeWeight_AllZeroInBand_NoDivisionByZero` |

### Phase A 검증 블록

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

실패 시: **해당 커밋만 롤백** — 스펙 수정(단계 2)과 테스트 추가(단계 0–1)를 분리했는지 확인.

---

## Phase B — 조건 분기·중복 제거

**원칙:** 커밋당 **한 종류** 변경; 동작 동일 refactor와 스펙 수정 커밋 분리.

### 커밋별 작업 (되돌리기 쉬운 단위)

| 커밋 | 작업명 | 변경 파일 | 예상 diff | 선행 테스트 | 금지 (같은 커밋) |
|------|--------|-----------|-----------|-------------|------------------|
| B1 | BMI=25 스펙 수정 | `SHealth.cpp` L71 | ~1줄 | `ClassifyBmi_At25_IsObesity` | `classifyBmi` 추출·테이블 |
| B2 | `classifyBmi(double)` private 추출 | `SHealth.h`, `SHealth.cpp` | ~25줄 | 경계 TC 전부 | `enum class`·테이블 |
| B3 | `constexpr` BMI·단위 상수 | `SHealth.h` 또는 anonymous namespace | ~15줄 | 동일 Green | 동작 변경 |
| B4 | 연령대 필터 헬퍼 | `SHealth.cpp` L29–48, L62–64 | ~30줄 | Impute·연령 TC | 24 멤버 제거 |
| B5 | 0 나눗셈 가드 | `SHealth.cpp` L44, L77–105 | ~10줄 | #6–7, sum=0 TC | API 변경 |

### 현재 스멜 위치 (인용)

| 스멜 | 위치 |
|------|------|
| BMI 분류 if-else, **비만 `> 25` 버그** | `SHealth.cpp` L65–73 |
| 연령대 루프 3회 (`[a,a+10)`) | L29–48, L56–107 |
| `a==20/30/…/70` 저장 6×4줄 | L76–106 |
| `getBmiRatio` 24-way 사다리 | L111–136 |

### Phase B 검증 블록

```bash
cd build
cmake --build .
ctest --output-on-failure
```

실패 시: 직전 커밋이 **동작 변경**인지 **추출만**인지 구분 — 추출 커밋이면 TC 기대값·fixture 데이터 오류를 먼저 의심.

---

## Phase C — 테이블·타입 분리

**목표:** `code_quality_report` 1순위 권장 — 집계·조회를 `std::array` 6×4로 통합, OCP 회복.

### `getBmiRatio` 단순화 설계 스케치 (의사코드, 10줄 이내)

```cpp
// ageClass 20|30|…|70 → band 0..5, type 100|200|300|400 → cat 0..3
int band = (ageClass - 20) / 10;  // 유효성 검사: 20<=ageClass<=70, ageClass%10==0
int cat  = (type / 100) - 1;      // 100→0 … 400→3
if (band < 0 || band > 5 || cat < 0 || cat > 3) return 0.0;
return ratioTable_[band][cat];
```

### `classifyBmi` + 상수 (Phase B/C 경계)

```cpp
enum class BmiCategory { Underweight, Normal, Overweight, Obesity };
BmiCategory classifyBmi(double bmi) {
  if (bmi <= kBmiUnderweightMax) return Underweight;
  if (bmi < kBmiNormalMax) return Normal;
  if (bmi < kBmiOverweightMax) return Overweight;
  return Obesity;  // BMI >= 25
}
```

### 선택 구조 (Facade 유지)

| 구성요소 | 역할 | 단계 |
|----------|------|------|
| `ratioTable_[6][4]` | 집계 결과 단일 소스 | 8–9 |
| `AgeGroupStats` | 4비율 응집 (named 필드 가능) | 8 |
| `BmiClassifier` / policy | 향후 규칙 교체 | Phase D 선택 |
| `CsvReader::parseLine` | `split` 이전 | 15 (낮은 우선순위) |

### Phase C 검증 블록

```bash
cd build
cmake --build .
ctest --output-on-failure
```

실패 시: 테이블 **인덱스 매핑**(band/cat)과 기존 `underweight20` 등 **열 순서** 불일치 여부 확인.

---

## Phase D — SRP·README 4단계 확장

### God Method 방지 원칙

1. `calculateBmi(filename)` = **파이프라인 오케스트레이션**만 (load → impute weight → impute height → compute BMI → aggregate).
2. README 4단계 신규 요구는 **별도 public 메서드**; 본문에 로직 복붙 금지.
3. 내부 서비스(`ImputationService` 등)는 선택 — Facade `SHealth`가 공개 API 유지.

### 신규 메서드 목록 (시그니처는 구현 시 확정, 공개 API 2개는 유지)

| 메서드 (안) | README 항목 | 의존 |
|-------------|-------------|------|
| `imputeMissingHeightsByAgeBand()` (private) | height=0 평균 보정 | weight 보정 후, BMI 전 |
| `std::vector<int> getNormalBmiUsers()` | 정상 BMI 사용자 id 목록 | `18.5 < BMI < 23` |
| `double getOverallCategoryRatio(int type)` | 전체 대비 4분류 % | 연령 무관 전체 count |
| (기존) `getBmiRatio(ageClass, type)` | 연령대별 분포 | 테이블 조회로 유지 |

### Phase D 검증 블록

```bash
cd build
cmake --build .
ctest --output-on-failure
```

실패 시: height 보정 **순서**(weight → height → BMI) 위반 여부 확인.

---

## 리스크·회귀 포인트

| 리스크 | 원인 | 완화 |
|--------|------|------|
| BMI=25 미집계 | L71 `> 25` | 단계 2 전용 커밋 + `ClassifyBmi_At25_IsObesity` |
| 연령대 비율 합 ≠ 100% | BMI=25 누락·`sum` 오류 | `GetBmiRatio_FourTypes_SumNear100` |
| `sum / ageCount` UB | 연령대 전원 weight=0 | 단계 6 가드 + TC #7 |
| `(double)*100 / sum` UB | 연령대 인원 0 | `sum==0` → 0% 고정 + TC |
| `count > 10000` | L21–24 무검사 적재 | 단계 12 `vector` 또는 상한 검사 |
| 테이블 인덱스 오류 | ageClass/type 매핑 | 20대 4 type 합≈100% TC |
| height=0 | L52 0 나눗셈 | Phase D 보정 선행 |
| 리팩터 중 스펙 혼입 | 한 커밋에 fix+refactor | 커밋 묶음 표 준수 |

---

## 전체 완료 체크리스트

### 빌드·품질

- [ ] `cmake --build . && ctest` 전 단계 Green
- [ ] 공개 API 2개 시그니처 유지
- [ ] `shealth.dat` 포맷 미변경
- [ ] 전역 변수 없음

### `docs/requirements_analysis.md` §5 시나리오 커버리지 (1~30)

| 구간 | 시나리오 # | 계획 단계 | 최소 커버 |
|------|------------|-----------|-----------|
| BMI 계산 | 1–5 | 0–1, 10 | #1,2,5 필수 |
| weight 보정 | 6–10 | 0, 6–7, B4 | #6,7,10 |
| 분류·경계 | 11–18 | 2–4, B1–B2 | #11–18 중 11–16,17–18 |
| 비율·API | 19–22 | 8–9 | #19–22 |
| 예외·파일 | 23–25 | 0–1 | #23–25 |
| README 4단계 | 26–30 | 13–15 | #26–30 |

| # | 시나리오 | Phase/단계 |
|---|----------|------------|
| 1–5 | BMI·건수 | 0–1, 10 |
| 6–10 | weight 0 보정 | 6–7, B4 |
| 11–18 | 분류·연령 경계 | 2–4, B1–B2 |
| 19–22 | getBmiRatio | 8–9 |
| 23–25 | 파일·빈 데이터 | 0–1 |
| 26–30 | height·신규 API·통합 | 13–15 |

---

## 매직 넘버 상수화 매핑 (Gilded Rose 대응)

| Gilded Rose (참고) | SHealth BMI | 상수화 제안 | 단계 |
|--------------------|-------------|-------------|------|
| quality `0`, `50` | BMI `18.5`, `23`, `25` | `kBmiUnderweightMax`, `kBmiNormalMax`, `kBmiOverweightMax` | 5 |
| `sellIn` 경계 | 연령 `19`/`20`, `29`/`30`, `[a,a+10)` | `kAgeBandWidth`, `AgeBand` | 5, 11 |
| Backstage `11`, `6` | type `100`~`400`, age step `10` | `enum class BmiCategory` | 11 |
| — | `10000`, `100.0`, `100`(%) | `kMaxRecords` / `vector`, `kCmPerMeter`, `kPercentFactor` | 5, 12 |

---

## 커밋 묶음 금지 매트릭스 (요약)

| 한 커밋에 섞지 말 것 |
|---------------------|
| `FAIL()` 제거 + `calculateBmi` 대규모 분리 |
| BMI=25 fix + 테이블 리팩토링 |
| `classifyBmi` 추출 + `enum class` + 테이블 동시 |
| `constexpr` + 동작 변경 |
| 연령 헬퍼 + 24 멤버 제거 + `getBmiRatio` 시그니처 변경 |
| 테스트 추가 + Phase D 신규 기능 |

---

## 부록: 핵심 코드 앵커

| 관심사 | 파일·라인 |
|--------|-----------|
| God Method 파이프라인 | `SHealth.cpp` L6–108 |
| weight 보정 2-pass | L29–48 |
| BMI 계산 | L50–53 |
| 분류·비율·6분기 저장 | L55–107 |
| getBmiRatio 사다리 | L111–136 |
| parallel array·24 ratio | `SHealth.h` L12–25 |
| 테스트 placeholder | `SHealthBMITest.cpp` L4–6 |

---

*본 문서는 `tasks/3.refactoring_plan_prompt.md` 실행 산출물이며, 실제 `SHealth.cpp` 수정은 사용자가 단계 번호를 지정할 때 수행한다.*
