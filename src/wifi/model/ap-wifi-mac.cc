/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ap-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "mac-low.h"
#include "mac-tx-middle.h"
#include "wifi-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ApWifiMac");

NS_OBJECT_ENSURE_REGISTERED (ApWifiMac);

TypeId
ApWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ApWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ApWifiMac> ()
    .AddAttribute ("BeaconInterval",
                   "Delay between two beacons",
                   TimeValue (MicroSeconds (102400)),
                   MakeTimeAccessor (&ApWifiMac::GetBeaconInterval,
                                     &ApWifiMac::SetBeaconInterval),
                   MakeTimeChecker ())
    .AddAttribute ("BeaconJitter",
                   "A uniform random variable to cause the initial beacon starting time (after simulation time 0) "
                   "to be distributed between 0 and the BeaconInterval.",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&ApWifiMac::m_beaconJitter),
                   MakePointerChecker<UniformRandomVariable> ())
    .AddAttribute ("EnableBeaconJitter",
                   "If beacons are enabled, whether to jitter the initial send event.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::m_enableBeaconJitter),
                   MakeBooleanChecker ())
    .AddAttribute ("BeaconGeneration",
                   "Whether or not beacons are generated.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::SetBeaconGeneration,
                                        &ApWifiMac::GetBeaconGeneration),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonErpProtection", "Whether or not protection mechanism should be used when non-ERP STAs are present within the BSS."
                   "This parameter is only used when ERP is supported by the AP.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::m_enableNonErpProtection),
                   MakeBooleanChecker ())
    .AddAttribute ("RifsMode", "If non-HT STAs are detected, whether to force RIFS to be disabled within the BSS."
                   "This parameter is only used when HT is supported by the AP.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::m_disableRifs),
                   MakeBooleanChecker ())
  ;
  return tid;
}

ApWifiMac::ApWifiMac ()
  : m_enableBeaconGeneration (false)
{
  NS_LOG_FUNCTION (this);
  m_beaconDca = CreateObject<DcaTxop> ();
  m_beaconDca->SetAifsn (1);
  m_beaconDca->SetMinCw (0);
  m_beaconDca->SetMaxCw (0);
  m_beaconDca->SetLow (m_low);
  m_beaconDca->SetManager (m_dcfManager);
  m_beaconDca->SetTxMiddle (m_txMiddle);

  //Let the lower layers know that we are acting as an AP.
  SetTypeOfStation (AP);
}

ApWifiMac::~ApWifiMac ()
{
  NS_LOG_FUNCTION (this);
  m_staList.clear ();
  m_nonErpStations.clear ();
  m_nonHtStations.clear ();
}

void
ApWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_beaconDca->Dispose ();
  m_beaconDca = 0;
  m_enableBeaconGeneration = false;
  m_beaconEvent.Cancel ();
  RegularWifiMac::DoDispose ();
}

void
ApWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  //As an AP, our MAC address is also the BSSID. Hence we are
  //overriding this function and setting both in our parent class.
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}

void
ApWifiMac::SetBeaconGeneration (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  if (!enable)
    {
      m_beaconEvent.Cancel ();
    }
  else if (enable && !m_enableBeaconGeneration)
    {
      m_beaconEvent = Simulator::ScheduleNow (&ApWifiMac::SendOneBeacon, this);
    }
  m_enableBeaconGeneration = enable;
}

bool
ApWifiMac::GetBeaconGeneration (void) const
{
  NS_LOG_FUNCTION (this);
  return m_enableBeaconGeneration;
}

Time
ApWifiMac::GetBeaconInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beaconInterval;
}

void
ApWifiMac::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_beaconDca->SetWifiRemoteStationManager (stationManager);
  RegularWifiMac::SetWifiRemoteStationManager (stationManager);
}

void
ApWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  //The approach taken here is that, from the point of view of an AP,
  //the link is always up, so we immediately invoke the callback if
  //one is set
  linkUp ();
}

void
ApWifiMac::SetBeaconInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  if ((interval.GetMicroSeconds () % 1024) != 0)
    {
      NS_LOG_WARN ("beacon interval should be multiple of 1024us (802.11 time unit), see IEEE Std. 802.11-2012");
    }
  m_beaconInterval = interval;
}

void
ApWifiMac::StartBeaconing (void)
{
  NS_LOG_FUNCTION (this);
  SendOneBeacon ();
}

int64_t
ApWifiMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_beaconJitter->SetStream (stream);
  return 1;
}

bool
ApWifiMac::GetShortSlotTimeEnabled (void) const
{
  if (m_nonErpStations.size () != 0)
    {
      return false;
    }
  if (m_erpSupported == true && GetShortSlotTimeSupported () == true)
    {
      for (std::map<uint16_t, Mac48Address>::const_iterator i = m_staList.begin (); i != m_staList.end (); i++)
        {
          if (m_stationManager->GetShortSlotTimeSupported (i->second) == false)
            {
              return false;
            }
        }
      return true;
    }
  return false;
}

bool
ApWifiMac::GetShortPreambleEnabled (void) const
{
  if (m_erpSupported || m_phy->GetShortPlcpPreambleSupported ())
    {
      for (std::list<Mac48Address>::const_iterator i = m_nonErpStations.begin (); i != m_nonErpStations.end (); i++)
        {
          if (m_stationManager->GetShortPreambleSupported (*i) == false)
            {
              return false;
            }
        }
      return true;
    }
  return false;
}

bool
ApWifiMac::IsNonGfHtStasPresent (void) const
{
  bool isNonGfHtStasPresent = false;
  for (std::map<uint16_t, Mac48Address>::const_iterator i = m_staList.begin (); i != m_staList.end (); i++)
    {
      if (m_stationManager->GetGreenfieldSupported (i->second) == false)
        {
          isNonGfHtStasPresent = true;
          break;
        }
    }
  m_stationManager->SetUseGreenfieldProtection (isNonGfHtStasPresent);
  return isNonGfHtStasPresent;
}

uint8_t
ApWifiMac::GetVhtOperationalChannelWidth (void) const
{
  uint8_t channelWidth = m_phy->GetChannelWidth ();
  for (std::map<uint16_t, Mac48Address>::const_iterator i = m_staList.begin (); i != m_staList.end (); i++)
    {
      if (m_stationManager->GetVhtSupported (i->second))
        {
          if (m_stationManager->GetChannelWidthSupported (i->second) < channelWidth)
            {
              channelWidth = m_stationManager->GetChannelWidthSupported (i->second);
            }
        }
    }
  return channelWidth;
}

void
ApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from,
                        Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  //If we are not a QoS AP then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  //If we are a QoS AP then we attempt to get a TID for this packet
  if (m_qosSupported)
    {
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which'll
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
    }

  ForwardDown (packet, from, to, tid);
}

