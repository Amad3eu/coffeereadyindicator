# Convenience targets. Everything here is also runnable by hand; see README.md.
#
#   make test        native unit tests (no hardware)
#   make test-sanitize  same tests under AddressSanitizer + UBSan
#   make coverage    line coverage report of the native tests (gcovr)
#   make check       cppcheck on the platform-independent layers
#   make build       ESP32 firmware
#   make upload      flash the firmware over USB
#   make monitor     serial monitor at 115200 baud
#   make calibrate   flash the calibration firmware and open the monitor
#   make format      apply clang-format to src/ and test/
#   make check-format  fail if formatting differs (what CI runs)
#   make docs        docs/generated/coffee-sentinel-architecture.pdf
#   make diagrams    render docs/diagrams/*.mmd to docs/generated/diagrams/
#   make gif         docs/images/coffee-flow.gif
#   make clean       remove build output

PIO ?= pio
PYTHON ?= python3
PORT ?=
UPLOAD_FLAGS := $(if $(PORT),--upload-port $(PORT),)
MONITOR_FLAGS := $(if $(PORT),--port $(PORT),)

.PHONY: all test test-sanitize coverage check build upload monitor calibrate format check-format docs diagrams gif clean

all: test build

test:
	$(PIO) test -e native

test-sanitize:
	$(PIO) test -e native-sanitize

coverage:
	scripts/coverage.sh --html

check:
	$(PIO) check -e native --fail-on-defect medium --fail-on-defect high

build:
	$(PIO) run -e esp32dev

upload:
	$(PIO) run -e esp32dev -t upload $(UPLOAD_FLAGS)

monitor:
	$(PIO) device monitor -b 115200 $(MONITOR_FLAGS)

calibrate:
	$(PIO) run -e esp32dev-calibration -t upload $(UPLOAD_FLAGS)
	$(PIO) device monitor -b 115200 $(MONITOR_FLAGS)

format:
	scripts/check-format.sh --fix

check-format:
	scripts/check-format.sh

docs:
	$(PYTHON) scripts/build_docs.py

diagrams:
	$(PYTHON) scripts/build_docs.py --diagrams-only

gif:
	$(PYTHON) scripts/generate_demo_gif.py

clean:
	$(PIO) run -t clean
	rm -rf .pio docs/generated/*
	touch docs/generated/.gitkeep
