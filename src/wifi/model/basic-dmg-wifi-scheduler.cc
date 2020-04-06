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

#include <ns3/assert.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/pointer.h>
#include <ns3/boolean.h>

#include "basic-dmg-wifi-scheduler.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BasicDmgWifiScheduler");

NS_OBJECT_ENSURE_REGISTERED (BasicDmgWifiScheduler);

TypeId
BasicDmgWifiScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicDmgWifiScheduler")
      .SetParent<DmgWifiScheduler> ()
      .SetGroupName ("Wifi")
      .AddConstructor<BasicDmgWifiScheduler> ()

      .AddAttribute ("MinBroadcastCbapDuration", "The minimum duration in microseconds of a broadcast CBAP in the DTI",
                     UintegerValue (4096),
                     MakeUintegerAccessor (&BasicDmgWifiScheduler::m_minBroadcastCbapDuration),
                     MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("InterAllocationDistance", "The time distance in microseconds between two adjacent allocations "
                     "This distance will be allocated as broadcast CBAP",
                     UintegerValue (0),
                     MakeUintegerAccessor (&BasicDmgWifiScheduler::m_interAllocationDistance),
                     MakeUintegerChecker<uint32_t> (10, 65535))
  ;
  return tid;
}

BasicDmgWifiScheduler::BasicDmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

BasicDmgWifiScheduler::~BasicDmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
BasicDmgWifiScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  DmgWifiScheduler::DoDispose ();
}

void
BasicDmgWifiScheduler::UpdateStartAndRemainingTime (void)
{
  NS_LOG_FUNCTION (this);
  if (m_addtsAllocationList.empty ())
    {
      /* No existing allocations */
      m_allocationStartTime = 0;
      m_remainingDtiTime = m_dtiDuration.GetMicroSeconds ();
    }
  else
    {
      /* At least one allocation. Get last one */
      AllocationField lastAllocation = m_addtsAllocationList.back ();
      m_allocationStartTime = lastAllocation.GetAllocationStart () + lastAllocation.GetAllocationBlockDuration () + m_guardTime;
      m_remainingDtiTime = m_dtiDuration.GetMicroSeconds () - m_allocationStartTime;
    }
}

void
BasicDmgWifiScheduler::AdjustExistingAllocations (AllocationFieldListI iter, uint32_t duration, bool isToAdd)
{
  NS_LOG_FUNCTION (this << duration << isToAdd);
  if (isToAdd)
    {
      for (AllocationFieldListI nextAlloc = iter; nextAlloc != m_addtsAllocationList.end (); ++nextAlloc)
        {
          nextAlloc->SetAllocationStart (nextAlloc->GetAllocationStart () + duration);
        }
    }
  else
    {
      for (AllocationFieldListI nextAlloc = iter; nextAlloc != m_addtsAllocationList.end (); ++nextAlloc)
        {
          nextAlloc->SetAllocationStart (nextAlloc->GetAllocationStart () - duration);
        }
    } 
}

uint32_t
BasicDmgWifiScheduler::GetAllocationDuration (uint32_t minAllocation, uint32_t maxAllocation)
{
  NS_LOG_FUNCTION (this << minAllocation << maxAllocation);
  return ((minAllocation + maxAllocation) / 2);
}

