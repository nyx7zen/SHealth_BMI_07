# SHealth BMI — 코드 품질·SOLID 분석 보고서

| 항목 | 내용 |
|------|------|
| 프로젝트 | SHealth_BMI_07 (C++17, CMake, Google Test) |
| 분석 대상 | `SHealth::calculateBmi`, `SHealth::getBmiRatio`, 연관 private 멤버·`split()` |
| 작성일 | 2026-05-19 |
| 참조 | `README.md`, `tasks/0.setting.md`, `Report/0.SHealth_BMI_코드분석.md` |

---

## 문제점 분석 표

| 문제점 | 위반 원칙/스멜 | 영향 | 개선 방향 | 우선순위 (1~5) |
|--------|-----------------|------|-----------|----------------|
| God Method `calculateBmi` (~102줄, 4단계 파이프라인) | SRP, Long Method | I/O·보정·BMI·통계 변경 시 한 함수 전면 수정, 단위 테스트 불가 | `loadRecordsFromCsv`, `imputeMissingWeightsByAgeBand`, `computeAllBmis`, `aggregateRatiosByAgeBand`로 추출; `calculateBmi`는 Facade 오케스트레이션만 유지 | 2 |
| BMI=25 스펙 불일치 (`> 25` vs README `≥ 25`) | 스펙 불일치, 조건문 복잡도 | BMI 정확히 25.0인 사용자가 **어느 분류에도 미포함** → 비율 합 < 100%, 통계 왜곡 | 경계값 테스트(`ClassifyBmi_At25_IsObesity`) 추가 후 `else if (bmis[i] >= 25)` 또는 `classifyBmi(double)` 단일 진입점으로 수정 | 1 |
| 연령대 `sum == 0` 시 0 나눗셈 | 방어적 코딩 부재 | 해당 연령대 인원 0명일 때 `* 100 / sum` (77~105행)에서 미정의 동작·크래시 | `if (sum == 0) continue` 또는 비율 0.0 초기화; `aggregate` 반환 타입에 `std::optional` 검토 | 1 |
| 체중 보정 `ageCount == 0` 시 0 나눗셈 | 방어적 코딩 부재 | 구간 내 유효 체중이 없을 때 `sum / ageCount` (44행) 크래시·NaN 전파 | 보정 스킵 또는 명시적 예외/로그; 테스트 `ImputeWeight_NoValidWeights_LeavesZero` | 1 |
| 단위 테스트 부재 (`FAIL()`만 존재) | 테스트 부재 | 회귀·스펙 수정·리팩토링 시 안전망 없음, Red→Green→Refactor 불가 | `TEST_F(SHealthBMITest, …)` + 임시 CSV fixture로 BMI·보정·분류·`getBmiRatio` 경계값 테스트 우선 작성 | 1 |
| `getBmiRatio` 24-way if-else 사다리 | OCP, Duplicated Code, Switch Statements | 연령대·분류 추가 시 111~136행 전체 수정, 누락·오타 위험 | `std::array<std::array<double, 4>, 6>` 또는 `(AgeBand, BmiCategory) → ratio` 맵; `enum class`로 인덱싱 | 2 |
| 24개 비율 `double` 멤버 + 4개 parallel array | Data Clumps, Primitive Obsession | 헤더 비대(18~25행), 필드 추가 시 클래스 전면 확장 | `HealthRecord` + `AgeGroupStats` 구조체, `std::vector<HealthRecord>`; 비율은 2차원 테이블로 응집 | 3 |
| Magic Number 다수 (18.5, 23, 25, 100~400, 20~70 등) | Magic Number | 임계값·코드 변경 시 검색·누락 위험, 의도 불명확 | `constexpr double kBmiUnderweightMax = 18.5` 등; `enum class BmiCategory`, `enum class AgeBand` | 3 |
| 연령대 루프·`a==20/30/…` 분기 중복 | Duplicated Code | 동일 패턴 3회(보정 29~48, 집계 56~107, 조회 111~136) | `for (AgeBand band : kAllAgeBands)` + 테이블 저장으로 `a==20` 분기 제거 | 3 |
| `count` 상한 미검증 (`ages[10000]`) | 경계 미검증 | CSV 10000행 초과 시 버퍼 오버플로 | `if (count >= kMaxRecords) break` 또는 `std::vector` + `reserve` | 2 |
| `stoi`/`stod` 예외 미처리 | 예외 안전성 부재 | 잘못된 CSV 시 `std::invalid_argument` 등으로 프로세스 종료 | try-catch 또는 `std::from_chars`(C++17); 손상 행 스킵·로그 | 4 |
| Poor Naming (`sum`=인원 수, `type` 모호) | Poor Naming | 가독성·리뷰 비용 증가 | `memberCount`, `bmiCategoryCode` / `BmiCategory` enum | 4 |
| `height == 0` 보정 미구현 | 기능 누락, OCP | README 4단계 요구 미충족; height 0 시 BMI 무한/NaN (52행) | `imputeMissingHeightsByAgeBand` 추출, weight 보정과 대칭 구조 | 3 |
| README 4단계 기능을 `calculateBmi`에 누적 시 결합 | OCP, SRP | 정상 BMI 목록·전체 범주 비율·연령대 API가 God Method에 추가됨 | **새 public 메서드** 또는 내부 서비스(`BmiQueryService`)로 확장; Facade `SHealth`가 기존 API 유지 | 2 |
| BMI 분류 if-else 중복 조건 | 조건문 복잡도 | `> 18.5 && < 23` 등 중복; 23.0은 과체중이나 25.0 누락 | `BmiClassifier::classify(double bmi) -> BmiCategory` 단일 함수; 경계는 상수 비교만 | 2 |
| 공개 API 변경 없이 내부만 개선 필요 | 설계 제약 | `SHealthBMI.cpp`·향후 테스트가 `calculateBmi`/`getBmiRatio` 시그니처에 의존 | 리팩토링은 private 추출·Facade 위임; 시그니처 변경 시 테스트·main 동시 갱신 | — |

