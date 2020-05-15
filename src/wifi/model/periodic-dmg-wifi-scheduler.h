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

#ifndef PERIODIC_DMG_WIFI_SCHEDULER_H
#define PERIODIC_DMG_WIFI_SCHEDULER_H

#include "dmg-wifi-scheduler.h"

namespace ns3 {
/**
 * \brief Scheduler  scheduling features for IEEE 802.11ad
 *
 * This class provides the implementation of a basic set of scheduling features
 * for IEEE 802.11ad. In particular, this class develops the admission and control
 * policy in the case of new ADDTS requests or modification ADDTS requests received.
 * Also, it manages requests for periodic allocations of resources.
 * The presence of a minimum broadcast CBAP time is considered when evaluating ADDTS requests.
 * The remaining DTI time is allocated as broadcast CBAP.
 */
class PeriodicDmgWifiScheduler : public DmgWifiScheduler
{
public:
  static TypeId GetTypeId (void);

  PeriodicDmgWifiScheduler ();
  virtual ~PeriodicDmgWifiScheduler ();

protected:
  virtual void DoDispose (void);
  /**
   * Evaluate the duration of the allocation based on the minumum and maximum duration constraints
   * \param minAllocation The minimum acceptable allocation in us for each allocation period.
   * \param maxAllocation The desired allocation in us for each allocation period.
   * \return The allocation duration for the allocation period.
   */
  virtual uint32_t GetAllocationDuration (uint32_t minAllocation, uint32_t maxAllocation);
  /**
   * Implement the policy that accepts or rejects a new ADDTS request.
   * \param sourceAid The AID of the requesting STA.
   * \param dmgTspec The DMG Tspec element of the ADDTS request.
   * \param info The DMG Allocation Info element of the request.
   * \return The Status Code to be included in the ADDTS response.
   */
  virtual StatusCode AddNewAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info);
  /**
   * Implement the policy that accepts or rejects a modification request.
   * The current version supports only requests of reduction of the duration of the allocations.
   * \param sourceAid The AID of the requesting STA.
   * \param dmgTspec The DMG Tspec element of the ADDTS request.
   * \param info The DMG Allocation Info element of the request.
   * \return The StatusCode to be included in the ADDTS response.
   */
  virtual StatusCode ModifyExistingAllocation (uint8_t sourceAid, const DmgTspecElement &dmgTspec, const DmgAllocationInfo &info);
  /**
   * Adjust the existing allocations when an allocation is removed or modified.
   * \param iter The iterator pointing to the next element in the addtsAllocationList.
   * \param duration The duration of the time to manage.
   * \param isToAdd Whether the duration is to be added or subtracted.
   */
  virtual void AdjustExistingAllocations (AllocationFieldListI iter, uint32_t duration, bool isToAdd);
  /**
   * Update start time and remaining DTI time for the next request to be evaluated.
   */
  virtual void UpdateStartAndRemainingTime (void);
  /**
   * Add broadcast CBAP allocations in the DTI.
   */
  virtual void AddBroadcastCbapAllocations (void);

private:
  /**
   * Verify how many blocks (in our case, one SP corresponds to one block) we can guarantee to a periodic request.
   * \param allocDuration the duration associated to the SPs.
   * \param spInterval time between two consecutive periodic SPs.
   * \return number of blocks that can be allocated in the DTI
   */
  uint8_t GetAvailableBlocks (uint32_t allocDuration, uint32_t spInterval);
  /**
   * Update the list of available time slots in the DTI. 
   * The current version supports only the reduction of pre-existing allocations.
   * \param startAlloc start time of the allocation that has to be excluded from the available time.
   * \param endAlloc end time of the allocation that has to be excluded from the available time.
   */
  void UpdateAvailableSlots (uint32_t startAlloc, uint32_t endAlloc);
  /**
   * Update the list of available time slots in the DTI based on some changes in previously allocated slots. 
   * The current version supports only the reduction of pre-existing allocations.
   * \param startAlloc start time of the allocation that has to be excluded from the available time.
   * \param endAlloc end time of the allocation that has to be excluded from the available time.
   * \param difference it represents how much an allocation has been reduced.
   */
  void UpdateAvailableSlots (uint32_t startAlloc, uint32_t endAlloc, uint32_t difference);

  // std::pair<uint32_t, uint32_t> is a struct used to store the start and 
  // end time (first and second member of the pair, respectively) of the available time chunks
  std::vector<std::pair<uint32_t, uint32_t> > m_availableSlots;       //!< List of available time chunks in the DTI.

};

} // namespace ns3

#endif /* PERIODIC_DMG_WIFI_SCHEDULER_H */
