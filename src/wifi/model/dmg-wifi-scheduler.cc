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
      .AddConstructor<DmgWifiScheduler> ()

      .AddAttribute ("MinBroadcastCbapDuration", "The minimum duration of a broadcast CBAP in the DTI",
                     UintegerValue (4096),
                     MakeUintegerAccessor (&DmgWifiScheduler::m_minBroadcastCbapDuration),
                     MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("InterAllocationDistance", "The time distance between two adjacent allocations "
                     "This distance will be allocated as broadcast CBAP",
                     UintegerValue (0),
                     MakeUintegerAccessor (&DmgWifiScheduler::m_interAllocationDistance),
                     MakeUintegerChecker<uint32_t> (0, 65535))
  ;
  return tid;
}

DmgWifiScheduler::DmgWifiScheduler ()
{
  NS_LOG_FUNCTION (this);
  m_isAddtsAccepted = false;
  m_isAllocationModified = false;
  m_isNonStatic = false;
  m_isDeltsReceived = false;
  m_guardTime = GUARD_TIME.GetMicroSeconds ();
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
  isConnected = m_mac->TraceConnectWithoutContext ("DTIStarted", MakeCallback (&DmgWifiScheduler::DataTransferIntervalStarted, this));
  NS_ASSERT_MSG (isConnected, "Connection to Trace DTIStarted failed.");
  isConnected = m_mac->TraceConnectWithoutContext ("DELTSReceived", MakeCallback (&DmgWifiScheduler::ReceiveDeltsRequest, this));
  NS_ASSERT_MSG (isConnected, "Connection to Trace DELTSReceived failed.");
}

AllocationFieldList
DmgWifiScheduler::GetAllocationList (void)
{
  /* Return the global allocation list */
  return m_globalAllocationList;
}

void
DmgWifiScheduler::SetAllocationList (AllocationFieldList allocationList)
{
  m_globalAllocationList = allocationList;
}

void
DmgWifiScheduler::SetAllocationsAnnounced (void)
{
  for (AllocationFieldListI iter = m_addtsAllocationList.begin (); iter != m_addtsAllocationList.end (); ++iter)
    {
      iter->SetAllocationAnnounced (); 
    }
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

  if (m_atiDuration.IsStrictlyPositive ())
    {
      Simulator::Schedule (m_bhiDuration - m_atiDuration - m_mac->GetMbifs (), 
                           &DmgWifiScheduler::AnnouncementTransmissionIntervalStarted, this);
    }
  Simulator::Schedule (m_biDuration, &DmgWifiScheduler::BeaconIntervalEnded, this);
}

void
DmgWifiScheduler::AnnouncementTransmissionIntervalStarted (void)
{
  NS_LOG_INFO ("ATI started at " << Simulator::Now ());
  m_atiStartTime = Simulator::Now ();
  m_accessPeriod = CHANNEL_ACCESS_ATI;
}

void 
DmgWifiScheduler::DataTransferIntervalStarted (Mac48Address address, Time dtiDuration)
{
  NS_LOG_INFO ("DTI started at " << Simulator::Now ());
  m_dtiStartTime = Simulator::Now ();
  m_dtiDuration = dtiDuration;
  m_accessPeriod = CHANNEL_ACCESS_DTI;
}

