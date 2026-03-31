/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "lib/e2/e2sm/e2sm_kpm/e2sm_kpm_du_meas_provider_impl.h"
#include "tests/unittests/e2/common/e2_test_helpers.h"
#include "srsran/ran/du_types.h"
#include <gtest/gtest.h>
#include <array>

using namespace srsran;
using namespace asn1::e2sm;

static span<const e2sm_kpm_metric_t> e2sm_kpm_28_552_metrics = get_e2sm_kpm_28_552_metrics();
static span<const e2sm_kpm_metric_t> e2sm_kpm_oran_metrics   = get_e2sm_kpm_oran_metrics();

bool get_metric_definition(std::string metric_name, e2sm_kpm_metric_t& e2sm_kpm_metric_def)
{
  auto name_matches = [&metric_name](const e2sm_kpm_metric_t& x) {
    return (x.name == metric_name.c_str() or x.name == metric_name);
  };

  const auto* it = std::find_if(e2sm_kpm_28_552_metrics.begin(), e2sm_kpm_28_552_metrics.end(), name_matches);
  if (it != e2sm_kpm_28_552_metrics.end()) {
    e2sm_kpm_metric_def = *it;
    return true;
  }

  it = std::find_if(e2sm_kpm_oran_metrics.begin(), e2sm_kpm_oran_metrics.end(), name_matches);
  if (it != e2sm_kpm_oran_metrics.end()) {
    e2sm_kpm_metric_def = *it;
    return true;
  }

  return false;
}

static rlc_metrics generate_non_zero_rlc_metrics(uint32_t ue_idx, uint32_t bearer_id)
{
  rlc_metrics rlc_metric;
  rlc_metric.metrics_period        = std::chrono::milliseconds(1000);
  rlc_metric.ue_index              = static_cast<du_ue_index_t>(ue_idx);
  rlc_metric.rb_id                 = rb_id_t(drb_id_t(bearer_id));
  rlc_metric.rx.mode               = rlc_mode::am;
  rlc_metric.rx.num_pdus           = 5;
  rlc_metric.rx.num_pdu_bytes      = rlc_metric.rx.num_pdus * 1000;
  rlc_metric.rx.num_sdus           = 5;
  rlc_metric.rx.num_sdu_bytes      = rlc_metric.rx.num_sdus * 1000;
  rlc_metric.rx.num_lost_pdus      = 1;
  rlc_metric.rx.num_malformed_pdus = 1;
  rlc_metric.rx.sdu_latency_us     = 1000;

  rlc_metric.tx.tx_high.num_sdus                     = 10;
  rlc_metric.tx.tx_high.num_sdu_bytes                = rlc_metric.tx.tx_high.num_sdus * 1000;
  rlc_metric.tx.tx_high.num_dropped_sdus             = 1;
  rlc_metric.tx.tx_high.num_discarded_sdus           = 1;
  rlc_metric.tx.tx_high.num_discard_failures         = 1;
  rlc_metric.tx.tx_low.sum_sdu_latency_us            = 1000;
  rlc_metric.tx.tx_low.num_of_pulled_sdus            = 1;
  rlc_metric.tx.tx_low.num_pdus_no_segmentation      = 10;
  rlc_metric.tx.tx_low.num_pdu_bytes_no_segmentation = rlc_metric.tx.tx_low.num_pdus_no_segmentation * 1000;

  rlc_metric.tx.tx_low.mode_specific = rlc_am_tx_metrics_lower{};
  auto& am                           = std::get<rlc_am_tx_metrics_lower>(rlc_metric.tx.tx_low.mode_specific);
  am.num_pdus_with_segmentation      = 2;
  am.num_pdu_bytes_with_segmentation = am.num_pdus_with_segmentation * 1000;

  return rlc_metric;
}

