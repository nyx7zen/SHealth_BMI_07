# SHealth BMI — 단계별 리팩토링 계획

| 항목 | 내용 |
|------|------|
| 프로젝트 | SHealth_BMI_07 (C++17, CMake, Google Test) |
| 대상 | `SHealth::calculateBmi`, `SHealth::getBmiRatio` 및 연관 private 멤버 |
| 작성일 | 2026-05-19 |
| 근거 | `docs/code_quality_report.md`, `docs/requirements_analysis.md`, `tasks/0.setting.md` |
| 산출 목적 | 커밋 단위 리팩토링 로드맵·체크리스트 (코드 변경은 단계별 요청 시 수행) |

---

## README Activity 2 대응 매핑

| README 체크 항목 | 본 계획 Phase·단계 |
|------------------|-------------------|
| **네이밍 개선** | 단계 3 (`enum class`, 상수명), 단계 5 (`classifyBmi`, `memberCount`), 단계 9~10 (SRP 함수명) |
| **하드코드 및 전역변수 제거** | 단계 3 (`constexpr` BMI·연령·type), 단계 8 (`kMaxRecordCount`), 전역 변수 추가 금지 유지 |
| **함수 추출** | 단계 5 (`classifyBmi`), 단계 6 (`forAgeBand`), 단계 9~10 (load / impute / compute / aggregate) |
| **반복/중복 제거** | 단계 6~7 (연령대 3중 루프·`a==20` 분기), 단계 7 (`getBmiRatio` 24-way → 테이블) |

---

## 1. 제약·전제 요약

| 제약 | 내용 |
|------|------|
| **공개 API** | `int calculateBmi(const std::string& filename)`, `double getBmiRatio(int ageClass, int type)` 시그니처·반환 의미 유지. 변경 시 `SHealthBMI.cpp`·테스트 동시 갱신만 계획에 명시 |
| **도메인 규칙** | BMI 임계값 18.5 / 23 / 25, 연령대 `[20,30,…,70)` 구간 `[a,a+10)`, type 100/200/300/400, `shealth.dat` CSV(`id,age,weight,height`) 불변 |
| **Green-only** | 각 커밋 후 `cmake --build` && `ctest` 통과. Red 상태에서 리팩토링 확대 금지 |
| **전역 변수** | 추가 금지 (`tasks/0.setting.md`) |
| **과도한 추상화** | premature template/variant 남용 지양; 1차는 `constexpr`·`enum class`·`std::array` 테이블 |
| **God Method** | README 4단계(height 보정·정상 목록·전체 비율)는 `calculateBmi`에 누적하지 않고 **신규 public 메서드**로 Phase D 분리 |

---

## 2. 단계별 로드맵 표

