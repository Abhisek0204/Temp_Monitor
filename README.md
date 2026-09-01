# ldd-tsensor

A Linux kernel character device driver that simulates a temperature sensor using a random-walk algorithm.
---

## 1. Prerequisites & Environment Setup

This project requires a Linux environment with kernel header support.

> [!WARNING]
> This will not work with WSL

### Installing Dependencies

Run the following command to install required build tools and matching kernel headers:

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) g++ make git
```

#### Check
Verify that kernel headers for your active kernel exist:

```bash
ls -d /usr/src/linux-headers-$(uname -r)
```

---

## 2. Building the Project

### Option A: Build Everything from Project Root 

From the project root directory, run:

```bash
make
```

To clean build artifacts:

```bash
make clean
```

### Option B: Build Driver and App Separately

Building the Driver:
```bash
cd driver
make
```

Building the C++ Application:
```bash
cd app
make
```

---

## 3. Loading and Testing the Kernel Driver

### Step 1: Load the Driver Module

```bash
cd driver
sudo insmod tempsensor.ko
```

### Step 2: Verify Kernel Log and Device Node

Check kernel message buffer:
```bash
dmesg | tail -5
```
*Expected output:* `tempsensor: module loaded, /dev/tempsensor ready (major=XXX minor=0)`

Verify character device node existence and permissions:
```bash
ls -l /dev/tempsensor
```
*Expected output:* `crw-rw---- 1 root root XXX, 0 ... /dev/tempsensor`

### Step 3: Manual Driver Verification (`cat`)

Read from the device manually using `cat`:

```bash
sudo cat /dev/tempsensor
```
*Output example:* `25.3`

Run it multiple times to observe the random walk drift:
```bash
sudo cat /dev/tempsensor
# Output: 25.8
sudo cat /dev/tempsensor
# Output: 26.1
```

---

## 4. Running the C++ Temperature Monitor

Navigate to the `app` directory (or run from root):

### 1. Help & Usage
```bash
./app/temp_monitor --help
```

### 2. Standard Monitoring Loop
Run continuous monitoring at a 1-second sampling rate:

```bash
sudo ./app/temp_monitor
```
Output:

![output example](screenshots/example.png)

Press `Ctrl+C` to stop gracefully.

### 3. Reset Sensor to Baseline (25.0 °C)
```bash
sudo ./app/temp_monitor --reset
```
*Output:* `[12:01:00] Sensor reset to baseline temperature (25.0 C).`

---

## 5. Unloading the Driver

To remove the kernel driver module from the running kernel:

```bash
sudo rmmod tempsensor
```

Check `dmesg` to verify cleanup:
```bash
dmesg | tail -3
```
*Expected output:* `tempsensor: module unloaded`

---
