#!/usr/bin/env bash
set -e

if [[ "$OSTYPE" == "msys" ]]; then
  PLATFORM=win
else
  echo 'build-service-rest.sh currently supports Windows/MSYS builds only.'
  exit 1
fi

if [[ "$ARCH" == "x64" ]]; then
  VS_ARCH=x64
elif [[ "$ARCH" == "arm64" ]]; then
  VS_ARCH=ARM64
else
  echo 'Invalid architecture.'
  exit 1
fi

if [ -z "$QT_BASE_DIR" ]; then
  echo 'QT_BASE_DIR is not set.'
  exit 1
fi

if [ -z "$BUILD_COMPONENT" ]; then
  BUILD_COMPONENT=all
fi

if [ -z "$INCLUDE_SMOKE_DEPS" ]; then
  INCLUDE_SMOKE_DEPS=0
fi

case "$BUILD_COMPONENT" in
  all)
    BUILD_TARGETS=("pcbu_unlock_service" "pcbu_unlockctl" "win-pcbiounlock")
    ;;
  service)
    BUILD_TARGETS=("pcbu_unlock_service")
    ;;
  rest)
    BUILD_TARGETS=("pcbu_unlockctl")
    ;;
  companion)
    BUILD_TARGETS=("win-pcbiounlock")
    ;;
  *)
    echo 'BUILD_COMPONENT must be one of: all, service, rest, companion.'
    exit 1
    ;;
esac

if [[ "$INCLUDE_SMOKE_DEPS" == "1" ]]; then
  if [[ "$BUILD_COMPONENT" == "service" ]]; then
    BUILD_TARGETS+=("pcbu_unlockctl")
  elif [[ "$BUILD_COMPONENT" == "rest" ]]; then
    BUILD_TARGETS+=("pcbu_unlock_service")
  fi
fi

BUILD_CORES=4

mkdir -p build-service-rest
cd build-service-rest

cmake ../../ -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=$ARCH -DQT_BASE_DIR=$QT_BASE_DIR -G "Visual Studio 17 2022" -A $VS_ARCH -DCMAKE_GENERATOR_PLATFORM=$VS_ARCH
cmake --build . --target "${BUILD_TARGETS[@]}" --config Release -- /maxcpucount:$BUILD_CORES

rm -rf service-rest-package service-unlock-package rest-unlock-package pc-bio-unlock-package

if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "service" ]]; then
  mkdir -p service-unlock-package
  cp desktop/Release/pcbu_unlock_service.exe service-unlock-package/
fi

if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "rest" ]]; then
  mkdir -p rest-unlock-package
  cp desktop/Release/pcbu_unlockctl.exe rest-unlock-package/
fi

if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "companion" ]]; then
  mkdir -p pc-bio-unlock-package
  cp natives/win-pcbiounlock/Release/win-pcbiounlock.dll pc-bio-unlock-package/
fi

if [[ "$BUILD_COMPONENT" == "all" ]]; then
  mkdir -p service-rest-package
  cp desktop/Release/pcbu_unlock_service.exe service-rest-package/
  cp desktop/Release/pcbu_unlockctl.exe service-rest-package/
  cp natives/win-pcbiounlock/Release/win-pcbiounlock.dll service-rest-package/
fi

if command -v powershell >/dev/null 2>&1; then
  if [[ "$BUILD_COMPONENT" == "all" ]]; then
    powershell -NoProfile -Command "Compress-Archive -Path 'service-rest-package\\*' -DestinationPath 'PCBioUnlock-ServiceRest-${ARCH}.zip' -Force"
  fi
  if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "service" ]]; then
    powershell -NoProfile -Command "Compress-Archive -Path 'service-unlock-package\\*' -DestinationPath 'PCBioUnlock-ServiceUnlock-${ARCH}.zip' -Force"
  fi
  if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "rest" ]]; then
    powershell -NoProfile -Command "Compress-Archive -Path 'rest-unlock-package\\*' -DestinationPath 'PCBioUnlock-RestUnlock-${ARCH}.zip' -Force"
  fi
  if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "companion" ]]; then
    powershell -NoProfile -Command "Compress-Archive -Path 'pc-bio-unlock-package\\*' -DestinationPath 'PCBioUnlock-PCBioUnlock-${ARCH}.zip' -Force"
  fi
else
  echo 'powershell is required to package the service/rest bundle.'
  exit 1
fi

if [[ "$BUILD_COMPONENT" == "all" ]]; then
  echo "Created service/rest bundle: $(pwd)/PCBioUnlock-ServiceRest-${ARCH}.zip"
fi
if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "service" ]]; then
  echo "Created service-only bundle: $(pwd)/PCBioUnlock-ServiceUnlock-${ARCH}.zip"
fi
if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "rest" ]]; then
  echo "Created rest-only bundle: $(pwd)/PCBioUnlock-RestUnlock-${ARCH}.zip"
fi
if [[ "$BUILD_COMPONENT" == "all" || "$BUILD_COMPONENT" == "companion" ]]; then
  echo "Created pc-bio-unlock bundle: $(pwd)/PCBioUnlock-PCBioUnlock-${ARCH}.zip"
fi
