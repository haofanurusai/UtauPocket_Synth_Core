#include "platform.h"
#include <asm/termbits.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
namespace
{
    std::runtime_error serial_error(const char *operation)
    {
        return std::runtime_error(std::string(operation) + ": " + std::strerror(errno));
    }
}
struct MidiInput::Impl
{
    int fd = -1;
    termios2 original{};
    bool restore = false;
    size_t pending_replies = 0;
    void flushReplies()
    {
        const char reply = 'A';
        while (pending_replies)
        {
            ssize_t n = ::write(fd, &reply, 1);
            if (n == 1)
                --pending_replies;
            else if (n < 0 && errno == EINTR)
                continue;
            else if (n == 0 || (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)))
                return;
            else
                throw serial_error("MIDI serial reply failed");
        }
    }
    ~Impl()
    {
        if (fd >= 0)
        {
            if (restore)
                ioctl(fd, TCSETS2, &original);
            ::close(fd);
        }
    }
};
MidiInput::MidiInput(const PlayerOptions &options) : impl(new Impl)
{
    impl->fd = ::open(options.serial, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (impl->fd < 0)
        throw serial_error("Cannot open MIDI serial port");
    if (ioctl(impl->fd, TCGETS2, &impl->original) < 0)
        throw serial_error("Cannot read serial settings");
    impl->restore = true;
    termios2 settings = impl->original;
    settings.c_iflag = 0;
    settings.c_oflag = 0;
    settings.c_lflag = 0;
    settings.c_cflag = BOTHER | CS8 | CLOCAL | CREAD;
    settings.c_ispeed = settings.c_ospeed = static_cast<unsigned>(options.baud);
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;
    if (ioctl(impl->fd, TCSETS2, &settings) < 0)
        throw serial_error("Cannot set serial baud/raw 8N1");
    std::fprintf(stderr, "MIDI serial: %s at %d baud\n", options.serial, options.baud);
}
MidiInput::~MidiInput() = default;
void MidiInput::sendTuneReply()
{
    ++impl->pending_replies;
    impl->flushReplies();
}
void MidiInput::list() { throw std::runtime_error("MIDI port enumeration is Windows-only"); }
size_t MidiInput::read(uint8_t *buffer, size_t capacity)
{
    impl->flushReplies();
    pollfd descriptor{impl->fd, POLLIN, 0};
    int ready = poll(&descriptor, 1, 0);
    if (ready < 0)
    {
        if (errno == EINTR)
            return 0;
        throw serial_error("MIDI serial poll failed");
    }
    if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
        throw std::runtime_error("MIDI serial port disconnected");
    if (!ready || !(descriptor.revents & POLLIN))
        return 0;
    ssize_t n = ::read(impl->fd, buffer, capacity);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return 0;
        throw serial_error("MIDI serial read failed");
    }
    return size_t(n);
}
