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
  : m_isAddtsAccepted (false),
    m_isAllocationModified (false),
    m_isNonStaticRemoved (false),
    m_isDeltsReceived (false),
    m_guardTime (GUARD_TIME.GetMicroSeconds ())
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
  m_allocatedAddtsRequestMap.clear ();
  m_addtsAllocationList.clear ();
  m_allocationList.clear ();
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
  /* Creates an ESE of MAX permitted size to be used for 
   * calculation of the BTI duration
   */
  CreateFullExtentdedScheduleElement ();
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

void
DmgWifiScheduler::CreateFullExtentdedScheduleElement (void)
{
  m_fullEse = Create<ExtendedScheduleElement> ();
  uint16_t maxLength = 17;
  AllocationField field;
  for (uint16_t i = 0; i < maxLength; ++i)
  {
    m_fullEse->AddAllocationField (field);
  }
}

Ptr<ExtendedScheduleElement>
DmgWifiScheduler::GetFullExtendedScheduleElement (void)
{
  return m_fullEse;
}

AllocationFieldList
DmgWifiScheduler::GetAllocationList (void)
{
  return m_allocationList;
}

void
DmgWifiScheduler::SetAllocationList (const AllocationFieldList &allocationList)
{
  m_allocationList = allocationList;
}

void
DmgWifiScheduler::SetAllocationsAnnounced (void)
{
  NS_LOG_FUNCTION (this);
  for (AllocationFieldListI iter = m_addtsAllocationList.begin (); iter != m_addtsAllocationList.end (); ++iter)
    {
      iter->SetAllocationAnnounced (); 
    }
}

TsDelayElement
DmgWifiScheduler::GetTsDelayElement (void)
{
  NS_LOG_FUNCTION (this);
  /* TODO: Send a TS delay element based on the presence of non-static allocation (if any) */
  TsDelayElement element;
  element.SetDelay (1);
  return element;
}

void 
DmgWifiScheduler::BeaconIntervalStarted (Mac48Address address, Time biDuration, Time bhiDuration, Time atiDuration)
{
  NS_LOG_INFO ("Beacon Interval started at " << Simulator::Now ());
  m_currentAccessPeriodStartTime = Simulator::Now ();
  m_currentAccessPeriod = CHANNEL_ACCESS_BHI;
  m_biDuration = biDuration;
  m_atiDuration = atiDuration;

  if (m_atiDuration.IsStrictlyPositive ())
    {
      Simulator::Schedule (bhiDuration - atiDuration - m_mac->GetMbifs (), 
                           &DmgWifiScheduler::AnnouncementTransmissionIntervalStarted, this);
    }
}

void
DmgWifiScheduler::AnnouncementTransmissionIntervalStarted (void)
{
  NS_LOG_INFO ("ATI started at " << Simulator::Now ());
  m_currentAccessPeriodStartTime = Simulator::Now ();
  m_currentAccessPeriod = CHANNEL_ACCESS_ATI;
}

void 
DmgWifiScheduler::DataTransferIntervalStarted (Mac48Address address, Time dtiDuration)
{
  NS_LOG_INFO ("DTI started at " << Simulator::Now ());
  m_currentAccessPeriodStartTime = Simulator::Now ();
  m_dtiDuration = dtiDuration;
  m_currentAccessPeriod = CHANNEL_ACCESS_DTI;
}

void
DmgWifiScheduler::BeaconIntervalEnded (void)
{
  NS_LOG_INFO ("Beacon Interval ended at " << Simulator::Now ());
  /* Cleanup non-static allocations */
  CleanupAllocations ();
  /* Allocate broadcast CBAPs */
  AddBroadcastCbap ();
}

