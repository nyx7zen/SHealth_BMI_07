# SHealth BMI — 기능 개선(Activity 4) 종합 결과 보고서

| 항목 | 내용 |
|------|------|
| 프로젝트 | SHealth_BMI_07 |
| 작성일 | 2026-05-20 |
| 브랜치 | `feature` |
| 대상 | README Activity 4 — **기능 개선 (2시간)** 5항목 |
| 계획 문서 | `tasks/5.feature_plan.md` |
| 단계별 보고서 | `Report/8` ~ `Report/12` (F1~F5) |

---

## 1. 종합 결과

| README Activity 4 항목 | Phase | 커밋 메시지(TC) | 단계 보고서 | 최종 |
|------------------------|-------|-----------------|-------------|------|
| SRP에 따른 책임 분리등 리팩토링 | F1 | `Refactor_CalculateBmi_StillPassesIntegration` | `Report/8.SHealth_BMI_SRP책임분리.md` | ✅ |
| 특정 연령대의 BMI 분포 비율 계산 기능 추가 | F2 | `GetBmiRatio_20Underweight_MatchesAggregatedPercent` | `Report/9.SHealth_BMI_연령대BMI분포비율.md` | ✅ |
| Height가 0인 경우에 대한 평균치 보정 로직 추가 | F3 | `ImputeHeight_ZeroInBand_UsesAverageHeight` | `Report/10.SHealth_BMI_Height평균치보정.md` | ✅ |
| BMI 정상 범위 사용자 목록 조회 기능 추가 | F4 | `GetNormalBmiUsers_Mixed_ReturnsOnlyNormalIds` | `Report/11.SHealth_BMI_정상BMI목록.md` | ✅ |
| 전체 사용자 대비 각 BMI 범주 비율 계산 기능 추가 | F5 | `GetOverallBmiRatio_AllObese_Returns100PercentObesity` | `Report/12.SHealth_BMI_전체범주비율.md` | ✅ |

**Activity 4 다섯 항목 전체 완료. 최종 테스트: 29개 `TEST_F` — 100% Passed.**

```text
100% tests passed, 0 tests failed out of 29
```

### 검증 명령

```powershell
cd build
cmake --build .
ctest --output-on-failure
```

---

## 2. 최종 아키텍처

### 2.1 파이프라인 (`calculateBmi` Facade)

```
loadRecordsFromCsv
  → imputeMissingWeightsByAgeBand()
  → imputeMissingHeightsByAgeBand()
  → computeAllBmis()
  → aggregateRatiosByAgeBand()
  → aggregateOverallRatios()
```

- God Method 해소: `calculateBmi`는 위 단계 **오케스트레이션만** 수행
- 보정 순서 고정: weight → height → BMI (의존성 보장)

### 2.2 공개 API (최종)

| 메서드 | 역할 | Activity 4 항목 |
|--------|------|-----------------|
| `int calculateBmi(const std::string& filename)` | 파이프라인 실행·처리 건수 반환 | F1 (유지) |
| `double getBmiRatio(int ageClass, int type)` | 연령대별 4분류 비율(%) | F2 |
| `std::vector<int> getNormalBmiUserIds() const` | 정상 BMI(18.5<BMI<23) 사용자 id | F4 |
| `double getOverallBmiRatio(int type) const` | 전체 사용자 4분류 비율(%) | F5 |

### 2.3 내부 상태·집계 저장소

| 멤버 | 용도 |
|------|------|
| `ids_[]`, `ages_[]`, `weights_[]`, `heights_[]`, `bmis_[]` | 레코드별 데이터 |
| `bmiRatios_[6][4]` | 연령대×4분류 비율 — `getBmiRatio` 단일 소스 |
| `overallBmiRatios_[4]` | 전체 4분류 비율 — `getOverallBmiRatio` 단일 소스 |

### 2.4 도메인 규칙 (공통)

| BMI 구간 | 분류 | type |
|----------|------|------|
| ≤ 18.5 | 저체중 | 100 |
| 18.5 < BMI < 23 | 정상 | 200 |
| 23 ≤ BMI < 25 | 과체중 | 300 |
| ≥ 25 | 비만 | 400 |

- 연령대: `[a, a+10)`, a ∈ {20, 30, 40, 50, 60, 70}
- `weight==0` / `height==0`: 동 연령대 유효 값 평균 보정 (유효 0건 시 스킵)

---

## 3. 단계별 요약 (Report 8~12)

### Phase F1 — SRP 책임 분리 (`Report/8`)

