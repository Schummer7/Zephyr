# Real-Time Avionics Telemetry & Safety Monitor (Zephyr RTOS)

A robust, multithreaded embedded telemetry monitoring application developed with **Zephyr RTOS** targeting the `native_sim` architecture. Built applying safety-critical software principles aligned with **DO-178C** guidelines (static resource allocation, deterministic IPC, and runtime integrity verification).

---

## Key Architecture & Features

- **Zero-Allocation Policy (No `malloc`)**: All kernel primitives (threads, semaphores, queues) are statically defined at compile time using `K_THREAD_DEFINE`, `K_SEM_DEFINE`, and `K_MSGQ_DEFINE`, eliminating runtime heap fragmentation.
- **Producer-Consumer Design**: Sensor sampling and telemetry display are decoupled via a thread-safe Zephyr message queue (`k_msgq`), avoiding race conditions.
- **Preemptive Emergency Task (`rti`)**: A high-priority thread (Priority 0) blocked on a binary semaphore (`k_sem`). It preempts normal telemetry execution instantly if sensor parameters exceed critical safety margins.
- **8-Bit Checksum Integrity Check**: Implements byte-level memory inspection with pointer casting (`uint8_t *`) to seal and verify packets before consumption, preventing the processing of corrupted frames.
- **Defensive Error Handling**: Explicit validation of kernel API returns and discard policies for corrupted frames.

---

## Flight Envelope Thresholds

| Parameter | Operational Range | Critical Trigger Threshold |
| :--- | :--- | :--- |
| **Altitude** | 10,000 – 12,000 ft | > 11,820 ft |
| **Temperature** | -35 °C to -15 °C | Nominal monitoring |
| **Radiation** | 0.00 – 5.00 uSv/h | > 3.00 uSv/h |

---

## File Structure

```text
.
├── CMakeLists.txt        # CMake build configuration for Zephyr
├── prj.conf              # Kernel Kconfig options (random, floating point print)
├── README.md             # Project documentation
└── src/
    └── main.c           # Core application logic & thread implementations
