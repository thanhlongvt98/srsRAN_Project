#
# Copyright 2021-2024 Software Radio Systems Limited
#
# This file is part of srsRAN
#
# srsRAN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsRAN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

"""
Steps related with stubs / resources
"""

import logging
from concurrent.futures import as_completed, ThreadPoolExecutor
from contextlib import contextmanager, suppress
from time import sleep
from typing import Dict, Generator, List, Optional, Sequence, Tuple

import grpc
import pytest
from retina.client.exception import ErrorReportedByAgent
from retina.launcher.artifacts import RetinaTestData
from retina.protocol import RanStub
from retina.protocol.base_pb2 import (
    Empty,
    Metrics,
    PingRequest,
    PLMN,
    StartInfo,
    StopResponse,
    String,
    UEDefinition,
    UInteger,
)
from retina.protocol.fivegc_pb2 import FiveGCStartInfo, IPerfResponse
from retina.protocol.fivegc_pb2_grpc import FiveGCStub
from retina.protocol.gnb_pb2 import GNBStartInfo
from retina.protocol.gnb_pb2_grpc import GNBStub
from retina.protocol.ue_pb2 import IPerfDir, IPerfProto, IPerfRequest, UEAttachedInfo, UEStartInfo
from retina.protocol.ue_pb2_grpc import UEStub

RF_MAX_TIMEOUT: int = 3 * 60  # Time enough in RF when loading a new image in the sdr
UE_STARTUP_TIMEOUT: int = RF_MAX_TIMEOUT
GNB_STARTUP_TIMEOUT: int = 5  # GNB delay (we wait x seconds and check it's still alive). UE later and has a big timeout
FIVEGC_STARTUP_TIMEOUT: int = RF_MAX_TIMEOUT
ATTACH_TIMEOUT: int = 1 * 60


# pylint: disable=too-many-arguments,too-many-locals
def start_and_attach(
    ue_array: Sequence[UEStub],
    gnb: GNBStub,
    fivegc: FiveGCStub,
    ue_startup_timeout: int = UE_STARTUP_TIMEOUT,
    gnb_startup_timeout: int = GNB_STARTUP_TIMEOUT,
    fivegc_startup_timeout: int = FIVEGC_STARTUP_TIMEOUT,
    gnb_pre_cmd: str = "",
    gnb_post_cmd: str = "",
    attach_timeout: int = ATTACH_TIMEOUT,
    plmn: Optional[PLMN] = None,
) -> Dict[UEStub, UEAttachedInfo]:
    """
    Start stubs & wait until attach
    """
    start_network(
        ue_array,
        gnb,
        fivegc,
        gnb_startup_timeout,
        fivegc_startup_timeout,
        gnb_pre_cmd,
        gnb_post_cmd,
        plmn=plmn,
    )

    return ue_start_and_attach(
        ue_array,
        gnb,
        fivegc,
        ue_startup_timeout=ue_startup_timeout,
        attach_timeout=attach_timeout,
    )


def _get_hplmn(imsi: str) -> PLMN:
    """
    Obtain home PLMN (HPLMN) from IMSI prefix as specified in TS 23.003 Sec. 2.2
    """
    hplmn = PLMN()
    hplmn.mcc = imsi[0:3]
    hplmn.mnc = imsi[3:5]
    return hplmn


