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
 * Authors: Salman Mohebi <s.mohebi22@gmail.com>
 *
 */

#include "ns3/log.h"
#include "timestamp-tag.h"
#include "crazytaxi-streaming-server.h"
#include "three-lognormal-random-variable.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CrazyTaxiStreamingServer");

NS_OBJECT_ENSURE_REGISTERED (CrazyTaxiStreamingServer);

TypeId
CrazyTaxiStreamingServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CrazyTaxiStreamingServer")
    .SetParent<GamingStreamingServer> ()
    .SetGroupName ("Applications")
    .AddConstructor<CrazyTaxiStreamingServer> ()
  ;
  return tid;
}

CrazyTaxiStreamingServer::CrazyTaxiStreamingServer ()
{
  NS_LOG_FUNCTION (this);
  InitializeStreams ();
}

CrazyTaxiStreamingServer::CrazyTaxiStreamingServer (Address ip, uint16_t port)
  : GamingStreamingServer (ip, port)
{
  NS_LOG_FUNCTION (this << ip << port);
  InitializeStreams ();
}

CrazyTaxiStreamingServer::~CrazyTaxiStreamingServer ()
{
  NS_LOG_FUNCTION (this);
}

void
CrazyTaxiStreamingServer::InitializeStreams ()
{
  /** Add CBR audio stream */
  Ptr<ConstantRandomVariable> pktCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (216));
  Ptr<ConstantRandomVariable> iatCbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (10));
  AddNewTrafficStream (pktCbrAudio, iatCbrAudio);

  /** Add Cursor stream */
  Ptr<ConstantRandomVariable> pktCursor = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (28));
  Ptr<ConstantRandomVariable> iatCursor = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  AddNewTrafficStream (pktCursor, iatCursor);

  /** Add VBR audio stream */
  // Packet size
  Ptr<ConstantRandomVariable> pktVbrAudio1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (244));
  Ptr<ConstantRandomVariable> pktVbrAudio2 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (1384));

  Ptr<MixtureRandomVariable> pktVbrAudio = CreateObject<MixtureRandomVariable> ();
  pktVbrAudio->SetRandomVariables ({pktVbrAudio1, pktVbrAudio2}, {0.0776, 0.9224});

  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatVbrAudio = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (50));
  AddNewTrafficStream (pktVbrAudio, iatVbrAudio);

  /** Add video stream */
  // Packet size
  Ptr<UniformRandomVariable> pktVideo1 = CreateObjectWithAttributes<UniformRandomVariable> ("Min", DoubleValue (0),
                                                                                            "Max", DoubleValue (1355));
  Ptr<ConstantRandomVariable> pktVideo2 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (1356));

  Ptr<MixtureRandomVariable> pktVideo = CreateObject<MixtureRandomVariable> ();
  pktVideo->SetRandomVariables ({pktVideo1, pktVideo2}, {0.3606, 0.6394});

  // Packet inter-arrival time
  Ptr<ConstantRandomVariable> iatVideo1 = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (0));
  Ptr<ThreeLogNormalRandomVariable> iatVideo2 = CreateObjectWithAttributes<ThreeLogNormalRandomVariable> ("Mu", DoubleValue (0.34),
                                                                                                          "Sigma", DoubleValue (1.73),
                                                                                                          "Threshold", DoubleValue (-2.25));

  Ptr<MixtureRandomVariable> iatVideo = CreateObject<MixtureRandomVariable> ();
  iatVideo->SetRandomVariables ({iatVideo1, iatVideo2}, {0.5725, 0.4275});

  AddNewTrafficStream (pktVideo, iatVideo);
}

} // Namespace ns3