void
DmgWifiScheduler::BeaconIntervalEnded (void)
{
  NS_LOG_INFO ("Beacon Interval ended at " << Simulator::Now ());
  /* Cleanup non-static allocations */
  CleanupAllocations ();
  /* Update start and remaining DTI times
   * This update here takes into account the possible cleanup of non-static allocations
   * and the possible reception of DELTS requests
   */
  UpdateStartandRemainingTime ();
  /* Do something with the ADDTS requests received in the last DTI (if any) */
  if (!m_receiveAddtsRequests.empty ())
    {
      /* At least one ADDTS request has been received */
      ManageAddtsRequests ();
      /* Start and remaining DTI times are updated according to the allocated requests */
    }
  AddBroadcastCbap ();
  m_mac->StartBeaconInterval ();
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
      for (AllocationFieldListI iter = m_addtsAllocationList.begin (); iter != m_addtsAllocationList.end ();)
        {
          allocation = (*iter);
          if ((allocation.GetAllocationID () == info.GetAllocationID ()) &&
              (allocation.GetSourceAid () == stationAid) &&
              (allocation.GetDestinationAid () == info.GetDestinationAid ()))
            {
              m_isDeltsReceived = true;
              iter = m_addtsAllocationList.erase (iter);
              /* Adjust the other allocations in addtsAllocationList */
              AdjustExistingAllocations (iter, allocation);
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
              m_isAllocationModified = true;
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
              m_isAddtsAccepted = true;
            }
        }
      SendAddtsResponse (iter->sourceAddr, status, dmgTspec);
      /* Update remaining time and start time */  
    }
  /* clear list of received ADDTS requests */
  m_receiveAddtsRequests.clear (); 
}

void
DmgWifiScheduler::UpdateStartandRemainingTime (void)
{
  if (m_addtsAllocationList.empty ())
    {
      /* No existing allocations */
      m_schedulingTime.startTime = 0;
      m_schedulingTime.remainingDtiTime = m_dtiDuration.GetMicroSeconds ();
    }
  else
    {
      /* At least one allocation. Get last one */
      AllocationField lastAllocation = m_addtsAllocationList.back ();
      m_schedulingTime.startTime = lastAllocation.GetAllocationStart () + lastAllocation.GetAllocationBlockDuration () + m_guardTime;
      m_schedulingTime.remainingDtiTime = m_dtiDuration.GetMicroSeconds () - m_schedulingTime.startTime - m_guardTime;
    }
}

void
DmgWifiScheduler::AdjustExistingAllocations (AllocationFieldListI iter, AllocationField removedAlloc)
{
  for (AllocationFieldListI nextAlloc = iter; nextAlloc != m_addtsAllocationList.end (); ++nextAlloc)
    {
      nextAlloc->SetAllocationStart (nextAlloc->GetAllocationStart () - removedAlloc.GetAllocationBlockDuration () - m_guardTime);
    }
}

uint32_t
DmgWifiScheduler::GetAllocationDuration (uint32_t minAllocation, uint32_t maxAllocation)
{
  return ((minAllocation + maxAllocation) / 2);
}

