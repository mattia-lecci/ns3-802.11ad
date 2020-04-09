/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020, University of Padova, Department of Information
 * Engineering, SIGNET Lab.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Tommy Azzino <tommy.azzino@gmail.com>
 *
 */

#include <ns3/log.h>
#include "cbap-only-dmg-wifi-scheduler.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CbapOnlyDmgWifiScheduler");

NS_OBJECT_ENSURE_REGISTERED (CbapOnlyDmgWifiScheduler);

TypeId
CbapOnlyDmgWifiScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbapOnlyDmgWifiScheduler")
      .SetParent<DmgWifiScheduler> ()
      .SetGroupName ("Wifi")
      .AddConstructor<CbapOnlyDmgWifiScheduler> ()
  ;
  return tid;
}

CbapOnlyDmgWifiScheduler::CbapOnlyDmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

CbapOnlyDmgWifiScheduler::~CbapOnlyDmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
CbapOnlyDmgWifiScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  DmgWifiScheduler::DoDispose ();
}

void
CbapOnlyDmgWifiScheduler::UpdateStartAndRemainingTime (void)
{
  NS_LOG_FUNCTION (this);
}

void
CbapOnlyDmgWifiScheduler::AdjustExistingAllocations (AllocationFieldListI iter, uint32_t duration, bool isToAdd)
{
  NS_LOG_FUNCTION (this << duration << isToAdd); 
}

uint32_t
CbapOnlyDmgWifiScheduler::GetAllocationDuration (uint32_t minAllocation, uint32_t maxAllocation)
{
  NS_LOG_FUNCTION (this << minAllocation << maxAllocation);
  return 0;
}

StatusCode
CbapOnlyDmgWifiScheduler::AddNewAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info)
{
  NS_LOG_FUNCTION (this);
  /* The newly received ADDTS request is not accepted. */
  StatusCode status;
  status.SetFailure ();
  return status;
}

StatusCode
CbapOnlyDmgWifiScheduler::ModifyExistingAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info)
{
  NS_LOG_FUNCTION (this);
  /* The modification ADDTS request is not accepted. */
  StatusCode status;
  status.SetFailure ();
  return status;
}

void 
CbapOnlyDmgWifiScheduler::AddBroadcastCbapAllocations (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3