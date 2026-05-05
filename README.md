<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
</head>
<body>

  <h1>SyncParking: Process Control & Synchronization (Windows & Linux)</h1>

  <p>
        This repository contains two implementations of a multi-threaded parking simulation system. The project focuses on advanced 
        <strong>Inter-Process Communication (IPC)</strong>, thread synchronization, and memory management algorithms applied to physical space allocation.
    </p>

  <hr>

  <h2>Overview</h2>
    <p>
        The simulation manages a parking lane where cars (represented by threads) arrive and attempt to park using different 
        dynamic allocation strategies. The system ensures collision-free movement and orderly access to shared resources 
        using low-level OS primitives.
    </p>

  <h2>Core Features</h2>
    <ul>
        <li><strong>Multi-threaded Architecture:</strong> Each vehicle operates as an independent thread (Chauffeur thread).</li>
        <li><strong>Memory Allocation Algorithms:</strong> Implementation of the four classic strategies for spot searching:
            <ul>
                <li><em>First Fit:</em> Allocates the first available block large enough.</li>
                <li><em>Next Fit:</em> Circular search starting from the last allocation point.</li>
                <li><em>Best Fit:</em> Search for the smallest available block that fits.</li>
                <li><em>Worst Fit:</em> Allocates the largest available block.</li>
            </ul>
        </li>
        <li><strong>Synchronization Primitives:</strong> Comprehensive use of Mutexes and Events to manage shared state.</li>
    </ul>

  <hr>

   <h2>Windows Implementation (Win32 API)</h2>
    <p>The Windows version utilizes the following synchronization mechanisms:</p>
    <ul>
        <li><strong>Mutexes (<code>hMutexAcera</code>):</strong> Ensures exclusive access to global arrays (parking spots and carril).</li>
        <li><strong>Auto-reset Events (<code>hEventoTurno</code>):</strong> Implements a <strong>FIFO Barrier</strong> to manage the entry order of vehicles.</li>
        <li><strong>Manual-reset Events (<code>hEventoCarril</code>):</strong> Controls traffic flow, allowing threads to proceed or wait based on lane occupancy.</li>
        <li><strong>Dynamic Linking:</strong> Integration with external <code>.dll</code> libraries using function pointers.</li>
    </ul>

   <h2>Linux Implementation (POSIX Threads)</h2>
    <p>The Linux version mirrors the logic using POSIX standards:</p>
    <ul>
        <li><strong>Pthreads:</strong> Thread management for vehicle lifecycle.</li>
        <li><strong>Semaphores/Mutexes:</strong> Protection of shared memory segments.</li>
        <li><strong>Condition Variables:</strong> Used to replicate event-based signaling.</li>
    </ul>

  <hr>

   <h2>How to Run</h2>
    
  <h3>Windows</h3>
    <div style="background-color: #f6f8fa; padding: 10px; border-radius: 6px;">
        <code>
            # Compile using GCC<br>
            gcc -o parking2.exe main.c -L. -lparking2<br><br>
            # Run with [delay] and optional [D] for Debug mode<br>
            ./parking2.exe 100 D
        </code>
    </div>

  <h3>Linux</h3>
    <div style="background-color: #f6f8fa; padding: 10px; border-radius: 6px; margin-top: 10px;">
        <code>
            # Compile using terminal<br>
            gcc -pthread -o parking_linux main.c<br><br>
            # Run<br>
            ./parking_linux 100
        </code>
    </div>

  <hr>

  <h2>Technical Concepts Covered</h2>
    <ul>
        <li><strong>Race Conditions:</strong> Prevented through strict Mutex locking.</li>
        <li><strong>Deadlocks:</strong> Avoided using atomic operations like <code>SignalObjectAndWait</code>.</li>
        <li><strong>Fragmentation:</strong> Analyzed through different allocation algorithms.</li>
        <li><strong>Daisy Chain Signaling:</strong> Ensuring thread wake-up propagation in FIFO queues.</li>
    </ul>

</body>
</html>