void
DmgWifiScheduler::ReceiveDeltsRequest (Mac48Address address, DmgAllocationInfo info)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Receive DELTS request from " << address);
  uint8_t sourceAid = m_mac->GetStationAid (address);
  /* Check whether this allocation has been previously allocated */
  AllocatedRequestMapI it = m_allocatedAddtsRequestMap.find (UniqueIdentifier (info.GetAllocationID (), 
                                                                             sourceAid, info.GetDestinationAid ()));
  if (it != m_allocatedAddtsRequestMap.end ())
    {
      /* Delete allocation from m_allocatedAddtsRequestMap and m_allocationList */
      m_allocatedAddtsRequestMap.erase (it);
      AllocationField allocation;
      for (AllocationFieldListI iter = m_addtsAllocationList.begin (); iter != m_addtsAllocationList.end ();)
        {
          allocation = (*iter);
          if ((allocation.GetAllocationID () == info.GetAllocationID ()) &&
              (allocation.GetSourceAid () == sourceAid) &&
              (allocation.GetDestinationAid () == info.GetDestinationAid ()))
            {
              m_isDeltsReceived = true;
              iter = m_addtsAllocationList.erase (iter);
              /* Adjust the other allocations in addtsAllocationList */
              AdjustExistingAllocations (iter, allocation.GetAllocationBlockDuration () + m_guardTime, false);
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
      NS_LOG_WARN ("Cannot find the allocation with ID: " << info.GetAllocationID ()
                   << " Source AID: " << sourceAid << " Destination AID: " << info.GetDestinationAid ());
    }
}

void
DmgWifiScheduler::ReceiveAddtsRequest (Mac48Address address, DmgTspecElement element)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Received ADDTS request from " << address);
  /* Update start and remaining DTI for the new ADDTS request to evaluate */
  UpdateStartAndRemainingTime ();
  /* Evaluate ADDTS request as soon as it is received */
  ManageAddtsRequests (m_mac->GetStationAid (address), address, element);
}

void
DmgWifiScheduler::SendAddtsResponse (const Mac48Address &address, const StatusCode &status, DmgTspecElement &dmgTspec)
{
  NS_LOG_FUNCTION (this);
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
        NS_FATAL_ERROR ("ADDTS response status code = " << status.GetStatusCodeValue () << " not supported");
    }
  /* Send ADDTS response to the source STA of the allocation */
  m_mac->SendDmgAddTsResponse (address, status, tsDelay, dmgTspec);
}

void
DmgWifiScheduler::ManageAddtsRequests (uint8_t sourceAid, const Mac48Address &sourceAddr, const DmgTspecElement &dmgTspec)
{
  NS_LOG_FUNCTION (this);

  StatusCode status;
  AddtsRequest request;
  request.sourceAid = sourceAid;
  request.sourceAddr = sourceAddr;
  request.dmgTspec = dmgTspec;
  DmgAllocationInfo info = dmgTspec.GetDmgAllocationInfo ();
  UniqueIdentifier allocIdentifier = UniqueIdentifier (info.GetAllocationID (), sourceAid, info.GetDestinationAid ());
  AllocatedRequestMapI it = m_allocatedAddtsRequestMap.find (allocIdentifier);

  if (it != m_allocatedAddtsRequestMap.end ())
    {
      /* Requesting modification of an existing allocation */
      status = ModifyExistingAllocation (sourceAid, dmgTspec, info);
      if (status.IsSuccess ())
        {
          /* The modification request has been accepted
           * Replace the accepted ADDTS request in the allocated requests */
          m_allocatedAddtsRequestMap.at (it->first) = request;
          m_isAllocationModified = true;
        }
    }
  else
    {
      /* Requesting new allocation */
      status = AddNewAllocation (sourceAid, dmgTspec, info);
      if (status.IsSuccess ())
        {
          /* The new request has been accepted
           * Save the accepted ADDTS request among the allocated requests */
          m_allocatedAddtsRequestMap.insert (std::make_pair (allocIdentifier, request));
          m_isAddtsAccepted = true;
        }
    }
  /* Send ADDTS response to source STA */
  if (sourceAid != AID_AP)
    {
      SendAddtsResponse (sourceAddr, status, request.dmgTspec);
    }
  /* Send ADDTS response to destination STA */
  if (info.GetDestinationAid () != AID_AP)
    {
      SendAddtsResponse (m_mac->GetStationAddress (info.GetDestinationAid ()), status, request.dmgTspec);
    }   
}

