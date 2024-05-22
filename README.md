# FOS (Faculty of Computer and Information Sciences Operating System) Project

This project is a microkernel-based operating system developed as part of the Faculty of Computer and Information Sciences curriculum. It incorporates fundamental functionalities of modern operating systems, including modules for memory management, scheduling, and handling concurrency, starvation, and deadlock concepts.

## Project Overview

The microkernel architecture of FOS focuses on minimalism and modularity, implementing only the most essential services in the kernel, while other services run in user space. This design enhances system stability and security.

### Key Features

- **Memory Management**: Implements user/kernel spaces, placement/replacement allocation algorithms, and memory management modules.
- **Scheduling Algorithms**: Integrates the BSD scheduling algorithm for efficient process management.
- **Replacement Allocation Algorithms**: Supports LRU (Least Recently Used) and FIFO (First In, First Out) algorithms for memory management.
- **Concurrency, Starvation, and Deadlock Handling**: Incorporates fundamental concepts to manage concurrent processes, avoid starvation, and handle deadlocks.

### Languages Used

- **C**: The primary programming language used for developing this project.

## Detailed Functionality

### Memory Management

- **User/Kernel Spaces**: Separates the memory into user and kernel spaces for better protection and management.
- **Placement Algorithms**: Allocates memory efficiently to processes.
- **Replacement Algorithms**: Uses LRU and FIFO algorithms to manage memory allocation when the system runs out of space.

### Scheduling Algorithms

- **BSD Scheduler**: Implements the BSD scheduling algorithm, which is known for its efficiency and fairness in managing process execution.

### Concurrency, Starvation, and Deadlock

- **Concurrency**: Manages multiple processes running simultaneously without conflict.
- **Starvation**: Implements strategies to ensure that all processes get the necessary CPU time and resources.
- **Deadlock**: Provides mechanisms to detect and resolve deadlocks, ensuring smooth operation of the OS.

### Prerequisites

- **C Compiler**: Ensure you have a C compiler installed on your system (e.g., GCC).
