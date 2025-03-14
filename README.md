# 분산 처리를 이용한 큐브 솔빙
<img src="/img/cube.png" width="70%" height="70%">
## 목차
1. 프로젝트 소개
2. 프로젝트 목표
3. 팀원 구성 및 역할 분담
4. 실행 환경 및 방법
5. 프로젝트 후기
6. 참고 자료

## 1. 프로젝트 소개
- 전체 개발 기간 : 2024-11-11 ~ 2024-12-08
- 프로젝트 주제 선정 기간 : 2024-11-11 ~ 2024-11-14
- 프로토타입 제작 기간 : 2024-11-15 ~ 2024-12-08

- 본 프로젝트는 4대의 라즈베리파이를 사용하여 2x2x2 루빅스 큐브를 맞춰주는 분산 시스템입니다.
- 사용자가 초기 큐브의 상태를 카메라로 촬영하면 시스템은 해당 큐브를 맞추기 위한 솔루션을 제시합니다.
- 3대의 라즈베리파이가 1대의 메인 라즈베리파이와 통신하며 해를 구하는 과정을 분산 처리하여 수행합니다.

## 2. 프로젝트 소개
- 전체 개발 기간 : 2024-11-11 ~ 2024-12-08
- 프로젝트 주제 선정 기간 : 2024-11-11 ~ 2024-11-14
- 프로토타입 제작 기간 : 2024-11-15 ~ 2024-12-08

- 본 프로젝트는 4대의 라즈베리파이를 사용하여 2x2x2 루빅스 큐브를 맞춰주는 분산 시스템입니다.
- 사용자가 초기 큐브의 상태를 카메라로 촬영하면 시스템은 해당 큐브를 맞추기 위한 솔루션을 제시합니다.
- 3대의 라즈베리파이가 1대의 메인 라즈베리파이와 통신하며 해를 구하는 과정을 분산 처리하여 수행합니다.
<img src="/img/3steps.png" width="70%" height="70%">

# 2. 프로젝트 목표
- **단일 고성능 CPU&GPU에 의존하는 대신, 여러 대의 저성능 IoT CPU들을 활용**, 연산을 분산 처리하여 계산 속도를 끌어올리는 것이 목표입니다.
- 온 디바이스 인공지능을 이용한 IoT에 적용할 수 있는 ****분산 시스템을 설계 및 구현하고자 하였습니다. 서버 기준에서 현재 대기 상태인 IoT 가구들의 CPU를 활용할 수 있습니다.
- 시스템 구조는 유지한 채, ‘큐브 솔빙’만 다른 연산으로 치환하여 딥러닝 등 많은 양의 행렬 연산이 필요한 기술들에 적용 가능합니다.

# 3. 팀원 구성 및 역할 분담
| 주상우 | https://github.com/cuffyluv |
| --- | --- |
| 이재혁 | https://github.com/Lee37373 |
| 임상인 | https://github.com/RaCuNi |

## **주상우 (소프트웨어학과)**

- **서브 Pi 1(클라이언트):** 초음파 센서 및 LED 제어
- **서브 Pi 3(클라이언트)**: 조도 센서 및 LED 제어
- 발표 및 PPT 제작

## **이재혁 (소프트웨어학과)**

- **메인 Pi 0(서버)**: 시스템 제어 및 소켓 통신 프로토콜 구상
- 큐브 솔빙 자체 최적화 알고리즘 개발

## **임상인 (소프트웨어학과)**

- **서브 Pi 2(클라이언트)**: 카메라 및 서보 모터 제어
- 픽셀 색상 인식 알고리즘 개발
- 쉘 스크립팅을 통해 Robust 기능 구현

## 공통

- 메인 Pi와 서브Pi 간의 **서버-클라이언트 통신 프로토콜** 구현
- 큐브 솔빙 **분산 처리 시스템** 구현

# 4. 실행 환경 및 방법
## 실행 환경

- Raspberry Pi OS Legacy 64-bit Bullseye (2024-07-04)가 설치된 4 대의 Raspberry Pi 4 Model B

## 실행 방법(보고서 및 PPT 참고)

### 1. 라즈베리파이, 센서 및 에뮬레이터들 배치
| 스키마 | 실제 사진 |
| --- | --- |
| <img src="/img/scheme2.png" width="100%" height="100%"> | <img src="/img/overview.png" width="100%" height="100%"> |

### 2. 메인 파이에서 서버 프로그램 컴파일 후 실행

```c
gcc -o main main.c -lpthread
./main <port>
```

### 3. 서브 Pi에서 클라이언트 프로그램 컴파일 후 실행

```c
gcc -o serve serve.c -lpthread
./serve <ip> <port>
```

### 3. 큐브 6면을 순서대로 촬영
<img src="/img/sensors_with_points.png" width="33%" height="33%">

### 4. 입력된 큐브 상태를 보고 버튼으로 수정
<img src="/img/last.png" width="33%" height="33%">

### 5. 시스템이 자동으로 큐브를 해결할 때까지 대기

### 6. 결과에 따라 큐브를 회전시키며 맞춤
<img src="/img/last2.png" width="33%" height="33%">

# 5. 프로젝트 후기

- 분산 처리 시스템의 설계 및 구현의 실질적인 어려움과 효율성에 대해 깊이 이해하게 되었습니다. ****분산처리 시스템은 이론적으로는 간단해 보이지만 실제 구현에서는 네트워크, 중복처리 등 생각할 것이 많다는 것을 알았습니다.
- 제한된 자원 내에서 창의적이고 효율적인 대안을 찾는 중요성을 배웠습니다.
- 소프트웨어와 하드웨어의 통합 과정을 통해 IoT 시스템 설계의 실제 사례를 경험했습니다.
- 소프트웨어적인 부분은 비교적 정교한 조절이 가능했지만 하드웨어적인 부분에서 복잡성이 매우 커서 쉽지 않았습니다.
- 실제 완성품을 만드는 과정에서 수많은 오류 및 예외가 발생하며 이를 계속 처리해도 완벽할수는 없다는 것을 느꼈습니다.
- 개발을 진행하면서 프로젝트 마감 기한에 대한 고려를 했음에도, 투자한 시간 대비 개발의 진척이 꽤 더뎌 다양한 시나리오에 대한 많은 테스트를 진행하지 못했던 것 같습니다. 특히 코드를 짜고 디버깅하는 과정에서, GDB의 활용법을 개발 막바지에 알게 되어 Segmentation Fault와 같은 런타임 에러에 대한 디버깅 과정에 많은 시간이 소요됐는데, 개발 과정에서의 효율성과 생산성의 중요성을 뼈저리게 느꼈습니다.

# 6. 참고 자료

## 리눅스 카메라 제어 라이브러리

low-level로 카메라를 직접 제어하기 위해 필요한 라이브러리.

- [Part I - Video for Linux API — The Linux Kernel documentation](https://www.kernel.org/doc/html/v4.9/media/uapi/v4l/v4l2.html)
- [Capture an image with V4L2 | Marcus Folkesson Blog](https://www.marcusfolkesson.se/blog/capture-a-picture-with-v4l2/)
- [Capture an image using V4L2 API. Since the last few days, I was trying… | by Athul P | Medium](https://medium.com/@athul929/capture-an-image-using-v4l2-api-5b6022d79e1d)

## 이미지 프로세싱 라이브러리

이미지 파일에서 픽셀을 추출하기 위해 필요한 라이브러리.

- [nothings/stb: stb single-file public domain libraries for C/C++](https://github.com/nothings/stb)
- [stb/stb_image.h at master · nothings/stb](https://github.com/nothings/stb/blob/master/stb_image.h)
