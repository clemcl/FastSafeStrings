#ifndef FSS_JSON_HPP
#define FSS_JSON_HPP

#include <string_view>
#include <optional>
#include "fss_json.h"

// copyright Clement Victor Clarke 2026
 
namespace fss {

inline std::optional<std::string_view> json_find(std::string_view json, std::string_view key) {
    fss_slice_t slice;
    if (fss_json_find(json.data(), json.size(), key.data(), key.size(), &slice)) {
        return std::string_view(slice.ptr, slice.len);
    }
    return std::nullopt;
}

} // namespace fss

#endif // FSS_JSON_HPP