| 단계 | 커밋 목표 | 주요 변경 | 선행/추가 테스트 | 검증 명령 | 완료 기준 | CQ 우선순위 |
|------|-----------|-----------|------------------|-----------|-----------|-------------|
| **0** | 테스트 스위트 Green 기반 | `SHealthBMITest.cpp` `FAIL()` 제거; `TEST_F` + 임시 CSV fixture; BMI 계산 1~2건, 파일 없음 TC | `CalculateBmi_StandardWeightHeight_*`, `CalculateBmi_FileNotFound_ReturnsZero` | `cmake --build . && ctest` | `ctest` 전체 Green | **#1** 테스트 부재 |
| **1** | 스펙 버그 BMI=25 | `SHealth.cpp:71` `> 25` → `>= 25` (또는 `classifyBmi` 경유) | `ClassifyBmi_At25_IsObesity` (Red→Green) | 동일 | BMI=25.0이 type 400(비만) 집계 | **#1** 스펙 불일치 |
| **2** | 0 나눗셈 방어 | `sum==0` 시 해당 연령대 비율 0.0 스킵; `ageCount==0` 시 보정 스킵 | `ImputeWeight_AllZeroInBand_NoDivisionByZero`, 헤더만 CSV | 동일 | 크래시 없이 Green | **#1** ageCount/sum |
| **3** | 매직 넘버 상수화 | `constexpr` BMI 18.5/23/25, `kCmPerMeter`, `kPercentFactor`, `kMaxRecordCount`; `enum class BmiCategory`, `AgeBand` (내부) | 기존 Green 회귀만 | 동일 | 동작 동일, 리터럴 제거 | **#3** Magic Number |
| **4** | `count` 상한 검증 | `count >= kMaxRecordCount` 시 로드 중단 | (선택) 대용량 스킵 TC | 동일 | 10000행 초과 오버플로 없음 | **#2** count>10000 |
| **5** | `classifyBmi` 추출 | private `BmiCategory classifyBmi(double bmi)` 단일 진입점; `calculateBmi` 65~73행 분기 1곳으로 | `ClassifyBmi_At18_5_*`, `At23_*`, `At25_*`, `JustBelow25_*` | 동일 | 경계 일원화, 중복 `> 18.5 &&` 제거 | **#2** 조건문 복잡도 |
| **6** | 연령대 필터 헬퍼 | `forAgeBand(AgeBand, fn)` 또는 `isInAgeBand(age, band)`; 보정·집계 루프의 `a` 반복 통합 | `ClassifyBmi_Age19_*`, `Age30_*` | 동일 | 연령대 `for (a=20…70)` 패턴 1곳으로 수렴 | **#3** Duplicated Code |
| **7** | 집계 저장 테이블화 | 24개 `underweight20` 등 멤버 → `std::array<std::array<double,4>,6> ratios_`; `a==20/30/…` 76~106행 제거 | `GetBmiRatio_20Underweight_*`, `FourTypes_SumNear100` | 동일 | 집계·저장 단일 테이블 | **#2** 24-way·a== 분기 |
| **8** | `getBmiRatio` 단순화 | ageClass/type → 인덱스 lookup 1~3줄; 111~136행 사다리 삭제 | `GetBmiRatio_InvalidType_*`, `InvalidAgeClass_*` | 동일 | 24분기 제거, API 동일 | **#2** getBmiRatio 사다리 |
| **9** | parallel array → `vector` | `HealthRecord` + `std::vector<HealthRecord>`; 동작·API 동일 | `CalculateBmi_MultipleRows_*`, 통합 1건 | 동일 | Data Clumps 해소 | **#3** Primitive Obsession |
| **10** | SRP 파이프라인 분리 | `loadRecordsFromCsv`, `imputeMissingWeightsByAgeBand`, `computeAllBmis`, `aggregateRatiosByAgeBand`; `calculateBmi`는 Facade 오케스트레이션 | `Refactor_CalculateBmi_StillPassesIntegration` | 동일 | God Method (~102줄) 해소 | **#5** SRP |
| **11** | Phase D 신규 API·문서 | height=0 보정, 정상 BMI 목록, 전체 비율 — **각각 신규 public 메서드**; 계획·요구사항 문서 갱신 | `requirements_analysis` §5 시나리오 26~30 | 동일 | 기존 API 유지, 4단계 기능 분리 | **#2** README 4단계 OCP |

### 커밋 메시지 예시 (단계별)

| 단계 | 커밋 메시지 예시 |
|------|------------------|
| 0 | `test: remove FAIL() and add BMI calculation baseline tests` |
| 1 | `fix: classify BMI 25.0 as obesity per README spec` |
| 2 | `fix: guard division by zero in imputation and ratio aggregation` |
| 3 | `refactor: extract BMI and age band constants with enum class` |
| 4 | `fix: cap record count at kMaxRecordCount to prevent overflow` |
| 5 | `refactor: extract classifyBmi for single classification entry point` |
| 6 | `refactor: deduplicate age band iteration with forAgeBand helper` |
| 7 | `refactor: replace 24 ratio members with 6x4 table storage` |
| 8 | `refactor: simplify getBmiRatio to table lookup` |
| 9 | `refactor: replace parallel arrays with vector of HealthRecord` |
| 10 | `refactor: split calculateBmi into load/impute/compute/aggregate` |
| 11 | `feat: add height imputation and BMI query APIs (SRP, new methods)` |

---

## 3. Phase A — 테스트·스펙 고정 (Red→Green)

**목표:** `code_quality_report` 우선순위 1 — 테스트 없이는 리팩토링 불가.

### 체크리스트

