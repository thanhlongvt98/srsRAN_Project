/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
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

#include "ngap_pdu_session_resource_release_procedure.h"
#include "../ngap/ngap_asn1_helpers.h"
#include "srsran/asn1/ngap/common.h"
#include "srsran/ngap/ngap_message.h"

using namespace srsran;
using namespace srsran::srs_cu_cp;
using namespace asn1::ngap;

ngap_pdu_session_resource_release_procedure::ngap_pdu_session_resource_release_procedure(
    const cu_cp_pdu_session_resource_release_command& command_,
    const ngap_ue_ids&                                ue_ids_,
    ngap_du_processor_control_notifier&               du_processor_ctrl_notif_,
    ngap_message_notifier&                            amf_notif_,
    ngap_ue_logger&                                   logger_) :
  command(command_),
  ue_ids(ue_ids_),
  du_processor_ctrl_notifier(du_processor_ctrl_notif_),
  amf_notifier(amf_notif_),
  logger(logger_)
{
}

void ngap_pdu_session_resource_release_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.log_debug("\"{}\" initialized", name());

  // Handle mandatory IEs
  CORO_AWAIT_VALUE(response, du_processor_ctrl_notifier.on_new_pdu_session_resource_release_command(command));

  // TODO: Handle optional IEs

  send_pdu_session_resource_release_response();

  logger.log_debug("\"{}\" finalized", name());

  CORO_RETURN();
}

void ngap_pdu_session_resource_release_procedure::send_pdu_session_resource_release_response()
{
  ngap_message ngap_msg = {};

  ngap_msg.pdu.set_successful_outcome();
  ngap_msg.pdu.successful_outcome().load_info_obj(ASN1_NGAP_ID_PDU_SESSION_RES_RELEASE);

  // TODO: needs more handling in the coro above?
  if (!fill_asn1_pdu_session_resource_release_response(
          ngap_msg.pdu.successful_outcome().value.pdu_session_res_release_resp(), response)) {
    logger.log_warning("Cannot fill ASN1 PDU Session Resource Release Response");
    return;
  }

  auto& pdu_session_res_release_resp           = ngap_msg.pdu.successful_outcome().value.pdu_session_res_release_resp();
  pdu_session_res_release_resp->amf_ue_ngap_id = amf_ue_id_to_uint(ue_ids.amf_ue_id);
  pdu_session_res_release_resp->ran_ue_ngap_id = ran_ue_id_to_uint(ue_ids.ran_ue_id);

  logger.log_info("Sending PduSessionResourceReleaseResponse");
  amf_notifier.on_new_message(ngap_msg);
}
