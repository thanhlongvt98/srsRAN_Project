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

#include "lib/e2/common/e2ap_asn1_packer.h"
#include "srsran/e2/e2.h"
#include "srsran/support/io/io_broker.h"

namespace srsran {

/// \brief E2AP bridge that uses the IO broker to handle the SCTP connection
class e2ap_network_adapter : public e2_message_notifier,
                             public e2_message_handler,
                             public sctp_network_gateway_control_notifier,
                             public network_gateway_data_notifier
{
public:
  e2ap_network_adapter(io_broker& broker_, dlt_pcap& pcap_) : broker(broker_), pcap(pcap_)
  {
    if (gateway_ctrl_handler != nullptr) {
      broker.unregister_fd(gateway_ctrl_handler->get_socket_fd());
    }
  }

  void connect_gateway(sctp_network_gateway_controller*   gateway_ctrl_handler_,
                       sctp_network_gateway_data_handler* gateway_data_handler_)
  {
    gateway_ctrl_handler = gateway_ctrl_handler_;
    gateway_data_handler = gateway_data_handler_;

    packer = std::make_unique<e2ap_asn1_packer>(*gateway_data_handler, *this, pcap);

    if (!gateway_ctrl_handler->create_and_connect()) {
      report_error("Failed to create SCTP gateway.\n");
    }
    broker.register_fd(gateway_ctrl_handler->get_socket_fd(), [this](int fd) { gateway_ctrl_handler->receive(); });
  }

  void connect_e2ap(e2_message_handler* e2ap_msg_handler_, e2_event_handler* event_handler_)
  {
    e2ap_msg_handler = e2ap_msg_handler_;
    event_handler    = event_handler_;
  }

  void disconnect_gateway()
  {
    report_fatal_error_if_not(gateway_ctrl_handler, "Gateway handler not set.");
    broker.unregister_fd(gateway_ctrl_handler->get_socket_fd());

    gateway_ctrl_handler = nullptr;
    gateway_data_handler = nullptr;

    packer.reset();
  }

private:
  // E2AP calls interface to send (unpacked) E2AP PDUs
  void on_new_message(const e2_message& msg) override
  {
    if (packer) {
      packer->handle_message(msg);
    } else {
      logger.debug("E2AP ASN1 packer disconnected, dropping msg");
    }
  }

  // SCTP network gateway calls interface to inject received PDUs (ASN1 packed)
  void on_new_pdu(byte_buffer pdu) override
  {
    if (packer) {
      packer->handle_packed_pdu(pdu);
    } else {
      logger.debug("E2AP ASN1 packer disconnected, dropping pdu");
    }
  }

  // The packer calls this interface to inject unpacked E2AP PDUs
  void handle_message(const e2_message& msg) override
  {
    report_fatal_error_if_not(e2ap_msg_handler, "E2AP handler not set.");
    e2ap_msg_handler->handle_message(msg);
  }

  void on_connection_loss() override
  {
    report_fatal_error_if_not(event_handler, "E2AP handler not set.");
    event_handler->handle_connection_loss();
  }

  void on_connection_established() override
  {
    // TODO: extend event interface to inform about connection establishment
    logger.debug("on_connection_established");
  }

  io_broker&                         broker;
  dlt_pcap&                          pcap;
  std::unique_ptr<e2ap_asn1_packer>  packer;
  sctp_network_gateway_controller*   gateway_ctrl_handler = nullptr;
  sctp_network_gateway_data_handler* gateway_data_handler = nullptr;
  e2_message_handler*                e2ap_msg_handler     = nullptr;
  e2_event_handler*                  event_handler        = nullptr;

  srslog::basic_logger& logger = srslog::fetch_basic_logger("SCTP-GW");
};

}; // namespace srsran