- [ ] `SHealthBMITest.cpp` `TEST(SHealthBMITest, FailedTest)` + `FAIL()` 제거
- [ ] `TEST_F(SHealthBMITest, …)` fixture: `SetUp`에서 임시 CSV 생성·`TearDown` 삭제
- [ ] BMI 순수 계산 1~2건 (`CalculateBmi_StandardWeightHeight_*`, `HeightInMeters_*`)
- [ ] 파일 없음 → `returns 0` (`CalculateBmi_FileNotFound_ReturnsZero`)
- [ ] **Red→Green:** `ClassifyBmi_At25_IsObesity` — 스펙 `≥ 25`, 레거시 `SHealth.cpp:71` `> 25` 수정
- [ ] **Red→Green:** `ImputeWeight_AllZeroInBand_NoDivisionByZero`, `sum==0` 연령대 비율 방어

### `ctest`로 확인할 테스트 이름 (최소)

| 우선순위 | 테스트 이름 | 검증 내용 |
|----------|-------------|-----------|
| P0 | (Green 확보) 기존 스켈레톤 제거 후 1건 이상 PASS | `ctest` Green |
| P0 | `ClassifyBmi_At25_IsObesity` | BMI=25 → 비만(type 400) |
| P1 | `ClassifyBmi_At18_5_IsUnderweight` | 경계 18.5 |
| P1 | `ClassifyBmi_At23_IsOverweight` | 경계 23.0 |
| P1 | `ImputeWeight_OneZeroInAgeBand_UsesPeerAverage` | weight=0 보정 |
| P1 | `ImputeWeight_AllZeroInBand_NoDivisionByZero` | ageCount=0 |
| P2 | `GetBmiRatio_FourTypes_SumNear100` | 연령대 비율 합 ≈ 100% |

### Phase A 검증 블록

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

**Windows (PowerShell):**

```powershell
New-Item -ItemType Directory -Force -Path build | Out-Null
Set-Location build
cmake ..
cmake --build .
ctest -C Debug --output-on-failure
```

실패 시: **해당 커밋만 `git revert` 또는 스태시 롤백** 후, 실패 TC 이름·기대값·실제값을 1~2문장으로 기록하고 다음 단계 진행 금지.

---

## 4. Phase B — 조건 분기·중복 제거

**목표:** `calculateBmi`·`getBmiRatio`의 분기·복제를 **되돌리기 쉬운** 작은 커밋으로 축소.  
**금지:** 한 커밋에 “테스트 추가 + 테이블화 + SRP” 혼합.

### 커밋별 작업 (C1~C7 대응)

| 커밋 | 작업명 | 변경 파일 | 예상 diff | 선행 테스트 | 되돌리기 |
|------|--------|-----------|-----------|-------------|----------|
| **C1** | `FAIL()` 제거 + BMI 1~2 TC | `SHealthBMITest.cpp` | ~80줄 | — | 테스트 파일만 revert |
| **C2** | BMI=25 스펙 수정 | `SHealth.cpp` (71행 근처) | ~3줄 | `ClassifyBmi_At25_IsObesity` | 1줄 조건 revert |
| **C3** | 0 나눗셈 가드 | `SHealth.cpp` (44, 77~105행) | ~15줄 | Impute/sum=0 TC | 가드 if만 revert |
| **C4** | `constexpr` + `enum class` (동작 동일) | `SHealth.h`, `SHealth.cpp` | ~40줄 | 전체 Green 회귀 | 상수만 치환, 로직 무변경 |
| **C5** | `classifyBmi(double)` private 추출 | `SHealth.h`, `SHealth.cpp` | ~25줄 | 경계 TC 11~16 | 함수 인라인화로 복원 가능 |
| **C6** | `forAgeBand` / 동일 필터 함수 | `SHealth.cpp` (29~48, 56~75행) | ~35줄 | 연령 경계 TC 17~18 | 헬퍼 제거·루프 복원 |
| **C7** | `a==20/30/…` → 테이블 인덱스 저장 | `SHealth.h`, `SHealth.cpp` (76~106행) | ~50줄 | `GetBmiRatio_*` | 24 멤버 복원 (단계 7과 분리 권장) |

### BMI 분류 분기 축소 (스펙 vs 레거시)

현재 (`SHealth.cpp:65-73`):

- `> 25` → README `≥ 25` 불일치 → **단계 1 전용 커밋**에서만 수정
- `> 18.5 && < 23` 중복 → **단계 5** `classifyBmi`에서 `else if` 체인 단순화