void
ApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from,
                        Mac48Address to, uint8_t tid)
{
  NS_LOG_FUNCTION (this << packet << from << to << +tid);
  WifiMacHeader hdr;

  //For now, an AP that supports QoS does not support non-QoS
  //associations, and vice versa. In future the AP model should
  //support simultaneously associated QoS and non-QoS STAs, at which
  //point there will need to be per-association QoS state maintained
  //by the association state machine, and consulted here.
  if (m_qosSupported)
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      //Transmission of multiple frames in the same Polled TXOP is not supported for now
      hdr.SetQosTxopLimit (0);
      //Fill in the QoS control field in the MAC header
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetType (WIFI_MAC_DATA);
    }

  if (m_qosSupported || m_htSupported || m_vhtSupported || m_heSupported)
    {
      hdr.SetNoOrder ();
    }
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (from);
  hdr.SetDsFrom ();
  hdr.SetDsNotTo ();

  if (m_qosSupported)
    {
      //Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
    }
  else
    {
      m_dca->Queue (packet, hdr);
    }
}

void
ApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << to << from);
  if (to.IsBroadcast () || m_stationManager->IsAssociated (to))
    {
      ForwardDown (packet, from, to);
    }
  else
    {
      NotifyTxDrop (packet);
    }
}

void
ApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  //We're sending this packet with a from address that is our own. We
  //get that address from the lower MAC and make use of the
  //from-spoofing Enqueue() method to avoid duplicated code.
  Enqueue (packet, to, m_low->GetAddress ());
}

bool
ApWifiMac::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

SupportedRates
ApWifiMac::GetSupportedRates (void) const
{
  NS_LOG_FUNCTION (this);
  SupportedRates rates;
  //If it is an HT-AP or VHT-AP or HE-AP, then add the BSSMembershipSelectorSet
  //The standard says that the BSSMembershipSelectorSet
  //must have its MSB set to 1 (must be treated as a Basic Rate)
  //Also the standard mentioned that at least 1 element should be included in the SupportedRates the rest can be in the ExtendedSupportedRates
  if (m_htSupported || m_vhtSupported || m_heSupported)
    {
      for (uint8_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          rates.AddBssMembershipSelectorRate (m_phy->GetBssMembershipSelector (i));
        }
    }
  //
  //Send the set of supported rates and make sure that we indicate
  //the Basic Rate set in this set of supported rates.
  for (uint8_t i = 0; i < m_phy->GetNModes (); i++)
    {
      WifiMode mode = m_phy->GetMode (i);
      uint64_t modeDataRate = mode.GetDataRate (m_phy->GetChannelWidth ());
      NS_LOG_DEBUG ("Adding supported rate of " << modeDataRate);
      rates.AddSupportedRate (modeDataRate);
      //Add rates that are part of the BSSBasicRateSet (manufacturer dependent!)
      //here we choose to add the mandatory rates to the BSSBasicRateSet,
      //except for 802.11b where we assume that only the non HR-DSSS rates are part of the BSSBasicRateSet
      if (mode.IsMandatory () && (mode.GetModulationClass () != WIFI_MOD_CLASS_HR_DSSS))
        {
          NS_LOG_DEBUG ("Adding basic mode " << mode.GetUniqueName ());
          m_stationManager->AddBasicMode (mode);
        }
    }
  //set the basic rates
  for (uint8_t j = 0; j < m_stationManager->GetNBasicModes (); j++)
    {
      WifiMode mode = m_stationManager->GetBasicMode (j);
      uint64_t modeDataRate = mode.GetDataRate (m_phy->GetChannelWidth ());
      NS_LOG_DEBUG ("Setting basic rate " << mode.GetUniqueName ());
      rates.SetBasicRate (modeDataRate);
    }

  return rates;
}

Ptr<DsssParameterSet>
ApWifiMac::GetDsssParameterSet (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<DsssParameterSet> dsssParameters = Create<DsssParameterSet> ();
  if (m_dsssSupported)
    {
      dsssParameters->SetDsssSupported (1);
      dsssParameters->SetCurrentChannel (m_phy->GetChannelNumber ());
    }
  return dsssParameters;
}

CapabilityInformation
ApWifiMac::GetCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  CapabilityInformation capabilities;
  capabilities.SetShortPreamble (GetShortPreambleEnabled ());
  capabilities.SetShortSlotTime (GetShortSlotTimeEnabled ());
  capabilities.SetEss ();
  return capabilities;
}

Ptr<ErpInformation>
ApWifiMac::GetErpInformation (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<ErpInformation> information = Create<ErpInformation> ();
  information->SetErpSupported (1);
  if (m_erpSupported)
    {
      information->SetNonErpPresent (!m_nonErpStations.empty ());
      information->SetUseProtection (GetUseNonErpProtection ());
      if (GetShortPreambleEnabled ())
        {
          information->SetBarkerPreambleMode (0);
        }
      else
        {
          information->SetBarkerPreambleMode (1);
        }
    }
  return information;
}

Ptr<EdcaParameterSet>
ApWifiMac::GetEdcaParameterSet (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<EdcaParameterSet> edcaParameters = Create<EdcaParameterSet> ();
  if (m_qosSupported)
    {
      edcaParameters->SetQosSupported (1);
      Ptr<EdcaTxopN> edca;
      Time txopLimit;

      edca = m_edca.find (AC_BE)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters->SetBeAci(0);
      edcaParameters->SetBeCWmin(edca->GetMinCw ());
      edcaParameters->SetBeCWmax(edca->GetMaxCw ());
      edcaParameters->SetBeAifsn(edca->GetAifsn ());
      edcaParameters->SetBeTXOPLimit(txopLimit.GetMicroSeconds () / 32);
      edcaParameters->SetBeAcm (0);

      edca = m_edca.find (AC_BK)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters->SetBkAci(1);
      edcaParameters->SetBkCWmin(edca->GetMinCw ());
      edcaParameters->SetBkCWmax(edca->GetMaxCw ());
      edcaParameters->SetBkAifsn(edca->GetAifsn ());
      edcaParameters->SetBkTXOPLimit(txopLimit.GetMicroSeconds () / 32);
      edcaParameters->SetBkAcm (0);

      edca = m_edca.find (AC_VI)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters->SetViAci(2);
      edcaParameters->SetViCWmin(edca->GetMinCw ());
      edcaParameters->SetViCWmax(edca->GetMaxCw ());
      edcaParameters->SetViAifsn(edca->GetAifsn ());
      edcaParameters->SetViTXOPLimit(txopLimit.GetMicroSeconds () / 32);
      edcaParameters->SetViAcm (0);

      edca = m_edca.find (AC_VO)->second;
      txopLimit = edca->GetTxopLimit ();
      edcaParameters->SetVoAci(3);
      edcaParameters->SetVoCWmin(edca->GetMinCw ());
      edcaParameters->SetVoCWmax(edca->GetMaxCw ());
      edcaParameters->SetVoAifsn(edca->GetAifsn ());
      edcaParameters->SetVoTXOPLimit(txopLimit.GetMicroSeconds () / 32);
      edcaParameters->SetVoAcm (0);

      edcaParameters->SetQosInfo (0);
    }
  return edcaParameters;
}