---

## 1. SRP / OCP 요약

### SRP 위반

`calculateBmi`(`SHealth.cpp` 6~108행)가 **단일 책임**이 아닌 다음을 한 번에 수행합니다.

- CSV 파일 I/O 및 헤더 스킵 (8~15행)
- 행 파싱·`split`·`stoi`/`stod`로 parallel array 적재 (16~25행)
- 연령대별 `weight == 0` 평균 보정 (28~48행)
- BMI 산출 `weight / (height/100)²` (50~53행)
- 연령대별 4분류 집계 및 24개 멤버에 비율(%) 저장 (55~107행)

`getBmiRatio`는 조회만 담당하지만, 저장 구조(24개 필드)와 결합되어 **집계 책임이 클래스 전체에 분산**되어 있습니다.

### OCP 위반

- **BMI 4분류 규칙 변경**(예: WHO 기준) → `calculateBmi` 65~73행 if-else 직접 수정.
- **연령대 추가**(예: 80대) → 보정 루프 상한(29, 56행), 집계 `a==80` 분기, `SHealth.h` 멤버 4개 추가, `getBmiRatio` 분기 4개 추가 — **열린 확장에 닫혀 있음**.
- **README 4단계**(height=0 보정, 정상 BMI 목록, 전체 범주 비율)를 기존 메서드에 이어 붙이면 God Method가 악화되고, 각 기능마다 `calculateBmi` 본문이 비대해집니다. OCP 관점에서는 **새 타입·새 메서드·정책 클래스**로 확장하고 `SHealth`는 Facade로 위임하는 것이 적합합니다.

`getBmiRatio`의 age×type 사다리(111~136행)는 Gilded Rose의 item-type 분기와 동일하게, **조합 하나 추가 = 함수 전체에 elseif 추가** 구조입니다.

---

## 2. Magic Number 상수화 표

