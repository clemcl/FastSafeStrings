#include <iostream>
#include "fss_json.hpp"

int main() {
    std::string_view doc = R"({
        "sender": "clem",
        "command": "PROCESS_VB",
        "record_count": 5000000
    })";

    if (auto cmd = fss::json_find(doc, "command")) {
        // Natural C++ string_view comparison (no strlen, no copies)
        if (*cmd == "PROCESS_VB") {
            std::cout << "Command verified: " << *cmd << '\n';
        }
    }

    if (auto count = fss::json_find(doc, "record_count")) {
        std::cout << "Records to process: " << *count << '\n';
    }

    return 0;
}