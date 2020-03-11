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

namespace ns3	{

class DmgApWifiMac;

/**
 * \brief scheduling features for IEEE 802.11ad
 *
 * This class provides the implementation of scheduling features related to
 * IEEE 802.11ad. In particular, this class organizes the medium access 
 * according to the availability of contention-free access periods (SPs)
 * and contention-based access periods (CBAPs) as foresee by 802.11ad amendment.
 */
class DmgWifiScheduler : public Object
{
public:
  static TypeId GetTypeId (void);

  DmgWifiScheduler ();
  virtual ~DmgWifiScheduler ();
  void Initialize (void);
  /**
   * \param mac the MAC layer connected with the scheduler.
   */
  void SetMac (Ptr<DmgApWifiMac> mac);
  /**
   * Handle an ADDTS request received by the PCP/AP.
   * \param address the MAC address of the source STA.
   * \param element the Dmg Tspec Element associated with the request.
   */
  void ReceiveAddtsRequest (Mac48Address address, DmgTspecElement element);

protected:
	virtual void DoDispose (void);
  virtual void DoInitialize (void);

  Ptr<DmgApWifiMac> m_mac; //!< Pointer to the MAC high of PCP/AP.

private:

};

} // namespace ns3

#endif /* DMG_WIFI_SCHEDULER_H */