Ptr<HtOperation>
ApWifiMac::GetHtOperation (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<HtOperation> operation = Create<HtOperation> ();
  if (m_htSupported)
    {
      operation->SetHtSupported (1);
      operation->SetRifsMode (GetRifsMode ());
      operation->SetNonGfHtStasPresent (IsNonGfHtStasPresent ());
      if (m_phy->GetChannelWidth () > 20)
        {
          operation->SetSecondaryChannelOffset (1);
          operation->SetStaChannelWidth (1);
        }
      if (m_nonHtStations.empty ())
        {
          operation->SetHtProtection (NO_PROTECTION);
        }
      else
        {
          operation->SetHtProtection (MIXED_MODE_PROTECTION);
        }
      uint64_t maxSupportedRate = 0; //in bit/s
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if (mcs.GetModulationClass () != WIFI_MOD_CLASS_HT)
            {
              continue;
            }
          uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
          NS_ASSERT (nss > 0 && nss < 5);
          uint64_t dataRate = mcs.GetDataRate (m_phy->GetChannelWidth (), m_phy->GetShortGuardInterval () ? 400 : 800, nss);
          if (dataRate > maxSupportedRate)
            {
              maxSupportedRate = dataRate;
              NS_LOG_DEBUG ("Updating maxSupportedRate to " << maxSupportedRate);
            }
        }
      uint8_t maxSpatialStream = m_phy->GetMaxSupportedTxSpatialStreams ();
      uint8_t nMcs = m_phy->GetNMcs ();
      for (std::map<uint16_t, Mac48Address>::const_iterator i = m_staList.begin (); i != m_staList.end (); i++)
        {
          if (m_stationManager->GetHtSupported (i->second))
            {
              uint64_t maxSupportedRateByHtSta = 0; //in bit/s
              for (uint8_t j = 0; j < (std::min (nMcs, m_stationManager->GetNMcsSupported (i->second))); j++)
                {
                  WifiMode mcs = m_phy->GetMcs (j);
                  if (mcs.GetModulationClass () != WIFI_MOD_CLASS_HT)
                    {
                      continue;
                    }
                  uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
                  NS_ASSERT (nss > 0 && nss < 5);
                  uint64_t dataRate = mcs.GetDataRate (m_stationManager->GetChannelWidthSupported (i->second), m_stationManager->GetShortGuardInterval (i->second) ? 400 : 800, nss);
                  if (dataRate > maxSupportedRateByHtSta)
                    {
                      maxSupportedRateByHtSta = dataRate;
                    }
                }
              if (maxSupportedRateByHtSta < maxSupportedRate)
                {
                  maxSupportedRate = maxSupportedRateByHtSta;
                }
              if (m_stationManager->GetNMcsSupported (i->second) < nMcs)
                {
                  nMcs = m_stationManager->GetNMcsSupported (i->second);
                }
              if (m_stationManager->GetNumberOfSupportedStreams (i->second) < maxSpatialStream)
                {
                  maxSpatialStream = m_stationManager->GetNumberOfSupportedStreams (i->second);
                }
            }
        }
      operation->SetRxHighestSupportedDataRate (maxSupportedRate / 1e6); //in Mbit/s
      operation->SetTxMcsSetDefined (nMcs > 0);
      operation->SetTxMaxNSpatialStreams (maxSpatialStream);
      //To be filled in once supported
      operation->SetObssNonHtStasPresent (0);
      operation->SetDualBeacon (0);
      operation->SetDualCtsProtection (0);
      operation->SetStbcBeacon (0);
      operation->SetLSigTxopProtectionFullSupport (0);
      operation->SetPcoActive (0);
      operation->SetPhase (0);
      operation->SetRxMcsBitmask (0);
      operation->SetTxRxMcsSetUnequal (0);
      operation->SetTxUnequalModulation (0);
    }
  return operation;
}

Ptr<VhtOperation>
ApWifiMac::GetVhtOperation (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<VhtOperation> operation = Create<VhtOperation> ();
  if (m_vhtSupported)
    {
      operation->SetVhtSupported (1);
      uint8_t channelWidth = GetVhtOperationalChannelWidth ();
      if (channelWidth == 160)
        {
          operation->SetChannelWidth (2);
        }
      else if (channelWidth == 80)
        {
          operation->SetChannelWidth (1);
        }
      else
        {
          operation->SetChannelWidth (0);
        }
      for (uint8_t nss = 1; nss <= 8; nss++)
        {
          uint8_t maxMcs;
          if (nss <= m_phy->GetMaxSupportedRxSpatialStreams ())
            {
              maxMcs = 9; //TBD: hardcode to 9 for now since we assume all MCS values are supported
            }
          else
            {
              maxMcs = 0;
            }
          operation->SetMaxVhtMcsPerNss (nss, maxMcs);
        }
    }
  return operation;
}

Ptr<HeOperation>
ApWifiMac::GetHeOperation (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<HeOperation> operation = Create<HeOperation> ();
  if (m_heSupported)
    {
      operation->SetHeSupported (1);
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedRxSpatialStreams (); nss++)
        {
          operation->SetMaxHeMcsPerNss (nss, 11); //TBD: hardcode to 11 for now since we assume all MCS values are supported
        }
    }
  return operation;
}

void
ApWifiMac::SendProbeResp (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_PROBE_RESPONSE);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeResponseHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetSupportedRates (GetSupportedRates ());
  probe.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());
  probe.SetCapabilities (GetCapabilities ());
  m_stationManager->SetShortPreambleEnabled (GetShortPreambleEnabled ());
  m_stationManager->SetShortSlotTimeEnabled (GetShortSlotTimeEnabled ());
  if (m_dsssSupported)
    {
      probe.AddWifiInformationElement (GetDsssParameterSet ());
    }
  if (m_erpSupported)
    {
      probe.AddWifiInformationElement (GetErpInformation ());
    }
  if (m_qosSupported)
    {
      probe.AddWifiInformationElement (GetEdcaParameterSet ());
    }
  if (m_htSupported || m_vhtSupported || m_heSupported)
    {
      probe.AddWifiInformationElement (GetExtendedCapabilities ());
      probe.AddWifiInformationElement (GetHtCapabilities ());
      probe.AddWifiInformationElement (GetHtOperation ());
    }
  if (m_vhtSupported || m_heSupported)
    {
      probe.AddWifiInformationElement (GetVhtCapabilities ());
      probe.AddWifiInformationElement (GetVhtOperation ());
    }
  if (m_heSupported)
    {
      probe.AddWifiInformationElement (GetHeCapabilities ());
      probe.AddWifiInformationElement (GetHeOperation ());
    }
  packet->AddHeader (probe);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

