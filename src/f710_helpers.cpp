
// \author: Blaise Gassend
#include "f710.h"
#include <memory>
#include <cstring>
#include <string>
#include <cinttypes>

#include <dirent.h>
#include <fcntl.h>
#include <climits>
#include <vector>
#include <optional>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <cmath>
#include <sys/stat.h>
#include <unistd.h>

namespace f710 {

/*! \brief Returns the device path of the first joystick that matches joy_name.
 *  If no match is found, an empty string is returned.
 */
    std::optional<std::string> get_dev_by_joy_name() {
        const char path[] = "/dev/input";  // no trailing / here
        struct dirent *entry;
        struct stat stat_buf;

        DIR *dev_dir = opendir(path);
        if (dev_dir == nullptr) {
            printf("Couldn't open %s. Error %i: %s.", path, errno, strerror(errno));
            return "";
        }

        while ((entry = readdir(dev_dir)) != nullptr) {
            // filter entries
            if (strncmp(entry->d_name, "js", 2) != 0)  // skip device if it's not a joystick
            {
                continue;
            }
            std::string current_path = std::string(path) + "/" + entry->d_name;
            if (stat(current_path.c_str(), &stat_buf) == -1) {
                continue;
            }
            if (!S_ISCHR(stat_buf.st_mode))  // input devices are character devices, skip other
            {
                continue;
            }

            // get joystick name
            int joy_fd = open(current_path.c_str(), O_RDONLY);
            if (joy_fd == -1) {
                continue;
            }

            char current_joy_name[128];
            if (ioctl(joy_fd, JSIOCGNAME(sizeof(current_joy_name)), current_joy_name) < 0) {
                strncpy(current_joy_name, "Unknown", sizeof(current_joy_name));
            }

            close(joy_fd);

            printf("Found joystick: %s (%s).\n", current_joy_name, current_path.c_str());
            closedir(dev_dir);
            return current_path;
        }
        closedir(dev_dir);
        return {};
    }
    int open_fd_non_blocking(std::string device_path)
    {
        int f710_fd;
        bool first_fault = true;
        while (true) {
            f710_fd = open(device_path.c_str(), O_RDONLY);
            if (f710_fd != -1) {
                // There seems to be a bug in the driver or something where the
                // initial events that are to define the initial state of the
                // joystick are not the values of the joystick when it was opened
                // but rather the values of the joystick when it was last closed.
                // Opening then closing and opening again is a hack to get more
                // accurate initial state data.
                close(f710_fd);
                f710_fd = open(device_path.c_str(), O_RDONLY);
            }
            if (f710_fd != -1) {
                // make the f710_fd non-blocking
                int status = fcntl(f710_fd, F_SETFL, fcntl(f710_fd, F_GETFL, 0) | O_NONBLOCK);
                if (status == -1){
                    perror("calling fcntl");
                    return -1;
                }
                break;
            }
            if (first_fault) {
                printf("Couldn't open joystick %s. Will retry every second.", device_path.c_str());
                first_fault = false;
            }
            sleep(1.0);
        }
        // here if we got a device - file descriptor is in joy_fd

        char current_joy_name[128];
        if (ioctl(f710_fd, JSIOCGNAME(sizeof(current_joy_name)), current_joy_name) < 0) {
            strncpy(current_joy_name, "Unknown", sizeof(current_joy_name));
        }

        printf("Opened joystick: %s (%s). ", device_path.c_str(), device_path.c_str());
        return f710_fd;
    }


} // namespace f710