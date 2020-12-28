// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <utility>
#include "common/common_types.h"

namespace Common {

// This function multiplies 2 u64 values and divides it by a u64 value.
[[nodiscard]] u64 MultiplyAndDivide64(u64 a, u64 b, u64 d);

} // namespace Common
