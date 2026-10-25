# Flight Software Telemetry & FSM System (Zephyr RTOS)

Embedded flight software prototype developed on **Zephyr RTOS**, designed under strict real-time, deterministic, and fault-tolerant architecture patterns commonly found in aerospace avionics (inspired by ECSS and DO-178C guidelines).

The application implements a decoupled producer-consumer pipeline with message queue buffering, memory-aligned transfers, frame integrity verification, out-of-envelope anomaly handling via high-priority preemption, and a lock-free Finite State Machine (FSM).

---

## 🛠️ System Architecture

### 1. Lock-Free Finite State Machine (FSM)
Global operating modes are managed using Zephyr kernel atomic primitives (`atomic_t`, `atomic_get`, `atomic_set`). This design eliminates mutual exclusion primitives (`k_mutex`) during critical transitions, entirely preventing **priority inversion** and blocking delays inside the Priority 0 emergency task.

* **`MODO_BOOT`:** Initial boot sequence and Power-On Self Test (POST).
* **`MODO_NORMAL`:** Nominal operation. Telemetry is sampled and processed at 1 Hz.
* **`MODO_EMERGENCIA` (Safe Hold Mode):** Contingency state triggered on critical anomalies, throttling sensor acquisition from 1 s to 3 s to reduce processing load and simulate spacecraft power/thermal conservation.

### 2. Thread Topology & Concurrency Model

[ Producer: rtos (Prio 3) ]
│
├── (Out-of-bounds trigger) ──> k_sem_give(&semaforo) ──> [ Emergency: rti (Prio 0) ]
│                                                                 │
│                                                      atomic_set(&modo, EMERGENCY)
│
k_msgq_put(&comunicacion)
│
▼
[ k_msgq: 20 slots x 16B ] (4-byte aligned, lock-free ring buffer)
│
▼
k_msgq_get(&comunicacion)
│
[ Consumer: main (Prio 0) ] ──> Checksum Verify ──> Telemetry Console Logging


| Thread / Task | Priority | Scheduling | Role & Description |
| :--- | :---: | :---: | :--- |
| **`aux` (`rti`)** | **0** (Highest) | Preemptive | Emergency response task. Blocks indefinitely on a binary semaphore (`k_sem_take`). When released by the producer, it preempts lower-priority tasks and executes an immediate atomic transition to `MODO_EMERGENCIA`. |
| **`rtos` (`llamar_hilo`)** | **3** | Preemptive | Sensor acquisition task. Simulates flight telemetry, checks safety thresholds, computes frame checksums, and pushes packets into the message queue via `k_msgq_put`. Adjusts its period dynamically (1 s nominal, 3 s emergency). |
| **`main`** | **0** (Sys) | Preemptive | Consumer task. Completes system initialization, sets `MODO_NORMAL`, blocks on `k_msgq_get`, validates data integrity, and formats telemetry output. |

---

## 📬 Inter-Thread Communication & Queuing

Inter-thread communication relies strictly on Zephyr kernel primitives designed for deterministic real-time execution:

### 1. Message Queue (`k_msgq`)
* **Macro:** `K_MSGQ_DEFINE(comunicacion, sizeof(struct telemetria), 20, 4);`
* **Buffer Depth:** 20 slots. Provides ample buffering margin to absorb transient scheduling delays without packet loss.
* **4-Byte Memory Alignment (`align = 4`):** Ensures each telemetry struct aligns to a 32-bit boundary, optimizing CPU memory bus access and preventing unaligned memory exceptions on ARM Cortex-M/RISC-V architectures.
* **Non-Blocking Write (`k_msgq_put(..., K_NO_WAIT)`):** The acquisition task never blocks while enqueuing. If the queue is saturated, it drops the sample rather than introducing deadline jitter into the telemetry loop.
* **Blocking Read (`k_msgq_get(..., K_FOREVER)`):** The consumer task yields CPU time completely while waiting for incoming frames, avoiding polling and saving compute cycles.

### 2. Binary Semaphore (`k_sem`)
* **Macro:** `K_SEM_DEFINE(semaforo, 0, 1);`
* **Initial Count:** 0 (starts locked).
* **Max Count:** 1 (binary mode).
* **Signaling Mechanism:** Serves as a direct wake-up trigger from the producer (`k_sem_give`) to the high-priority supervisor task (`rti`), minimizing interrupt latency and avoiding polling overhead.

---

## 📋 Telemetry Frame Layout & Data Integrity

```c
struct telemetria {
    int16_t altura;                  /* Altitude in feet */
    int16_t temperatura;             /* Sensor temperature in Celsius */
    float radiacion;                 /* Radiation dose rate in uSv/h */
    int16_t numero_comprobaciones;   /* Monotonic packet sequence counter */
    uint8_t Check_sum;               /* Byte-wise modular checksum */
};
