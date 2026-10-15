# Round-Robin-Algorithm

## Operating System Scheduling Metrics

### 1. CPU Utilization (CPU Efficiency)

The percentage of total time during which the CPU is busy.

$$
\text{CPU Utilization} = \frac{\text{Total Time} - \text{Idle Time}}{\text{Total Time}} \times 100\%
$$

### 2. Throughput

The number of processes completed per unit of time.

$$
\text{Throughput} = \frac{\text{Number of Processes}}{\text{Total Time}}
$$

### 3. Turnaround Time

The time elapsed from when a process enters the system (`New`) until it terminates (`Terminate`).

$$
\text{Turnaround Time} = \text{Termination Time} - \text{Arrival Time}
$$

### 4. Waiting Time

The total time a process spends waiting in the ready queue before being allocated the CPU.

$$
\text{Waiting Time} = \text{Total Ready Queue Waiting Time}
$$

### 5. Response Time

The time elapsed from when a process enters the system (`New`) until its first I/O operation occurs.

$$
\text{Response Time} = \text{First I/O Time} - \text{Arrival Time}
$$
