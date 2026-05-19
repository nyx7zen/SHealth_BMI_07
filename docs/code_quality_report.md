# SHealth BMI — 코드 품질·SOLID 분석 보고서

| 항목 | 내용 |
|------|------|
| 프로젝트 | SHealth_BMI_07 |
| 작성일 | 2026-05-19 |
| 분석 대상 | `SHealth::calculateBmi()`, `getBmiRatio()`, 연관 private 멤버·`split()` |
| 근거 | `SHealth.cpp` / `SHealth.h`, `README.md`, `tasks/0.setting.md`, `Report/0.SHealth_BMI_코드분석보고서.md` |

---

## 문제점 분석 표

| 문제점 | 위반 원칙/스멜 | 영향 | 개선 방향 | 우선순위 (1~5) |
|--------|-----------------|------|-----------|----------------|
| God Method `calculateBmi` (~103줄, 4단계 파이프라인) | SRP, Long Method | I/O·보정·BMI·통계 변경 시 한 함수 전면 수정, 단위 테스트 격리 불가 | `loadRecordsFromCsv`, `imputeMissingWeightsByAgeBand`, `computeAllBmis`, `aggregateRatiosByAgeBand`로 추출; `SHealth`는 Facade로 공개 API 유지 | 2 |
| 스펙 불일치: 비만 조건 `bmis[i] > 25` (L71) vs README `≥ 25` | 스펙 드리프트, 조건문 복잡도 | BMI=25.0 레코드가 4분류 어디에도 집계되지 않음 → 비율 합 < 100% | `ClassifyBmi_At25_IsObesity` 등 경계 테스트 추가 후 `>= kObesityThreshold`로 수정 | 1 |
| BMI=23.0 경계: 정상은 `< 23`, 과체중은 `≥ 23` (L67–69) — 23.0은 과체중으로 일치 | 조건문 복잡도 (부분) | 23.0은 스펙과 일치하나, `> 18.5 && < 23` 등 중복 조건으로 가독성·유지보수 저하 | `classifyBmi(double)` 단일 함수 + `enum class BmiCategory`로 경계 일원화 | 2 |
| 0 나눗셈: `weights[i] = sum / ageCount` (L44), `(double)*100 / sum` (L77–105) | 방어적 프로그래밍 부재 | `ageCount==0` 또는 연령대 인원 `sum==0` 시 UB/NaN/inf, 크래시 가능 | 보정 전 `ageCount` 검사·스킵; 집계 시 `sum==0`이면 비율 0 또는 `std::optional` 반환; `ImputeWeights_NoValidPeers_LeavesZero` 테스트 | 1 |
| 테스트 부재: `SHealthBMITest.cpp` `FAIL()` only (L4–6) | 테스트 불가, TDD 불가 | 회귀·스펙 수정·리팩토링 안전망 없음 | `TEST_F(SHealthBMITest, …)` + 임시 CSV fixture로 BMI·보정·분류·비율·엣지 케이스 Green 확보 후 리팩토링 | 1 |
| Magic Number 산재 (18.5/23/25, 20~70, 100~400, 10000 등) | Magic Number | 임계값·type 코드 변경 시 누락·오타, README와 불일치 재발 | `constexpr double kBmiUnderweightMax = 18.5` 등, `enum class BmiCategory : int`, `enum class AgeBand` | 2 |
| `getBmiRatio` 24-way if-else 사다리 (L111–136) | OCP, Duplicated Code | 연령대 80대 추가·type 500 추가 시 분기 4~6개씩 증가, `calculateBmi` 저장부와 이중 수정 | `std::array<std::array<double,4>,6>` 또는 `map<pair<AgeBand,BmiCategory>,double>` + `(ageClass,type)`→인덱스 변환 | 3 |
| Data Clumps: parallel array 4개 + 비율 멤버 24개 (`SHealth.h` L12–25) | Data Clump, Feature Envy | 필드 추가 시 헤더·두 메서드 동시 수정, 레코드 단위 추적 불가 | `struct HealthRecord { int id; int age; double weight, height, bmi; }`, `AgeGroupStats`에 4비율 응집 | 3 |
| `count` 상한 미검증 (L21–24, 배열 10000) | 경계 미검증 | 10000건 초과 시 버퍼 오버플로우 | `std::vector<HealthRecord>` 또는 로드 시 `count >= kMaxRecords` 검사·예외/조기 반환 | 3 |
| `stoi`/`stod` 예외 미처리 (L21–23) | 예외 안전성 | 잘못된 CSV 한 줄로 전체 `calculateBmi` 실패 | `try/catch` 또는 검증 후 스킵·로그; `ParseRow_InvalidToken_SkipsOrFails` 테스트 | 4 |
| 연령대 루프·`a==20/30/…` 분기 중복 (L29–48, L56–106) | Duplicated Code | 동일 `[a,a+10)` 필터 3회, 저장 시 6분기 복사 | `for (AgeBand band : kAllBands)` + `stats[band][category]` 테이블 쓰기/읽기 공통화 | 3 |
| Poor Naming: 집계 `sum`(인원 수), `type` int (L61, `getBmiRatio`) | Poor Naming | 가독성·리뷰 비용, 도메인 의도 불명확 | `memberCount`, `BmiCategory category` / `enum class` | 4 |
| height=0 보정 미구현 (README 4단계) | OCP, 미완 기능 | 키 0인 레코드 BMI 계산 시 0 나눗셈(L52); 향후 `calculateBmi`에 또 누적될 위험 | `ImputationService::imputeHeight` 대칭 추출; weight 보정과 동일 연령대 평균 패턴 재사용 | 4 |
| README 4단계(정상 BMI 목록·전체 범주 비율)를 `calculateBmi`에 누적 시 | OCP, SRP | God Method 악화, 공개 API·`SHealthBMI.cpp` 결합 증가 | `getNormalBmiUsers()`, `getOverallCategoryRatio(BmiCategory)` 등 **신규 메서드** + 내부 서비스 위임 | 5 |
| `split()`이 `SHealth`에 결합 (L139–147) | SRP (경미) | CSV 파싱 책임이 도메인 클래스에 혼재 | `CsvReader::parseLine` 또는 파일 단위 `load`로 이동 | 5 |
| `height==0` BMI: `(heights[i]/100)²` (L52) | 런타임 버그 | BMI 무한대/NaN → 분류·비율 왜곡 | height 0 보정 선행 또는 BMI 계산 전 유효성 검사 | 4 |

