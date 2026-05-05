# 🚗 SyncParking: Process Control & Synchronization

This repository contains dual implementations (Windows & Linux) of a multi-threaded parking simulation. It explores **Inter-Process Communication (IPC)** and advanced synchronization primitives.

> [!IMPORTANT]
> **Core Objective:** The project simulates physical space allocation using **Dynamic Memory Management Algorithms** applied to a parking lane.

---

## 🧠 Memory Allocation Algorithms 
<span style="background-color: #2188ff; color: white; padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: bold; text-transform: uppercase;">Top Feature</span>

Implementation of the four classic strategies for dynamic spot searching:

* <span style="background-color: #dafbe1; color: #1a7f37; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">First Fit</span> Allocates the first available block large enough.
* <span style="background-color: #ddf4ff; color: #0969da; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">Next Fit</span> Circular search starting from the last allocation point.
* <span style="background-color: #fff8c5; color: #9a6700; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">Best Fit</span> Finds the smallest available block to minimize fragmentation.
* <span style="background-color: #ffebe9; color: #cf222e; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">Worst Fit</span> Allocates the largest available block.

---

## 🪟 Windows Implementation (Win32 API)
<span style="background-color: #f85149; color: white; padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: bold; text-transform: uppercase;">Critical Sync</span>

* **Mutexes:** Thread-safe access to shared parking arrays.
* **Auto-reset Events:** Implements a **FIFO Barrier** for orderly entry.
* **Manual-reset Events:** Global traffic flow control (Open/Closed lane).
* **Dynamic Linking:** Advanced DLL integration using `GetProcAddress`.

> [!TIP]
> This implementation uses **Function Pointers** to decouple the core logic from the graphical library.

---

## 🐧 Linux Implementation (POSIX)
* **Pthreads:** Management of the vehicle lifecycle.
* **Semaphores:** Critical section protection.
* **Condition Variables:** Signal-based thread coordination.

> [!CAUTION]
> **Technical Note:** Atomic operations like `SignalObjectAndWait` are used to prevent **Deadlocks** and **Race Conditions** during high-concurrency maneuvers.

---

## 📊 Key Concepts Covered

| Concept | Resolution Mechanism |
| :--- | :--- |
| **Entry Order** | FIFO Queue via Event Signaling |
| **Lane Collisions** | Manual-reset Traffic Events |
| **Resource Protection** | Mutex-guarded Global Arrays |

> [!NOTE]
> This project was developed as part of the Operating Systems course to demonstrate mastery over concurrency and process management.