void
DmgWifiScheduler::AddBroadcastCbap (void)
{
  NS_LOG_FUNCTION (this);
  if (m_addtsAllocationList.empty ())
    {
      NS_LOG_DEBUG ("No Addts allocations. Entire DTI as CBAP");
      /* No allocations granted with an ADDTS request are present 
       * The entire DTI is allocated as CBAP broadcast (CbapOnly field)
       * The allocation list is emptied
       */
      m_allocationList.clear ();
      m_isNonStaticRemoved = false;
      m_isDeltsReceived = false;
      return;
    }
  if (m_isAddtsAccepted || m_isAllocationModified || m_isNonStaticRemoved || m_isDeltsReceived)
    {
      /* Update start and remaining DTI times
       * This update here takes into account the possible cleanup of non-static allocations
       * and the possible reception of DELTS requests
       */
      UpdateStartAndRemainingTime ();
      AddBroadcastCbapAllocations (); 
      m_isAddtsAccepted = false;
      m_isAllocationModified = false;
      m_isNonStaticRemoved = false;
      m_isDeltsReceived = false;
    }  
}

AllocationFieldList
DmgWifiScheduler::GetBroadcastCbapAllocation (bool staticAllocation, uint32_t allocationStart, uint32_t blockDuration)
{
  NS_LOG_FUNCTION (this);
  uint16_t numberCbapBlocks = blockDuration / MAX_CBAP_BLOCK_DURATION;
  uint16_t lastCbapLength = blockDuration % MAX_CBAP_BLOCK_DURATION;
  AllocationFieldList list;
  AllocationField field;
  field.SetAllocationID (BROADCAST_CBAP);
  field.SetAllocationType (CBAP_ALLOCATION);
  field.SetAsPseudoStatic (staticAllocation);
  field.SetSourceAid (AID_BROADCAST);
  field.SetDestinationAid (AID_BROADCAST);
  field.SetAllocationBlockPeriod (0);
  field.SetNumberOfBlocks (1);

  for (uint16_t i = 0; i < numberCbapBlocks; ++i)
    {
      field.SetAllocationStart (allocationStart);
      field.SetAllocationBlockDuration (MAX_CBAP_BLOCK_DURATION - m_guardTime);
      allocationStart += (MAX_CBAP_BLOCK_DURATION);
      list.push_back (field);
    }
  if (lastCbapLength > m_guardTime) // guard time before the end of DTI
    {
      field.SetAllocationStart (allocationStart);
      field.SetAllocationBlockDuration (lastCbapLength - m_guardTime);
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
  return AddAllocationPeriod (allocationId, allocationType, staticAllocation, sourceAid, destAid,
                               allocationStart, blockDuration, 0, 1);
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
DmgWifiScheduler::AllocateDtiAsServicePeriod (AllocationID allocationId, uint8_t sourceAid, uint8_t destAid)
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
  /* return size of the  allocation list */
  return m_allocationList.size ();
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
          m_isNonStaticRemoved = true;
          it = m_allocatedAddtsRequestMap.find (UniqueIdentifier (allocation.GetAllocationID (),
                                                                allocation.GetSourceAid (), allocation.GetDestinationAid ()));
          if (it != m_allocatedAddtsRequestMap.end ())
            {
              m_allocatedAddtsRequestMap.erase (it);
            }
          iter = m_addtsAllocationList.erase (iter);
          AdjustExistingAllocations (iter, allocation.GetAllocationBlockDuration () + m_guardTime, false);
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