---

## 1. SRP / OCP 요약

### `calculateBmi`가 담당하는 책임 (SRP 위반)

한 메서드(`SHealth.cpp` L6–108)가 다음을 **모두** 수행한다.

1. **CSV 파일 I/O** — `ifstream` 열기, 헤더 스킵, 행 읽기 (L8–16)
2. **파싱·저장** — `split`, `stoi`/`stod`, parallel array에 적재 (L17–24)
3. **weight=0 보정** — 연령대별 평균 대체 (L28–48)
4. **BMI 산출** — cm→m, 제곱 나눗셈 (L50–53)
5. **연령대별 4분류 집계·비율 저장** — 분류 if-else + `a==20`…`70` 분기 (L55–107)

각 책임은 독립적으로 변경·테스트될 수 있으나 현재는 단일 진입점에 묶여 **단일 책임 원칙(SRP)** 을 위반한다.

### OCP — 신규 요구 시 수정 지점

| README 4단계 요구 | 수정이 필요한 위치 |
|-------------------|-------------------|
| height=0 평균 보정 | `calculateBmi` 내 보정 루프(L28–48) 옆에 유사 루프 **추가** 또는 BMI 루프 **전** 삽입 |
| 특정 연령대 BMI 분포 | 집계 루프(L56–107) 또는 `getBmiRatio` |
| 정상 BMI 사용자 목록 | 로드·BMI 배열 순회 **신규** — 현재 private 배열에 직접 의존 |
| 전체 대비 범주 비율 | 전 연령 집계 **신규** — 또 `calculateBmi`에 누적 가능성 높음 |

