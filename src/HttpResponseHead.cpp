#include "../include/HttpResponseHead.h"

#include <cstring>

my::HttpResponseHead::HttpResponseHead(const char *http_data, int size)
{
    this->set_all(http_data, size);
}
void my::HttpResponseHead::set_all(const char *http_data, int size)
{
    const char *line_start = http_data;
    const char *line_end;

    line_end = strchr(line_start, '\n');
    ::std::string_view line(line_start, line_end - line_start - 1);

    ::std::string_view version = line.substr(0, ::std::min(line.find(' '), line.find('\r')));
    this->version = ::std::string(version);
    line.remove_prefix(version.size() + 1);

    ::std::string_view status = line.substr(0, ::std::min(line.find(' '), line.find('\r')));
    this->status = ::std::string(status);
    line.remove_prefix(status.size() + 1);

    this->message = ::std::string(line);

    line_start = line_end + 1;
    while (!isspace(*line_start)) {
        line_end = strchr(line_start, '\n');
        ::std::string_view line(line_start, line_end - line_start - 1);
        if (line.empty()) {
            break;
        }

        ::std::string_view key = line.substr(0, line.find(':'));
        line.remove_prefix(key.size() + 2);
        ::std::string_view value = line;
        this->headers[::std::string(key)] = ::std::string(value);

        line_start = line_end + 1;
    }
}