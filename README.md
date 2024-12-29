# Simulated Operating System Shell

This project is a fully-functional, simulated operating system shell designed to showcase essential operating system features, including command execution, process scheduling, and memory management. The shell offers a streamlined environment that mimics the behavior of a modern OS while remaining lightweight and intuitive.

---

## Features and Capabilities

### **1. Shell Environment**
The shell provides an interactive and batch processing environment with support for a wide range of commands. It includes:
- **Command Execution**: Run built-in commands or execute scripts seamlessly.
- **Variable Management**: Store and retrieve variables with the `set`, `print`, and `echo` commands.
- **Filesystem Simulation**:
  - `my_ls`: Lists files and directories in the current folder.
  - `my_mkdir`: Creates directories.
  - `my_touch`: Creates files.
  - `my_cd`: Changes the working directory.
- **Chained Commands**: Multiple commands can be executed on a single line using semicolons.

### **2. Process Scheduling**
The shell supports concurrent execution of programs and implements robust scheduling techniques:
- **Concurrency**: Run up to three programs simultaneously with shared memory.
- **Scheduling Policies**:
  - **First-Come-First-Serve (FCFS)**: Processes are executed in the order they arrive.
  - **Shortest Job First (SJF)**: Shortest processes are prioritized, ensuring efficiency.
  - **Round Robin (RR)**: Processes are rotated with a fixed time slice for fairness.
  - **Aging**: Long-running processes are gradually prioritized to avoid starvation.
- **Multithreading**: A multithreaded scheduler enables concurrent execution of processes using two worker threads.

### **3. Memory Management**
The shell incorporates a paging system to handle larger workloads and simulate virtual memory:
- **Demand Paging**:
  - Only the necessary pages of a program are loaded into memory.
  - Eviction of least recently used (LRU) pages ensures efficient use of memory.
- **Dynamic Partitioning**:
  - Memory is divided into a frame store (for program pages) and a variable store.
  - Frame and variable sizes can be adjusted dynamically.
- **Backing Store Simulation**:
  - Provides storage for program pages that are not currently in memory.
  - Ensures that programs exceeding memory size can still execute efficiently.

---

## Use Cases Enabled by These Features

### **Efficient Multi-Tasking**
The shell can run multiple programs concurrently, switch between them efficiently using scheduling policies, and manage shared memory to ensure fairness and responsiveness.

### **Dynamic Resource Management**
With demand paging and dynamic memory partitioning, the shell can handle programs larger than the available memory, dynamically allocating and evicting pages as needed.

### **Interactive and Automated Workflows**
The shell's support for both interactive and batch modes allows users to run commands manually or automate workflows using script files. Advanced chaining of commands further enhances automation capabilities.

### **Filesystem Interaction**
Basic filesystem commands enable users to create, navigate, and manage files and directories within the simulated environment, offering an OS-like experience.

---

This shell demonstrates the fundamental principles of operating systems, providing a powerful and flexible tool for running programs, managing resources, and exploring key concepts in OS design.
