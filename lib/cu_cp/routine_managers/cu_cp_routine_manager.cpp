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

#include "cu_cp_routine_manager.h"
#include "../routines/amf_connection_setup_routine.h"
#include "../routines/ue_removal_routine.h"
#include "srsran/support/async/coroutine.h"

using namespace srsran;
using namespace srs_cu_cp;

cu_cp_routine_manager::cu_cp_routine_manager(ue_task_scheduler_manager& ue_task_sched_) :
  ue_task_sched(ue_task_sched_), main_ctrl_loop(128)
{
}

bool cu_cp_routine_manager::schedule_async_task(async_task<void> task)
{
  return main_ctrl_loop.schedule(std::move(task));
}

void cu_cp_routine_manager::start_ue_removal_routine(ue_index_t                      ue_index,
                                                     cu_cp_rrc_ue_removal_notifier&  rrc_du_notifier,
                                                     cu_cp_e1ap_ue_removal_notifier* e1ap_notifier,
                                                     cu_cp_f1ap_ue_removal_notifier& f1ap_notifier,
                                                     cu_cp_ngap_control_notifier&    ngap_notifier,
                                                     cell_meas_manager&              cell_meas_mng,
                                                     ue_manager&                     ue_mng,
                                                     srslog::basic_logger&           logger)
{
  ue_task_sched.handle_ue_async_task(
      ue_index,
      launch_async<ue_removal_routine>(
          ue_index, rrc_du_notifier, e1ap_notifier, f1ap_notifier, ngap_notifier, cell_meas_mng, ue_mng, logger));
}
