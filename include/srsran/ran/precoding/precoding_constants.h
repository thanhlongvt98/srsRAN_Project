/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once
#include "srsran/ran/resource_block.h"
#include "srsran/support/math_utils.h"

namespace srsran {

namespace precoder_constants {

/// Minimum precoding PRG size.
static constexpr unsigned MIN_PRG_SIZE = 4;

/// Maximum number of PRG.
static constexpr unsigned MAX_NOF_PRG = divide_ceil(MAX_NOF_PRBS, MIN_PRG_SIZE);

/// Maximum number of layers supported by the precoder.
static constexpr unsigned MAX_NOF_LAYERS = 8;

/// Maximum number of ports supported by the precoder.
static constexpr unsigned MAX_NOF_PORTS = 16;

} // namespace precoder_constants

} // namespace srsran