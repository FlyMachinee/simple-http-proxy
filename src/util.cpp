#include "../include/util.h"
#include "../include/format_log.hpp"

#include <cstring>

void ::my::out_http_data(int indent, const char *data, int size)
{
    const char *line_start = data;
    const char *line_end;

    while (!isspace(*line_start)) {
        line_end = strchr(line_start, '\n');
        out(indent, "{}", ::std::string_view(line_start, line_end - line_start - 1));
        line_start = line_end + 1;
    }

    line_start += 2;
    if (isspace(*line_start)) {
        return;
    } else {
        out("");
        out(indent, "...(total {} bytes)", size);
    }
}