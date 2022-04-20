#ifndef SRSGNB_UNITTESTS_PHY_LOWER_MODULATION_OFDM_MODULATOR_TEST_DATA_H
#define SRSGNB_UNITTESTS_PHY_LOWER_MODULATION_OFDM_MODULATOR_TEST_DATA_H

// This file was generated using the following MATLAB class:
//   + "srsOFDMmodulatorUnittest.m"

#include "srsgnb/adt/complex.h"
#include "srsgnb/phy/lower/modulation/ofdm_modulator.h"
#include "srsgnb/support/file_vector.h"
#include <array>

namespace srsgnb {

struct ofdm_modulator_test_configuration {
  ofdm_modulator_configuration config;
  uint8_t port_idx;
  uint8_t slot_idx;
};

struct test_case_t {
  ofdm_modulator_test_configuration test_config;
  file_vector<resource_grid_writer_spy::expected_entry_t> data;
  file_vector<cf_t> modulated;
};

static const std::vector<test_case_t> ofdm_modulator_test_data = {
    // clang-format off
  {{0, 12, 256, cyclic_prefix::NORMAL, 0.13744, 8, 0},{"ofdm_modulator_test_input0.dat"},{"ofdm_modulator_test_output0.dat"}},
  {{0, 24, 512, cyclic_prefix::NORMAL, 0.87118, 7, 0},{"ofdm_modulator_test_input1.dat"},{"ofdm_modulator_test_output1.dat"}},
  {{0, 48, 1024, cyclic_prefix::NORMAL, 0.024856, 0, 0},{"ofdm_modulator_test_input2.dat"},{"ofdm_modulator_test_output2.dat"}},
  {{0, 96, 2048, cyclic_prefix::NORMAL, 0.9016, 8, 0},{"ofdm_modulator_test_input3.dat"},{"ofdm_modulator_test_output3.dat"}},
  {{0, 192, 4096, cyclic_prefix::NORMAL, -0.38665, 7, 0},{"ofdm_modulator_test_input4.dat"},{"ofdm_modulator_test_output4.dat"}},
  {{1, 12, 256, cyclic_prefix::NORMAL, -0.79867, 4, 1},{"ofdm_modulator_test_input5.dat"},{"ofdm_modulator_test_output5.dat"}},
  {{1, 24, 512, cyclic_prefix::NORMAL, 0.86309, 8, 1},{"ofdm_modulator_test_input6.dat"},{"ofdm_modulator_test_output6.dat"}},
  {{1, 48, 1024, cyclic_prefix::NORMAL, -0.32175, 6, 1},{"ofdm_modulator_test_input7.dat"},{"ofdm_modulator_test_output7.dat"}},
  {{1, 96, 2048, cyclic_prefix::NORMAL, -0.65846, 15, 0},{"ofdm_modulator_test_input8.dat"},{"ofdm_modulator_test_output8.dat"}},
  {{1, 192, 4096, cyclic_prefix::NORMAL, -0.25704, 13, 0},{"ofdm_modulator_test_input9.dat"},{"ofdm_modulator_test_output9.dat"}},
  {{2, 12, 256, cyclic_prefix::NORMAL, 0.28391, 7, 3},{"ofdm_modulator_test_input10.dat"},{"ofdm_modulator_test_output10.dat"}},
  {{2, 12, 256, cyclic_prefix::EXTENDED, 0.24097, 3, 1},{"ofdm_modulator_test_input11.dat"},{"ofdm_modulator_test_output11.dat"}},
  {{2, 24, 512, cyclic_prefix::NORMAL, 0.35683, 8, 1},{"ofdm_modulator_test_input12.dat"},{"ofdm_modulator_test_output12.dat"}},
  {{2, 24, 512, cyclic_prefix::EXTENDED, 0.79231, 11, 3},{"ofdm_modulator_test_input13.dat"},{"ofdm_modulator_test_output13.dat"}},
  {{2, 48, 1024, cyclic_prefix::NORMAL, -0.92496, 11, 1},{"ofdm_modulator_test_input14.dat"},{"ofdm_modulator_test_output14.dat"}},
  {{2, 48, 1024, cyclic_prefix::EXTENDED, 0.19785, 7, 3},{"ofdm_modulator_test_input15.dat"},{"ofdm_modulator_test_output15.dat"}},
  {{2, 96, 2048, cyclic_prefix::NORMAL, 0.81835, 12, 2},{"ofdm_modulator_test_input16.dat"},{"ofdm_modulator_test_output16.dat"}},
  {{2, 96, 2048, cyclic_prefix::EXTENDED, -0.75082, 0, 0},{"ofdm_modulator_test_input17.dat"},{"ofdm_modulator_test_output17.dat"}},
  {{2, 192, 4096, cyclic_prefix::NORMAL, 0.41209, 6, 3},{"ofdm_modulator_test_input18.dat"},{"ofdm_modulator_test_output18.dat"}},
  {{2, 192, 4096, cyclic_prefix::EXTENDED, 0.3732, 11, 2},{"ofdm_modulator_test_input19.dat"},{"ofdm_modulator_test_output19.dat"}},
  {{3, 12, 256, cyclic_prefix::NORMAL, 0.31564, 5, 3},{"ofdm_modulator_test_input20.dat"},{"ofdm_modulator_test_output20.dat"}},
  {{3, 24, 512, cyclic_prefix::NORMAL, 0.88687, 4, 5},{"ofdm_modulator_test_input21.dat"},{"ofdm_modulator_test_output21.dat"}},
  {{3, 48, 1024, cyclic_prefix::NORMAL, -0.82864, 3, 4},{"ofdm_modulator_test_input22.dat"},{"ofdm_modulator_test_output22.dat"}},
  {{3, 96, 2048, cyclic_prefix::NORMAL, 0.14767, 5, 2},{"ofdm_modulator_test_input23.dat"},{"ofdm_modulator_test_output23.dat"}},
  {{3, 192, 4096, cyclic_prefix::NORMAL, -0.73715, 12, 7},{"ofdm_modulator_test_input24.dat"},{"ofdm_modulator_test_output24.dat"}},
  {{4, 12, 256, cyclic_prefix::NORMAL, -0.8777, 7, 2},{"ofdm_modulator_test_input25.dat"},{"ofdm_modulator_test_output25.dat"}},
  {{4, 24, 512, cyclic_prefix::NORMAL, -0.50074, 4, 8},{"ofdm_modulator_test_input26.dat"},{"ofdm_modulator_test_output26.dat"}},
  {{4, 48, 1024, cyclic_prefix::NORMAL, -0.067956, 6, 10},{"ofdm_modulator_test_input27.dat"},{"ofdm_modulator_test_output27.dat"}},
  {{4, 96, 2048, cyclic_prefix::NORMAL, 0.94239, 14, 6},{"ofdm_modulator_test_input28.dat"},{"ofdm_modulator_test_output28.dat"}},
  {{4, 192, 4096, cyclic_prefix::NORMAL, -0.14552, 6, 8},{"ofdm_modulator_test_input29.dat"},{"ofdm_modulator_test_output29.dat"}},
    // clang-format on
};

} // namespace srsgnb

#endif // SRSGNB_UNITTESTS_PHY_LOWER_MODULATION_OFDM_MODULATOR_TEST_DATA_H