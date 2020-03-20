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
                      .AddAttribute ("BroadcastCbapDuration", "The duration of a Broadcast CBAP allocation."
                       UintegerValue (2528),
                       MakeUintegerAccessor (&DmgWifiScheduler::m_broadcastCbapDuration),
                       MakeUintegerChecker<uint32_t> ())
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
  m_receiveAddtsRequests.clear ();
  m_allocatedAddtsRequests.clear ();
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
  bool isConnected;
  isConnected = m_mac->TraceConnectWithoutContext ("ADDTSReceived", MakeCallback (&DmgWifiScheduler::ReceiveAddtsRequest, this));
  NS_ASSERT_MSG (isConnected, "Connection to Trace ADDTSReceived failed.");
  isConnected = m_mac->TraceConnectWithoutContext ("BIStarted", MakeCallback (&DmgWifiScheduler::BeaconIntervalStarted, this));
  NS_ASSERT_MSG (isConnected, "Connection to Trace BIStarted failed.");
  isConnected = m_mac->TraceConnectWithoutContext ("DELTSReceived", MakeCallback (&DmgWifiScheduler::ReceiveDeltsRequest, this));
  NS_ASSERT_MSG (isConnected, "Connection to Trace DELTSReceived failed.");
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

TsDelayElement
DmgWifiScheduler::GetTsDelayElement (void)
{
  TsDelayElement element;
  element.SetDelay (1);
  return element;
}

void 
DmgWifiScheduler::BeaconIntervalStarted (Mac48Address address, Time biDuration, Time bhiDuration, Time atiDuration)
{
  NS_LOG_INFO ("Beacon Interval started at " << Simulator::Now ());
  m_biStartTime = Simulator::Now ();
  m_accessPeriod = CHANNEL_ACCESS_BHI;
  m_biDuration = biDuration;
  m_bhiDuration = bhiDuration;
  m_atiDuration = atiDuration;
  m_dtiDuration = m_biDuration - m_bhiDuration;
  if (m_atiDuration.IsStrictlyPositive ())
    {
      Simulator::Schedule (m_bhiDuration - m_atiDuration - m_mac->GetMbifs (), 
                           &DmgWifiScheduler::AnnouncementTransmissionIntervalStarted, this);
    }
  else
    {
      Simulator::Schedule (m_bhiDuration, &DmgWifiScheduler::DataTransferIntervalStarted, this);
    }
}

void
DmgWifiScheduler::AnnouncementTransmissionIntervalStarted (void)
{
  NS_LOG_INFO ("ATI started at " << Simulator::Now ());
  m_atiStartTime = Simulator::Now ();
  m_accessPeriod = CHANNEL_ACCESS_ATI;
  Simulator::Schedule (m_atiDuration, &DmgWifiScheduler::DataTransferIntervalStarted, this);
}

void 
DmgWifiScheduler::DataTransferIntervalStarted (void)
{
  NS_LOG_INFO ("DTI started at " << Simulator::Now ());
  m_dtiStartTime = Simulator::Now ();
  m_accessPeriod = CHANNEL_ACCESS_DTI;
  Simulator::Schedule (m_dtiDuration, &DmgWifiScheduler::BeaconIntervalEnded, this);
}

void
DmgWifiScheduler::BeaconIntervalEnded (void)
{
  NS_LOG_INFO ("Beacon Interval ended at " << Simulator::Now ());
  /* Cleanup non-static allocations */
  CleanupAllocations ();
  /* Do something with the ADDTS requests received in the last DTI (if any) */
  if (!m_receiveAddtsRequests.empty ())
    {
      /* Remove Broadcast CBAP allocations */
      // RemoveBroadcastCbapAllocations ();
      /* At least one ADDTS request has been received */
      ManageAddtsRequests (); 
    }
  /* Add Broadcast CBAP allocations */
  // AddBroadcastCbapAllocations ();
}

