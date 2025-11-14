FROM gcc:14.2.0 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake ninja-build \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .
COPY include ./include
COPY src ./src
COPY main.cpp .
COPY test ./test

ARG BUILD_TYPE=Release
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
 && cmake --build build -j$(nproc)

FROM gcc:14.2.0 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build /app/build

CMD ["./build/gtest_exe"]
