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

#include "dmg-wifi-scheduler.h"
#include "dmg-ap-wifi-mac.h"
#include "wifi-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgWifiScheduler");

NS_OBJECT_ENSURE_REGISTERED (DmgWifiScheduler);

TypeId
DmgWifiScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiScheduler")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
  ;
  return tid;
}

DmgWifiScheduler::DmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

DmgWifiScheduler::~DmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgWifiScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_mac = 0;
}

void 
DmgWifiScheduler::SetMac (Ptr<DmgApWifiMac> mac)
{
  NS_LOG_FUNCTION (this << mac);
  m_mac = mac;
}

void
DmgWifiScheduler::Initialize (void)
{
  NS_LOG_FUNCTION (this);
  DoInitialize ();
}

void
DmgWifiScheduler::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_mac->TraceConnectWithoutContext ("ADDTSReceived", MakeCallback (&DmgWifiScheduler::ReceiveAddtsRequest, this));
  m_mac->TraceConnectWithoutContext ("BIStarted", MakeCallback (&DmgWifiScheduler::BeaconIntervalStarted, this));
  m_mac->TraceConnectWithoutContext ("DTIStarted", MakeCallback (&DmgWifiScheduler::DataTransferIntervalStarted, this));
  m_mac->TraceConnectWithoutContext ("DELTSReceived", MakeCallback (&DmgWifiScheduler::ReceiveDeltsRequest, this));
}

AllocationFieldList
DmgWifiScheduler::GetAllocationList (void)
{
  return m_allocationList;
}

void
DmgWifiScheduler::SetAllocationList (AllocationFieldList allocationList)
{
  m_allocationList = allocationList;
}

void 
DmgWifiScheduler::BeaconIntervalStarted (Mac48Address address, Time bhiDuration, Time atiDuration)
{
  NS_LOG_DEBUG ("Beacon Interval started at " << Simulator::Now ());
  m_biStartTime = Simulator::Now ();
  m_accessPeriod = CHANNEL_ACCESS_BHI;
  m_bhiDuration = bhiDuration;
  m_atiDuration = atiDuration;
  if (m_atiDuration.IsStrictlyPositive ())
    {
      Simulator::Schedule (m_bhiDuration - m_atiDuration - m_mac->GetMbifs (), 
                           &DmgWifiScheduler::AnnouncementTransmissionIntervalStarted, this);
    }
}

void
DmgWifiScheduler::AnnouncementTransmissionIntervalStarted (void)
{
  NS_LOG_DEBUG ("ATI started at " << Simulator::Now ());
  m_atiStartTime = Simulator::Now ();
  m_accessPeriod = CHANNEL_ACCESS_ATI;
}

void 
DmgWifiScheduler::DataTransferIntervalStarted (Mac48Address address, Time dtiDuration)
{
  NS_LOG_DEBUG ("DTI started at " << Simulator::Now ());
  m_dtiStartTime = Simulator::Now ();
  m_accessPeriod = CHANNEL_ACCESS_DTI;
  m_dtiDuration = dtiDuration;
  Simulator::Schedule (m_dtiDuration, &DmgWifiScheduler::BeaconIntervalEnded, this);
}

void
DmgWifiScheduler::BeaconIntervalEnded (void)
{
  NS_LOG_DEBUG ("Beacon Interval ended at " << Simulator::Now ());
  /* Cleanup non-static allocations. */
  CleanupAllocations ();
  /* Do something with the ADDTS requests received in the last DTI (if any) */
}

void
DmgWifiScheduler::ReceiveDeltsRequest (Mac48Address address, DmgAllocationInfo info)
{
  NS_LOG_DEBUG ("Receive DELTS request from " << address);
  AllocationField allocation;
  uint8_t stationAid = m_mac->GetStationAid (address);
  for (AllocationFieldListI iter = m_allocationList.begin (); iter != m_allocationList.end ();)
    {
      allocation = (*iter);
      if ((allocation.GetAllocationID () == info.GetAllocationID ()) &&
          (allocation.GetSourceAid () == stationAid) &&
          (allocation.GetDestinationAid () == info.GetDestinationAid ()))
        {
          iter = m_allocationList.erase (iter);
          break;
        }
      else
        {
          ++iter;
        }
    }
}

void
DmgWifiScheduler::ReceiveAddtsRequest (Mac48Address address, DmgTspecElement element)
{
  NS_LOG_DEBUG ("Receive ADDTS request from " << address);
}

uint32_t
DmgWifiScheduler::AllocateCbapPeriod (bool staticAllocation, uint32_t allocationStart, uint16_t blockDuration)
{
  NS_LOG_FUNCTION (this << staticAllocation << allocationStart << blockDuration);
  AllocateSingleContiguousBlock (0, CBAP_ALLOCATION, staticAllocation, AID_BROADCAST, AID_BROADCAST, allocationStart, blockDuration);
  return (allocationStart + blockDuration);
}

uint32_t
DmgWifiScheduler::AllocateSingleContiguousBlock (AllocationID allocationId, AllocationType allocationType, bool staticAllocation,
                                                 uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, uint16_t blockDuration)
{
  NS_LOG_FUNCTION (this);
  return (AddAllocationPeriod (allocationId, allocationType, staticAllocation, sourceAid, destAid,
                               allocationStart, blockDuration, 0, 1));
}