void
DmgWifiScheduler::ReceiveDeltsRequest (Mac48Address address, DmgAllocationInfo info)
{
  NS_LOG_INFO ("Receive DELTS request from " << address);
  uint8_t stationAid = m_mac->GetStationAid (address);
  /* Check whether this allocation has been previously allocated */
  AllocatedRequestMapI it = m_allocatedAddtsRequests.find (UniqueIdentifier (info.GetAllocationID (), 
                                                                             stationAid, info.GetDestinationAid ()));
  if (it != m_allocatedAddtsRequests.end ())
    {
      /* Delete allocation from m_allocatedAddtsRequests and m_allocationList */
      m_allocatedAddtsRequests.erase (it);
      AllocationField allocation;
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
  else
    {
      /* The allocation does not exist */
      NS_LOG_DEBUG ("Cannot find the allocation");
    }
}

void
DmgWifiScheduler::ReceiveAddtsRequest (Mac48Address address, DmgTspecElement element)
{
  NS_LOG_INFO ("Receive ADDTS request from " << address);
  /* Store the ADDTS request received in the current DTI */
  AddtsRequest request;
  request.sourceAid = m_mac->GetStationAid (address);
  request.sourceAddr = address;
  request.dmgTspec = element;
  m_receiveAddtsRequests.push_back (request);
}

void
DmgWifiScheduler::SendAddtsResponse (Mac48Address address, StatusCode status, DmgTspecElement dmgTspec)
{
  NS_LOG_INFO ("Send ADDTS response to " << address);
  TsDelayElement tsDelay;
  switch (status.GetStatusCodeValue ())
    {
      case STATUS_CODE_SUCCESS:
      case STATUS_CODE_FAILURE:
      case STATUS_CODE_REJECTED_WITH_SUGGESTED_CHANGES:
      case STATUS_CODE_REJECT_WITH_SCHEDULE:
        break;
      case STATUS_CODE_REJECTED_FOR_DELAY_PERIOD:
        tsDelay = GetTsDelayElement ();
        break;
      case STATUS_CODE_PENDING_ADMITTING_FST_SESSION:
      case STATUS_CODE_PERFORMING_FST_NOW:
      case STATUS_CODE_PENDING_GAP_IN_BA_WINDOW:
      case STATUS_CODE_DENIED_WITH_SUGGESTED_BAND_AND_CHANNEL:
      case STATUS_CODE_DENIED_DUE_TO_SPECTRUM_MANAGEMENT:
        break;
      default: 
        NS_FATAL_ERROR ("ADDTS response status code not supported");
    }
  /* Send ADDTS response to the source STA of the allocation */
  m_mac->SendDmgAddTsResponse (address, status, tsDelay, dmgTspec);
}

void
DmgWifiScheduler::ManageAddtsRequests (void)
{
  /* Manage the ADDTS requests received in the last DTI.
   * Implementation of admission policies for IEEE 802.11ad.
   * Channel access organization during the DTI.
   */
  NS_LOG_FUNCTION (this);

  DmgTspecElement dmgTspec;
  DmgAllocationInfo info;
  StatusCode status;
  UniqueIdentifier allocIdentifier;
  AllocatedRequestMapI it;
  /* Cycle over the list of received ADDTS requests; remainingDtiTime is updated each time an allocation is accepted.
   * Once all requests have been evaluated (accepted or rejected):
   * allocate remainingDtiTime (if > 0) as CBAP with destination & source AID to Broadcast */
  for (AddtsRequestListCI iter = m_receiveAddtsRequests.begin (); iter != m_receiveAddtsRequests.end (); iter++)
    {
      dmgTspec = iter->dmgTspec;
      info = dmgTspec.GetDmgAllocationInfo ();
      allocIdentifier = UniqueIdentifier (info.GetAllocationID (), iter->sourceAid, info.GetDestinationAid ());
      it = m_allocatedAddtsRequests.find (allocIdentifier);
      if (it != m_allocatedAddtsRequests.end ())
        {
          /* Requesting modification of an existing allocation */
          status = ModifyExistingAllocation (iter->sourceAid, dmgTspec, info);
          if (status.IsSuccess ())
            {
              /* The modification request has been accepted
               * Replace the accepted ADDTS request in the allocated requests */
              m_allocatedAddtsRequests.at (it->first) = (*iter);
            }
        }
      else
        {
          /* Requesting new allocation */
          status = AddNewAllocation (iter->sourceAid, dmgTspec, info);
          if (status.IsSuccess ())
            {
              /* The new request has been accepted
               * Save the accepted ADDTS request among the allocated requests */
              m_allocatedAddtsRequests.insert (std::make_pair (allocIdentifier, (*iter)));
            }
        }
      SendAddtsResponse (iter->sourceAddr, status, dmgTspec);
      /* Update remaining time and start time */  
    }
  /* clear list of received ADDTS requests */
  m_receiveAddtsRequests.clear (); 
}

uint32_t
DmgWifiScheduler::GetAllocationDuration (uint32_t minAllocation, uint32_t maxAllocation)
{
  return (minAllocation + maxAllocation) / 2;
}

StatusCode
DmgWifiScheduler::AddNewAllocation (uint8_t sourceAid, DmgTspecElement dmgTspec, DmgAllocationInfo info)
{
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
      NS_LOG_WARN ("Allocation Format not supported");
    }      
  return status;
}

StatusCode
DmgWifiScheduler::ModifyExistingAllocation (uint8_t sourceAid, DmgTspecElement dmgTspec, DmgAllocationInfo info)
{
  StatusCode status;
  return status;
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
  for (AllocationFieldListI iter = m_addtsAllocationList.begin (); iter != m_addtsAllocationList.end (); iter++)
    {
      if ((iter->GetAllocationID () == allocationId) &&
          (iter->GetSourceAid () == sourceAid) && (iter->GetDestinationAid () == destAid))
        {
          iter->SetAllocationStart (newStartTime);
          iter->SetAllocationBlockDuration (newDuration);
          break;
        }
    }
}

} // namespace ns3