void
ApWifiMac::SendAssocResp (Mac48Address to, bool success, bool isReassoc)
{
  NS_LOG_FUNCTION (this << to << success << isReassoc);
  WifiMacHeader hdr;
  hdr.SetType (isReassoc ? WIFI_MAC_MGT_REASSOCIATION_RESPONSE : WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtAssocResponseHeader assoc;
  StatusCode code;
  if (success)
    {
      code.SetSuccess ();
      uint16_t aid;
      bool found = false;
      if (isReassoc)
        {
          for (std::map<uint16_t, Mac48Address>::const_iterator i = m_staList.begin (); i != m_staList.end (); ++i)
            {
              if (i->second == to)
                {
                  aid = i->first;
                  found = true;
                  break;
                }
            }
        }
      if (!found)
        {
          aid = GetNextAssociationId ();
          m_staList.insert (std::make_pair (aid, to));
        }
      assoc.SetAssociationId (aid);
    }
  else
    {
      code.SetFailure ();
    }
  assoc.SetSupportedRates (GetSupportedRates ());
  assoc.SetStatusCode (code);
  assoc.SetCapabilities (GetCapabilities ());
  if (m_erpSupported)
    {
      assoc.AddWifiInformationElement (GetErpInformation ());
    }
  if (m_qosSupported)
    {
      assoc.AddWifiInformationElement (GetEdcaParameterSet ());
    }
  if (m_htSupported || m_vhtSupported || m_heSupported)
    {
      assoc.AddWifiInformationElement (GetExtendedCapabilities ());
      assoc.AddWifiInformationElement (GetHtCapabilities ());
      assoc.AddWifiInformationElement (GetHtOperation ());
    }
  if (m_vhtSupported || m_heSupported)
    {
      assoc.AddWifiInformationElement (GetVhtCapabilities ());
      assoc.AddWifiInformationElement (GetVhtOperation ());
    }
  if (m_heSupported)
    {
      assoc.AddWifiInformationElement (GetHeCapabilities ());
      assoc.AddWifiInformationElement (GetHeOperation ());
    }
  packet->AddHeader (assoc);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

void
ApWifiMac::SendOneBeacon (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_BEACON);
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtBeaconHeader beacon;
  beacon.SetSsid (GetSsid ());
  beacon.SetSupportedRates (GetSupportedRates ());
  beacon.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());
  beacon.SetCapabilities (GetCapabilities ());
  m_stationManager->SetShortPreambleEnabled (GetShortPreambleEnabled ());
  m_stationManager->SetShortSlotTimeEnabled (GetShortSlotTimeEnabled ());
  if (m_dsssSupported)
    {
      beacon.AddWifiInformationElement (GetDsssParameterSet ());
    }
  if (m_erpSupported)
    {
      beacon.AddWifiInformationElement (GetErpInformation ());
    }
  if (m_qosSupported)
    {
      beacon.AddWifiInformationElement (GetEdcaParameterSet ());
    }
  if (m_htSupported || m_vhtSupported)
    {
      beacon.AddWifiInformationElement (GetExtendedCapabilities ());
      beacon.AddWifiInformationElement (GetHtCapabilities ());
      beacon.AddWifiInformationElement (GetHtOperation ());
    }
  if (m_vhtSupported || m_heSupported)
    {
      beacon.AddWifiInformationElement (GetVhtCapabilities ());
      beacon.AddWifiInformationElement (GetVhtOperation ());
    }
  if (m_heSupported)
    {
      beacon.AddWifiInformationElement (GetHeCapabilities ());
      beacon.AddWifiInformationElement (GetHeOperation ());
    }
  if (m_supportMultiBand)
    {
      beacon.AddWifiInformationElement (GetMultiBandElement ());
    }
  packet->AddHeader (beacon);

  //The beacon has it's own special queue, so we load it in there
  m_beaconDca->Queue (packet, hdr);
  m_beaconEvent = Simulator::Schedule (m_beaconInterval, &ApWifiMac::SendOneBeacon, this);

  //If a STA that does not support Short Slot Time associates,
  //the AP shall use long slot time beginning at the first Beacon
  //subsequent to the association of the long slot time STA.
  if (m_erpSupported)
    {
      if (GetShortSlotTimeEnabled () == true)
        {
          //Enable short slot time
          SetSlot (MicroSeconds (9));
        }
      else
        {
          //Disable short slot time
          SetSlot (MicroSeconds (20));
        }
    }
}

void
ApWifiMac::TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxOk (packet, hdr);
  if ((hdr.IsAssocResp () || hdr.IsReassocResp ())
      && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("associated with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxOk (hdr.GetAddr1 ());
    }
}

void
ApWifiMac::TxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxFailed (hdr);

  if ((hdr.IsAssocResp () || hdr.IsReassocResp ())
      && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("association failed with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxFailed (hdr.GetAddr1 ());
    }
}

Ptr<MultiBandElement>
ApWifiMac::GetMultiBandElement (void) const
{
  Ptr<MultiBandElement> multiband = Create<MultiBandElement> ();
  multiband->SetStaRole (ROLE_AP);
  multiband->SetStaMacAddressPresent (false); /* The same MAC address is used across all the bands */
  multiband->SetBandID (Band_2_4GHz);
  multiband->SetOperatingClass (18);          /* Europe */
  multiband->SetChannelNumber (m_phy->GetChannelNumber ());
  multiband->SetBssID (GetAddress ());
  multiband->SetBeaconInterval (m_beaconInterval.GetMicroSeconds ());
  multiband->SetConnectionCapability (1);     /* AP */
  multiband->SetFstSessionTimeout (m_fstTimeout);
  return multiband;
}