StatusCode
DmgWifiScheduler::AddNewAllocation (uint8_t sourceAid, DmgTspecElement dmgTspec, DmgAllocationInfo info)
{
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
      NS_LOG_WARN ("Allocation Format not supported");
    }    

  if (allocDuration <= (m_schedulingTime.remainingDtiTime - m_minBroadcastCbapDuration)) // broadcast CBAP must be present
    {
      m_schedulingTime.startTime = AllocateSingleContiguousBlock (info.GetAllocationID (), info.GetAllocationType (), info.IsPseudoStatic (),
                                                                  sourceAid, info.GetDestinationAid (), m_schedulingTime.startTime, allocDuration);
      m_schedulingTime.remainingDtiTime -= (allocDuration + m_guardTime);
      status.SetSuccess ();
    }
  else if ((info.GetAllocationFormat () == ISOCHRONOUS) && // check Minimum Allocation for Isochronous request
           (dmgTspec.GetMinimumAllocation () <= (m_schedulingTime.remainingDtiTime - m_minBroadcastCbapDuration))) 
    {
      m_schedulingTime.startTime = AllocateSingleContiguousBlock (info.GetAllocationID (), info.GetAllocationType (), info.IsPseudoStatic (),
                                                                  sourceAid, info.GetDestinationAid (), m_schedulingTime.startTime, 
                                                                  dmgTspec.GetMinimumAllocation ());
      m_schedulingTime.remainingDtiTime -= (dmgTspec.GetMinimumAllocation () + m_guardTime);
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
DmgWifiScheduler::ModifyExistingAllocation (uint8_t sourceAid, DmgTspecElement dmgTspec, DmgAllocationInfo info)
{
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
      NS_LOG_WARN ("Allocation Format not supported");
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
  uint32_t timeDifference;
  if (newDuration > currentDuration)
    {
      timeDifference = newDuration - currentDuration;
      if (timeDifference <= (m_schedulingTime.remainingDtiTime - m_minBroadcastCbapDuration))
        {
          allocation->SetAllocationBlockDuration (newDuration);
          m_schedulingTime.remainingDtiTime -= timeDifference;
          status.SetSuccess ();
          goto reorderList;
        }
      if ((info.GetAllocationFormat () == ISOCHRONOUS) && (dmgTspec.GetMinimumAllocation () > currentDuration))
        {
          timeDifference = dmgTspec.GetMinimumAllocation () - currentDuration;
          if (timeDifference <= (m_schedulingTime.remainingDtiTime - m_minBroadcastCbapDuration))
            {
              allocation->SetAllocationBlockDuration (dmgTspec.GetMinimumAllocation ());
              m_schedulingTime.remainingDtiTime -= timeDifference;
              status.SetSuccess ();
            }
          goto reorderList;
        }
      status.SetFailure ();
      goto doNothing; // The request is not accepted; maintaining old allocation duration
    }
  else
    {
      timeDifference = currentDuration - newDuration;
      allocation->SetAllocationBlockDuration (newDuration);
      m_schedulingTime.remainingDtiTime += timeDifference;
      status.SetSuccess ();
    }
    uint32_t newStartTime;
  reorderList:  
    newStartTime = allocation->GetAllocationStart () + allocation->GetAllocationBlockDuration () + m_guardTime;
    for (AllocationFieldListI iter = ++allocation; iter != m_addtsAllocationList.end (); ++iter)
      {
        iter->SetAllocationStart (newStartTime);   
        newStartTime += iter->GetAllocationBlockDuration () + m_guardTime;
      }
    m_schedulingTime.startTime = newStartTime;
    return status;
  doNothing:
    /* no need to update next start time */ 
    return status;
}

void
DmgWifiScheduler::AddBroadcastCbap (void)
{
  if (m_addtsAllocationList.empty ())
    {
      /* No allocations granted with an ADDTS request are present 
       * The entire DTI is allocated as CBAP broadcast (CbapOnly field)
       * The global allocation list is emptied
       */
      m_globalAllocationList.clear ();
      m_isNonStatic = false;
      m_isDeltsReceived = false;
      return;
    }
  if (m_isAddtsAccepted || m_isAllocationModified || m_isNonStatic || m_isDeltsReceived)
    {
      AddBroadcastCbapAllocations (); 
      m_isAddtsAccepted = false;
      m_isAllocationModified = false;
      m_isNonStatic = false;
      m_isDeltsReceived = false;
    }  
}

void 
DmgWifiScheduler::AddBroadcastCbapAllocations (void)
{
  uint32_t totalBroadcastCbapTime = 0;
  /* Addts allocation list is copied to the Global allocation list */
  m_globalAllocationList = m_addtsAllocationList;
  AllocationFieldList broadcastCbapList;
  uint32_t start, nextStart;
  AllocationFieldListI iter = m_globalAllocationList.begin ();
  AllocationFieldListI nextIter = iter + 1;
  while (nextIter != m_globalAllocationList.end ())
    {
      start = iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime;
      nextStart = nextIter->GetAllocationStart () + m_guardTime;
      if ((m_schedulingTime.remainingDtiTime >= m_interAllocationDistance)
          && (m_interAllocationDistance > 0)) // here the decision to place a broadcast CBAP among allocated requests
        {
          broadcastCbapList = GetBroadcastCbapAllocation (true, start, m_interAllocationDistance);
          iter = m_globalAllocationList.insert (nextIter, broadcastCbapList.begin (), broadcastCbapList.end ());
          iter += broadcastCbapList.size ();
          iter->SetAllocationStart (nextStart + m_interAllocationDistance);
          nextIter = iter + 1;
          totalBroadcastCbapTime += m_interAllocationDistance;
          m_schedulingTime.remainingDtiTime -= (m_interAllocationDistance + m_guardTime);
        }
      else
        {
          ++iter;
          ++nextIter;
        }
    }
  /* iter points to the last element of Global allocation list (i.e. last element of Addts allocation list)
   * Check the presence of remaining DTI time to be allocated as broadcast CBAP
   */
  start = iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime;
  if (m_schedulingTime.remainingDtiTime > m_guardTime)
    {
      broadcastCbapList = GetBroadcastCbapAllocation (true, start, m_schedulingTime.remainingDtiTime - m_guardTime);
      iter = m_globalAllocationList.insert (m_globalAllocationList.end (), broadcastCbapList.begin (), broadcastCbapList.end ());
      iter += broadcastCbapList.size () - 1;
      totalBroadcastCbapTime += m_schedulingTime.remainingDtiTime;
    }
  /* Check if at least one broadcast CBAP is present */
  NS_ASSERT_MSG ((totalBroadcastCbapTime >= m_minBroadcastCbapDuration),
                 "The overall broadcast CBAP time needed is " << m_minBroadcastCbapDuration);
  /* Check DTI fully allocated */
  NS_ASSERT_MSG ((iter->GetAllocationStart () + iter->GetAllocationBlockDuration () + m_guardTime) == m_dtiDuration.GetMicroSeconds (),
                 "The DTI is not totally allocated");
}

AllocationFieldList
DmgWifiScheduler::GetBroadcastCbapAllocation (bool staticAllocation, uint32_t allocationStart, uint32_t blockDuration)
{
  uint16_t numberCbapBlocks = blockDuration / MAX_CBAP_BLOCK_DURATION;
  uint16_t lastCbapLegnth = blockDuration % MAX_CBAP_BLOCK_DURATION;
  AllocationFieldList list;
  AllocationField field;
  field.SetAllocationID (0);
  field.SetAllocationType (CBAP_ALLOCATION);
  field.SetAsPseudoStatic (staticAllocation);
  field.SetSourceAid (AID_BROADCAST);
  field.SetDestinationAid (AID_BROADCAST);
  field.SetAllocationBlockPeriod (0);
  field.SetNumberOfBlocks (1);

  for (uint16_t i = 0; i < numberCbapBlocks; ++i)
    {
      field.SetAllocationStart (allocationStart);
      field.SetAllocationBlockDuration (MAX_CBAP_BLOCK_DURATION);
      allocationStart += MAX_CBAP_BLOCK_DURATION;
      list.push_back (field);
    }
  if (lastCbapLegnth > 0)
    {
      field.SetAllocationStart (allocationStart);
      field.SetAllocationBlockDuration (lastCbapLegnth);
      list.push_back (field);
    }

  return list;
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
  m_addtsAllocationList.push_back (field);

  return (allocationStart + blockDuration + m_guardTime);
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
  m_addtsAllocationList.push_back (field);

  return (allocationStart + allocationDuration + 1000); // 1000 = 1 us protection period
}

uint32_t
DmgWifiScheduler::GetAllocationListSize (void) const
{
  /* return size of the global allocation list */
  return m_globalAllocationList.size ();
}

void
DmgWifiScheduler::CleanupAllocations (void)
{
  NS_LOG_FUNCTION (this);
  AllocationField allocation;
  AllocatedRequestMapI it;
  for (AllocationFieldListI iter = m_addtsAllocationList.begin (); iter != m_addtsAllocationList.end ();)
    {
      allocation = (*iter);
      if (!allocation.IsPseudoStatic () && allocation.IsAllocationAnnounced ())
        {
          m_isNonStatic = true;
          it = m_allocatedAddtsRequests.find (UniqueIdentifier (allocation.GetAllocationID (),
                                                                allocation.GetSourceAid (), allocation.GetDestinationAid ()));
          if (it != m_allocatedAddtsRequests.end ())
            {
              m_allocatedAddtsRequests.erase (it);
            }
          iter = m_addtsAllocationList.erase (iter);
          AdjustExistingAllocations (iter, allocation);
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