#pragma once

class Config
{
    public:
    Config() = default;
    Config(const Config &other) = default;
    Config &operator=(const Config &other) = default;
    ~Config() = default;

    int max_poll_events() const { return 1024; } // TODO: replace it with a configurable value
    int max_poll_timeout() const { return 1000; } // TODO: replace it with a configurable value
};