| 항목 | 내용 |
|------|------|
| 목표 | `calculateBmi` Facade화, 파이프라인 5단계 명시 |
| 핵심 변경 | height 보정 훅(`imputeMissingHeights*`) 추가, private 단계 함수 분리 |
| 신규 TC | `Refactor_CalculateBmi_StillPassesIntegration` (§5 #30) |
| 테스트 | 22 → **23** Passed |

**Before:** load → weight 보정 → BMI → 연령대 집계  
**After:** load → weight → **height** → BMI → 연령대 집계

---

### Phase F2 — 연령대 BMI 분포 비율 (`Report/9`)

| 항목 | 내용 |
|------|------|
| 목표 | `getBmiRatio` + `bmiRatios_` 집계·조회 **단일 소스** 확립 |
| 구현 | 로직 변경 없음(테이블 lookup 기존 완료) — 문서화·TC 보강 |
| 신규 TC | `GetBmiRatio_20Underweight_MatchesAggregatedPercent` (§5 #19) |
| 회귀 TC | `GetBmiRatio_FourTypes_SumNear100` 등 #20~22 |
| 테스트 | 23 → **24** Passed |

---

### Phase F3 — Height=0 평균치 보정 (`Report/10`)

| 항목 | 내용 |
|------|------|
| 목표 | weight=0 보정과 대칭인 height=0 보정 검증 |
| 구현 | F1에서 구현된 `imputeMissingHeightsForBand` — F3는 전용 TC 추가 |
| 신규 TC | `ImputeHeight_ZeroInBand_UsesAverageHeight` (#26) |
| | `CalculateBmi_HeightZeroBeforeImpute_DivisionRisk` (#29) |
| 테스트 | 24 → **26** Passed |

---

### Phase F4 — 정상 BMI 사용자 목록 (`Report/11`)

| 항목 | 내용 |
|------|------|
| 목표 | 18.5 < BMI < 23 사용자 id 목록 조회 |
| 신규 API | `getNormalBmiUserIds() const` |
| 데이터 | CSV id(1열) → `ids_[]` 적재 |
| 신규 TC | `GetNormalBmiUsers_Mixed_ReturnsOnlyNormalIds` (§5 #27) |
| 테스트 | 26 → **27** Passed |

---

### Phase F5 — 전체 BMI 범주 비율 (`Report/12`)

| 항목 | 내용 |
|------|------|
| 목표 | 연령대 무관 전체 사용자 4분류 비율 |
| 신규 API | `getOverallBmiRatio(int type) const` |
| 집계 | `aggregateOverallRatios()` → `overallBmiRatios_` |
| 신규 TC | `GetOverallBmiRatio_AllObese_Returns100PercentObesity` (#28) |
| | `GetOverallBmiRatio_FourTypes_Each25Percent` |
| 테스트 | 27 → **29** Passed |

---

## 4. Activity 4 신규·핵심 TC 매핑 (§5.6)

| # | TEST_F | Phase | Report |
|---|--------|-------|--------|
| 19 | `GetBmiRatio_20Underweight_MatchesAggregatedPercent` | F2 | 9 |
| 26 | `ImputeHeight_ZeroInBand_UsesAverageHeight` | F3 | 10 |
| 27 | `GetNormalBmiUsers_Mixed_ReturnsOnlyNormalIds` | F4 | 11 |
| 28 | `GetOverallBmiRatio_AllObese_Returns100PercentObesity` | F5 | 12 |
| 29 | `CalculateBmi_HeightZeroBeforeImpute_DivisionRisk` | F3 | 10 |
| 30 | `Refactor_CalculateBmi_StillPassesIntegration` | F1 | 8 |

추가 TC: `GetOverallBmiRatio_FourTypes_Each25Percent` (F5, 전체 4분류 각 25%).

---

## 5. 변경 파일 누적

| 파일 | F1 | F2 | F3 | F4 | F5 |
|------|:--:|:--:|:--:|:--:|:--:|
| `src/main/cpp/SHealth.h` | ● | ● | ● | ● | ● |
| `src/main/cpp/SHealth.cpp` | ● | | ● | ● | ● |
| `src/test/cpp/SHealthBMITest.cpp` | ● | ● | ● | ● | ● |
| `README.md` (Activity 4 체크) | ● | ● | ● | ● | ● |

---

## 6. Git 커밋 이력 (Activity 4)

| 순서 | 커밋 메시지 | Phase |
|------|-------------|-------|
| 1 | `Refactor_CalculateBmi_StillPassesIntegration` | F1 |
| 2 | `GetBmiRatio_20Underweight_MatchesAggregatedPercent` | F2 |
| 3 | `ImputeHeight_ZeroInBand_UsesAverageHeight` | F3 |
| 4 | `GetNormalBmiUsers_Mixed_ReturnsOnlyNormalIds` | F4 |
| 5 | `GetOverallBmiRatio_AllObese_Returns100PercentObesity` | F5 |

브랜치: `feature` → `origin/feature`

---

## 7. 제약 준수 여부

| 제약 (`tasks/5.feature_plan.md`) | 준수 |
|----------------------------------|------|
| `calculateBmi` / `getBmiRatio` 시그니처 유지 | ✅ |
| 신규 기능은 새 public 메서드 | ✅ (`getNormalBmiUserIds`, `getOverallBmiRatio`) |
| God Method에 로직 누적 금지 | ✅ |
| Activity 3 TC 22건 회귀 | ✅ (최종 29건 중 기존 TC 포함) |
| 임시 CSV fixture, `shealth.dat` 전체 의존 금지 | ✅ |
| Phase별 커밋·TC 단위 진행 | ✅ |

---

## 8. 산출물 목록

| 유형 | 경로 |
|------|------|
| 종합 보고서 (본 문서) | `docs/feature_report.md` |
| 단계별 보고서 | `Report/8` ~ `Report/12` |
| 대화 Export | `Prompting/8` ~ `Prompting/12` (`{Report파일명}-prompt.md`) |
| 계획 프롬프트 | `tasks/5.feature_plan.md` |

---

## 9. 결론

README Activity 4 **기능 개선 5항목**을 Phase F1~F5 순으로 완료했다. SRP 파이프라인 위에 연령대별 비율·height 보정·정상 사용자 목록·전체 범주 비율을 단계적으로 추가했으며, 각 단계마다 Google Test로 Green을 확인했다. Activity 3(22 TC) 회귀를 포함해 **최종 29 TC 100% Pass**로 기능 개선 단계를 마쳤다.