| 리터럴 | 의미 | 변경 시 영향 | 상수화 제안 |
|--------|------|--------------|-------------|
| `18.5` | 저체중 상한 (README: ≤18.5) | 분류·비율 전반 | `constexpr double kBmiUnderweightMax = 18.5;` |
| `23` | 정상 상한 / 과체중 하한 | 23.0 경계 집계 | `constexpr double kBmiNormalMax = 23.0;` |
| `25` | 과체중 상한 / 비만 하한 | **25.0 누락 버그**와 직결 | `constexpr double kBmiOverweightMax = 25.0;` |
| `20`, `30`, … `70` | 연령대 구간 시작 `a` | 루프·`getBmiRatio`·멤버명 | `enum class AgeBand { Twenties = 20, …, Seventies = 70 };` |
| `10` | 연령 구간 폭 `[a, a+10)` | 경계 29/30 등 | `constexpr int kAgeBandWidth = 10;` |
| `100`, `200`, `300`, `400` | `getBmiRatio` type 코드 | API·main 출력 | `enum class BmiCategory : int { Underweight = 100, Normal = 200, Overweight = 300, Obesity = 400 };` |
| `100.0` | cm → m 변환 | BMI 스케일 | `constexpr double kCmPerMeter = 100.0;` |
| `100` (집계) | count → 백분율(%) | 출력 단위 | `constexpr double kPercentFactor = 100.0;` |
| `10000` | 레코드 배열 상한 | 오버플로 경계 | `constexpr std::size_t kMaxRecordCount = 10000;` |
| `0.0` | 결측 체중 표식 | 보정 조건 | `constexpr double kMissingWeight = 0.0;` (또는 `std::optional`) |
| `1`, `2`, `3` | CSV 컬럼 age, weight, height | 파싱 오류 | `enum CsvColumn { Id = 0, Age = 1, Weight = 2, Height = 3 };` |
| `','` | CSV 구분자 | `split` 호출 | `constexpr char kCsvDelimiter = ',';` |

---

## 3. Code Smell 요약

### Long Method / God Method

`calculateBmi`는 파일 열기부터 비율 저장까지 **한 트랜잭션처럼** 이어지며 약 102줄입니다. 파이프라인 4단계(로드 → 보정 → BMI → 집계)가 명시적 함수 경계 없이 중첩 `for`와 `if`로만 구분되어, **한 단계만 검증·재사용하기 어렵습니다**. Gilded Rose의 `updateQuality()`와 같이 “한 메서드 = 전체 배치 작업” 형태입니다.

### Duplicated Code

- 연령대 `for (int a = 20; a <= 70; a += 10)` 패턴이 **체중 보정(29~48)**, **BMI 집계(56~107)** 에 반복됩니다.
- 집계 후 `if (a == 20) … else if (a == 30) …` (76~106행)는 동일 대입식 6회 복제입니다.
- `getBmiRatio`(111~136행)는 age×type **24갈래** 동일 형태 반환으로, 집계 저장과 **이중 유지보수** 구조입니다.

### 조건문 복잡도 (스펙 vs 레거시)

현재 분류 (`SHealth.cpp` 65~73행):

```cpp
if (bmis[i] <= 18.5) underweight++;
else if (bmis[i] > 18.5 && bmis[i] < 23) normalweight++;
else if (bmis[i] >= 23 && bmis[i] < 25) overweight++;
else if (bmis[i] > 25) obesity++;   // README: BMI ≥ 25
```

- **18.5, 23**: README와 대체로 일치 (18.5는 저체중, 23.0은 과체중).
- **25.0**: `>= 23 && < 25`에 해당하지 않고, `> 25`도 false → **미분류** (README는 비만).
- 중복 조건(`> 18.5 &&`)은 가독성만 해치며, 단일 `classifyBmi`로 통합하는 편이 안전합니다.

### 기타

- **Data Clumps**: `ages`/`weights`/`heights`/`bmis` 인덱스 `i`로 묶인 레코드 + 24개 비율 필드.
- **테스트**: `SHealthBMITest.cpp`의 `FAIL()`로 `ctest`가 항상 Red.
- **파싱**: `tokens[1~3]` 범위 검사 없음; `count++`만으로 상한 방어 없음.

---

## 4. C++17 개선 방향

| 패턴 | 적용 영역 | 비고 |
|------|-----------|------|
| 테이블/맵 | `getBmiRatio`, 연령대×4 비율 저장 | **1순위 권장** — 24-way 분기 제거, OCP·테스트 용이 |
| `enum class` | `BmiCategory`, `AgeBand` | type 100~400·연령 20~70 치환 |
| 전략/Policy | `BmiClassifier`, 향후 height 보정 | 분류·보정 규칙 대칭 확장 시 |
| `std::variant`/`optional` | 결측 weight/height | 선택; 과도한 상태 기계는 지양 |
| Facade `SHealth` | 공개 API 유지 | 내부 `CsvReader`, `ImputationService` 위임 (`Report/0` 구조) |

### 이 프로젝트 1순위: 테이블 기반 + `enum class`