**개방-폐쇄 원칙(OCP)** 관점에서, 확장(새 보정 규칙·새 통계)마다 `calculateBmi` 본문을 **열어 수정**해야 하며, 동시에 `SHealth.h`의 24개 `double` 멤버와 `getBmiRatio` 24분기를 맞춰야 한다. 연령대 **80대** 추가 시: 루프 상한 `a <= 70`→`80`, 멤버 4개·`a==80` 분기 4줄, `getBmiRatio` 분기 4개 — **최소 3곳 동시 변경**으로 OCP 위반이 명확하다.

### `getBmiRatio`의 age×type 사다리

`(ageClass, type)` 조합 6×4=24개를 각각 `if`로 매핑(L111–135). 새 `type` 코드나 연령대는 **새 else-if 추가**뿐이며, 집계부(`underweight20` 등)와 **이중 유지보수**가 발생한다. 테이블·맵으로 조회하면 저장·조회가 한 구조에 묶여 OCP를 부분적으로 회복할 수 있다.

---

## 2. Magic Number 상수화 표

| 리터럴 | 의미 | 변경 시 영향 | 상수화 제안 |
|--------|------|--------------|-------------|
| `18.5` | 저체중 상한 (README: ≤18.5) | 분류·테스트 경계 전부 | `constexpr double kBmiUnderweightMax = 18.5;` |
| `23` | 정상 상한 / 과체중 하한 | 23.0 과체중 여부 | `kBmiNormalMax = 23.0` (정상: `< 23`) |
| `25` | 과체중 상한 / 비만 하한 | **BMI=25 비만 누락 버그** | `kBmiOverweightMax = 25.0`, 비만: `>= kBmiOverweightMax` |
| `20`, `30`, … `70` | 연령대 구간 시작 `a` | 구간·`getBmiRatio` 키 | `enum class AgeBand { Twenties = 20, …, Seventies = 70 };` |
| `10` | 구간 폭 `[a, a+10)` | 29/30 경계 테스트 | `constexpr int kAgeBandWidth = 10;` |
| `100`, `200`, `300`, `400` | `getBmiRatio` type 코드 | API 호출부·main 출력 | `enum class BmiCategory : int { Underweight = 100, Normal = 200, Overweight = 300, Obesity = 400 };` |
| `100.0` | cm → m (L52) | BMI 스케일 | `constexpr double kCmPerMeter = 100.0;` |
| `100` (집계) | 카운트→백분율 (L77 등) | 출력 단위 | `constexpr double kPercentFactor = 100.0;` |
| `10000` | 레코드 배열 상한 (`SHealth.h` L13–16) | 메모리·오버플로 | `constexpr size_t kMaxRecords = 10000;` 또는 `vector`로 제거 |
| `0.0` | 결측 체중 (L34, L43) | 보정 대상 | `constexpr double kMissingWeight = 0.0;` |
| `1`, `2`, `3` | CSV 컬럼 age/weight/height (`tokens[1]` 등 L21–23) | 파싱 오류 | `enum CsvColumn { Id = 0, Age = 1, Weight = 2, Height = 3 };` |
| `','` | CSV 구분자 (L17) | 포맷 변경 시 (금지) | `constexpr char kCsvDelimiter = ',';` |

---

## 3. Code Smell 요약

### Long Method / God Method

`calculateBmi`는 파일 열기부터 비율 멤버 대입까지 **단일 트랜잭션**처럼 동작한다. Gilded Rose의 `updateQuality()`와 같이 “한 번 호출로 모든 타입(여기서는 파이프라인 단계)을 처리”하는 형태라, 중간 상태(보정만 된 데이터)를 외부에서 검증할 수 없다. 약 100줄은 주석·공백을 제외해도 4개의 논리 블록이 명확히 구분되므로, **추출 후에도 공개 시그니처 `int calculateBmi(const std::string&)`는 orchestration만** 남기는 것이 적절하다.

