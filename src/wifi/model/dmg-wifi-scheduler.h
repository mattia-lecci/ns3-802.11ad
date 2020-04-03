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

#ifndef DMG_WIFI_SCHEDULER_H
#define DMG_WIFI_SCHEDULER_H

#include <ns3/traced-callback.h>

#include "dmg-capabilities.h"
#include "dmg-information-elements.h"
#include "dmg-wifi-mac.h"

namespace ns3 {

class DmgApWifiMac;

/**
 * \brief Abstract scheduling features for IEEE 802.11ad
 *
 * This abstract class provides the implementation of scheduling features for
 * IEEE 802.11ad. In particular, this class organizes the medium access 
 * according to the availability of contention-free access periods (SPs)
 * and contention-based access periods (CBAPs) as provided by the 802.11ad amendment.
 */
class DmgWifiScheduler : public Object
{
public:
  static TypeId GetTypeId (void);

  DmgWifiScheduler ();
  virtual ~DmgWifiScheduler ();
  void Initialize (void);
  /**
   * \param mac The MAC layer connected with the scheduler.
   */
  void SetMac (Ptr<DmgApWifiMac> mac);
  /**
   * Allocate CBAP period to be announced in DMG Beacon or Announce Frame.
   * \param staticAllocation Is the allocation static.
   * \param allocationStart The start time in microseconds of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period in microseconds.
   * \return The start time in microseconds of the next allocation period.
   */
  uint32_t AllocateCbapPeriod (bool staticAllocation, uint32_t allocationStart, uint16_t blockDuration);
  /**
   * Add a new allocation with one single block. The duration of the block is limited to 32 767 microseconds for an SP allocation.
   * and to 65 535 microseconds for a CBAP allocation. The allocation is announced in the following DMG Beacon or Announce Frame.
   * \param allocationId The unique identifier for the allocation.
   * \param allocationType The type of the allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time in microseconds of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period in microseconds.
   * \return The start time in microseconds of the next allocation period.
   */
  uint32_t AllocateSingleContiguousBlock (AllocationID allocationId, AllocationType allocationType, bool staticAllocation,
                                          uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, uint16_t blockDuration);
  /**
   * Add a new allocation consisting of consectuive allocation blocks.
   * The allocation is announced in the following DMG Beacon or Announce Frame.
   * \param allocationId The unique identifier for the allocation.
   * \param allocationType The type of the allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time in microseconds of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period in microseconds.
   * \param blocks The number of blocks making up the allocation.
   * \return The start time in microseconds of the next allocation period.
   */
  uint32_t AllocateMultipleContiguousBlocks (AllocationID allocationId, AllocationType allocationType, bool staticAllocation,
                                             uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, uint16_t blockDuration, uint8_t blocks);
  /**
   * Allocate SP allocation for Beamforming training.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time in microseconds of the allocation relative to the beginning of DTI.
   * \param isTxss Is the Beamforming TxSS or RxSS.
   * \return The start time in microseconds of the next allocation period.
   */
  uint32_t AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, bool isTxss);

protected:
  friend class DmgApWifiMac;

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  /**
   * Handle the end of the BI.
   */
  virtual void BeaconIntervalEnded (void);
  /**
   * Handle an ADDTS request received at the PCP/AP.
   * \param address The MAC address of the source STA.
   * \param element The Dmg Tspec Element associated with the request.
   */
  virtual void ReceiveAddtsRequest (Mac48Address address, DmgTspecElement element);
  /**
   * Handle a DELTS request received at the PCP/AP.
   * \param address The MAC address of the requesting STA.
   * \param info The Dmg Allocation Info field associated with the request.
   */
  virtual void ReceiveDeltsRequest (Mac48Address address, DmgAllocationInfo info);
  /**
   * Manage the ADDTS request received at the PCP/AP.
   * \param sourceAid The AID of the station that sent the ADDTS request.
   * \param sourceAddr The MAC address of the station that sent the ADDTS request.
   * \param dmgTspec The DMG Tspec element associated with the ADDTS request.
   */
  virtual void ManageAddtsRequests (uint8_t sourceAid, Mac48Address &sourceAddr, DmgTspecElement &dmgTspec);
  /**
   * \param minAllocation The minimum acceptable allocation in us for each allocation period.
   * \param maxAllocation The desired allocation in us for each allocation period.
   * \return The allocation duration for the allocation period.
   */
  virtual uint32_t GetAllocationDuration (uint32_t minAllocation, uint32_t maxAllocation) = 0;
  /**
   * Implement the policy that accept, reject a new ADDTS request.
   * \param sourceAid The AID of the requesting STA.
   * \param dmgTspec The DMG Tspec element of the ADDTS request.
   * \param info The DMG Allocation Info element of the request.
   * \return The Status Code to be included in the ADDTS response.
   */
  virtual StatusCode AddNewAllocation (uint8_t sourceAid, DmgTspecElement &dmgTspec, DmgAllocationInfo &info) = 0;
  /**
   * Implement the policy that accept, reject a modification request.
   * \param sourceAid The AID of the requesting STA.
   * \param dmgTspec The DMG Tspec element of the ADDTS request.
   * \param info The DMG Allocation Info element of the request.
   * \return The Status Code to be included in the ADDTS response.
   */
  virtual StatusCode ModifyExistingAllocation (uint8_t sourceAid, DmgTspecElement &dmgTspec, DmgAllocationInfo &info) = 0;
  /**
   * Adjust the existing allocations when an allocation is removed or modified.
   * \param iter The iterator pointing to the next element in the addtsAllocationList.
   * \param duration The duration of the time to manage.
   * \param isToAdd Whether the duration is to be added or subtracted.
   */
  virtual void AdjustExistingAllocations (AllocationFieldListI iter, uint32_t duration, bool isToAdd) = 0;
  /**
   * \return The TS Delay element to be included in the ADDTS response.
   */
  virtual TsDelayElement GetTsDelayElement (void);
  /**
   * Update start time and remaining DTI time for the next request to be evaluated.
   */
  virtual void UpdateStartAndRemainingTime (void) = 0;
  /**
   * Add broadcast CBAP allocations in the DTI.
   */
  virtual void AddBroadcastCbapAllocations (void) = 0;
  /**
   * Creates a broadcast CBAP allocation field.
   * \param staticAllocation Is the allocation static.
   * \param allocationStart The starting time of the allocation.
   * \param blockDuration The duration of the allocation.
   * \return The broadcast CBAP allocation field.
   */
  AllocationFieldList GetBroadcastCbapAllocation (bool staticAllocation, uint32_t allocationStart, uint32_t blockDuration);
  /**
   * Allocate maximum part of DTI as an SP.
   * \param allocationId The unique identifier for the allocation.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   */
  void AllocateDtiAsServicePeriod (AllocationID allocationId, uint8_t sourceAid, uint8_t destAid);
  /**
   * Add a new allocation period to be announced in DMG Beacon or Announce Frame.
   * \param allocationId The unique identifier for the allocation.
   * \param allocationType The type of allocation (CBAP or SP).
   * \param staticAllocation Is the allocation static.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param allocationStart The start time in microseconds of the allocation relative to the beginning of DTI.
   * \param blockDuration The duration of the allocation period in microseconds.
   * \param blocks The number of blocks making up the allocation.
   * \return The start time in microseconds of the following allocation period.
   */
  uint32_t AddAllocationPeriod (AllocationID allocationId, AllocationType allocationType, bool staticAllocation,
                                uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, uint16_t blockDuration,
                                uint16_t blockPeriod, uint8_t blocks);
  /**
   * Allocate SP allocation for Beamforming training.
   * \param srcAid The AID of the source DMG STA.
   * \param dstAid The AID of the destination DMG STA.
   * \param allocationStart The start time in miccroseconds of the allocation relative to the beginning of DTI.
   * \param allocationDuration The duration of the beamforming allocation in microseconds.
   * \param isInitiatorTxss Is the Initiator Beamforming TxSS or RxSS.
   * \param isResponderTxss Is the Responder Beamforming TxSS or RxSS.
   * \return The start time in microseconds of the next allocation period.
   */
  uint32_t AllocateBeamformingServicePeriod (uint8_t sourceAid, uint8_t destAid, uint32_t allocationStart, 
                                             uint16_t allocationDuration, bool isInitiatorTxss, bool isResponderTxss);
  /**
   * \return The full extended schedule element.
   */
  Ptr<ExtendedScheduleElement> GetFullExtendedScheduleElement (void);
  /**
   * \return The current Allocation list.
   */
  AllocationFieldList GetAllocationList (void);
  /**
   * \param The new Allocation list.
   */
  void SetAllocationList (AllocationFieldList allocationList);
  /**
   * \return The size of the current Allocation list.
   */
  uint32_t GetAllocationListSize (void) const;
  /**
   * The allocations have been announced in the DTI. 
   */
  void SetAllocationsAnnounced (void);
  /**
   * Cleanup non-static allocations.
   */
  void CleanupAllocations (void);