StatusCode
BasicDmgWifiScheduler::AddNewAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info)
{
  NS_LOG_FUNCTION (this);
  /* Implementation of an admission policy for newly received requests. */
  if (dmgTspec.GetAllocationPeriod () != 0)
    {
      NS_FATAL_ERROR ("Multiple allocations are not supported by DmgWifiScheduler");
    }

  StatusCode status;
  uint32_t allocDuration;
  if (info.GetAllocationFormat () == ISOCHRONOUS)
    {
      allocDuration = GetAllocationDuration (dmgTspec.GetMinimumAllocation (), dmgTspec.GetMaximumAllocation ());
    }
  else if (info.GetAllocationFormat () == ASYNCHRONOUS)
    {
      /* for asynchronous allocations, the Maximum Allocation field is reserved (IEEE 802.11ad 8.4.2.136) */
      allocDuration = dmgTspec.GetMinimumAllocation ();
    }
  else
    {
      NS_FATAL_ERROR ("Allocation Format not supported");
    }    

  if (allocDuration <= (m_remainingDtiTime - m_minBroadcastCbapDuration)) // broadcast CBAP must be present
    {
      m_allocationStartTime = AllocateSingleContiguousBlock (info.GetAllocationID (), info.GetAllocationType (), info.IsPseudoStatic (),
                                                             sourceAid, info.GetDestinationAid (), m_allocationStartTime, allocDuration);
      m_remainingDtiTime -= (allocDuration + m_guardTime);
      status.SetSuccess ();
    }
  else if ((info.GetAllocationFormat () == ISOCHRONOUS) && // check Minimum Allocation for Isochronous request
           (dmgTspec.GetMinimumAllocation () <= (m_remainingDtiTime - m_minBroadcastCbapDuration))) 
    {
      m_allocationStartTime = AllocateSingleContiguousBlock (info.GetAllocationID (), info.GetAllocationType (), info.IsPseudoStatic (),
                                                             sourceAid, info.GetDestinationAid (), m_allocationStartTime, 
                                                             dmgTspec.GetMinimumAllocation ());
      m_remainingDtiTime -= (dmgTspec.GetMinimumAllocation () + m_guardTime);
      status.SetSuccess ();
    }
  else
    {
      /* The ADDTS request is not accepted based on the current policy */
      status.SetFailure (); 
    }

  return status;
}

StatusCode
BasicDmgWifiScheduler::ModifyExistingAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info)
{
  NS_LOG_FUNCTION (this);
  /* Implementation of an admission policy for modification requests. */
  if (dmgTspec.GetAllocationPeriod () != 0)
    {
      NS_FATAL_ERROR ("Multiple allocations are not supported by DmgWifiScheduler");
    }

  StatusCode status;
  uint32_t newDuration;
  if (info.GetAllocationFormat () == ISOCHRONOUS)
    {
      newDuration = GetAllocationDuration (dmgTspec.GetMinimumAllocation (), dmgTspec.GetMaximumAllocation ());
    }
  else if (info.GetAllocationFormat () == ASYNCHRONOUS)
    {
      /* for asynchronous allocations, the Maximum Allocation field is reserved (IEEE 802.11ad 8.4.2.136) */
      newDuration = dmgTspec.GetMinimumAllocation ();
    }
  else
    {
      NS_FATAL_ERROR ("Allocation Format not supported");
    }

  AllocationFieldListI allocation;
  /* Retrieve the allocation for which a modification has been requested */
  for (allocation = m_addtsAllocationList.begin (); allocation != m_addtsAllocationList.end ();)
    {
      if ((allocation->GetAllocationID () == info.GetAllocationID ()) &&
          (allocation->GetSourceAid () == sourceAid) && (allocation->GetDestinationAid () == info.GetDestinationAid ()))
        {
          break;
        }
      else
        {
          ++allocation;
        }
    }

  uint32_t currentDuration = allocation->GetAllocationBlockDuration ();
  NS_LOG_DEBUG ("Current duration=" << currentDuration << ", New Duration=" << newDuration);
  uint32_t timeDifference;
  if (newDuration > currentDuration)
    {
      timeDifference = newDuration - currentDuration;
      if (timeDifference <= (m_remainingDtiTime - m_minBroadcastCbapDuration))
        {
          allocation->SetAllocationBlockDuration (newDuration);
          status.SetSuccess ();
          AdjustExistingAllocations (++allocation, timeDifference, true);
          UpdateStartAndRemainingTime ();
          return status;
        }
      if ((info.GetAllocationFormat () == ISOCHRONOUS) && (dmgTspec.GetMinimumAllocation () > currentDuration))
        {
          timeDifference = dmgTspec.GetMinimumAllocation () - currentDuration;
          if (timeDifference <= (m_remainingDtiTime - m_minBroadcastCbapDuration))
            {
              allocation->SetAllocationBlockDuration (dmgTspec.GetMinimumAllocation ());
              status.SetSuccess ();
              AdjustExistingAllocations (++allocation, timeDifference, true);
              UpdateStartAndRemainingTime ();
              return status;
            }
        }
      /* The request cannot be accepted; maintaining old allocation duration */
      /* No need to update allocation start time and remaining DTI time */
      status.SetFailure ();
    }
  else
    {
      timeDifference = currentDuration - newDuration;
      allocation->SetAllocationBlockDuration (newDuration);
      status.SetSuccess ();
      AdjustExistingAllocations (++allocation, timeDifference, false);
      UpdateStartAndRemainingTime ();
    }

  return status;
}

