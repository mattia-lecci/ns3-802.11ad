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

#include "ns3/traced-callback.h"

namespace ns3	{

class DmgWifiScheduler : public Object
{
public:
  static TypeId GetTypeId (void);

  DmgWifiScheduler ();
  virtual ~DmgWifiScheduler ();

protected:
	virtual void DoDispose (void);
  virtual void DoInitialize (void);

private:

};

} // namespace ns3

#endif /* DMG_WIFI_SCHEDULER_H */