# pylint: disable=too-many-arguments,too-many-locals
def start_network(
    ue_array: Sequence[UEStub],
    gnb: GNBStub,
    fivegc: FiveGCStub,
    gnb_startup_timeout: int = GNB_STARTUP_TIMEOUT,
    fivegc_startup_timeout: int = FIVEGC_STARTUP_TIMEOUT,
    gnb_pre_cmd: str = "",
    gnb_post_cmd: str = "",
    plmn: Optional[PLMN] = None,
):
    """
    Start Network (5GC + gNB)
    """

    ue_def_for_gnb = UEDefinition()
    for ue_stub in ue_array:
        ue_def: UEDefinition = ue_stub.GetDefinition(Empty())
        ue_hplmn = _get_hplmn(ue_def.subscriber.imsi)
        if plmn:
            # Warn if UEs have different HPLMNs
            if plmn != ue_hplmn:
                logging.warning(
                    "HPLMN of UE with IMSI [%s] differs from PLMN. Expected MCC=%s MNC=%s",
                    ue_def.subscriber.imsi,
                    plmn.mcc,
                    plmn.mnc,
                )
        else:
            # Set PLMN to HPLMN of first UE
            plmn = ue_hplmn
            logging.info("Setting PLMN to HPLMN of first UE. MCC=%s MNC=%s", plmn.mcc, plmn.mnc)
        fivegc.AddUESubscriber(ue_def.subscriber)
        if ue_def.zmq_ip is not None:
            ue_def_for_gnb = ue_def

    with handle_start_error(name=f"5GC [{id(fivegc)}]"):
        # 5GC Start
        fivegc.Start(
            FiveGCStartInfo(
                plmn=plmn,
                start_info=StartInfo(timeout=fivegc_startup_timeout),
            )
        )

    with handle_start_error(name=f"GNB [{id(gnb)}]"):
        # GNB Start
        gnb.Start(
            GNBStartInfo(
                plmn=plmn,
                ue_definition=ue_def_for_gnb,
                fivegc_definition=fivegc.GetDefinition(Empty()),
                start_info=StartInfo(
                    timeout=gnb_startup_timeout,
                    pre_commands=gnb_pre_cmd,
                    post_commands=gnb_post_cmd,
                ),
            )
        )


def ue_start_and_attach(
    ue_array: Sequence[UEStub],
    gnb: GNBStub,
    fivegc: FiveGCStub,
    ue_startup_timeout: int = UE_STARTUP_TIMEOUT,
    attach_timeout: int = ATTACH_TIMEOUT,
) -> Dict[UEStub, UEAttachedInfo]:
    """
    Start an array of UEs and wait until attached to already running gnb and 5gc
    """

    for ue_stub in ue_array:
        with handle_start_error(name=f"UE [{id(ue_stub)}]"):
            ue_stub.Start(
                UEStartInfo(
                    gnb_definition=gnb.GetDefinition(Empty()),
                    fivegc_definition=fivegc.GetDefinition(Empty()),
                    start_info=StartInfo(timeout=ue_startup_timeout),
                )
            )

    # Attach in parallel
    ue_attach_task_dict: Dict[UEStub, grpc.Future] = {
        ue_stub: ue_stub.WaitUntilAttached.future(UInteger(value=attach_timeout)) for ue_stub in ue_array
    }
    for ue_stub, task in ue_attach_task_dict.items():
        task.add_done_callback(lambda _task, _ue_stub=ue_stub: _log_attached_ue(_task, _ue_stub))

    ue_attach_info_dict: Dict[UEStub, UEAttachedInfo] = {}
    with suppress(grpc.RpcError):
        ue_attach_info_dict = {
            ue_stub: task.result() for ue_stub, task in ue_attach_task_dict.items()  # Waiting for attach
        }

    if not ue_attach_info_dict:
        pytest.fail("Attach timeout reached")

    return ue_attach_info_dict


@contextmanager
def handle_start_error(name: str) -> Generator[None, None, None]:
    """
    Fail the test if the stub `name` could not start
    """
    raise_failed = False
    try:
        yield
        logging.info("%s started", name)
    except grpc.RpcError as err:
        if ErrorReportedByAgent(err).code is grpc.StatusCode.ABORTED:
            raise_failed = True
        else:
            raise err from None
    if raise_failed:
        pytest.fail(f"{name} failed to start")


def _log_attached_ue(future: grpc.Future, ue_stub: UEStub):
    with suppress(grpc.RpcError, ValueError):
        logging.info(
            "UE [%s] attached: \n%s%s ",
            id(ue_stub),
            ue_stub.GetDefinition(Empty()).subscriber,
            future.result(),
        )


def ping(ue_attach_info_dict: Dict[UEStub, UEAttachedInfo], fivegc: FiveGCStub, ping_count, time_step: int = 1):
    """
    Ping command between an UE and a 5GC
    """
    ping_task_array = ping_start(ue_attach_info_dict, fivegc, ping_count, time_step)
    ping_wait_until_finish(ping_task_array)


