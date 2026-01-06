# Zonal570 - Android 포팅 준비

이 저장소는 원래 Visual Studio 기반의 ZONAL570 프로젝트입니다. 이 리포지토리에는 Android 포팅을 위한 초기 인벤토리 리포트와 스크립트가 포함되어 있습니다.

빠른 시작
- 인벤토리 스캔 스크립트: `scripts/inventory_scan.py`
- 생성된 리포트: `ANDROID_PORT_INVENTORY.json`, `ANDROID_PORT_INVENTORY.txt`
- Android 포팅 계획: `ANDROID_PORT_PLAN.txt`

스크립트 실행
```bash
python3 scripts/inventory_scan.py -o ANDROID_PORT_INVENTORY
```

CI 통합
- GitHub Actions 워크플로(`.github/workflows/windows-build.yml`)에 인벤토리 리포트를 업로드하는 job이 추가되어 있습니다.

다음 권장 작업
- `CMakeLists.txt`로 네이티브 빌드 시스템 변환(핵심 모듈 우선)
- JNI 샘플로 Android에서 네이티브 호출 확인
- 하드웨어 종속성(드라이버, SDK) 목록화 및 대체 전략 수립

문의: 변경 사항을 `main`에 병합/검토하고 싶으시면 알려주세요.
