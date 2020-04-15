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
 *
 */

#include <ns3/assert.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/pointer.h>
#include <ns3/boolean.h>

#include "periodic-dmg-wifi-scheduler.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PeriodicDmgWifiScheduler");

NS_OBJECT_ENSURE_REGISTERED (PeriodicDmgWifiScheduler);

TypeId
PeriodicDmgWifiScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PeriodicDmgWifiScheduler")
      .SetParent<DmgWifiScheduler> ()
      .SetGroupName ("Wifi")
      .AddConstructor<PeriodicDmgWifiScheduler> ()

      .AddAttribute ("MinBroadcastCbapDuration", "The minimum duration in microseconds of a broadcast CBAP in the DTI",
                     UintegerValue (4096),
                     MakeUintegerAccessor (&PeriodicDmgWifiScheduler::m_minBroadcastCbapDuration),
                     MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("InterAllocationDistance", "The time distance in microseconds between two adjacent allocations "
                     "This distance will be allocated as broadcast CBAP",
                     UintegerValue (10),
                     MakeUintegerAccessor (&PeriodicDmgWifiScheduler::m_interAllocationDistance),
                     MakeUintegerChecker<uint32_t> (10, 65535))
  ;
  return tid;
}

PeriodicDmgWifiScheduler::PeriodicDmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

PeriodicDmgWifiScheduler::~PeriodicDmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
PeriodicDmgWifiScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  DmgWifiScheduler::DoDispose ();
}

void
PeriodicDmgWifiScheduler::UpdateStartAndRemainingTime (void)
{
  NS_LOG_FUNCTION (this);
}

void
PeriodicDmgWifiScheduler::AdjustExistingAllocations (AllocationFieldListI iter, uint32_t duration, bool isToAdd)
{
  NS_LOG_FUNCTION (this << duration << isToAdd);
}

uint32_t
PeriodicDmgWifiScheduler::GetAllocationDuration (uint32_t minAllocation, uint32_t maxAllocation)
{
  NS_LOG_FUNCTION (this << minAllocation << maxAllocation);
  return ((minAllocation + maxAllocation) / 2);
}

StatusCode
PeriodicDmgWifiScheduler::AddNewAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info)
{
  NS_LOG_FUNCTION (this);

  StatusCode status;

  return status;
}

StatusCode
PeriodicDmgWifiScheduler::ModifyExistingAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info)
{
  NS_LOG_FUNCTION (this);

  StatusCode status;

  return status;
}

void
PeriodicDmgWifiScheduler::AddBroadcastCbapAllocations (void)
{
  NS_LOG_FUNCTION (this);

}

} // namespace ns3