def ping_start(
    ue_attach_info_dict: Dict[UEStub, UEAttachedInfo], fivegc: FiveGCStub, ping_count, time_step: int = 1
) -> List[grpc.Future]:
    """
    Ping command between an UE and a 5GC
    """

    # Launch ping (ue -> 5gc and 5gc -> ue) for each attached ue in parallel

    ping_task_array: List[grpc.Future] = []
    for ue_stub, ue_attached_info in ue_attach_info_dict.items():
        ue_to_fivegc: grpc.Future = ue_stub.Ping.future(
            PingRequest(address=ue_attached_info.ipv4_gateway, count=ping_count)
        )
        ue_to_fivegc.add_done_callback(
            lambda _task, _msg=f"[{ue_attached_info.ipv4}] UE -> 5GC": _print_ping_result(_msg, _task)
        )
        fivegc_to_ue: grpc.Future = fivegc.Ping.future(PingRequest(address=ue_attached_info.ipv4, count=ping_count))
        fivegc_to_ue.add_done_callback(
            lambda _task, _msg=f"[{ue_attached_info.ipv4}] 5GC -> UE": _print_ping_result(_msg, _task)
        )
        ping_task_array.append(ue_to_fivegc)
        ping_task_array.append(fivegc_to_ue)
        sleep(time_step)

    return ping_task_array


def ping_wait_until_finish(ping_task_array: List[grpc.Future]) -> None:
    """
    Wait until the requested ping has finished.
    """
    ping_success = True
    for ping_task in ping_task_array:
        ping_success &= ping_task.result().status

    if not ping_success:
        pytest.fail("Ping. Some packages got lost.")


def _print_ping_result(msg: str, task: grpc.Future):
    """
    Print ping result
    """
    log_fn = logging.info
    try:
        result = task.result()
        if not result.status:
            log_fn = logging.error
    except (grpc.RpcError, grpc.FutureCancelledError, grpc.FutureTimeoutError) as err:
        log_fn = logging.error
        result = ErrorReportedByAgent(err)
    finally:
        log_fn("Ping %s: %s", msg, result)


def iperf_parallel(
    ue_attach_info_dict: Dict[UEStub, UEAttachedInfo],
    fivegc: FiveGCStub,
    protocol: IPerfProto,
    direction: IPerfDir,
    iperf_duration: int,
    bitrate: int,
    bitrate_threshold_ratio: float = 0,  # real_bitrate > (bitrate_threshold_ratio * ideal_bitrate)
    parallel_iperfs: int = 8,
) -> List[IPerfResponse]:
    """
    iperf command between multiple UEs and a 5GC. Runs at <parallel_iperfs> in parallel.
    """

    iperf_result_list: List[IPerfResponse] = []

    with ThreadPoolExecutor(max_workers=parallel_iperfs) as executor:
        future_array = (
            executor.submit(
                iperf_sequentially,
                ue_stub,
                ue_attached_info,
                fivegc,
                protocol,
                direction,
                iperf_duration,
                bitrate,
                bitrate_threshold_ratio,
            )
            for ue_stub, ue_attached_info in ue_attach_info_dict.items()
        )

        iperf_success = True
        for future in as_completed(future_array):
            iperf_response = future.result()
            iperf_success &= iperf_response[0]
            iperf_result_list.append(iperf_response[1])

    if not iperf_success:
        pytest.fail("iperf did not achieve the expected data rate.")

    return iperf_result_list


def iperf_sequentially(
    ue_stub: UEStub,
    ue_attached_info: UEAttachedInfo,
    fivegc: FiveGCStub,
    protocol: IPerfProto,
    direction: IPerfDir,
    iperf_duration: int,
    bitrate: int,
    bitrate_threshold_ratio: float = 0,  # real_bitrate > (bitrate_threshold_ratio * ideal_bitrate)
    max_retries: int = 5,
    sleep_between_retries: int = 3,
) -> Tuple[bool, IPerfResponse]:
    """
    iperf command between an UE and a 5GC
    """

    for _ in range(max_retries):
        try:
            task, iperf_request = iperf_start(
                ue_stub,
                ue_attached_info,
                fivegc,
                protocol,
                direction,
                iperf_duration,
                bitrate,
            )
            sleep(iperf_duration)
            iperf_success, iperf_data = iperf_wait_until_finish(
                ue_attached_info, fivegc, task, iperf_request, bitrate_threshold_ratio
            )
            if iperf_success:
                return iperf_success, iperf_data
        except grpc.RpcError as err:
            logging.warning(
                "Iperf %s [%s %s] failed due to %s",
                ue_attached_info.ipv4,
                _iperf_proto_to_str(iperf_request.proto),
                _iperf_dir_to_str(iperf_request.direction),
                ErrorReportedByAgent(err).details,
            )
        sleep(sleep_between_retries)

    return False, IPerfResponse()


