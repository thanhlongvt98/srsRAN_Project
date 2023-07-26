/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/e2/common/e2ap_asn1_packer.h"
#include "lib/e2/common/e2ap_asn1_utils.h"
#include "tests/unittests/e2/common/e2_test_helpers.h"
#include "srsran/support/async/async_test_utils.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>

using namespace srsran;

/// Test the initial e2ap setup procedure with own task worker
TEST_F(e2_external_test, on_start_send_e2ap_setup_request)
{
  test_logger.info("Launch e2 setup request procedure with task worker...");
  e2->start();

  // Status: received E2 Setup Request.
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.type().value, asn1::e2ap::e2_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.init_msg().value.type().value,
            asn1::e2ap::e2_ap_elem_procs_o::init_msg_c::types_opts::e2setup_request);

  // Action 2: E2 setup response received.
  unsigned   transaction_id    = get_transaction_id(msg_notifier->last_e2_msg.pdu).value();
  e2_message e2_setup_response = generate_e2_setup_response(transaction_id);
  test_logger.info("Injecting E2SetupResponse");
  e2->handle_message(e2_setup_response);
}

/// Test successful cu-cp initiated e2 setup procedure
TEST_F(e2_test, when_e2_setup_response_received_then_e2_connected)
{
  // Action 1: Launch E2 setup procedure
  e2_message request_msg = generate_e2_setup_request_message();
  test_logger.info("Launch e2 setup request procedure...");
  e2_setup_request_message request;
  request.request                                 = request_msg.pdu.init_msg().value.e2setup_request();
  async_task<e2_setup_response_message>         t = e2->handle_e2_setup_request(request);
  lazy_task_launcher<e2_setup_response_message> t_launcher(t);

  // Status: received E2 Setup Request.
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.type().value, asn1::e2ap::e2_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.init_msg().value.type().value,
            asn1::e2ap::e2_ap_elem_procs_o::init_msg_c::types_opts::e2setup_request);

  // Status: Procedure not yet ready.
  ASSERT_FALSE(t.ready());
  // Action 2: E2 setup response received.
  unsigned   transaction_id    = get_transaction_id(msg_notifier->last_e2_msg.pdu).value();
  e2_message e2_setup_response = generate_e2_setup_response(transaction_id);
  test_logger.info("Injecting E2SetupResponse");
  e2->handle_message(e2_setup_response);

  ASSERT_TRUE(t.ready());
  ASSERT_TRUE(t.get().success);
}

TEST_F(e2_test, when_e2_setup_failure_received_then_e2_setup_failed)
{
  // Action 1: Launch E2 setup procedure
  e2_message request_msg = generate_e2_setup_request_message();
  test_logger.info("Launch e2 setup request procedure...");
  e2_setup_request_message request;
  request.request                                 = request_msg.pdu.init_msg().value.e2setup_request();
  async_task<e2_setup_response_message>         t = e2->handle_e2_setup_request(request);
  lazy_task_launcher<e2_setup_response_message> t_launcher(t);

  // Status: received E2 Setup Request.
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.type().value, asn1::e2ap::e2_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.init_msg().value.type().value,
            asn1::e2ap::e2_ap_elem_procs_o::init_msg_c::types_opts::e2setup_request);

  // Status: Procedure not yet ready.
  ASSERT_FALSE(t.ready());
  // Action 2: E2 setup response received.
  unsigned   transaction_id    = get_transaction_id(msg_notifier->last_e2_msg.pdu).value();
  e2_message e2_setup_response = generate_e2_setup_failure(transaction_id);
  test_logger.info("Injecting E2SetupFailure");
  e2->handle_message(e2_setup_response);

  ASSERT_TRUE(t.ready());
  ASSERT_FALSE(t.get().success);
}

TEST_F(e2_test_setup, e2_sends_correct_ran_function_definition)
{
  using namespace asn1::e2sm_kpm;
  using namespace asn1::e2ap;
  e2_message request_msg = generate_e2_setup_request_message();
  test_logger.info("Launch e2 setup request procedure...");
  e2_setup_request_message request;
  request.request                                 = request_msg.pdu.init_msg().value.e2setup_request();
  async_task<e2_setup_response_message>         t = e2->handle_e2_setup_request(request);
  lazy_task_launcher<e2_setup_response_message> t_launcher(t);

  // Status: received E2 Setup Request.
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.type().value, asn1::e2ap::e2_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.init_msg().value.type().value,
            asn1::e2ap::e2_ap_elem_procs_o::init_msg_c::types_opts::e2setup_request);

  ra_nfunction_item_s& ran_func_added1 = msg_notifier->last_e2_msg.pdu.init_msg()
                                             .value.e2setup_request()
                                             ->ra_nfunctions_added.value[0]
                                             .value()
                                             .ra_nfunction_item();
  asn1::cbit_ref                                       bref1(ran_func_added1.ran_function_definition);
  asn1::e2sm_kpm::e2_sm_kpm_ra_nfunction_description_s ran_func_def = {};
  if (ran_func_def.unpack(bref1) != asn1::SRSASN_SUCCESS) {
    printf("Couldn't unpack E2 PDU");
  }
  // check contents of E2SM-KPM-RANfunction-Description
  ASSERT_EQ(ran_func_def.ran_function_name.ran_function_short_name.to_string(), "ORAN-E2SM-KPM");
  ric_report_style_item_s& ric_report_style = ran_func_def.ric_report_style_list[0];
  ASSERT_EQ(ric_report_style.ric_report_style_type, 3);
  meas_info_action_item_s& meas_cond_it = ric_report_style.meas_info_action_list[0];
  ASSERT_EQ(meas_cond_it.meas_name.to_string(), "CQI");

  // Status: Procedure not yet ready.
  ASSERT_FALSE(t.ready());
  // Action 2: E2 setup response received.
  unsigned   transaction_id    = get_transaction_id(msg_notifier->last_e2_msg.pdu).value();
  e2_message e2_setup_response = generate_e2_setup_response(transaction_id);
  test_logger.info("Injecting E2SetupResponse");
  e2->handle_message(e2_setup_response);

  ASSERT_TRUE(t.ready());
  ASSERT_TRUE(t.get().success);
}

TEST_F(e2_external_test, correctly_unpack_e2_response)
{
  test_logger.info("Launch e2 setup request procedure with task worker...");
  e2->start();

  // Status: received E2 Setup Request.
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.type().value, asn1::e2ap::e2_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(msg_notifier->last_e2_msg.pdu.init_msg().value.type().value,
            asn1::e2ap::e2_ap_elem_procs_o::init_msg_c::types_opts::e2setup_request);

  uint8_t     e2_resp[] = {0x20, 0x01, 0x00, 0x38, 0x00, 0x00, 0x04, 0x00, 0x31, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04,
                           0x00, 0x07, 0x00, 0x00, 0xf1, 0x10, 0x00, 0x01, 0x90, 0x00, 0x09, 0x00, 0x0a, 0x00, 0x00,
                           0x06, 0x40, 0x05, 0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x34, 0x00, 0x12, 0x00, 0x00, 0x00,
                           0x35, 0x00, 0x0c, 0x00, 0x00, 0xe0, 0x6e, 0x67, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x66, 0x00};
  byte_buffer e2_resp_buf(e2_resp, e2_resp + sizeof(e2_resp));
  packer->handle_packed_pdu(std::move(e2_resp_buf));
}