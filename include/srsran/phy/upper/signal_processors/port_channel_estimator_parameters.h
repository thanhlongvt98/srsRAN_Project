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

namespace srsran {

/// Port channel estimator frequency domain smoothing strategy.
enum class port_channel_estimator_fd_smoothing_strategy {
  /// No smoothing strategy.
  none = 0,
  /// Averages all frequency domain estimates.
  mean,
  /// Filters in the frequency domain with a low pass filter.
  filter,
};

} // namespace srsran
