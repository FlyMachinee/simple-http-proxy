#include "../include/HttpRequest.h"

#include <cstring>

::my::HttpRequest::HttpRequest(const char *http_data, int size)
{
    const char *line_start = http_data;
    const char *line_end;
    const char *body_start;

    while (!isspace(*line_start)) {
        line_end = strchr(line_start, '\n');
        ::std::string_view line(line_start, line_end - line_start - 1);
        if (line.empty()) {
            break;
        }

        if (this->method.empty()) {
            ::std::string_view method = line.substr(0, line.find(' '));
            this->method = ::std::string(method);
            line.remove_prefix(method.size() + 1);

            ::std::string_view url = line.substr(0, line.find(' '));
            this->url = ::std::string(url);
            line.remove_prefix(url.size() + 1);

            this->version = ::std::string(line);
        } else {
            ::std::string_view key = line.substr(0, line.find(':'));
            line.remove_prefix(key.size() + 2);
            ::std::string_view value = line;
            this->headers[::std::string(key)] = ::std::string(value);
        }

        line_start = line_end + 1;
    }

    body_start = line_start + 2;
    this->body = ::std::string(body_start, size - (body_start - http_data));
}

::std::pair<::std::string, unsigned short> my::HttpRequest::get_host_port() const
{
    ::std::string host;
    unsigned short port = 80;

    auto it = this->headers.find("Host");
    if (it != this->headers.end()) {
        ::std::string_view host_port = it->second;
        auto pos = host_port.find(':');
        if (pos != ::std::string_view::npos) {
            host = host_port.substr(0, pos);
            port = ::std::stoi(::std::string(host_port.substr(pos + 1)));
        } else {
            host = ::std::string(host_port);
        }
    }

    return {host, port};
}
::std::string my::HttpRequest::to_string() const
{
    ::std::string str = this->method + " " + this->url + " " + this->version + "\r\n";
    for (const auto &[key, value] : this->headers) {
        str += key + ": " + value + "\r\n";
    }
    str += "\r\n" + this->body;
    return str;
}