uint32_t
DmgWifiScheduler::AllocateMultipleContiguousBlocks (AllocationID allocationId, AllocationType allocationType, bool staticAllocation,
                                                    uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, uint16_t blockDuration, uint8_t blocks)
{
  NS_LOG_FUNCTION (this);
  AddAllocationPeriod (allocationId, allocationType, staticAllocation, sourceAid, destAid,
                       allocationStart, blockDuration, 0, blocks);
  return (allocationStart + blockDuration * blocks);
}

void
DmgWifiScheduler::AllocateDTIAsServicePeriod (AllocationID allocationId, uint8_t sourceAid, uint8_t destAid)
{
  NS_LOG_FUNCTION (this);
  uint16_t spDuration = floor (m_dtiDuration.GetMicroSeconds () / MAX_NUM_BLOCKS);
  AddAllocationPeriod (allocationId, SERVICE_PERIOD_ALLOCATION, true, sourceAid, destAid,
                       0, spDuration, 0, MAX_NUM_BLOCKS);
}

uint32_t
DmgWifiScheduler::AddAllocationPeriod (AllocationID allocationId, AllocationType allocationType, bool staticAllocation,
                                       uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, uint16_t blockDuration,
                                       uint16_t blockPeriod, uint8_t blocks)
{
  NS_LOG_FUNCTION (this << +allocationId << allocationType << staticAllocation << +sourceAid 
                   << +destAid << allocationStart << blockDuration << blockPeriod << +blocks);
  AllocationField field;
  /* Allocation Control Field */
  field.SetAllocationID (allocationId);
  field.SetAllocationType (allocationType);
  field.SetAsPseudoStatic (staticAllocation);
  /* Allocation Field */
  field.SetSourceAid (sourceAid);
  field.SetDestinationAid (destAid);
  field.SetAllocationStart (allocationStart);
  field.SetAllocationBlockDuration (blockDuration);
  field.SetAllocationBlockPeriod (blockPeriod);
  field.SetNumberOfBlocks (blocks);
  /**
   * When scheduling two adjacent SPs, the PCP/AP should allocate the SPs separated by at least
   * aDMGPPMinListeningTime if one or more of the source or destination DMG STAs participate in both SPs.
   */
  m_allocationList.push_back (field);

  return (allocationStart + blockDuration);
}

uint32_t
DmgWifiScheduler::AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, bool isTxss)
{
  return AllocateBeamformingServicePeriod (sourceAid, destAid, allocationStart, 2000, isTxss, isTxss);
}

uint32_t
DmgWifiScheduler::AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart,
                                                    uint16_t allocationDuration, bool isInitiatorTxss, bool isResponderTxss)
{
  NS_LOG_FUNCTION (this << +sourceAid << +destAid << allocationStart << allocationDuration << isInitiatorTxss << isResponderTxss);
  AllocationField field;
  /* Allocation Control Field */
  field.SetAllocationType (SERVICE_PERIOD_ALLOCATION);
  field.SetAsPseudoStatic (false);
  /* Allocation Field */
  field.SetSourceAid (sourceAid);
  field.SetDestinationAid (destAid);
  field.SetAllocationStart (allocationStart);
  field.SetAllocationBlockDuration (allocationDuration);     // Microseconds
  field.SetNumberOfBlocks (1);

  BF_Control_Field bfField;
  bfField.SetBeamformTraining (true);
  bfField.SetAsInitiatorTxss (isInitiatorTxss);
  bfField.SetAsResponderTxss (isResponderTxss);

  field.SetBfControl (bfField);
  m_allocationList.push_back (field);

  return (allocationStart + allocationDuration + 1000); // 1000 = 1 us protection period
}

uint32_t
DmgWifiScheduler::GetAllocationListSize (void) const
{
  return m_allocationList.size ();
}

void
DmgWifiScheduler::CleanupAllocations (void)
{
  NS_LOG_FUNCTION (this);
  AllocationField allocation;
  for(AllocationFieldListI iter = m_allocationList.begin (); iter != m_allocationList.end ();)
    {
      allocation = (*iter);
      if (!allocation.IsPseudoStatic () && iter->IsAllocationAnnounced ())
        {
          iter = m_allocationList.erase (iter);
        }
      else
        {
          ++iter;
        }
    }
}

void
DmgWifiScheduler::ModifyAllocation (AllocationID allocationId, uint8_t sourceAid, uint8_t destAid, 
                                    uint32_t newStartTime, uint16_t newDuration)
{
  NS_LOG_FUNCTION (this << +allocationId << +sourceAid << +destAid << newStartTime << newDuration);
  for (AllocationFieldListI iter = m_allocationList.begin (); iter != m_allocationList.end (); iter++)
    {
      AllocationField field = (*iter);
      if ((field.GetAllocationID () == allocationId) &&
          (field.GetSourceAid () == sourceAid) && (field.GetDestinationAid () == destAid))
        {
          field.SetAllocationStart (newStartTime);
          field.SetAllocationBlockDuration (newDuration);
          break;
        }
    }
}

} // namespace ns3