static scheduler_cell_metrics generate_non_zero_sched_metrics()
{
  scheduler_cell_metrics sched_metric;
  sched_metric.nof_prbs            = 52;
  sched_metric.nof_dl_slots        = 14;
  sched_metric.nof_ul_slots        = 14;
  sched_metric.nof_prach_preambles = 10;

  scheduler_ue_metrics ue_metrics;
  ue_metrics.ue_index            = to_du_ue_index(0);
  ue_metrics.pci                 = 1;
  ue_metrics.rnti                = static_cast<rnti_t>(0x1000 + 1);
  ue_metrics.tot_pdsch_prbs_used = 1200;
  ue_metrics.tot_pusch_prbs_used = 1200;
  ue_metrics.dl_brate_kbps       = 2000.5;
  ue_metrics.dl_nof_ok           = 12;
  ue_metrics.dl_nof_nok          = 2;
  ue_metrics.dl_bs               = 3456;
  ue_metrics.pusch_rsrp_db       = -10.5;
  ue_metrics.pusch_snr_db        = 10.25;
  ue_metrics.pucch_snr_db        = 8.75;
  ue_metrics.dl_mcs              = sch_mcs_index{17};
  ue_metrics.ul_mcs              = sch_mcs_index{13};
  ue_metrics.ul_brate_kbps       = 1500.75;
  ue_metrics.ul_nof_ok           = 11;
  ue_metrics.ul_nof_nok          = 3;
  ue_metrics.bsr                 = 7890;
  ue_metrics.last_phr            = 42;
  ue_metrics.max_pusch_distance_ms = 7;
  ue_metrics.max_pdsch_distance_ms = 9;
  ue_metrics.nof_pucch_f0f1_invalid_harqs   = 1;
  ue_metrics.nof_pucch_f2f3f4_invalid_harqs = 2;
  ue_metrics.nof_pucch_f2f3f4_invalid_csis  = 3;
  ue_metrics.nof_pusch_invalid_harqs        = 4;
  ue_metrics.nof_pusch_invalid_csis         = 5;
  ue_metrics.avg_ce_delay_ms                = 3.5F;
  ue_metrics.max_ce_delay_ms                = 4.5F;
  ue_metrics.avg_crc_delay_ms               = 5.5F;
  ue_metrics.max_crc_delay_ms               = 6.5F;
  ue_metrics.avg_pusch_harq_delay_ms        = 7.5F;
  ue_metrics.max_pusch_harq_delay_ms        = 8.5F;
  ue_metrics.avg_pucch_harq_delay_ms        = 9.5F;
  ue_metrics.max_pucch_harq_delay_ms        = 10.5F;
  ue_metrics.avg_sr_to_pusch_delay_ms       = 11.5F;
  ue_metrics.max_sr_to_pusch_delay_ms       = 12.5F;
  ue_metrics.ta_stats.update(1.2e-6F);
  ue_metrics.pusch_ta_stats.update(1.4e-6F);
  ue_metrics.pucch_ta_stats.update(1.6e-6F);
  ue_metrics.srs_ta_stats.update(1.8e-6F);
  for (auto i = 0; i < 10; i++) {
    ue_metrics.cqi_stats.update(9);
    ue_metrics.dl_ri_stats.update(2);
    ue_metrics.ul_ri_stats.update(1);
  }
  sched_metric.ue_metrics.push_back(ue_metrics);

  return sched_metric;
}

class dummy_e2_du_metrics_notifier : public e2_du_metrics_notifier, public e2_du_metrics_interface
{
public:
  void report_metrics(const scheduler_cell_metrics& metrics) override
  {
    if (e2_meas_provider) {
      e2_meas_provider->report_metrics(metrics);
    }
  }

  void report_metrics(const rlc_metrics& metrics) override
  {
    if (e2_meas_provider) {
      e2_meas_provider->report_metrics(metrics);
    }
  }

  void connect_e2_du_meas_provider(std::unique_ptr<e2_du_metrics_notifier> meas_provider) override {}

  void connect_e2_du_meas_provider(e2_du_metrics_notifier* meas_provider) { e2_meas_provider = meas_provider; }

private:
  e2_du_metrics_notifier* e2_meas_provider;
};