### 연령대 3중 루프 통합 경로

| 루프 위치 | 행(대략) | 단계 |
|-----------|----------|------|
| weight=0 보정 | 29~48 | C6 |
| BMI 4분류 집계 | 56~75 | C6 |
| 비율 저장 `a==20…` | 76~106 | C7 |

### `getBmiRatio` 24-way 사다리

| 현재 | 목표 | 단계 |
|------|------|------|
| 111~136행 if-else 24갈래 | `ratios_[ageIndex][catIndex]` lookup | **8** (C7 저장 테이블 선행 필수) |

**한 커밋에 섞지 말 것:** C2(스펙) + C7(테이블), C5(classify) + C4(enum) 동시 도입.

### Phase B 검증 블록

```bash
cd build && cmake --build . && ctest --output-on-failure
```

---

## 5. Phase C — 테이블·타입 분리

**목표:** `code_quality_report` 1순위 권장 — `std::array` 테이블 + `enum class`로 집계·조회 통합.  
**전제:** Phase A·B Green.

### 설계 스케치 (`getBmiRatio` 단순화, 의사코드 ≤10줄)

```cpp
// ratios_[bandIndex][categoryIndex] — 집계 시 쓰기, getBmiRatio 시 읽기
int bandIdx = ageClassToIndex(ageClass);   // 20→0 … 70→5
int catIdx  = typeToCategoryIndex(type);   // 100→0 … 400→3
if (bandIdx < 0 || catIdx < 0) return 0.0;
return ratios_[bandIdx][catIdx];
```

### 타입·정책 분리 매핑

| 패턴 | 적용 | 단계 |
|------|------|------|
| `classifyBmi(double) -> BmiCategory` | 분류 규칙 일원화 | 5 |
| `std::array<double,4>` × 6 | 연령대×4 비율 | 7~8 |
| `enum class BmiCategory` / `AgeBand` | type 100~400, 연령 20~70 | 3 |
| `HealthRecord` + `vector` | parallel array 제거 | 9 |
| `BmiClassifier` policy (선택) | height 보정·규칙 교체 시 | 11 (Phase D) |
| `CsvReader` / `ImputationService` (선택) | I/O·보정 분리 | 10~11 |
| Facade `SHealth` | 공개 API 유지 | 전 Phase |

### 선택: private 서비스 분해 (시그니처 변경 없음)

```
calculateBmi(filename)
  → loadRecordsFromCsv(filename)
  → imputeMissingWeightsByAgeBand()
  → computeAllBmis()
  → aggregateRatiosByAgeBand()
  → return count
```

### Phase C 검증 블록

```bash
cd build && cmake --build . && ctest --output-on-failure
```

---

## 6. Phase D — SRP·README 4단계 확장

**원칙:** `calculateBmi`에 기능 누적 금지. Facade `SHealth`가 기존 API 유지, 신규 기능은 **별도 public 메서드**.

### 신규 메서드 목록 (제안)

| 메서드 (제안명) | 책임 | README 4단계 |
|-----------------|------|--------------|
| `imputeMissingHeightsByAgeBand()` (private 또는 서비스) | `height==0` → 동일 연령대 평균 cm | height 보정 |
| `std::vector<int> getNormalBmiUserIds()` | 18.5 < BMI < 23 인 id 목록 | 정상 사용자 목록 |
| `double getOverallBmiRatio(int type)` | 연령 무관 전체 4분류 % | 전체 범주 비율 |
| (기존) `getBmiRatio(ageClass, type)` | 연령대별 분포 | 특정 연령대 비율 — 리팩토링 후 테이블 단일 소스 |

### God Method 방지 체크

- [ ] `calculateBmi` 본문에 신규 4단계 로직 **직접 추가하지 않음**
- [ ] height 보정은 weight 보정 **이후**, BMI 계산 **이전** 파이프라인에 삽입
- [ ] 각 신규 API에 전용 TC (`requirements_analysis` 26~30)

### Phase D 검증 블록

```bash
cd build && cmake --build . && ctest --output-on-failure
```

---

## 7. 리스크·회귀 포인트

