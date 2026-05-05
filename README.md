<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
</head>
<body style="font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif; line-height: 1.6; color: #24292e;">
<h1 style="color: #0366d6; border-bottom: 2px solid #eaecef; padding-bottom: 0.3em;">
🚗 SyncParking: Process Control & Synchronization
</h1>
<p>
This repository contains dual implementations (Windows & Linux) of a multi-threaded parking simulation. It explores <strong>Inter-Process Communication (IPC)</strong> and advanced synchronization primitives.
</p>
<div style="background-color: #fff8c5; border-left: 5px solid #e3b341; padding: 15px; margin: 20px 0; border-radius: 4px;">
<span style="background-color: #bf8700; color: white; padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: bold; text-transform: uppercase; margin-right: 8px;">Important</span>
<strong style="color: #735c0f;">Core Objective:</strong> 
The project simulates physical space allocation using <strong>Dynamic Memory Management Algorithms</strong> applied to a parking lane.
</div>
<h2 style="color: #2f363d;">Overview</h2>
<p>
The simulation manages a parking lane where cars (independent threads) attempt to park using specific allocation strategies. Ordered access and collision-free movement are guaranteed via low-level OS primitives.
</p>
<h2 style="color: #2f363d;">
🧠 Memory Allocation Algorithms 
<span style="background-color: #2188ff; color: white; padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: bold; text-transform: uppercase; vertical-align: middle;">Top Feature</span>
</h2>
<p>Implementation of the four classic strategies for dynamic spot searching:</p>
<ul style="list-style-type: none; padding-left: 0;">
<li style="margin-bottom: 8px;">
<span style="background-color: #dafbe1; color: #1a7f37; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">First Fit</span> 
Allocates the first available block large enough.
</li>
<li style="margin-bottom: 8px;">
<span style="background-color: #ddf4ff; color: #0969da; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">Next Fit</span> 
Circular search starting from the last allocation point.
</li>
<li style="margin-bottom: 8px;">
<span style="background-color: #fff8c5; color: #9a6700; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">Best Fit</span> 
Finds the smallest available block to minimize fragmentation.
</li>
<li style="margin-bottom: 8px;">
<span style="background-color: #ffebe9; color: #cf222e; padding: 2px 6px; border-radius: 3px; font-weight: bold; font-size: 0.9em;">Worst Fit</span> 
Allocates the largest available block.
</li>
</ul>
<hr style="height: 0.25em; padding: 0; margin: 24px 0; background-color: #e1e4e8; border: 0;">
<h2 style="color: #0366d6;">
🪟 Windows Implementation (Win32 API)
<span style="background-color: #f85149; color: white; padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: bold; text-transform: uppercase; vertical-align: middle;">Critical Sync</span>
</h2>
<ul>
<li><strong>Mutexes:</strong> Thread-safe access to shared parking arrays.</li>
<li><strong>Auto-reset Events:</strong> Implements a <strong>FIFO Barrier</strong> for orderly entry.</li>
<li><strong>Manual-reset Events:</strong> Global traffic flow control (Open/Closed lane).</li>
<li><strong>Dynamic Linking:</strong> Advanced DLL integration using <code>GetProcAddress</code> and <code>typedef</code> function pointers.</li>
</ul>
<h2 style="color: #d73a49;">🐧 Linux Implementation (POSIX)</h2>
<ul>
<li><strong>Pthreads:</strong> Management of the vehicle lifecycle.</li>
<li><strong>Semaphores:</strong> Critical section protection.</li>
<li><strong>Condition Variables:</strong> Signal-based thread coordination.</li>
</ul>
<div style="background-color: #fff5f5; border-left: 5px solid #f44336; padding: 15px; margin: 20px 0; border-radius: 4px;">
<span style="background-color: #cf222e; color: white; padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: bold; text-transform: uppercase; margin-right: 8px;">Alert</span>
<strong style="color: #b71c1c;">Technical Note:</strong> 
Atomic operations like <code>SignalObjectAndWait</code> are used to prevent <strong>Deadlocks</strong> and <strong>Race Conditions</strong> during high-concurrency maneuvers.
</div>
<h2 style="color: #2f363d;">Key Concepts Covered</h2>
<table style="width: 100%; border-collapse: collapse; margin-top: 10px;">
<tr style="background-color: #f6f8fa;">
<th style="border: 1px solid #dfe2e5; padding: 8px; text-align: left;">Concept</th>
<th style="border: 1px solid #dfe2e5; padding: 8px; text-align: left;">Resolution Mechanism</th>
</tr>
<tr>
<td style="border: 1px solid #dfe2e5; padding: 8px;"><strong>Entry Order</strong></td>
<td style="border: 1px solid #dfe2e5; padding: 8px;">FIFO Queue via Event Signaling</td>
</tr>
<tr>
<td style="border: 1px solid #dfe2e5; padding: 8px;"><strong>Lane Collisions</strong></td>
<td style="border: 1px solid #dfe2e5; padding: 8px;">Manual-reset Traffic Events</td>
</tr>
<tr>
<td style="border: 1px solid #dfe2e5; padding: 8px;"><strong>Resource Protection</strong></td>
<td style="border: 1px solid #dfe2e5; padding: 8px;">Mutex-guarded Global Arrays</td>
</tr>
</table>
</body>
</html>