**이유:** 가장 큰 중복은 `getBmiRatio` 24분기와 `a==20/30/…` 저장 분기입니다. `std::array<std::array<double, 4>, 6>`(또는 `std::map`)과 `AgeBand`/`BmiCategory` 인덱스로 **한 곳에서 읽기·쓰기**가 가능하고, 연령대 추가 시 배열 크기·`kAllAgeBands`만 확장하면 됩니다. 전략 패턴은 BMI 분류 **규칙이 바뀔 때** 2단계로 도입해도 충분하며, premature abstraction을 피할 수 있습니다.

설계 수준 스케치 (실행 코드 변경은 본 단계 범위 외):

```cpp
enum class BmiCategory { Underweight, Normal, Overweight, Obesity };
enum class AgeBand { Twenties = 20, /* … */ Seventies = 70 };
// ratios_[ageIndex][categoryIndex] — getBmiRatio는 테이블 lookup만
```

---

## 5. 리팩토링 우선순위 Top 5

| 순위 | 작업 | 이유 |
|------|------|------|
| 1 | **단위 테스트 작성** (BMI 계산, weight=0 보정, 경계 18.5/23/25, `getBmiRatio` 합≈100%) | `FAIL()` 제거·Green 확보 없이는 스펙 수정·리팩토링 시 회귀 불가 (`tasks/0.setting.md` TDD 규칙) |
| 2 | **BMI=25 및 0 나눗셈 수정** (테스트 Red → Green) | 런타임 버그·통계 왜곡 직접 영향; 우선순위 1 테스트와 동시 진행 |
| 3 | **매직 넘버 상수화 + `classifyBmi` 추출** | 분류 단일 진입점으로 스펙 불일치 재발 방지, 가독성 |
| 4 | **연령대 집계·`getBmiRatio` 테이블화** (`AgeGroupStats`, 2차원 배열) | 24-way·`a==20` 중복 제거, OCP·유지보수 |
| 5 | **`calculateBmi` SRP 분리** (load / impute / compute / aggregate) | God Method 해소; README 4단계 기능을 새 메서드로 수용 가능 |

권장 순서: **테스트(스펙 고정) → 매직 넘버·분류 추출 → 데이터 구조(테이블) → 함수 추출·Facade**.

---

## 6. 개선 방향 요약

본 프로젝트의 핵심 레거시는 `calculateBmi` God Method와 `getBmiRatio` age×type 사다리이며, README 스펙과 구현의 **BMI=25 미분류**가 가장 시급한 기능 결함입니다. 리팩토링 전 `SHealthBMITest.cpp`의 `FAIL()`을 제거하고 Given-When-Then 기반 경계값 테스트로 **테스트를 진실의 원천**으로 삼아야 합니다. 공개 API `calculateBmi`/`getBmiRatio` 시그니처는 `SHealthBMI.cpp` 및 실습 규칙상 유지하고, 내부는 Facade·private 서비스로 단계 분리하는 것이 안전합니다. 매직 넘버는 `constexpr`와 `enum class`로 치환하고, 24개 `double` 멤버는 2차원 테이블과 `HealthRecord` 구조체로 Data Clumps를 해소합니다. `ageCount==0`, `sum==0`, `count>=10000`, 파싱 예외는 테스트로 고정한 뒤 명시적 방어 코드를 추가합니다. height=0 보정·정상 BMI 목록·전체 범주 비율은 `calculateBmi`에 누적하지 말 **새 책임·새 메서드**로 추가해 OCP를 지킵니다. C++17에서는 과도한 템플릿 메타프로그래밍 없이 `std::vector`, `std::array`, `enum class` 정도로 충분하며, 1차 구조 개선은 **테이블 기반 비율 저장**이 효과 대비 비용이 가장 좋습니다. 모든 변경은 `ctest` Green을 유지한 채 작은 커밋 단위(네이밍 → 상수 → 추출 → 구조 → SRP)로 진행하는 것을 권장합니다.

---

## 부록: 핵심 코드 위치

| 관심사 | 위치 |
|--------|------|
| CSV 로드·파싱 | `SHealth.cpp` 6~25 |
| weight=0 보정 | `SHealth.cpp` 28~48 |
| BMI 계산 | `SHealth.cpp` 50~53 |
| 분류·비율 집계 | `SHealth.cpp` 55~107 |
| 비율 조회 사다리 | `SHealth.cpp` 111~137 |
| parallel array·24 비율 멤버 | `SHealth.h` 12~25 |
| 테스트 스켈레톤 | `SHealthBMITest.cpp` 4~6 |
