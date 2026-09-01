#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../driver/tempsensor_ioctl.h"

enum class State { NORMAL, WARNING, CRITICAL };

static volatile std::sig_atomic_t g_running = 1;

static void handle_signal(int sig){
    (void)sig;
    g_running = 0;
}

static const char *state_name(State s){
    switch (s) {
        case State::NORMAL: return "NORMAL";
        case State::WARNING: return "WARNING";
        case State::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

static State classify(double temp_c){
    if (temp_c > 80.0) return State::CRITICAL;
    if (temp_c >= 60.0) return State::WARNING;
    return State::NORMAL;
}

static std::string timestamp(){
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

static void print_usage(const char *prog_name){
    printf("ldd-tsensor\n");
    printf("Usage:\n");
    printf("  sudo %s Run continuous monitoring loop\n", prog_name);
    printf("  sudo %s --reset Reset simulated temperature to baseline (25.0 C)\n", prog_name);
    printf("  sudo %s --drift <N> Set drift to N tenths of a degree (0-500) & run loop\n", prog_name);
    printf("  sudo %s --help Display this help message\n\n", prog_name);
    printf("Examples:\n");
    printf("  sudo %s --drift 40 Fast drift (up to 4.0 C/sec)\n", prog_name);
}

static double read_temperature(int fd){
    char buf[16] = {0};

    // Seek back to start of character device output
    if (lseek(fd, 0, SEEK_SET) < 0) {
        // If lseek is not supported, attempt open-read-close cycle
        int tmp_fd = open("/dev/tempsensor", O_RDONLY);
        if (tmp_fd < 0) return -1.0;
        ssize_t n = read(tmp_fd, buf, sizeof(buf) - 1);
        close(tmp_fd);
        if (n <= 0) return -1.0;
        buf[n] = '\0';
        return std::atof(buf);
    }

    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        // If offset was at EOF, attempt open-read-close cycle
        int tmp_fd = open("/dev/tempsensor", O_RDONLY);
        if (tmp_fd < 0) return -1.0;
        n = read(tmp_fd, buf, sizeof(buf) - 1);
        close(tmp_fd);
        if (n <= 0) return -1.0;
    }
    buf[n] = '\0';
    return std::atof(buf);
}

int main(int argc, char *argv[]){
    const char *dev_path = "/dev/tempsensor";

    // Handle --help option early before opening device
    if (argc >= 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        return 0;
    }

    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        perror("Error: unable to open /dev/tempsensor (is the kernel module loaded? run as root?)");
        return 1;
    }

    // Reset handler
    if (argc == 2 && std::strcmp(argv[1], "--reset") == 0) {
        if (ioctl(fd, TEMP_IOC_RESET) < 0) {
            perror("ioctl RESET failed");
            close(fd);
            return 1;
        }
        printf("[%s] Sensor reset to baseline temperature (25.0 C).\n", timestamp().c_str());
        close(fd);
        return 0;
    }

    // Drift handler
    if (argc >= 2 && std::strcmp(argv[1], "--drift") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: --drift requires an integer argument in tenths of a degree (0-500).\n");
            print_usage(argv[0]);
            close(fd);
            return 1;
        }
        if (argc > 3) {
            fprintf(stderr, "Error: Too many arguments provided for --drift command.\n");
            print_usage(argv[0]);
            close(fd);
            return 1;
        }
        int drift = std::atoi(argv[2]);
        if (drift < 0 || drift > 500) {
            fprintf(stderr, "Error: drift value out of range [0, 500] (0.0 C to 50.0 C per read).\n");
            close(fd);
            return 1;
        }
        if (ioctl(fd, TEMP_IOC_SET_DRIFT, &drift) < 0) {
            perror("ioctl SET_DRIFT failed");
            close(fd);
            return 1;
        }
        printf("[%s] Sensor drift set to %.1f C per reading.\n",
               timestamp().c_str(), drift / 10.0);
    } else if (argc > 1) {
        fprintf(stderr, "Error: Unknown argument '%s'\n", argv[1]);
        print_usage(argv[0]);
        close(fd);
        return 1;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    printf("[%s] Smart Temperature Monitor started. Press Ctrl+C to stop.\n",
           timestamp().c_str());

    // initial temperature reading
    double initial_temp = read_temperature(fd);
    State current_state = (initial_temp >= 0) ? classify(initial_temp) : State::NORMAL;

    if (initial_temp >= 0) {
        printf("[%s] Initial Temperature: %.1f C | Initial State: %s\n",
               timestamp().c_str(), initial_temp, state_name(current_state));
    } else {
        printf("[%s] Warning: Could not obtain initial reading. Defaulting state to %s\n",
               timestamp().c_str(), state_name(current_state));
    }

    // Main monitoring loop 
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!g_running) break;

        double temp = read_temperature(fd);
        if (temp < 0) {
            fprintf(stderr, "[%s] Error: Failed to read sensor value, retrying...\n",
                    timestamp().c_str());
        } else {
            State new_state = classify(temp);

            if (new_state != current_state) {
                printf("[%s] *** TRANSITION: %s -> %s ***  (temp = %.1f C)\n",
                       timestamp().c_str(),
                       state_name(current_state), state_name(new_state),
                       temp);
                current_state = new_state;
            } else {
                printf("[%s] temp = %.1f C  (state: %s)\n",
                       timestamp().c_str(), temp, state_name(current_state));
            }
        }
    }

    printf("\n[%s] Stopping monitor... Cleaning up and closing device.\n", timestamp().c_str());
    close(fd);
    return 0;
}