void
ApWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  Mac48Address from = hdr->GetAddr2 ();
  if (hdr->IsData ())
    {
      Mac48Address bssid = hdr->GetAddr1 ();
      if (!hdr->IsFromDs ()
          && hdr->IsToDs ()
          && bssid == GetAddress ()
          && m_stationManager->IsAssociated (from))
        {
          Mac48Address to = hdr->GetAddr3 ();
          if (to == GetAddress ())
            {
              NS_LOG_DEBUG ("frame for me from=" << from);
              if (hdr->IsQosData ())
                {
                  if (hdr->IsQosAmsdu ())
                    {
                      NS_LOG_DEBUG ("Received A-MSDU from=" << from << ", size=" << packet->GetSize ());
                      DeaggregateAmsduAndForward (packet, hdr);
                      packet = 0;
                    }
                  else
                    {
                      ForwardUp (packet, from, bssid);
                    }
                }
              else
                {
                  ForwardUp (packet, from, bssid);
                }
            }
          else if (to.IsGroup ()
                   || m_stationManager->IsAssociated (to))
            {
              NS_LOG_DEBUG ("forwarding frame from=" << from << ", to=" << to);
              Ptr<Packet> copy = packet->Copy ();

              //If the frame we are forwarding is of type QoS Data,
              //then we need to preserve the UP in the QoS control
              //header...
              if (hdr->IsQosData ())
                {
                  ForwardDown (packet, from, to, hdr->GetQosTid ());
                }
              else
                {
                  ForwardDown (packet, from, to);
                }
              ForwardUp (copy, from, to);
            }
          else
            {
              ForwardUp (packet, from, to);
            }
        }
      else if (hdr->IsFromDs ()
               && hdr->IsToDs ())
        {
          //this is an AP-to-AP frame
          //we ignore for now.
          NotifyRxDrop (packet);
        }
      else
        {
          //we can ignore these frames since
          //they are not targeted at the AP
          NotifyRxDrop (packet);
        }
      return;
    }
  else if (hdr->IsMgt ())
    {
      if (hdr->IsProbeReq ())
        {
          NS_ASSERT (hdr->GetAddr1 ().IsBroadcast ());
          NS_LOG_DEBUG ("Probe request received from " << from << ": send probe response");
          SendProbeResp (from);
          return;
        }
      else if (hdr->GetAddr1 () == GetAddress ())
        {
          if (hdr->IsAssocReq ())
            {
              NS_LOG_DEBUG ("Association request received from " << from);
              //first, verify that the the station's supported
              //rate set is compatible with our Basic Rate set
              MgtAssocRequestHeader assocReq;
              packet->RemoveHeader (assocReq);
              CapabilityInformation capabilities = assocReq.GetCapabilities ();
              m_stationManager->AddSupportedPlcpPreamble (from, capabilities.IsShortPreamble ());
              SupportedRates rates = assocReq.GetSupportedRates ();
              bool problem = false;
              bool isHtStation = false;
              bool isOfdmStation = false;
              bool isErpStation = false;
              bool isDsssStation = false;
              for (uint8_t i = 0; i < m_stationManager->GetNBasicModes (); i++)
                {
                  WifiMode mode = m_stationManager->GetBasicMode (i);
                  if (!rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                    {
                      if ((mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
                        {
                          isDsssStation = false;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
                        {
                          isErpStation = false;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM)
                        {
                          isOfdmStation = false;
                        }
                      if (isDsssStation == false && isErpStation == false && isOfdmStation == false)
                        {
                          problem = true;
                          break;
                        }
                    }
                  else
                    {
                      if ((mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
                        {
                          isDsssStation = true;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
                        {
                          isErpStation = true;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM)
                        {
                          isOfdmStation = true;
                        }
                    }
                }
              m_stationManager->AddSupportedErpSlotTime (from, capabilities.IsShortSlotTime () && isErpStation);
              if (m_htSupported)
                {
                  //check whether the HT STA supports all MCSs in Basic MCS Set
                  Ptr<HtCapabilities> htcapabilities =
                      StaticCast<HtCapabilities> (assocReq.GetInformationElement (IE_HT_CAPABILITIES));
                  if (htcapabilities->IsSupportedMcs (0))
                    {
                      isHtStation = true;
                      for (uint8_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!htcapabilities->IsSupportedMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (m_vhtSupported)
                {
                  //check whether the VHT STA supports all MCSs in Basic MCS Set
                  Ptr<VhtCapabilities> vhtCapabilities =
                      StaticCast<VhtCapabilities> (assocReq.GetInformationElement (IE_VHT_CAPABILITIES));
                  if (vhtCapabilities->GetVhtCapabilitiesInfo () != 0)
                    {
                      for (uint32_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!vhtCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (m_heSupported)
                {
                  //check whether the HE STA supports all MCSs in Basic MCS Set
                  Ptr<HeCapabilities> hecapabilities =
                      StaticCast<HeCapabilities> (assocReq.GetInformationElement (IE_HE_CAPABILITIES));
                  if (hecapabilities->GetSupportedMcsAndNss () != 0)
                    {
                      for (uint32_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!hecapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (problem)
                {
                  NS_LOG_DEBUG ("One of the Basic Rate set mode is not supported by the station: send association response with an error status");
                  SendAssocResp (hdr->GetAddr2 (), false, false);
                }
              else
                {
                  NS_LOG_DEBUG ("The Basic Rate set modes are supported by the station");
                  //record all its supported modes in its associated WifiRemoteStation
                  for (uint8_t j = 0; j < m_phy->GetNModes (); j++)
                    {
                      WifiMode mode = m_phy->GetMode (j);
                      if (rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                        {
                          m_stationManager->AddSupportedMode (from, mode);
                        }
                    }
                  if (m_htSupported)
                    {
                      Ptr<HtCapabilities> htCapabilities =
                          StaticCast<HtCapabilities> (assocReq.GetInformationElement (IE_HT_CAPABILITIES));
                      if (htCapabilities->IsSupportedMcs (0))
                        {
                          m_stationManager->AddStationHtCapabilities (from, htCapabilities);
                        }
                    }
                  if (m_vhtSupported)
                    {
                      Ptr<VhtCapabilities> vhtCapabilities =
                          StaticCast<VhtCapabilities> (assocReq.GetInformationElement (IE_VHT_CAPABILITIES));
                      //we will always fill in RxHighestSupportedLgiDataRate field at TX, so this can be used to check whether it supports VHT
                      if (vhtCapabilities->GetRxHighestSupportedLgiDataRate () > 0)
                        {
                          m_stationManager->AddStationVhtCapabilities (from, vhtCapabilities);
                          for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                            {
                              WifiMode mcs = m_phy->GetMcs (i);
                              if (mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT && vhtCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                                {
                                  m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                                  //here should add a control to add basic MCS when it is implemented
                                }
                            }
                        }
                    }
                  if (m_htSupported || m_vhtSupported)
                    {
                      Ptr<ExtendedCapabilities> extendedCapabilities =
                          StaticCast<ExtendedCapabilities> (assocReq.GetInformationElement (IE_EXTENDED_CAPABILITIES));
                      //TODO: to be completed
                    }
                  if (m_heSupported)
                    {
                      Ptr<HeCapabilities> heCapabilities =
                          StaticCast<HeCapabilities> (assocReq.GetInformationElement (IE_HE_CAPABILITIES));
                      //todo: once we support non constant rate managers, we should add checks here whether HE is supported by the peer
                      m_stationManager->AddStationHeCapabilities (from, heCapabilities);
                      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                        {
                          WifiMode mcs = m_phy->GetMcs (i);
                          if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HE && heCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                              //here should add a control to add basic MCS when it is implemented
                            }
                        }
                    }
                  m_stationManager->RecordWaitAssocTxOk (from);
                  if (!isHtStation)
                    {
                      m_nonHtStations.push_back (hdr->GetAddr2 ());
                      m_nonHtStations.unique ();
                    }
                  if (!isErpStation && isDsssStation)
                    {
                      m_nonErpStations.push_back (hdr->GetAddr2 ());
                      m_nonErpStations.unique ();
                    }
                  NS_LOG_DEBUG ("Send association response with success status");
                  SendAssocResp (hdr->GetAddr2 (), true, false);
                }
              return;
            }
          else if (hdr->IsReassocReq ())
            {
              NS_LOG_DEBUG ("Reassociation request received from " << from);
              //first, verify that the the station's supported
              //rate set is compatible with our Basic Rate set
              MgtReassocRequestHeader reassocReq;
              packet->RemoveHeader (reassocReq);
              CapabilityInformation capabilities = reassocReq.GetCapabilities ();
              m_stationManager->AddSupportedPlcpPreamble (from, capabilities.IsShortPreamble ());
              SupportedRates rates = reassocReq.GetSupportedRates ();
              bool problem = false;
              bool isHtStation = false;
              bool isOfdmStation = false;
              bool isErpStation = false;
              bool isDsssStation = false;
              for (uint8_t i = 0; i < m_stationManager->GetNBasicModes (); i++)
                {
                  WifiMode mode = m_stationManager->GetBasicMode (i);
                  if (!rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                    {
                      if ((mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
                        {
                          isDsssStation = false;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
                        {
                          isErpStation = false;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM)
                        {
                          isOfdmStation = false;
                        }
                      if (isDsssStation == false && isErpStation == false && isOfdmStation == false)
                        {
                          problem = true;
                          break;
                        }
                    }
                  else
                    {
                      if ((mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
                        {
                          isDsssStation = true;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
                        {
                          isErpStation = true;
                        }
                      else if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM)
                        {
                          isOfdmStation = true;
                        }
                    }
                }
              m_stationManager->AddSupportedErpSlotTime (from, capabilities.IsShortSlotTime () && isErpStation);
              if (m_htSupported)
                {
                  //check whether the HT STA supports all MCSs in Basic MCS Set
                  Ptr<HtCapabilities> htcapabilities
                      = StaticCast<HtCapabilities> (reassocReq.GetInformationElement (IE_HT_CAPABILITIES));
                  if (htcapabilities->IsSupportedMcs (0))
                    {
                      isHtStation = true;
                      for (uint8_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!htcapabilities->IsSupportedMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (m_vhtSupported)
                {
                  //check whether the VHT STA supports all MCSs in Basic MCS Set
                  Ptr<VhtCapabilities> vhtcapabilities
                      = StaticCast<VhtCapabilities> (reassocReq.GetInformationElement (IE_VHT_CAPABILITIES));
                  if (vhtcapabilities->GetVhtCapabilitiesInfo () != 0)
                    {
                      for (uint8_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!vhtcapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (m_heSupported)
                {
                  //check whether the HE STA supports all MCSs in Basic MCS Set
                  Ptr<HeCapabilities> hecapabilities
                      = StaticCast<HeCapabilities> (reassocReq.GetInformationElement (IE_HE_CAPABILITIES));
                  if (hecapabilities->GetSupportedMcsAndNss () != 0)
                    {
                      for (uint8_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                        {
                          WifiMode mcs = m_stationManager->GetBasicMcs (i);
                          if (!hecapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              problem = true;
                              break;
                            }
                        }
                    }
                }
              if (problem)
                {
                  NS_LOG_DEBUG ("One of the Basic Rate set mode is not supported by the station: send reassociation response with an error status");
                  SendAssocResp (hdr->GetAddr2 (), false, true);
                }
              else
                {
                  NS_LOG_DEBUG ("The Basic Rate set modes are supported by the station");
                  //update all its supported modes in its associated WifiRemoteStation
                  for (uint8_t j = 0; j < m_phy->GetNModes (); j++)
                    {
                      WifiMode mode = m_phy->GetMode (j);
                      if (rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                        {
                          m_stationManager->AddSupportedMode (from, mode);
                        }
                    }
                  if (m_htSupported)
                    {
                      Ptr<HtCapabilities> htCapabilities
                          = StaticCast<HtCapabilities> (reassocReq.GetInformationElement (IE_HT_CAPABILITIES));
                      if (htCapabilities->IsSupportedMcs (0))
                        {
                          m_stationManager->AddStationHtCapabilities (from, htCapabilities);
                        }
                    }
                  if (m_vhtSupported)
                    {
                      Ptr<VhtCapabilities> vhtCapabilities
                          = StaticCast<VhtCapabilities> (reassocReq.GetInformationElement (IE_VHT_CAPABILITIES));
                      //we will always fill in RxHighestSupportedLgiDataRate field at TX, so this can be used to check whether it supports VHT
                      if (vhtCapabilities->GetRxHighestSupportedLgiDataRate () > 0)
                        {
                          m_stationManager->AddStationVhtCapabilities (from, vhtCapabilities);
                          for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                            {
                              WifiMode mcs = m_phy->GetMcs (i);
                              if (mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT && vhtCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                                {
                                  m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                                  //here should add a control to add basic MCS when it is implemented
                                }
                            }
                        }
                    }
                  if (m_htSupported || m_vhtSupported)
                    {
                      Ptr<ExtendedCapabilities> extendedCapabilities
                          = StaticCast<ExtendedCapabilities> (reassocReq.GetInformationElement (IE_EXTENDED_CAPABILITIES));
                      //TODO: to be completed
                    }
                  if (m_heSupported)
                    {
                      Ptr<HeCapabilities> heCapabilities
                          = StaticCast<HeCapabilities> (reassocReq.GetInformationElement (IE_HE_CAPABILITIES));
                      //todo: once we support non constant rate managers, we should add checks here whether HE is supported by the peer
                      m_stationManager->AddStationHeCapabilities (from, heCapabilities);
                      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                        {
                          WifiMode mcs = m_phy->GetMcs (i);
                          if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HE && heCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                            {
                              m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                              //here should add a control to add basic MCS when it is implemented
                            }
                        }
                    }
                  m_stationManager->RecordWaitAssocTxOk (from);
                  if (!isHtStation)
                    {
                      m_nonHtStations.push_back (hdr->GetAddr2 ());
                      m_nonHtStations.unique ();
                    }
                  if (!isErpStation && isDsssStation)
                    {
                      m_nonErpStations.push_back (hdr->GetAddr2 ());
                      m_nonErpStations.unique ();
                    }
                  NS_LOG_DEBUG ("Send reassociation response with success status");
                  SendAssocResp (hdr->GetAddr2 (), true, true);
                }
              return;
            }
          else if (hdr->IsDisassociation ())
            {
              NS_LOG_DEBUG ("Disassociation received from " << from);
              m_stationManager->RecordDisassociated (from);
              for (std::map<uint16_t, Mac48Address>::const_iterator j = m_staList.begin (); j != m_staList.end (); j++)
                {
                  if (j->second == from)
                    {
                      m_staList.erase (j);
                      break;
                    }
                }
              for (std::list<Mac48Address>::const_iterator j = m_nonErpStations.begin (); j != m_nonErpStations.end (); j++)
                {
                  if ((*j) == from)
                    {
                      m_nonErpStations.erase (j);
                      break;
                    }
                }
              for (std::list<Mac48Address>::const_iterator j = m_nonHtStations.begin (); j != m_nonHtStations.end (); j++)
                {
                  if ((*j) == from)
                    {
                      m_nonHtStations.erase (j);
                      break;
                    }
                }
              return;
            }
        }
    }

  //Invoke the receive handler of our parent class to deal with any
  //other frames. Specifically, this will handle Block Ack-related
  //Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

void
ApWifiMac::DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << aggregatedPacket << hdr);
  MsduAggregator::DeaggregatedMsdus packets = MsduAggregator::Deaggregate (aggregatedPacket);
  for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin ();
       i != packets.end (); ++i)
    {
      if ((*i).second.GetDestinationAddr () == GetAddress ())
        {
          ForwardUp ((*i).first, (*i).second.GetSourceAddr (),
                     (*i).second.GetDestinationAddr ());
        }
      else
        {
          Mac48Address from = (*i).second.GetSourceAddr ();
          Mac48Address to = (*i).second.GetDestinationAddr ();
          NS_LOG_DEBUG ("forwarding QoS frame from=" << from << ", to=" << to);
          ForwardDown ((*i).first, from, to, hdr->GetQosTid ());
        }
    }
}

void
ApWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_beaconDca->Initialize ();
  m_beaconEvent.Cancel ();
  if (m_enableBeaconGeneration)
    {
      if (m_enableBeaconJitter)
        {
          int64_t jitter = m_beaconJitter->GetValue (0, m_beaconInterval.GetMicroSeconds ());
          NS_LOG_DEBUG ("Scheduling initial beacon for access point " << GetAddress () << " at time " << jitter << " microseconds");
          m_beaconEvent = Simulator::Schedule (MicroSeconds (jitter), &ApWifiMac::SendOneBeacon, this);
        }
      else
        {
          NS_LOG_DEBUG ("Scheduling initial beacon for access point " << GetAddress () << " at time 0");
          m_beaconEvent = Simulator::ScheduleNow (&ApWifiMac::SendOneBeacon, this);
        }
    }
  RegularWifiMac::DoInitialize ();
}

bool
ApWifiMac::GetUseNonErpProtection (void) const
{
  bool useProtection = !m_nonErpStations.empty () && m_enableNonErpProtection;
  m_stationManager->SetUseNonErpProtection (useProtection);
  return useProtection;
}


Ptr<ExtendedCapabilities>
ApWifiMac::GetExtendedCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<ExtendedCapabilities> capabilities = Create<ExtendedCapabilities> ();
  if (m_htSupported || m_vhtSupported)
    {
      if (m_htSupported)
        {
          capabilities->SetHtSupported (1);
        }
      if (m_vhtSupported)
        {
          capabilities->SetVhtSupported (1);
        }
    }
  return capabilities;
}

Ptr<HtCapabilities>
ApWifiMac::GetHtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<HtCapabilities> capabilities = Create<HtCapabilities> ();
  if (m_htSupported)
    {
      capabilities->SetHtSupported (1);
      capabilities->SetLdpc (m_phy->GetLdpc ());
      capabilities->SetSupportedChannelWidth (m_phy->GetChannelWidth () >= 40);
      capabilities->SetShortGuardInterval20 (m_phy->GetShortGuardInterval ());
      capabilities->SetShortGuardInterval40 (m_phy->GetChannelWidth () >= 40 && m_phy->GetShortGuardInterval ());
      capabilities->SetGreenfield (m_phy->GetGreenfield ());
      uint32_t maxAmsduLength = std::max (std::max (m_beMaxAmsduSize, m_bkMaxAmsduSize), std::max (m_voMaxAmsduSize, m_viMaxAmsduSize));
      capabilities->SetMaxAmsduLength (maxAmsduLength > 3839); //0 if 3839 and 1 if 7935
      capabilities->SetLSigProtectionSupport (!m_phy->GetGreenfield ());
      double maxAmpduLengthExponent = std::max (std::ceil ((std::log (std::max (std::max (m_beMaxAmpduSize, m_bkMaxAmpduSize), std::max (m_voMaxAmpduSize, m_viMaxAmpduSize))
                                                                      + 1.0)
                                                            / std::log (2.0))
                                                           - 13.0),
                                                0.0);
      NS_ASSERT (maxAmpduLengthExponent >= 0 && maxAmpduLengthExponent <= 255);
      capabilities->SetMaxAmpduLength (std::max<uint8_t> (3, static_cast<uint8_t> (maxAmpduLengthExponent))); //0 to 3 for HT
      uint64_t maxSupportedRate = 0; //in bit/s
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if (mcs.GetModulationClass () != WIFI_MOD_CLASS_HT)
            {
              continue;
            }
          capabilities->SetRxMcsBitmask (mcs.GetMcsValue ());
          uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
          NS_ASSERT (nss > 0 && nss < 5);
          uint64_t dataRate = mcs.GetDataRate (m_phy->GetChannelWidth (), m_phy->GetShortGuardInterval () ? 400 : 800, nss);
          if (dataRate > maxSupportedRate)
            {
              maxSupportedRate = dataRate;
              NS_LOG_DEBUG ("Updating maxSupportedRate to " << maxSupportedRate);
            }
        }
      capabilities->SetRxHighestSupportedDataRate (maxSupportedRate / 1e6); //in Mbit/s
      capabilities->SetTxMcsSetDefined (m_phy->GetNMcs () > 0);
      capabilities->SetTxMaxNSpatialStreams (m_phy->GetMaxSupportedTxSpatialStreams ());
      //we do not support unequal modulations
      capabilities->SetTxRxMcsSetUnequal (0);
      capabilities->SetTxUnequalModulation (0);
    }
  return capabilities;
}

Ptr<VhtCapabilities>
ApWifiMac::GetVhtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<VhtCapabilities> capabilities = Create<VhtCapabilities> ();
  if (m_vhtSupported)
    {
      capabilities->SetVhtSupported (1);
      if (m_phy->GetChannelWidth () == 160)
        {
          capabilities->SetSupportedChannelWidthSet (1);
        }
      else
        {
          capabilities->SetSupportedChannelWidthSet (0);
        }
      uint32_t maxMpduLength = std::max (std::max (m_beMaxAmsduSize, m_bkMaxAmsduSize), std::max (m_voMaxAmsduSize, m_viMaxAmsduSize)) + 56; //see section 9.11 of 11ac standard
      capabilities->SetMaxMpduLength (uint8_t (maxMpduLength > 3895) + uint8_t (maxMpduLength > 7991)); //0 if 3895, 1 if 7991, 2 for 11454
      capabilities->SetRxLdpc (m_phy->GetLdpc ());
      capabilities->SetShortGuardIntervalFor80Mhz ((m_phy->GetChannelWidth () == 80) && m_phy->GetShortGuardInterval ());
      capabilities->SetShortGuardIntervalFor160Mhz ((m_phy->GetChannelWidth () == 160) && m_phy->GetShortGuardInterval ());
      double maxAmpduLengthExponent = std::max (std::ceil ((std::log (std::max (std::max (m_beMaxAmpduSize, m_bkMaxAmpduSize), std::max (m_voMaxAmpduSize, m_viMaxAmpduSize)) + 1.0) / std::log (2.0)) - 13.0), 0.0);
      NS_ASSERT (maxAmpduLengthExponent >= 0 && maxAmpduLengthExponent <= 255);
      capabilities->SetMaxAmpduLengthExponent (std::max<uint8_t> (7, static_cast<uint8_t> (maxAmpduLengthExponent))); //0 to 7 for VHT
      uint8_t maxMcs = 0;
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if ((mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT)
              && (mcs.GetMcsValue () > maxMcs))
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      // Support same MaxMCS for each spatial stream
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedRxSpatialStreams (); nss++)
        {
          capabilities->SetRxMcsMap (maxMcs, nss);
        }
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedTxSpatialStreams (); nss++)
        {
          capabilities->SetTxMcsMap (maxMcs, nss);
        }
      uint64_t maxSupportedRateLGI = 0; //in bit/s
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if (mcs.GetModulationClass () != WIFI_MOD_CLASS_VHT || !mcs.IsAllowed (m_phy->GetChannelWidth (), 1))
            {
              continue;
            }
          if (mcs.GetDataRate (m_phy->GetChannelWidth ()) > maxSupportedRateLGI)
            {
              maxSupportedRateLGI = mcs.GetDataRate (m_phy->GetChannelWidth ());
              NS_LOG_DEBUG ("Updating maxSupportedRateLGI to " << maxSupportedRateLGI);
            }
        }
      capabilities->SetRxHighestSupportedLgiDataRate (maxSupportedRateLGI / 1e6); //in Mbit/s
      capabilities->SetTxHighestSupportedLgiDataRate (maxSupportedRateLGI / 1e6); //in Mbit/s
      //To be filled in once supported
      capabilities->SetRxStbc (0);
      capabilities->SetTxStbc (0);
    }
  return capabilities;
}

Ptr<HeCapabilities>
ApWifiMac::GetHeCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<HeCapabilities> capabilities = Create<HeCapabilities> ();
  if (m_heSupported)
    {
      capabilities->SetHeSupported (1);
      uint8_t channelWidthSet = 0;
      if (m_phy->GetChannelWidth () >= 40 && Is2_4Ghz (m_phy->GetFrequency ()))
        {
          channelWidthSet |= 0x01;
        }
      if (m_phy->GetChannelWidth () >= 80 && Is5Ghz (m_phy->GetFrequency ()))
        {
          channelWidthSet |= 0x02;
        }
      if (m_phy->GetChannelWidth () >= 160 && Is5Ghz (m_phy->GetFrequency ()))
        {
          channelWidthSet |= 0x04;
        }
      capabilities->SetChannelWidthSet (channelWidthSet);
      uint8_t gi = 0;
      if (m_phy->GetGuardInterval () <= NanoSeconds (1600))
        {
          //todo: We assume for now that if we support 800ns GI then 1600ns GI is supported as well
          gi |= 0x01;
        }
      if (m_phy->GetGuardInterval () == NanoSeconds (800))
        {
          gi |= 0x02;
        }
      capabilities->SetHeLtfAndGiForHePpdus (gi);
      double maxAmpduLengthExponent = std::max (std::ceil ((std::log (std::max (std::max (m_beMaxAmpduSize, m_bkMaxAmpduSize), std::max (m_voMaxAmpduSize, m_viMaxAmpduSize))
                                                                      + 1.0)
                                                            / std::log (2.0))
                                                           - 13.0),
                                                0.0);
      NS_ASSERT (maxAmpduLengthExponent >= 0 && maxAmpduLengthExponent <= 255);
      capabilities->SetMaxAmpduLengthExponent (std::max<uint8_t> (7, static_cast<uint8_t> (maxAmpduLengthExponent))); //assume 0 to 7 for HE
      uint8_t maxMcs = 0;
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if ((mcs.GetModulationClass () == WIFI_MOD_CLASS_HE)
              && (mcs.GetMcsValue () > maxMcs))
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      capabilities->SetHighestMcsSupported (maxMcs);
      capabilities->SetHighestNssSupported (m_phy->GetMaxSupportedTxSpatialStreams ());
    }
  return capabilities;
}

bool
ApWifiMac::GetRifsMode (void) const
{
  bool rifsMode = false;
  if (m_htSupported && !m_vhtSupported) //RIFS mode is forbidden for VHT
    {
      if (m_nonHtStations.empty () || !m_disableRifs)
        {
          rifsMode = true;
        }
    }
  if (GetRifsSupported () && rifsMode)
    {
      m_stationManager->SetRifsPermitted (true);
    }
  else
    {
      m_stationManager->SetRifsPermitted (false);
    }
  return rifsMode;
}

uint16_t
ApWifiMac::GetNextAssociationId (void)
{
  //Return the first free AID value between 1 and 2007
  for (uint16_t nextAid = 1; nextAid <= 2007; nextAid++)
    {
      if (m_staList.find (nextAid) == m_staList.end ())
        {
          return nextAid;
        }
    }
  NS_ASSERT_MSG (false, "No free association ID available!");
  return 0;
}

} //namespace ns3