void 
BasicDmgWifiScheduler::AddBroadcastCbapAllocations (void)
{
  NS_LOG_FUNCTION (this);
  uint32_t totalBroadcastCbapTime = 0;
  /* Addts allocation list is copied to the allocation list */
  m_allocationList = m_addtsAllocationList;
  AllocationFieldList broadcastCbapList;
  uint32_t start;
  AllocationFieldListI iter = m_allocationList.begin ();
  AllocationFieldListI nextIter = iter + 1;
  while (nextIter != m_allocationList.end ())
    {
      start = iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime;
      if ((m_remainingDtiTime >= m_interAllocationDistance)
          && (m_interAllocationDistance > 0)) // here the decision to place a broadcast CBAP among allocated requests
        {
          broadcastCbapList = GetBroadcastCbapAllocation (true, start, m_interAllocationDistance + m_guardTime);
          iter = m_allocationList.insert (nextIter, broadcastCbapList.begin (), broadcastCbapList.end ());
          start = iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime;
          iter += broadcastCbapList.size ();
          iter->SetAllocationStart (start);
          nextIter = iter + 1;
          totalBroadcastCbapTime += m_interAllocationDistance;
          m_remainingDtiTime -= (m_interAllocationDistance + m_guardTime);
        }
      else
        {
          ++iter;
          ++nextIter;
        }
    }
  /* iter points to the last element of allocation list (i.e. last element of Addts allocation list)
   * Check the presence of remaining DTI time to be allocated as broadcast CBAP
   */
  start = iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime;
  if (m_remainingDtiTime > 0)
    {
      broadcastCbapList = GetBroadcastCbapAllocation (true, start, m_remainingDtiTime);
      iter = m_allocationList.insert (m_allocationList.end (), broadcastCbapList.begin (), broadcastCbapList.end ());
      iter += broadcastCbapList.size () - 1;
      totalBroadcastCbapTime += m_remainingDtiTime;
    }
  /* Print the allocation list for debugging */
  for (AllocationFieldListI it = m_allocationList.begin (); it != m_allocationList.end (); ++it)
    {
      NS_LOG_DEBUG ("Alloc Id=" << +it->GetAllocationID () << ", Source AID=" << +it->GetSourceAid ()
                    << ", Destination AID: " << +it->GetDestinationAid () 
                    << ", Alloc Start: " << it->GetAllocationStart ()
                    << ", Alloc Duration: " << it->GetAllocationBlockDuration ());
    }
  /* Check if at least one broadcast CBAP is present */
  NS_ASSERT_MSG ((totalBroadcastCbapTime >= m_minBroadcastCbapDuration),
                 "The overall broadcast CBAP time needed is " << m_minBroadcastCbapDuration);
  /* Check DTI fully allocated */
  NS_LOG_DEBUG ("Last allocation start + duration + guard time: " 
                << iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime);
  NS_LOG_DEBUG ("DTI duration in microseconds: " << m_dtiDuration.GetMicroSeconds ());
  NS_ASSERT_MSG ((iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime) == m_dtiDuration.GetMicroSeconds (),
                 "The DTI is not totally allocated");
}

} // namespace ns3