| 리스크 | 위치 | 증상 | 완화 |
|--------|------|------|------|
| **BMI=25 미분류** | `SHealth.cpp:71` | 4분류 합 < 100% | 단계 1 + `ClassifyBmi_At25_IsObesity` |
| **BMI 경계 18.5/23** | 65~69행 | 저체중/정상/과체중 오분류 | `classifyBmi` + `EXPECT_NEAR` |
| **ageCount=0** | 44행 | NaN·크래시 | 보정 스킵, TC 7 |
| **sum=0** (연령대 무인원) | 77~105행 | 0 나눗셈 | 비율 0.0, TC |
| **count>10000** | 24행 `count++` | 버퍼 오버플로 | 단계 4 상한 |
| **height=0** | 52행 | BMI inf/NaN | Phase D 보정 선행 TC 29 |
| **24-way·테이블 인덱스 오류** | getBmiRatio | 잘못된 age/type 매핑 | enum 인덱스 함수 + Invalid TC 21~22 |
| **리팩토링 중 스펙 혼입** | 분류 조건 | 의도치 않은 동작 변경 | 스펙 수정은 **전용 커밋**(C2)만 |

---

## 8. 전체 완료 체크리스트

### 빌드·테스트

- [ ] `cmake --build .` 성공
- [ ] `ctest --output-on-failure` 전체 Green
- [ ] 공개 API 시그니처 미변경 (또는 변경 시 main·테스트 동기화 문서화)

### README Activity 2 (계획 대응 완료 시)

- [x] 네이밍 개선 — 단계 3, 5, 9~10에 명시
- [x] 하드코드 및 전역변수 제거 — 단계 3, 4, 8
- [x] 함수 추출 — 단계 5, 6, 9~10
- [x] 반복/중복 제거 — 단계 6, 7, 8

### `docs/requirements_analysis.md` §5 시나리오 커버리지 (최소)

| # | 시나리오 | 계획 단계 |
|---|----------|-----------|
| 1~5 | BMI 계산·건수 | 0, 9 |
| 6~10 | weight=0 보정 | 0, 2, 6 |
| 11~18 | 분류·경계 | 1, 5, 6 |
| 19~22 | getBmiRatio·합≈100% | 0, 7, 8 |
| 23~25 | 파일·예외 | 0, 2 |
| 26~30 | 4단계 신규 | 11 |

### 코드 품질 보고서 Top 5 매핑

| CQ 순위 | 작업 | 계획 단계 |
|---------|------|-----------|
| 1 | 테스트 + BMI=25 + 0 나눗셈 | 0, 1, 2 |
| 2 | 테이블화 + SRP + 4단계 API 분리 | 7, 8, 10, 11 |
| 3 | 상수화 + Data Clumps | 3, 9 |
| 4 | 파싱 예외 (선택) | 11 이후 |
| 5 | God Method 분리 | 10 |

---

## 9. 권장 실행 순서 요약

```
테스트 Green(0) → 스펙 버그 BMI=25(1) → 0 나눗셈(2) → 상수화(3)
→ classifyBmi(5) → 연령대 헬퍼(6) → 테이블 저장(7) → getBmiRatio lookup(8)
→ vector 구조(9) → SRP 분리(10) → README 4단계 신규 API(11)
```

단계 4(`count` 상한)는 3 직후 또는 9 이전에 삽입 가능.  
**낮은 우선순위:** `split` → CSV 파서 분리(단계 10 이후, 별도 커밋).

---

## 10. 부록: 핵심 코드 위치 (인용용)

| 관심사 | 파일·행 |
|--------|---------|
| God Method `calculateBmi` | `SHealth.cpp` 6~108 |
| weight 보정 루프 | `SHealth.cpp` 29~48 |
| BMI 분류 (BMI=25 버그) | `SHealth.cpp` 65~73 |
| `a==20/30/…` 저장 분기 | `SHealth.cpp` 76~106 |
| `getBmiRatio` 24-way | `SHealth.cpp` 111~136 |
| 24 비율 멤버 + parallel array | `SHealth.h` 12~25 |
| 테스트 `FAIL()` | `SHealthBMITest.cpp` 4~6 |

---

*본 문서는 계획 전용입니다. 실제 `SHealth.cpp` 수정은 사용자가 단계별로 요청할 때 수행합니다.*