### Duplicated Code

연령대 `[a, a+10)` 필터가 **체중 보정**(L32–33, L41–42), **BMI 분류 집계**(L63–64), **비율 저장**(L76–106)에 반복된다. 저장 단계는 `a==20` … `a==70`으로 **동일 계산식 4줄×6회** 복사이다. `getBmiRatio`는 이 24개 멤버를 **역으로 풀어내는** 24-way 분기로, 집계와 조회가 **대칭 중복** 관계다. 테이블 하나로 쓰기·읽기를 통합하면 Duplicated Code를 한 번에 줄일 수 있다.

### 조건문 복잡도

BMI 분류(L65–73)는 README 4구간과 **거의** 맞으나, 비만만 `> 25`로 **스펙(`≥ 25`)과 불일치**한다. BMI=25.0은 정상(`<23` false), 과체중(`≥23 && <25` false), 비만(`>25` false)에 모두 해당하지 않아 **미분류**된다. BMI=23.0은 `>= 23 && < 25`에 포함되어 스펙과 일치한다. 정상 구간의 `> 18.5 && < 23`은 `<= 18.5` 다음 분기라 `18.5`는 저체중으로 처리되며 스펙과 일치한다. 경계값 단위 테스트 없이는 `> 25` vs `>= 25` 수정 시 회귀를 잡을 수 없다.

---

## 4. C++17 개선 방향

| 패턴 | 적용 영역 | 비고 |
|------|-----------|------|
| 테이블 / `std::array` | `getBmiRatio`, 연령대×4비율 저장 | **1순위 권장** |
| `enum class` | `BmiCategory`, `AgeBand` | type 100~400, 연령 20~70 |
| 전략 (`BmiClassifier`) | BMI 분류·향후 height 보정 규칙 | weight/height 보정 대칭 확장 시 |
| `std::variant` | 레코드 결측 상태 | 선택; 과도한 사용 지양 |
| Facade `SHealth` | 공개 API 유지 | 리팩토링 후 호환 |

### 이 프로젝트에 권장하는 1순위: **테이블 기반 집계·조회**

**이유:** (1) 현재 가장 큰 중복이 `getBmiRatio` 24분기와 `a==20`…`70` 저장 분기이다. (2) 도메인 규칙(4분류×6연령대)이 **고정 크기**라 `std::array<std::array<double, 4>, 6>` 또는 `AgeBand`×`BmiCategory` 키 맵으로 O(1) 조회가 가능하다. (3) `enum class`와 결합하면 매직 type 코드를 제거하면서 테이블 인덱스로 변환할 수 있다. (4) 전략 패턴보다 변경 범위가 작고, 테스트로 비율 합≈100%만 검증하면 된다. (5) README 4단계의 “전체 범주 비율”은 테이블 합산 한 줄로 확장하기 쉽다.

설계 스케치(실행 코드 아님):

- `AgeGroupStats stats[6];` — 각각 `ratio[4]` 또는 named 필드
- `getBmiRatio(ageClass, type)` → `stats[index(ageClass)][index(type)]`
- `classifyBmi(double bmi) -> BmiCategory` — 경계는 상수 한곳

`HealthRecord` 구조체 도입은 parallel array 제거에 유효하나, **테이블화보다 diff가 크므로** 테스트 Green 이후 2단계로 진행하는 것이 `tasks/0.setting.md` 순서(상수화 → 추출 → 데이터 구조 → SRP)와 맞다.

---

## 5. 리팩토링 우선순위 Top 5