class e2sm_kpm_meas_provider_metrics_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    srslog::fetch_basic_logger("TEST").set_level(srslog::basic_levels::debug);
    srslog::init();
    f1ap_ue_id_mapper = std::make_unique<dummy_f1ap_ue_id_translator>();
    du_meas_provider  = std::make_unique<e2sm_kpm_du_meas_provider_impl>(*f1ap_ue_id_mapper);
    metrics           = std::make_unique<dummy_e2_du_metrics_notifier>();
    metrics->connect_e2_du_meas_provider(du_meas_provider.get());
  }

  void TearDown() override
  {
    // Flush logger after each test.
    srslog::flush();
  }

  std::unique_ptr<dummy_e2_du_metrics_notifier>   metrics;
  std::unique_ptr<dummy_f1ap_ue_id_translator>    f1ap_ue_id_mapper;
  std::unique_ptr<e2sm_kpm_du_meas_provider_impl> du_meas_provider;
  srslog::basic_logger&                           test_logger = srslog::fetch_basic_logger("TEST");
};

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_supported_metrics_are_supported)
{
  e2sm_kpm_metric_level_enum metric_level;
  meas_type_c                meas_type;
  meas_label_s               meas_label;
  meas_label.no_label_present         = true;
  meas_label.no_label                 = meas_label_s::no_label_e_::true_value;
  bool                     cell_scope = false;
  std::vector<std::string> supported_metrics;
  bool                     metric_supported = false;

  // E2-NODE-LEVEL metrics
  metric_level      = E2_NODE_LEVEL;
  supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);
    metric_supported = du_meas_provider->is_metric_supported(meas_type, meas_label, metric_level, cell_scope);
    ASSERT_TRUE(metric_supported) << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
                                  << " returned as supported but not supported ";
  }

  // UE-LEVEL metrics
  metric_level      = UE_LEVEL;
  supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);
    metric_supported = du_meas_provider->is_metric_supported(meas_type, meas_label, metric_level, cell_scope);
    ASSERT_TRUE(metric_supported) << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
                                  << " returned as supported but not supported ";
  }
}

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_return_e2_level_metric_with_no_measurements)
{
  // metrics that return no_value when no measurements are present. Specifically, they should not return 0
  std::vector<std::string> no_value_metrics = {
      "DRB.AirIfDelayUl", "DRB.RlcSduDelayDl", "DRB.RlcDelayUl", "DRB.RlcPacketDropRateDl", "DRB.RlcSduDelayDl"};

  // E2-NODE-LEVEL metrics have to be always returned, even if 0 or NAN
  e2sm_kpm_metric_level_enum       metric_level = E2_NODE_LEVEL;
  meas_type_c                      meas_type;
  std::optional<asn1::e2sm::cgi_c> cell_global_id = {};
  e2sm_kpm_metric_t                e2sm_kpm_metric_definition;

  label_info_list_l label_info_list;
  label_info_item_s label_info_item           = {};
  label_info_item.meas_label.no_label_present = true;
  label_info_item.meas_label.no_label         = meas_label_s::no_label_e_::true_value;
  label_info_list.push_back(label_info_item);
  std::vector<meas_record_item_c> meas_records_items;

  std::vector<std::string> supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);

    meas_records_items.clear();
    du_meas_provider->get_meas_data(meas_type, label_info_list, {}, cell_global_id, meas_records_items);

    ASSERT_TRUE(meas_records_items.size() == 1)
        << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
        << " returned no measurements (size=" << meas_records_items.size() << ")";

    // Check if metric should return no_value when no measurements are present
    bool found = std::any_of(
        no_value_metrics.begin(), no_value_metrics.end(), [&metric](const std::string& s) { return s == metric; });
    if (found) {
      ASSERT_EQ(meas_records_items[0].type(), meas_record_item_c::types::no_value)
          << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
          << " expected to return no_value when no measurements available.";
      continue;
    }
    if (get_metric_definition(metric, e2sm_kpm_metric_definition)) {
      if (e2sm_kpm_metric_definition.data_type == e2sm_kpm_metric_dtype_t::INTEGER) {
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::integer)
            << "Metric: " << e2sm_kpm_metric_definition.name.c_str() << " should return record of the integer type.";
      } else {
        // e2sm_kpm_metric_dtype_t::REAL
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::real)
            << "Metric: " << e2sm_kpm_metric_definition.name.c_str() << " should return record of the real type.";
      }
    }

    switch (meas_records_items[0].type()) {
      case meas_record_item_c::types::integer:
        ASSERT_EQ(meas_records_items[0].integer(), 0)
            << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level);
        break;
      case meas_record_item_c::types::real:
        ASSERT_FLOAT_EQ(meas_records_items[0].real().value, 0.0)
            << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level);
        break;
      default:
        printf("%s type: %i\n", metric.c_str(), meas_records_items[0].type().value);
        ASSERT_TRUE(false) << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
                           << " returned a record with wrong type.";
        break;
    }
  }
}

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_return_e2_level_metric_with_with_measurements)
{
  // E2-NODE-LEVEL metrics have to be always returned, even if 0 or NAN
  e2sm_kpm_metric_level_enum       metric_level = E2_NODE_LEVEL;
  meas_type_c                      meas_type;
  std::optional<asn1::e2sm::cgi_c> cell_global_id = {};
  e2sm_kpm_metric_t                e2sm_kpm_metric_definition;

  label_info_list_l label_info_list;
  label_info_item_s label_info_item           = {};
  label_info_item.meas_label.no_label_present = true;
  label_info_item.meas_label.no_label         = meas_label_s::no_label_e_::true_value;
  label_info_list.push_back(label_info_item);
  std::vector<meas_record_item_c> meas_records_items;

  // Fill e2sm-kpm measurements provider with RLC and SCHED metrics.
  // Generate dummy metrics that will generate non-zero e2sm-kpm metric records.
  rlc_metrics rlc_metric = generate_non_zero_rlc_metrics(0, 1);
  metrics->report_metrics(rlc_metric);
  scheduler_cell_metrics sched_metrics = generate_non_zero_sched_metrics();
  metrics->report_metrics(sched_metrics);

  std::vector<std::string> supported_metrics = du_meas_provider->get_supported_metric_names(metric_level);
  for (auto& metric : supported_metrics) {
    meas_type.set_meas_name().from_string(metric);

    meas_records_items.clear();
    du_meas_provider->get_meas_data(meas_type, label_info_list, {}, cell_global_id, meas_records_items);

    ASSERT_TRUE(meas_records_items.size() == 1)
        << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
        << " returned no measurements (size=" << meas_records_items.size() << ")";

    if (get_metric_definition(metric, e2sm_kpm_metric_definition)) {
      if (e2sm_kpm_metric_definition.data_type == e2sm_kpm_metric_dtype_t::INTEGER) {
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::integer)
            << "Metric: " << e2sm_kpm_metric_definition.name.c_str() << " should return record of the integer type.";
      } else {
        // e2sm_kpm_metric_dtype_t::REAL
        ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::real)
            << "Metric: " << e2sm_kpm_metric_definition.name.c_str() << " should return record of the real type.";
      }
    }

    switch (meas_records_items[0].type()) {
      case meas_record_item_c::types::integer:
        ASSERT_NE(meas_records_items[0].integer(), 0)
            << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level);
        break;
      case meas_record_item_c::types::real:
        ASSERT_NE(meas_records_items[0].real().value, 0.0)
            << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level);
        break;
      default:
        printf("%s type: %i\n", metric.c_str(), meas_records_items[0].type().value);
        ASSERT_TRUE(false) << "Metric: " << metric << " Level: " << e2sm_kpm_scope_2_str(metric_level)
                           << " returned a record with wrong type.";
        break;
    }
  }
}

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_returns_new_ue_scheduler_metrics)
{
  scheduler_cell_metrics sched_metrics = generate_non_zero_sched_metrics();
  metrics->report_metrics(sched_metrics);

  label_info_list_l label_info_list;
  label_info_item_s label_info_item           = {};
  label_info_item.meas_label.no_label_present = true;
  label_info_item.meas_label.no_label         = meas_label_s::no_label_e_::true_value;
  label_info_list.push_back(label_info_item);

  ue_id_c        ue_id;
  ue_id_gnb_du_s ue_id_gnb_du{};
  ue_id_gnb_du.gnb_cu_ue_f1ap_id = 0;
  ue_id_gnb_du.ran_ue_id_present = false;
  ue_id.set_gnb_du_ue_id()       = ue_id_gnb_du;
  std::vector<ue_id_c> ues = {ue_id};

  struct expected_metric_t {
    const char* name;
    bool        is_real;
    double      value;
  };

  const std::array<expected_metric_t, 41> expected_metrics = {{
      {"UE.UE-INDEX", false, 0},
      {"UE.PCI", false, 1},
      {"UE.RNTI", false, 0x1001},
      {"UE.CQI", false, 9},
      {"UE.DL-RI", true, 2.0},
      {"UE.UL-RI", true, 1.0},
      {"UE.DL-MCS", false, 17},
      {"UE.DL-BRATE", true, 2000.5},
      {"UE.DL-NOF-OK", false, 12},
      {"UE.DL-NOF-NOK", false, 2},
      {"UE.DL-BS", false, 3456},
      {"UE.PUSCH-SNR", true, 10.25},
      {"UE.PUSCH-RSRP", true, -10.5},
      {"UE.PUCCH-SNR", true, 8.75},
      {"UE.TA-NS", true, 1200.0},
      {"UE.PUSCH-TA-NS", true, 1400.0},
      {"UE.PUCCH-TA-NS", true, 1600.0},
      {"UE.SRS-TA-NS", true, 1800.0},
      {"UE.UL-MCS", false, 13},
      {"UE.UL-BRATE", true, 1500.75},
      {"UE.UL-NOF-OK", false, 11},
      {"UE.UL-NOF-NOK", false, 3},
      {"UE.LAST-PHR", false, 42},
      {"UE.MAX-PUSCH-DISTANCE", false, 7},
      {"UE.MAX-PDSCH-DISTANCE", false, 9},
      {"UE.BSR", false, 7890},
      {"UE.NOF-PUCCH-F0F1-INVALID-HARQS", false, 1},
      {"UE.NOF-PUCCH-F2F3F4-INVALID-HARQS", false, 2},
      {"UE.NOF-PUCCH-F2F3F4-INVALID-CSIS", false, 3},
      {"UE.NOF-PUSCH-INVALID-HARQS", false, 4},
      {"UE.NOF-PUSCH-INVALID-CSIS", false, 5},
      {"UE.AVG-CE-DELAY", true, 3.5},
      {"UE.MAX-CE-DELAY", true, 4.5},
      {"UE.AVG-CRC-DELAY", true, 5.5},
      {"UE.MAX-CRC-DELAY", true, 6.5},
      {"UE.AVG-PUSCH-HARQ-DELAY", true, 7.5},
      {"UE.MAX-PUSCH-HARQ-DELAY", true, 8.5},
      {"UE.AVG-PUCCH-HARQ-DELAY", true, 9.5},
      {"UE.MAX-PUCCH-HARQ-DELAY", true, 10.5},
      {"UE.AVG-SR-TO-PUSCH-DELAY", true, 11.5},
      {"UE.MAX-SR-TO-PUSCH-DELAY", true, 12.5},
  }};

  for (const auto& expected_metric : expected_metrics) {
    meas_type_c meas_type;
    meas_type.set_meas_name().from_string(expected_metric.name);

    std::vector<meas_record_item_c> meas_records_items;
    ASSERT_TRUE(du_meas_provider->get_meas_data(meas_type, label_info_list, ues, {}, meas_records_items));
    ASSERT_EQ(meas_records_items.size(), 1);

    if (expected_metric.is_real) {
      ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::real);
      ASSERT_FLOAT_EQ(meas_records_items[0].real().value, static_cast<float>(expected_metric.value));
    } else {
      ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::integer);
      ASSERT_EQ(meas_records_items[0].integer(), static_cast<uint64_t>(expected_metric.value));
    }
  }
}