  Ptr<DmgApWifiMac> m_mac;                     //!< Pointer to the MAC high of PCP/AP.

  /* Access Period Allocations */
  AllocationFieldList m_allocationList;        //!< List of access period allocations in DTI which includes broadcast CBAP allocations.
  AllocationFieldList m_addtsAllocationList;   //!< List of requested (ADDTS received) access period allocations in DTI.
  /* Allocation */
  typedef struct {
    uint8_t sourceAid;
    Mac48Address sourceAddr;
    DmgTspecElement dmgTspec;
  } AddtsRequest;

  /* An allocation is uniquely identified by the tuple: Allocation ID, Source AID, Destination AID (802.11ad 10.4). */
  typedef std::tuple<AllocationID, uint8_t, uint8_t> UniqueIdentifier;
  typedef std::map<UniqueIdentifier, AddtsRequest> AllocatedRequestMap;
  typedef AllocatedRequestMap::iterator AllocatedRequestMapI;
  AllocatedRequestMap m_allocatedAddtsRequestMap; //!< The map containing the allocated ADDTS requests with their original allocation parameters.

  bool m_isAddtsAccepted;                       //!< An ADDTS request received in the last DTI has been accepted.
  bool m_isAllocationModified;                  //!< An allocation has been successfully modified.
  bool m_isNonStaticRemoved;                    //!< A non-static allocation has been served in the last DTI.
  bool m_isDeltsReceived;                       //!< A DELTS request has been received in the last DTI. 

