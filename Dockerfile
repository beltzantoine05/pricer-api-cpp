# Multi-stage build: compile the C++ module in a full toolchain image,
# then ship only the compiled module and the API in a slim runtime image.

# ---- Stage 1: build the pybind11 module -----------------------------------
FROM python:3.12-slim AS build

RUN apt-get update \
 && apt-get install -y --no-install-recommends g++ \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY requirements.txt .
RUN pip install --no-cache-dir cmake ninja pybind11

COPY CMakeLists.txt .
COPY core core
COPY bindings bindings
RUN cmake -S . -B build -G Ninja -DPRICER_BUILD_TESTS=OFF \
      -DPython_EXECUTABLE="$(which python)" \
      -Dpybind11_DIR="$(python -m pybind11 --cmakedir)" \
 && cmake --build build

# ---- Stage 2: runtime ------------------------------------------------------
FROM python:3.12-slim

WORKDIR /app
RUN pip install --no-cache-dir fastapi uvicorn \
 && useradd --system --no-create-home app

COPY --from=build /src/python python
COPY api/main.py api/main.py

USER app
EXPOSE 8000
HEALTHCHECK --interval=30s --timeout=3s \
  CMD python -c "import urllib.request; urllib.request.urlopen('http://localhost:8000/health')"

CMD ["uvicorn", "api.main:app", "--host", "0.0.0.0", "--port", "8000"]