def iperf_start(
    ue_stub: UEStub,
    ue_attached_info: UEAttachedInfo,
    fivegc: FiveGCStub,
    protocol: IPerfProto,
    direction: IPerfDir,
    duration: int,
    bitrate: int,
) -> Tuple[grpc.Future, IPerfRequest]:
    """
    Start a Iperf and keep it running
    """

    iperf_request = IPerfRequest(
        server=fivegc.StartIPerfService(String(value=ue_attached_info.ipv4_gateway)),
        duration=duration,
        direction=direction,
        proto=protocol,
        bitrate=bitrate,
    )

    # Run iperf
    task: grpc.Future = ue_stub.IPerf.future(iperf_request)

    logging.info(
        "Iperf %s [%s %s] started",
        ue_attached_info.ipv4,
        _iperf_proto_to_str(iperf_request.proto),
        _iperf_dir_to_str(iperf_request.direction),
    )

    return (task, iperf_request)


def iperf_wait_until_finish(
    ue_attached_info: UEAttachedInfo,
    fivegc: FiveGCStub,
    task: grpc.Future,
    iperf_request: IPerfRequest,
    bitrate_threshold_ratio: float = 0,  # real_bitrate > (bitrate_threshold_ratio * ideal_bitrate)
) -> Tuple[bool, IPerfResponse]:
    """
    Wait until the requested iperf has finished.
    """

    # Stop server, get results and print it
    try:
        task.result()
    except grpc.RpcError as err:
        if ErrorReportedByAgent(err).code is not grpc.StatusCode.UNAVAILABLE:
            raise err from None

    iperf_data: IPerfResponse = fivegc.StopIPerfService(iperf_request.server)
    logging.info(
        "Iperf %s [%s %s] result %s",
        ue_attached_info.ipv4,
        _iperf_proto_to_str(iperf_request.proto),
        _iperf_dir_to_str(iperf_request.direction),
        iperf_data,
    )

    # Assertion
    iperf_success = True
    if (
        iperf_request.direction in (IPerfDir.DOWNLINK, IPerfDir.BIDIRECTIONAL)
        and iperf_data.downlink.bits_per_second <= bitrate_threshold_ratio * iperf_request.bitrate
    ):
        logging.warning(
            "Downlink bitrate too low. Requested: %s - Measured: %s",
            iperf_request.bitrate,
            iperf_data.downlink.bits_per_second,
        )
        iperf_success = False
    if (
        iperf_request.direction in (IPerfDir.UPLINK, IPerfDir.BIDIRECTIONAL)
        and iperf_data.uplink.bits_per_second <= bitrate_threshold_ratio * iperf_request.bitrate
    ):
        logging.warning(
            "Uplink bitrate too low. Requested: %s - Measured: %s",
            iperf_request.bitrate,
            iperf_data.uplink.bits_per_second,
        )
        iperf_success = False
    return (iperf_success, iperf_data)


def _iperf_proto_to_str(proto):
    return {IPerfProto.TCP: "tcp", IPerfProto.UDP: "udp"}[proto]


def _iperf_dir_to_str(direction):
    return {
        IPerfDir.DOWNLINK: "downlink",
        IPerfDir.UPLINK: "uplink",
        IPerfDir.BIDIRECTIONAL: "bidirectional",
    }[direction]


def stop(
    ue_array: Sequence[UEStub],
    gnb: Optional[GNBStub],
    fivegc: Optional[FiveGCStub],
    retina_data: RetinaTestData,
    ue_stop_timeout: int = 0,  # Auto
    gnb_stop_timeout: int = 0,
    fivegc_stop_timeout: int = 0,
    log_search: bool = True,
    warning_as_errors: bool = True,
    fail_if_kos: bool = False,
):
    """
    Stop ue(s), gnb and 5gc
    """
    # Stop
    error_msg_array = []
    for index, ue_stub in enumerate(ue_array):
        error_msg_array.append(
            _stop_stub(
                ue_stub,
                f"UE_{index+1}",
                retina_data,
                ue_stop_timeout,
                log_search,
                warning_as_errors,
            )
        )
    if gnb is not None:
        error_msg_array.append(_stop_stub(gnb, "GNB", retina_data, gnb_stop_timeout, log_search, warning_as_errors))
    if fivegc is not None:
        error_msg_array.append(
            _stop_stub(
                fivegc,
                "5GC",
                retina_data,
                fivegc_stop_timeout,
                log_search,
                warning_as_errors,
            )
        )

    # Fail if stop errors
    error_msg_array = list(filter(bool, error_msg_array))
    if error_msg_array:
        pytest.fail(
            f"Stop stage. {error_msg_array[0]}"
            + (("\nFull list of errors:\n - " + "\n - ".join(error_msg_array)) if len(error_msg_array) > 1 else "")
        )

    # Metrics after Stop
    metrics_msg_array = []
    for index, ue_stub in enumerate(ue_array):
        metrics_msg_array.append(_get_metrics(ue_stub, f"UE_{index+1}", fail_if_kos=fail_if_kos))
    if gnb is not None:
        metrics_msg_array.append(_get_metrics(gnb, "GNB", fail_if_kos=fail_if_kos))
    if fivegc is not None:
        metrics_msg_array.append(_get_metrics(fivegc, "5GC", fail_if_kos=fail_if_kos))

    # Fail if metric errors
    metrics_msg_array = list(filter(bool, metrics_msg_array))
    if metrics_msg_array:
        pytest.fail(
            f"Metrics validation. {metrics_msg_array[0]}"
            + (("\nFull list of errors:\n - " + "\n - ".join(metrics_msg_array)) if len(metrics_msg_array) > 1 else "")
        )