  uint32_t m_allocationStartTime;               //!< The start time for the next allocation in the DTI.
  uint32_t m_remainingDtiTime;                  //!< The remaining time that can be allocated in the DTI.
  uint32_t m_guardTime;                         //!< The guard time between allocations in us.
  Ptr<ExtendedScheduleElement> m_fullEse;       //!< Full ESE for calculation of BTI duration.

  /* Channel Access Period */
  ChannelAccessPeriod m_currentAccessPeriod;   //!< The type of the current channel access period.
  Time m_biDuration;                           //!< The length of the BI period.
  Time m_atiDuration;                          //!< The length of the ATI period.
  Time m_dtiDuration;                          //!< The length of the DTI period.
  Time m_currentAccessPeriodStartTime;         //!< The start time of the current channel acess period.

private:
  /**
   * Creates a dummy ESE with maximum size.
   */
  void CreateFullExtentdedScheduleElement (void);
  /**
   * Handle the start of the BI
   * \param address The MAC address of the PCP/AP.
   * \param biDuration The duration of the current BI interval.
   * \param bhiDuration The duration of the current BHI interval.
   * \param atiDuration The duration of the current ATI interval.
   */
  void BeaconIntervalStarted (Mac48Address address, Time biDuration, Time bhiDuration, Time atiDuration);
  /**
   * Handle the start of the DTI.
   * \param address The MAC address of the PCP/AP.
   * \param dtiDuration The duration of the current DTI.
   */
  void DataTransferIntervalStarted (Mac48Address address, Time dtiDuration);
  /**
   * Handle the start of the ATI.
   */
  void AnnouncementTransmissionIntervalStarted (void);
  /**
   * Send ADDTS response to the source STA of the allocation
   * \param address The MAC address of the source STA.
   * \param status The status code of the ADDTS response.
   * \param dmgTspec The DMG Tspec element associated.
   */
  void SendAddtsResponse (Mac48Address address, StatusCode status, DmgTspecElement dmgTspec);
  /**
   * Modify the scheduling parameters of an existing allocation.
   * \param allocationId The unique identifier for the allocation.
   * \param sourceAid The AID of the source DMG STA.
   * \param destAid The AID of the destination DMG STA.
   * \param newStartTime The new starting time of the allocation.
   * \param newDuration The new duration of the allocation.
   */
  void ModifyAllocation (AllocationID allocationId, uint8_t sourceAid, uint8_t destAid, 
                         uint32_t newStartTime, uint16_t newDuration);
  /**
   * Allocate entire DTI as CBAP if no requested allocations are granted.
   */
  void AddBroadcastCbap (void);

};

} // namespace ns3

#endif /* DMG_WIFI_SCHEDULER_H */