TEST_F(e2sm_kpm_meas_provider_metrics_test, e2sm_kpm_returns_default_ri_when_no_observations_exist)
{
  scheduler_cell_metrics sched_metrics;
  scheduler_ue_metrics   ue_metrics;
  ue_metrics.ue_index = to_du_ue_index(0);
  ue_metrics.pci      = 1;
  ue_metrics.rnti     = static_cast<rnti_t>(0x1001);
  sched_metrics.ue_metrics.push_back(ue_metrics);
  metrics->report_metrics(sched_metrics);

  label_info_list_l label_info_list;
  label_info_item_s label_info_item           = {};
  label_info_item.meas_label.no_label_present = true;
  label_info_item.meas_label.no_label         = meas_label_s::no_label_e_::true_value;
  label_info_list.push_back(label_info_item);

  ue_id_c        ue_id;
  ue_id_gnb_du_s ue_id_gnb_du{};
  ue_id_gnb_du.gnb_cu_ue_f1ap_id = 0;
  ue_id_gnb_du.ran_ue_id_present = false;
  ue_id.set_gnb_du_ue_id()       = ue_id_gnb_du;
  std::vector<ue_id_c> ues = {ue_id};

  for (const char* metric_name : {"UE.DL-RI", "UE.UL-RI"}) {
    meas_type_c meas_type;
    meas_type.set_meas_name().from_string(metric_name);

    std::vector<meas_record_item_c> meas_records_items;
    ASSERT_TRUE(du_meas_provider->get_meas_data(meas_type, label_info_list, ues, {}, meas_records_items));
    ASSERT_EQ(meas_records_items.size(), 1);
    ASSERT_EQ(meas_records_items[0].type().value, meas_record_item_c::types::real);
    ASSERT_FLOAT_EQ(meas_records_items[0].real().value, 1.0F);
  }
}