| 순위 | 작업 | 이유 |
|------|------|------|
| **1** | 단위 테스트 스위트 구축 (`FAIL()` 제거, 경계값·보정·비율·0나눗셈) | 회귀 방지·스펙(BMI=25) 확정·이후 모든 리팩토링의 전제 (`tasks/0.setting.md`: Green 선행) |
| **2** | BMI 분류 스펙 정합 (`>= 25` 비만) + `classifyBmi` 추출 | **런타임 버그**·비율 합 불일치 직접 수정; 테스트로 고정 |
| **3** | 매직 넘버 → `constexpr` / `enum class` | 임계값·type·연령대 변경 시 컴파일 타임 안전성 |
| **4** | 연령대×분류 **테이블**로 집계·`getBmiRatio` 통합 | OCP·24-way/Duplicated Code 해소; 80대·신규 type 확장 비용 감소 |
| **5** | `calculateBmi` SRP 분리 (CsvReader, Imputation, BmiCalculator, Facade) | README 4단계(height 보정·목록·전체 비율)를 God Method에 누적하지 않기 위함 |

---

## 6. 개선 방향 요약

본 프로젝트는 Gilded Rose와 같이 **핵심 진입점 하나**(`calculateBmi`)에 도메인 규칙이 누적된 레거시 형태이다. 가장 시급한 것은 `SHealthBMITest.cpp`의 `FAIL()`을 제거하고 README·`.cursorrules`에 맞는 **경계값 테스트**(18.5, 23, **25**, 연령 19/20·29/30)로 스펙을 고정하는 것이다. 그다음 BMI=25 비만 누락(L71)과 `ageCount`/`sum` 0 나눗셈(L44, L77–105)을 테스트가 붉게 만든 뒤 최소 수정으로 Green을 유지한다.

리팩토링 순서는 **테스트(또는 스펙 고정) → 매직 넘버 → 함수 추출 → 테이블·데이터 구조 → SRP 분리**를 따르며, `calculateBmi` / `getBmiRatio` **공개 시그니처는 유지**하고 내부만 Facade·서비스로 위임한다. `getBmiRatio`의 24-way 분기와 24개 `double` 멤버는 `std::array` 기반 2차원 통계로 대체하는 것이 비용 대비 효과가 가장 크다.

README 4단계(height=0 보정, 정상 BMI 목록, 전체 범주 비율)는 기존 메서드에 코드를 더 쌓기보다 **새 public 메서드**와 내부 컴포넌트로 추가해 OCP를 지킨다. `shealth.dat` 포맷은 변경하지 않는다. premature abstraction(과도한 템플릿·variant 남용)은 피하고, C++17의 `enum class`·`std::vector`·`constexpr` 정도로 단계적 현대화한다.

분석 단계에서는 **보고서 산출만** 수행하며, 실제 소스 수정은 테스트 Green 이후 별도 작업으로 진행한다. `Report/0.SHealth_BMI_코드분석보고서.md`의 Baseline 스멜·스펙 불일치 진단과 본 보고서를 Before & After 비교 기준으로 사용할 수 있다.

---

## 부록: 핵심 코드 위치

| 관심사 | 위치 |
|--------|------|
| CSV 로드·파싱 | `SHealth.cpp` L6–26 |
| weight=0 보정 | `SHealth.cpp` L28–48 |
| BMI 계산 | `SHealth.cpp` L50–53 |
| 분류·비율 집계 | `SHealth.cpp` L55–107 |
| `getBmiRatio` | `SHealth.cpp` L111–137 |
| 멤버·배열 | `SHealth.h` L12–25 |
| 테스트 | `SHealthBMITest.cpp` L4–6 |

### 레거시 분류 분기 (스펙 대조용)

```65:73:src/main/cpp/SHealth.cpp
                if (bmis[i] <= 18.5) {
                    underweight++;
                } else if (bmis[i] > 18.5 && bmis[i] < 23) {
                    normalweight++;
                } else if (bmis[i] >= 23 && bmis[i] < 25) {
                    overweight++;
                } else if (bmis[i] > 25) {
                    obesity++;
                }
```

README 스펙: 비만은 **BMI ≥ 25** → L71은 `>= 25` 또는 `else`로 정상·과체중 이후 나머지 처리가 필요하다.
