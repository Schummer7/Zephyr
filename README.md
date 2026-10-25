# Flight Software Telemetry & FSM System (Zephyr RTOS)

Embedded flight software prototype developed on **Zephyr RTOS**, designed under strict real-time, deterministic, and fault-tolerant architecture patterns commonly found in aerospace avionics (inspired by ECSS / DO-178C guidelines).

The application implements a decoupled producer-consumer pipeline with frame integrity checks, out-of-envelope telemetry monitoring via high-priority preemption, and a lock-free Finite State Machine (FSM).

---

## 🛠️ System Architecture

### 1. Lock-Free Finite State Machine (FSM)
Global operating modes are managed using Zephyr kernel atomic primitives (`atomic_t`, `atomic_get`, `atomic_set`). This design eliminates the need for mutual exclusion primitives (`k_mutex`) during critical transitions, entirely preventing **priority inversion** and blocking overhead within the Priority 0 emergency task.

* **`MODO_BOOT`:** Initial boot sequence and Power-On Self Test (POST).
* **`MODO_NORMAL`:** Periodic sensor sampling and nominal telemetry processing.
* **`MODO_EMERGENCIA` (Safe Hold Mode):** Contingency state triggered on critical anomalies, throttling sensor acquisition to conserve system resources.

### 2. Thread Topology & Concurrency

| Thread / Task | Priority | Role & Description |
| :--- | :---: | :--- |
| **`aux` (`rti`)** | **0** (Preemptive) | Fast reactive task. Blocks indefinitely on a binary semaphore until the producer flags an out-of-bounds parameter (altitude > 11820 ft or radiation > 3 µSv/h), immediately executing an atomic transition to `MODO_EMERGENCIA`. |
| **`rtos` (`llamar_hilo`)** | **3** | Flight parameter acquisition task. Dynamically adapts its polling rate according to the system state (1 s in Nominal, 3 s in Emergency). Computes payload checksum and queues data into `k_msgq`. |
| **`main`** | **0** (Sys) | Telemetry consumer. Performs system initialization/state promotion, extracts frames from `k_msgq`, verifies data integrity, and logs system telemetry along with current operational mode. |

### 3. Data Integrity & Verification
Each telemetry payload (`struct telemetria`) embeds a byte-wise modular checksum. The consumer recalculates the verification byte upon dequeuing and discards corrupted packets prior to telemetry handling.

---

## 📋 Telemetry Frame Layout

```c
struct telemetria {
    int16_t altura;
    int16_t temperatura;
    float radiacion;
    int16_t numero_comprobaciones;
    uint8_t Check_sum;
};
