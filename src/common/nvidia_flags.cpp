// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <filesystem>
#include <stdlib.h>

#include <fmt/format.h>

#include "common/file_util.h"
#include "common/nvidia_flags.h"

namespace Common {

void ConfigureNvidiaEnvironmentFlags() {
#ifdef _WIN32
    const std::string shader_path = Common::FS::SanitizePath(
        fmt::format("{}\\nvidia", Common::FS::GetUserPath(Common::FS::UserPath::ShaderDir)),
        Common::FS::DirectorySeparator::PlatformDefault);
    std::filesystem::create_directories(shader_path);
    void(_putenv(fmt::format("__GL_SHADER_DISK_CACHE_PATH={}", shader_path).c_str()));
    void(_putenv("__GL_SHADER_DISK_CACHE_SKIP_CLEANUP=1"));
#endif
}

} // namespace Common