def ue_stop(
    ue_array: Sequence[UEStub],
    retina_data: RetinaTestData,
    ue_stop_timeout: int = 0,  # Auto
    log_search: bool = True,
    warning_as_errors: bool = True,
):
    """
    Stop an array of UEs to detach from already running gnb and 5gc
    """
    error_msg_array = []
    for index, ue_stub in enumerate(ue_array):
        error_msg_array.append(
            _stop_stub(
                ue_stub,
                f"UE_{index+1}",
                retina_data,
                ue_stop_timeout,
                log_search,
                warning_as_errors,
            )
        )
    error_msg_array = list(filter(bool, error_msg_array))
    if error_msg_array:
        pytest.fail(
            f"UE Stop. {error_msg_array[0]}"
            + (("\nFull list of errors:\n - " + "\n - ".join(error_msg_array)) if len(error_msg_array) > 1 else "")
        )


def _stop_stub(
    stub: RanStub,
    name: str,
    retina_data: RetinaTestData,
    timeout: int = 0,
    log_search: bool = True,
    warning_as_errors: bool = True,
) -> str:
    """
    Stop a stub in the defined timeout (0=auto).
    It uses retina_data to save artifacts in case of failure
    """

    error_msg = ""

    with suppress(grpc.RpcError):
        stop_info: StopResponse = stub.Stop(UInteger(value=timeout))

        if stop_info.exit_code:
            retina_data.download_artifacts = True
            error_msg = f"{name} crashed with exit code {stop_info.exit_code}. "

        if log_search:
            log_msg = f"{name} has {stop_info.error_count} errors and {stop_info.warning_count} warnings. "
            if stop_info.error_count:
                log_msg += f"First error is: {stop_info.error_msg}"
            elif stop_info.warning_count:
                log_msg += f"First warning is: {stop_info.warning_msg}"

            if stop_info.error_count or (warning_as_errors and stop_info.warning_count):
                retina_data.download_artifacts = True
                error_msg += log_msg
            elif not warning_as_errors and stop_info.warning_count:
                logging.warning(log_msg)

        if error_msg:
            logging.error(error_msg)
        else:
            logging.info("%s has stopped", name)

    return error_msg


def ue_reestablishment(
    ue_array: Sequence[UEStub],
):
    """
    Reestablishment an array of UEs from already running gnb and 5gc
    """
    for ue_stub in ue_array:
        logging.info("Reestablishment UE [%s]", id(ue_stub))
        ue_stub.Reestablishment(Empty())


def _get_metrics(stub: RanStub, name: str, fail_if_kos: bool = False) -> str:
    error_msg = ""

    if fail_if_kos:
        with suppress(grpc.RpcError):
            metrics: Metrics = stub.GetMetrics(Empty())

            if metrics.nof_kos or metrics.nof_retx:
                error_msg = f"{name} has:"

            if metrics.nof_kos:
                kos_msg = f" {metrics.nof_kos} KOs"
                error_msg += kos_msg + "."
                logging.error("%s has%s", name, kos_msg)

            if metrics.nof_retx:
                retrx_msg = f" {metrics.nof_retx} retrxs"
                error_msg += retrx_msg + "."
                logging.error("%s has%s", name, retrx_msg)

    return error_msg
