/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "wifi-remote-station-manager.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/enum.h"
#include "wifi-mac.h"
#include "wifi-utils.h"
#include "wifi-mac-header.h"
#include "wifi-mac-trailer.h"

/***************************************************************
 *           Packet Mode Tagger
 ***************************************************************/

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiRemoteStationManager");

/**
 * HighLatencyDataTxVectorTag class
 */
class HighLatencyDataTxVectorTag : public Tag
{
public:
  HighLatencyDataTxVectorTag ();
  /**
   * Constructor
   *
   * \param dataTxVector TXVECTOR for data frames
   */
  HighLatencyDataTxVectorTag (WifiTxVector dataTxVector);
  /**
   * \returns the transmission mode to use to send this packet
   */
  WifiTxVector GetDataTxVector (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  WifiTxVector m_dataTxVector; ///< TXVECTOR for data frames
};

HighLatencyDataTxVectorTag::HighLatencyDataTxVectorTag ()
{
}

HighLatencyDataTxVectorTag::HighLatencyDataTxVectorTag (WifiTxVector dataTxVector)
  : m_dataTxVector (dataTxVector)
{
}

WifiTxVector
HighLatencyDataTxVectorTag::GetDataTxVector (void) const
{
  return m_dataTxVector;
}

TypeId
HighLatencyDataTxVectorTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HighLatencyDataTxVectorTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HighLatencyDataTxVectorTag> ()
  ;
  return tid;
}

TypeId
HighLatencyDataTxVectorTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
HighLatencyDataTxVectorTag::GetSerializedSize (void) const
{
  return sizeof (WifiTxVector);
}

void
HighLatencyDataTxVectorTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_dataTxVector, sizeof (WifiTxVector));
}

void
HighLatencyDataTxVectorTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_dataTxVector, sizeof (WifiTxVector));
}

void
HighLatencyDataTxVectorTag::Print (std::ostream &os) const
{
  os << "Data=" << m_dataTxVector;
}

/**
 * HighLatencyRtsTxVectorTag class
 */
class HighLatencyRtsTxVectorTag : public Tag
{
public:
  HighLatencyRtsTxVectorTag ();
  /**
   * Constructor
   *
   * \param rtsTxVector TXVECTOR for RTS frames
   */
  HighLatencyRtsTxVectorTag (WifiTxVector rtsTxVector);
  /**
   * \returns the transmission mode to use to send the RTS prior to the
   *          transmission of the data packet itself.
   */
  WifiTxVector GetRtsTxVector (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  WifiTxVector m_rtsTxVector; ///< TXVECTOR for data frames
};

HighLatencyRtsTxVectorTag::HighLatencyRtsTxVectorTag ()
{
}

HighLatencyRtsTxVectorTag::HighLatencyRtsTxVectorTag (WifiTxVector rtsTxVector)
  : m_rtsTxVector (rtsTxVector)
{
}

WifiTxVector
HighLatencyRtsTxVectorTag::GetRtsTxVector (void) const
{
  return m_rtsTxVector;
}

TypeId
HighLatencyRtsTxVectorTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HighLatencyRtsTxVectorTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HighLatencyRtsTxVectorTag> ()
  ;
  return tid;
}

TypeId
HighLatencyRtsTxVectorTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
HighLatencyRtsTxVectorTag::GetSerializedSize (void) const
{
  return sizeof (WifiTxVector);
}

void
HighLatencyRtsTxVectorTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_rtsTxVector, sizeof (WifiTxVector));
}

void
HighLatencyRtsTxVectorTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_rtsTxVector, sizeof (WifiTxVector));
}

void
HighLatencyRtsTxVectorTag::Print (std::ostream &os) const
{
  os << "Rts=" << m_rtsTxVector;
}

/**
 * HighLatencyCtsToSelfTxVectorTag class
 */
class HighLatencyCtsToSelfTxVectorTag : public Tag
{
public:
  HighLatencyCtsToSelfTxVectorTag ();
  /**
   * Constructor
   *
   * \param ctsToSelfTxVector TXVECTOR for CTS-to-self frames
   */
  HighLatencyCtsToSelfTxVectorTag (WifiTxVector ctsToSelfTxVector);
  /**
   * \returns the transmission mode to use for the CTS-to-self.
   */
  WifiTxVector GetCtsToSelfTxVector (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  WifiTxVector m_ctsToSelfTxVector; ///< TXVECTOR for CTS-to-self frames
};

HighLatencyCtsToSelfTxVectorTag::HighLatencyCtsToSelfTxVectorTag ()
{
}

HighLatencyCtsToSelfTxVectorTag::HighLatencyCtsToSelfTxVectorTag (WifiTxVector ctsToSelfTxVector)
  : m_ctsToSelfTxVector (ctsToSelfTxVector)
{
}

WifiTxVector
HighLatencyCtsToSelfTxVectorTag::GetCtsToSelfTxVector (void) const
{
  return m_ctsToSelfTxVector;
}

TypeId
HighLatencyCtsToSelfTxVectorTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HighLatencyCtsToSelfTxVectorTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HighLatencyCtsToSelfTxVectorTag> ()
  ;
  return tid;
}

TypeId
HighLatencyCtsToSelfTxVectorTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
HighLatencyCtsToSelfTxVectorTag::GetSerializedSize (void) const
{
  return sizeof (WifiTxVector);
}

void
HighLatencyCtsToSelfTxVectorTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_ctsToSelfTxVector, sizeof (WifiTxVector));
}

void
HighLatencyCtsToSelfTxVectorTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_ctsToSelfTxVector, sizeof (WifiTxVector));
}

void
HighLatencyCtsToSelfTxVectorTag::Print (std::ostream &os) const
{
  os << "Cts To Self=" << m_ctsToSelfTxVector;
}

} //namespace ns3

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WifiRemoteStationManager);

TypeId
WifiRemoteStationManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiRemoteStationManager")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("IsLowLatency",
                   "If true, we attempt to modelize a so-called low-latency device: "
                   "a device where decisions about tx parameters can be made on a per-packet basis and "
                   "feedback about the transmission of each packet is obtained before sending the next. "
                   "Otherwise, we modelize a high-latency device, that is a device where we cannot update "
                   "our decision about tx parameters after every packet transmission.",
                   TypeId::ATTR_GET,
                   BooleanValue (true), //this value is ignored because there is no setter
                   MakeBooleanAccessor (&WifiRemoteStationManager::IsLowLatency),
                   MakeBooleanChecker ())
    .AddAttribute ("MaxSsrc",
                   "The maximum number of retransmission attempts for an RTS. "
                   " This value will not have any effect on some rate control algorithms.",
                   UintegerValue (7),
                   MakeUintegerAccessor (&WifiRemoteStationManager::m_maxSsrc),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxSlrc",
                   "The maximum number of retransmission attempts for a DATA packet. "
                   "This value will not have any effect on some rate control algorithms.",
                   UintegerValue (7),
                   MakeUintegerAccessor (&WifiRemoteStationManager::m_maxSlrc),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RtsCtsThreshold",
                   "If the size of the PSDU is bigger than this value, we use an RTS/CTS handshake before sending the data frame."
                   "This value will not have any effect on some rate control algorithms.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&WifiRemoteStationManager::m_rtsCtsThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FragmentationThreshold",
                   "If the size of the PSDU is bigger than this value, we fragment it such that the size of the fragments are equal or smaller. "
                   "This value does not apply when it is carried in an A-MPDU. "
                   "This value will not have any effect on some rate control algorithms.",
                   UintegerValue (2346),
                   MakeUintegerAccessor (&WifiRemoteStationManager::DoSetFragmentationThreshold,
                                         &WifiRemoteStationManager::DoGetFragmentationThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NonUnicastMode",
                   "Wifi mode used for non-unicast transmissions.",
                   WifiModeValue (),
                   MakeWifiModeAccessor (&WifiRemoteStationManager::m_nonUnicastMode),
                   MakeWifiModeChecker ())
    .AddAttribute ("DefaultTxPowerLevel",
                   "Default power level to be used for transmissions. "
                   "This is the power level that is used by all those WifiManagers that do not implement TX power control.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiRemoteStationManager::m_defaultTxPowerLevel),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("ErpProtectionMode",
                   "Protection mode used when non-ERP STAs are connected to an ERP AP: Rts-Cts or Cts-To-Self",
                   EnumValue (WifiRemoteStationManager::CTS_TO_SELF),
                   MakeEnumAccessor (&WifiRemoteStationManager::SetErpProtectionMode,
                                     &WifiRemoteStationManager::GetErpProtectionMode),
                   MakeEnumChecker (WifiRemoteStationManager::RTS_CTS, "Rts-Cts",
                                    WifiRemoteStationManager::CTS_TO_SELF, "Cts-To-Self"))
    .AddAttribute ("HtProtectionMode",
                   "Protection mode used when non-HT STAs are connected to a HT AP: Rts-Cts or Cts-To-Self",
                   EnumValue (WifiRemoteStationManager::CTS_TO_SELF),
                   MakeEnumAccessor (&WifiRemoteStationManager::SetHtProtectionMode,
                                     &WifiRemoteStationManager::GetHtProtectionMode),
                   MakeEnumChecker (WifiRemoteStationManager::RTS_CTS, "Rts-Cts",
                                    WifiRemoteStationManager::CTS_TO_SELF, "Cts-To-Self"))
    .AddTraceSource ("MacTxRtsFailed",
                     "The transmission of a RTS by the MAC layer has failed",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxRtsFailed),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("MacTxDataFailed",
                     "The transmission of a data packet by the MAC layer has failed",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxDataFailed),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("MacTxFinalRtsFailed",
                     "The transmission of a RTS has exceeded the maximum number of attempts",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxFinalRtsFailed),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("MacTxFinalDataFailed",
                     "The transmission of a data packet has exceeded the maximum number of attempts",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxFinalDataFailed),
                     "ns3::Mac48Address::TracedCallback")
    /* New Traces */
    .AddTraceSource ("MacTxOK",
                     "The transmission of an MPDU packet by the MAC layer has successed",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxOk),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("MacRxOK",
                     "The reception of an MPDU packet by the MAC layer has successed",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macRxOk),
                     "ns3::WifiRemoteStationManager::MacRxOkTracedCallback")
  ;
  return tid;
}

WifiRemoteStationManager::WifiRemoteStationManager ()
  : m_qosSupported (false),
    m_htSupported (false),
    m_vhtSupported (false),
    m_heSupported (false),
    m_dmgSupported (false),
    m_useNonErpProtection (false),
    m_useNonHtProtection (false),
    m_useGreenfieldProtection (false),
    m_shortPreambleEnabled (false),
    m_shortSlotTimeEnabled (false),
    m_rifsPermitted (false)
{
  NS_LOG_FUNCTION (this);
}

WifiRemoteStationManager::~WifiRemoteStationManager ()
{
  NS_LOG_FUNCTION (this);
}

void
WifiRemoteStationManager::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Reset ();
}

void
WifiRemoteStationManager::SetupPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  //We need to track our PHY because it is the object that knows the
  //full set of transmit rates that are supported. We need to know
  //this in order to find the relevant mandatory rates when chosing a
  //transmit rate for automatic control responses like
  //acknowledgements.
  m_wifiPhy = phy;
  m_defaultTxMode = phy->GetMode (0);
  NS_ASSERT (m_defaultTxMode.IsMandatory ());
  if (HasHtSupported () || HasVhtSupported () || HasHeSupported ())
    {
      m_defaultTxMcs = phy->GetMcs (0);
    }
  Reset ();
}

void
WifiRemoteStationManager::SetupMac (const Ptr<WifiMac> mac)
{
  NS_LOG_FUNCTION (this << mac);
  //We need to track our MAC because it is the object that knows the
  //full set of interframe spaces.
  m_wifiMac = mac;
  Reset ();
}

void
WifiRemoteStationManager::SetQosSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_qosSupported = enable;
}

void
WifiRemoteStationManager::SetHtSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_htSupported = enable;
}

void
WifiRemoteStationManager::SetMaxSsrc (uint32_t maxSsrc)
{
  NS_LOG_FUNCTION (this << maxSsrc);
  m_maxSsrc = maxSsrc;
}

void
WifiRemoteStationManager::SetMaxSlrc (uint32_t maxSlrc)
{
  NS_LOG_FUNCTION (this << maxSlrc);
  m_maxSlrc = maxSlrc;
}

void
WifiRemoteStationManager::SetRtsCtsThreshold (uint32_t threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_rtsCtsThreshold = threshold;
}

void
WifiRemoteStationManager::SetFragmentationThreshold (uint32_t threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  DoSetFragmentationThreshold (threshold);
}

void
WifiRemoteStationManager::SetErpProtectionMode (WifiRemoteStationManager::ProtectionMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_erpProtectionMode = mode;
}

void
WifiRemoteStationManager::SetHtProtectionMode (WifiRemoteStationManager::ProtectionMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_htProtectionMode = mode;
}

void
WifiRemoteStationManager::SetShortPreambleEnabled (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortPreambleEnabled = enable;
}

void
WifiRemoteStationManager::SetShortSlotTimeEnabled (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortSlotTimeEnabled = enable;
}

void
WifiRemoteStationManager::SetRifsPermitted (bool allow)
{
  NS_LOG_FUNCTION (this << allow);
  m_rifsPermitted = allow;
}

bool
WifiRemoteStationManager::GetShortSlotTimeEnabled (void) const
{
  return m_shortSlotTimeEnabled;
}

bool
WifiRemoteStationManager::GetShortPreambleEnabled (void) const
{
  return m_shortPreambleEnabled;
}

bool
WifiRemoteStationManager::GetRifsPermitted (void) const
{
  return m_rifsPermitted;
}

WifiRemoteStationManager::ProtectionMode
WifiRemoteStationManager::GetErpProtectionMode (void) const
{
  return m_erpProtectionMode;
}

WifiRemoteStationManager::ProtectionMode
WifiRemoteStationManager::GetHtProtectionMode (void) const
{
  return m_htProtectionMode;
}

bool
WifiRemoteStationManager::HasQosSupported (void) const
{
  return m_qosSupported;
}

bool
WifiRemoteStationManager::HasHtSupported (void) const
{
  return m_htSupported;
}

void
WifiRemoteStationManager::SetVhtSupported (bool enable)
{
  m_vhtSupported = enable;
}

bool
WifiRemoteStationManager::HasVhtSupported (void) const
{
  return m_vhtSupported;
}

void
WifiRemoteStationManager::SetHeSupported (bool enable)
{
  m_heSupported = enable;
}

bool
WifiRemoteStationManager::HasHeSupported (void) const
{
  return m_heSupported;
}

void
WifiRemoteStationManager::SetDmgSupported (bool enable)
{
  m_dmgSupported = enable;
}

bool
WifiRemoteStationManager::HasDmgSupported (void) const
{
  return m_dmgSupported;
}

uint32_t
WifiRemoteStationManager::GetMaxSsrc (void) const
{
  return m_maxSsrc;
}

uint32_t
WifiRemoteStationManager::GetMaxSlrc (void) const
{
  return m_maxSlrc;
}

uint32_t
WifiRemoteStationManager::GetRtsCtsThreshold (void) const
{
  return m_rtsCtsThreshold;
}

uint32_t
WifiRemoteStationManager::GetFragmentationThreshold (void) const
{
  return DoGetFragmentationThreshold ();
}

void
WifiRemoteStationManager::AddSupportedPlcpPreamble (Mac48Address address, bool isShortPreambleSupported)
{
  NS_LOG_FUNCTION (this << address << isShortPreambleSupported);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_shortPreamble = isShortPreambleSupported;
}

void
WifiRemoteStationManager::AddSupportedErpSlotTime (Mac48Address address, bool isShortSlotTimeSupported)
{
  NS_LOG_FUNCTION (this << address << isShortSlotTimeSupported);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_shortSlotTime = isShortSlotTimeSupported;
}

void
WifiRemoteStationManager::AddSupportedMode (Mac48Address address, WifiMode mode)
{
  NS_LOG_FUNCTION (this << address << mode);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  for (WifiModeListIterator i = state->m_operationalRateSet.begin (); i != state->m_operationalRateSet.end (); i++)
    {
      if ((*i) == mode)
        {
          //already in.
          return;
        }
    }
  state->m_operationalRateSet.push_back (mode);
}

void
WifiRemoteStationManager::AddAllSupportedModes (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_operationalRateSet.clear ();
  for (uint8_t i = 0; i < m_wifiPhy->GetNModes (); i++)
    {
      state->m_operationalRateSet.push_back (m_wifiPhy->GetMode (i));
      if (m_wifiPhy->GetMode (i).IsMandatory ())
        {
          AddBasicMode (m_wifiPhy->GetMode (i));
        }
    }
}

void
WifiRemoteStationManager::AddAllSupportedMcs (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_operationalMcsSet.clear ();
  for (uint8_t i = 0; i < m_wifiPhy->GetNMcs (); i++)
    {
      state->m_operationalMcsSet.push_back (m_wifiPhy->GetMcs (i));
    }
}

void
WifiRemoteStationManager::RemoveAllSupportedMcs (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_operationalMcsSet.clear ();
}

void
WifiRemoteStationManager::AddSupportedMcs (Mac48Address address, WifiMode mcs)
{
  NS_LOG_FUNCTION (this << address << mcs);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  for (WifiModeListIterator i = state->m_operationalMcsSet.begin (); i != state->m_operationalMcsSet.end (); i++)
    {
      if ((*i) == mcs)
        {
          //already in.
          return;
        }
    }
  state->m_operationalMcsSet.push_back (mcs);
}

bool
WifiRemoteStationManager::GetShortPreambleSupported (Mac48Address address) const
{
  return LookupState (address)->m_shortPreamble;
}

bool
WifiRemoteStationManager::GetShortSlotTimeSupported (Mac48Address address) const
{
  return LookupState (address)->m_shortSlotTime;
}

bool
WifiRemoteStationManager::GetQosSupported (Mac48Address address) const
{
  return LookupState (address)->m_qosSupported;
}

bool
WifiRemoteStationManager::IsBrandNew (Mac48Address address) const
{
  if (address.IsGroup ())
    {
      return false;
    }
  return LookupState (address)->m_state == WifiRemoteStationState::BRAND_NEW;
}

bool
WifiRemoteStationManager::IsAssociated (Mac48Address address) const
{
  if (address.IsGroup ())
    {
      return true;
    }
  return LookupState (address)->m_state == WifiRemoteStationState::GOT_ASSOC_TX_OK;
}

bool
WifiRemoteStationManager::IsWaitAssocTxOk (Mac48Address address) const
{
  if (address.IsGroup ())
    {
      return false;
    }
  return LookupState (address)->m_state == WifiRemoteStationState::WAIT_ASSOC_TX_OK;
}

void
WifiRemoteStationManager::RecordWaitAssocTxOk (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::WAIT_ASSOC_TX_OK;
}

void
WifiRemoteStationManager::RecordGotAssocTxOk (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::GOT_ASSOC_TX_OK;
}

void
WifiRemoteStationManager::RecordGotAssocTxFailed (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::DISASSOC;
}

void
WifiRemoteStationManager::RecordDisassociated (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::DISASSOC;
}

void
WifiRemoteStationManager::PrepareForQueue (Mac48Address address, const WifiMacHeader *header, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << *header << packet);
  if (IsLowLatency () || address.IsGroup ())
    {
      return;
    }
  WifiRemoteStation *station = Lookup (address, header);
  WifiTxVector rts = DoGetRtsTxVector (station);
  WifiTxVector data = DoGetDataTxVector (station);
  WifiTxVector ctstoself = DoGetCtsToSelfTxVector ();
  HighLatencyDataTxVectorTag datatag;
  HighLatencyRtsTxVectorTag rtstag;
  HighLatencyCtsToSelfTxVectorTag ctstoselftag;
  //first, make sure that the tag is not here anymore.
  ConstCast<Packet> (packet)->RemovePacketTag (datatag);
  ConstCast<Packet> (packet)->RemovePacketTag (rtstag);
  ConstCast<Packet> (packet)->RemovePacketTag (ctstoselftag);
  datatag = HighLatencyDataTxVectorTag (data);
  rtstag = HighLatencyRtsTxVectorTag (rts);
  ctstoselftag = HighLatencyCtsToSelfTxVectorTag (ctstoself);
  //and then, add it back
  packet->AddPacketTag (datatag);
  packet->AddPacketTag (rtstag);
  packet->AddPacketTag (ctstoselftag);
}

uint16_t
WifiRemoteStationManager::GetChannelWidthForTransmission (WifiMode mode, uint16_t maxSupportedChannelWidth)
{
  NS_LOG_FUNCTION (mode << +maxSupportedChannelWidth);
  WifiModulationClass modulationClass = mode.GetModulationClass ();
  if (maxSupportedChannelWidth > 20
      && (modulationClass == WifiModulationClass::WIFI_MOD_CLASS_OFDM // all non-HT OFDM control and management frames
          || modulationClass == WifiModulationClass::WIFI_MOD_CLASS_ERP_OFDM)) // special case of beacons at 2.4 GHz
    {
      NS_LOG_LOGIC ("Channel width reduced to 20 MHz");
      return 20;
    }
  //at 2.4 GHz basic rate can be non-ERP DSSS
  if (modulationClass == WifiModulationClass::WIFI_MOD_CLASS_DSSS
      || modulationClass == WifiModulationClass::WIFI_MOD_CLASS_HR_DSSS)
    {
      return 22;
    }
  return maxSupportedChannelWidth;
}

WifiTxVector
WifiRemoteStationManager::GetDataTxVector (Mac48Address address, const WifiMacHeader *header, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << *header << packet);

  /* Beam Tracking is Requested */
  if (header->IsBeamTrackingRequested ())
    {
      WifiTxVector v = DoGetDataTxVector (Lookup (address, header));
      v.RequestBeamTracking ();
      v.SetPacketType (header->GetPacketType ());
      v.SetTrainngFieldLength (header->GetTrainngFieldLength ());
      return v;
    }

  if (address.IsGroup ())
    {
      WifiMode mode = GetNonUnicastMode ();
      WifiTxVector v;
      v.SetMode (mode);
      v.SetPreambleType (GetPreambleForTransmission (mode, address));
      v.SetTxPowerLevel (m_defaultTxPowerLevel);
      v.SetChannelWidth (GetChannelWidthForTransmission (mode, m_wifiPhy->GetChannelWidth ()));
      v.SetGuardInterval (ConvertGuardIntervalToNanoSeconds (mode, m_wifiPhy->GetShortGuardInterval (), m_wifiPhy->GetGuardInterval ()));
      v.SetNss (1);
      v.SetNess (0);
      v.SetStbc (m_wifiPhy->GetStbc ());
      return v;
    }

  if (!IsLowLatency ())
    {
      HighLatencyDataTxVectorTag datatag;
      bool found;
      found = ConstCast<Packet> (packet)->PeekPacketTag (datatag);
      NS_ASSERT (found);
      //cast found to void, to suppress 'found' set but not used
      //compiler warning
      (void) found;
      return datatag.GetDataTxVector ();
    }

  if (header->IsMgt () && m_dmgSupported)
    {
      WifiTxVector v;
      v.SetMode (WifiMode ("DMG_MCS0"));
      v.SetPreambleType (WIFI_PREAMBLE_LONG);
      v.SetTxPowerLevel (m_defaultTxPowerLevel);
      v.SetChannelWidth (m_wifiPhy->GetChannelWidth ());
      return v;
    }

  WifiTxVector txVector = DoGetDataTxVector (Lookup (address, header));
  if (header->IsMgt () && !m_dmgSupported)
    {
      //Use the lowest basic rate for management frames
      WifiMode mgtMode;
      if (GetNBasicModes () > 0)
        {
          mgtMode = GetBasicMode (0);
        }
      else
        {
          mgtMode = GetDefaultMode ();
        }
      txVector.SetMode (mgtMode);
      txVector.SetPreambleType (GetPreambleForTransmission (mgtMode, address));
      txVector.SetChannelWidth (GetChannelWidthForTransmission (mgtMode, m_wifiPhy->GetChannelWidth ()));
      txVector.SetGuardInterval (ConvertGuardIntervalToNanoSeconds (mgtMode, m_wifiPhy->GetShortGuardInterval (),
                                                                    m_wifiPhy->GetGuardInterval ()));
    }
  return txVector;
}

WifiTxVector
WifiRemoteStationManager::GetDmgTxVector (Mac48Address address, const WifiMacHeader *header, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << *header << packet);
  WifiTxVector v;

  if (header->IsActionNoAck ()) /* BRP Frame */
    {
      v.SetMode (WifiMode ("DMG_MCS0")); /* The BRP Frame shall be transmitted at DMG MCS0 */
      v.SetPacketType (header->GetPacketType ());
      v.SetTrainngFieldLength (header->GetTrainngFieldLength ());
    } 
  else if (header->IsDMGBeacon () || header->IsSSW () || header->IsSSW_FBCK () || header->IsSSW_ACK ()) /* Beamforming Training (SLS) */
    {
      v.SetMode (WifiMode ("DMG_MCS0"));
      v.SetTrainngFieldLength (0);
      v.SetPreambleType (WIFI_PREAMBLE_DMG_CTRL);
    }
  else if (header->IsPollFrame () || header->IsGrantFrame () || header->IsSprFrame ()) /* Dynamic Polling */
    {
      v.SetMode (WifiMode ("DMG_MCS1"));
      v.SetTrainngFieldLength (0);
    }

  v.SetPreambleType (WIFI_PREAMBLE_LONG);
  v.SetTxPowerLevel (m_defaultTxPowerLevel);
  v.SetChannelWidth (m_wifiPhy->GetChannelWidth ());

  return v;
}

WifiTxVector
WifiRemoteStationManager::GetCtsToSelfTxVector (const WifiMacHeader *header,
                                                Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << *header << packet);
  if (!IsLowLatency ())
    {
      HighLatencyCtsToSelfTxVectorTag ctstoselftag;
      bool found;
      found = ConstCast<Packet> (packet)->PeekPacketTag (ctstoselftag);
      NS_ASSERT (found);
      //cast found to void, to suppress 'found' set but not used
      //compiler warning
      (void) found;
      return ctstoselftag.GetCtsToSelfTxVector ();
    }
  return DoGetCtsToSelfTxVector ();
}

WifiTxVector
WifiRemoteStationManager::DoGetCtsToSelfTxVector (void)
{
  WifiMode defaultMode = GetDefaultMode ();
  WifiPreamble defaultPreamble;
  if (defaultMode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      defaultPreamble = WIFI_PREAMBLE_HE_SU;
    }
  else if (defaultMode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      defaultPreamble = WIFI_PREAMBLE_VHT;
    }
  else if (defaultMode.GetModulationClass () == WIFI_MOD_CLASS_HT)
    {
      defaultPreamble = WIFI_PREAMBLE_HT_MF;
    }
  else
    {
      defaultPreamble = WIFI_PREAMBLE_LONG;
    }

  return WifiTxVector (defaultMode,
                       GetDefaultTxPowerLevel (),
                       defaultPreamble,
                       ConvertGuardIntervalToNanoSeconds (defaultMode, m_wifiPhy->GetShortGuardInterval (), m_wifiPhy->GetGuardInterval ()),
                       GetNumberOfAntennas (),
                       GetMaxNumberOfTransmitStreams (),
                       0,
                       GetChannelWidthForTransmission (defaultMode, m_wifiPhy->GetChannelWidth ()),
                       false,
                       false);
}

WifiTxVector
WifiRemoteStationManager::GetRtsTxVector (Mac48Address address, const WifiMacHeader *header,
                                          Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << *header << packet);
  NS_ASSERT (!address.IsGroup ());
  if (!IsLowLatency ())
    {
      HighLatencyRtsTxVectorTag rtstag;
      bool found;
      found = ConstCast<Packet> (packet)->PeekPacketTag (rtstag);
      NS_ASSERT (found);
      //cast found to void, to suppress 'found' set but not used
      //compiler warning
      (void) found;
      return rtstag.GetRtsTxVector ();
    }
  if (HasDmgSupported ())
    {
      return GetDmgControlTxVector ();
    }
  return DoGetRtsTxVector (Lookup (address, header));
}

void
WifiRemoteStationManager::ReportRtsFailed (Mac48Address address, const WifiMacHeader *header)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  station->m_ssrc++;
  m_macTxRtsFailed (address);
  DoReportRtsFailed (station);
}

void
WifiRemoteStationManager::ReportDataFailed (Mac48Address address, const WifiMacHeader *header)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  station->m_slrc++;
  m_macTxDataFailed (address);
  DoReportDataFailed (station);
}

void
WifiRemoteStationManager::ReportRtsOk (Mac48Address address, const WifiMacHeader *header,
                                       double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << address << *header << ctsSnr << ctsMode << rtsSnr);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  station->m_state->m_info.NotifyTxSuccess (station->m_ssrc);
  station->m_ssrc = 0;
  m_macTxOk (address);
  m_txCallbackOk (address);
  DoReportRtsOk (station, ctsSnr, ctsMode, rtsSnr);
}

void
WifiRemoteStationManager::ReportDataOk (Mac48Address address, const WifiMacHeader *header,
                                        double ackSnr, WifiMode ackMode, double dataSnr)
{
  NS_LOG_FUNCTION (this << address << *header << ackSnr << ackMode << dataSnr);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  station->m_state->m_info.NotifyTxSuccess (station->m_slrc);
  station->m_slrc = 0;
  m_macTxOk (address);
  m_txCallbackOk (address);
  DoReportDataOk (station, ackSnr, ackMode, dataSnr);
}

void
WifiRemoteStationManager::ReportFinalRtsFailed (Mac48Address address, const WifiMacHeader *header)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  station->m_state->m_info.NotifyTxFailed ();
  station->m_ssrc = 0;
  m_macTxFinalRtsFailed (address);
  DoReportFinalRtsFailed (station);
}

void
WifiRemoteStationManager::ReportFinalDataFailed (Mac48Address address, const WifiMacHeader *header)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  station->m_state->m_info.NotifyTxFailed ();
  station->m_slrc = 0;
  m_macTxFinalDataFailed (address);
  DoReportFinalDataFailed (station);
}

void
WifiRemoteStationManager::ReportRxOk (Mac48Address address, const WifiMacHeader *header,
                                      double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << address << *header << rxSnr << txMode);
  m_rxSnr = rxSnr;
  if (address.IsGroup ())
    {
      return;
    }
  WifiRemoteStation *station = Lookup (address, header);
  m_rxCallbackOk (address);
  m_macRxOk (header->GetType (), address, rxSnr);
  DoReportRxOk (station, rxSnr, txMode);
}

void
WifiRemoteStationManager::ReportAmpduTxStatus (Mac48Address address, uint8_t tid,
                                               uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus,
                                               double rxSnr, double dataSnr)
{
  NS_LOG_FUNCTION (this << address << +tid << +nSuccessfulMpdus << +nFailedMpdus << rxSnr << dataSnr);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, tid);
  for (uint8_t i = 0; i < nFailedMpdus; i++)
    {
      m_macTxDataFailed (address);
    }
  DoReportAmpduTxStatus (station, nSuccessfulMpdus, nFailedMpdus, rxSnr, dataSnr);
}

bool
WifiRemoteStationManager::NeedRts (Mac48Address address, const WifiMacHeader *header,
                                   Ptr<const Packet> packet, WifiTxVector txVector)
{
  WifiMode mode = txVector.GetMode ();
  NS_LOG_FUNCTION (this << address << *header << packet << mode);
  if (address.IsGroup ())
    {
      return false;
    }
  if (m_erpProtectionMode == RTS_CTS
      && ((mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HE))
      && m_useNonErpProtection)
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedRTS returning true to protect non-ERP stations");
      return true;
    }
  else if (m_htProtectionMode == RTS_CTS
           && ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
               || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
           && m_useNonHtProtection
           && !(m_erpProtectionMode != RTS_CTS && m_useNonErpProtection))
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedRTS returning true to protect non-HT stations");
      return true;
    }
  bool normally = (packet->GetSize () + header->GetSize () + WIFI_MAC_FCS_LENGTH) > GetRtsCtsThreshold ();
  return DoNeedRts (Lookup (address, header), packet, normally);
}

bool
WifiRemoteStationManager::NeedCtsToSelf (WifiTxVector txVector)
{
  WifiMode mode = txVector.GetMode ();
  NS_LOG_FUNCTION (this << mode);
  if (m_erpProtectionMode == CTS_TO_SELF
      && ((mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HE))
      && m_useNonErpProtection)
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning true to protect non-ERP stations");
      return true;
    }
  else if (m_htProtectionMode == CTS_TO_SELF
           && ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
               || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
           && m_useNonHtProtection
           && !(m_erpProtectionMode != CTS_TO_SELF && m_useNonErpProtection))
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning true to protect non-HT stations");
      return true;
    }
  else if (!m_useNonErpProtection)
    {
      //search for the BSS Basic Rate set, if the used mode is in the basic set then there is no need for Cts To Self
      for (WifiModeListIterator i = m_bssBasicRateSet.begin (); i != m_bssBasicRateSet.end (); i++)
        {
          if (mode == *i)
            {
              NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning false");
              return false;
            }
        }
      if (HasHtSupported ())
        {
          //search for the BSS Basic MCS set, if the used mode is in the basic set then there is no need for Cts To Self
          for (WifiModeListIterator i = m_bssBasicMcsSet.begin (); i != m_bssBasicMcsSet.end (); i++)
            {
              if (mode == *i)
                {
                  NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning false");
                  return false;
                }
            }
        }
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning true");
      return true;
    }
  return false;
}

void
WifiRemoteStationManager::SetUseNonErpProtection (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_useNonErpProtection = enable;
}

bool
WifiRemoteStationManager::GetUseNonErpProtection (void) const
{
  return m_useNonErpProtection;
}

void
WifiRemoteStationManager::SetUseNonHtProtection (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_useNonHtProtection = enable;
}

bool
WifiRemoteStationManager::GetUseNonHtProtection (void) const
{
  return m_useNonHtProtection;
}

void
WifiRemoteStationManager::SetUseGreenfieldProtection (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_useGreenfieldProtection = enable;
}

bool
WifiRemoteStationManager::GetUseGreenfieldProtection (void) const
{
  return m_useGreenfieldProtection;
}

bool
WifiRemoteStationManager::NeedRtsRetransmission (Mac48Address address, const WifiMacHeader *header,
                                                 Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << packet << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  bool normally = station->m_ssrc < GetMaxSsrc ();
  NS_LOG_DEBUG ("WifiRemoteStationManager::NeedDataRetransmission count: " << station->m_ssrc << " result: " << std::boolalpha << normally);
  return DoNeedRtsRetransmission (station, packet, normally);
}

bool
WifiRemoteStationManager::NeedDataRetransmission (Mac48Address address, const WifiMacHeader *header,
                                                  Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << packet << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address, header);
  bool normally = station->m_slrc < GetMaxSlrc ();
  NS_LOG_DEBUG ("WifiRemoteStationManager::NeedDataRetransmission count: " << station->m_slrc << " result: " << std::boolalpha << normally);
  return DoNeedDataRetransmission (station, packet, normally);
}

bool
WifiRemoteStationManager::NeedFragmentation (Mac48Address address, const WifiMacHeader *header,
                                             Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << packet << *header);
  if (address.IsGroup ())
    {
      return false;
    }
  WifiRemoteStation *station = Lookup (address, header);
  bool normally = (packet->GetSize () + header->GetSize () + WIFI_MAC_FCS_LENGTH) > GetFragmentationThreshold ();
  NS_LOG_DEBUG ("WifiRemoteStationManager::NeedFragmentation result: " << std::boolalpha << normally);
  return DoNeedFragmentation (station, packet, normally);
}

void
WifiRemoteStationManager::DoSetFragmentationThreshold (uint32_t threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  if (threshold < 256)
    {
      /*
       * ASN.1 encoding of the MAC and PHY MIB (256 ... 8000)
       */
      NS_LOG_WARN ("Fragmentation threshold should be larger than 256. Setting to 256.");
      m_nextFragmentationThreshold = 256;
    }
  else
    {
      /*
       * The length of each fragment shall be an even number of octets, except for the last fragment if an MSDU or
       * MMPDU, which may be either an even or an odd number of octets.
       */
      if (threshold % 2 != 0)
        {
          NS_LOG_WARN ("Fragmentation threshold should be an even number. Setting to " << threshold - 1);
          m_nextFragmentationThreshold = threshold - 1;
        }
      else
        {
          m_nextFragmentationThreshold = threshold;
        }
    }
}

void
WifiRemoteStationManager::UpdateFragmentationThreshold (void)
{
  m_fragmentationThreshold = m_nextFragmentationThreshold;
}

uint32_t
WifiRemoteStationManager::DoGetFragmentationThreshold (void) const
{
  return m_fragmentationThreshold;
}

uint32_t
WifiRemoteStationManager::GetNFragments (const WifiMacHeader *header, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << *header << packet);
  //The number of bytes a fragment can support is (Threshold - WIFI_HEADER_SIZE - WIFI_FCS).
  uint32_t nFragments = (packet->GetSize () / (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH));

  //If the size of the last fragment is not 0.
  if ((packet->GetSize () % (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH)) > 0)
    {
      nFragments++;
    }
  NS_LOG_DEBUG ("WifiRemoteStationManager::GetNFragments returning " << nFragments);
  return nFragments;
}

uint32_t
WifiRemoteStationManager::GetFragmentSize (Mac48Address address, const WifiMacHeader *header,
                                           Ptr<const Packet> packet, uint32_t fragmentNumber)
{
  NS_LOG_FUNCTION (this << address << *header << packet << fragmentNumber);
  NS_ASSERT (!address.IsGroup ());
  uint32_t nFragment = GetNFragments (header, packet);
  if (fragmentNumber >= nFragment)
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentSize returning 0");
      return 0;
    }
  //Last fragment
  if (fragmentNumber == nFragment - 1)
    {
      uint32_t lastFragmentSize = packet->GetSize () - (fragmentNumber * (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH));
      NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentSize returning " << lastFragmentSize);
      return lastFragmentSize;
    }
  //All fragments but the last, the number of bytes is (Threshold - WIFI_HEADER_SIZE - WIFI_FCS).
  else
    {
      uint32_t fragmentSize = GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH;
      NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentSize returning " << fragmentSize);
      return fragmentSize;
    }
}

uint32_t
WifiRemoteStationManager::GetFragmentOffset (Mac48Address address, const WifiMacHeader *header,
                                             Ptr<const Packet> packet, uint32_t fragmentNumber)
{
  NS_LOG_FUNCTION (this << address << *header << packet << fragmentNumber);
  NS_ASSERT (!address.IsGroup ());
  NS_ASSERT (fragmentNumber < GetNFragments (header, packet));
  uint32_t fragmentOffset = fragmentNumber * (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH);
  NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentOffset returning " << fragmentOffset);
  return fragmentOffset;
}

bool
WifiRemoteStationManager::IsLastFragment (Mac48Address address, const WifiMacHeader *header,
                                          Ptr<const Packet> packet, uint32_t fragmentNumber)
{
  NS_LOG_FUNCTION (this << address << *header << packet << fragmentNumber);
  NS_ASSERT (!address.IsGroup ());
  bool isLast = fragmentNumber == (GetNFragments (header, packet) - 1);
  NS_LOG_DEBUG ("WifiRemoteStationManager::IsLastFragment returning " << std::boolalpha << isLast);
  return isLast;
}

bool
WifiRemoteStationManager::IsAllowedControlAnswerModulationClass (WifiModulationClass modClassReq, WifiModulationClass modClassAnswer) const
{
  switch (modClassReq)
    {
    case WIFI_MOD_CLASS_DSSS:
      return (modClassAnswer == WIFI_MOD_CLASS_DSSS);
    case WIFI_MOD_CLASS_HR_DSSS:
      return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS);
    case WIFI_MOD_CLASS_ERP_OFDM:
      return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS || modClassAnswer == WIFI_MOD_CLASS_ERP_OFDM);
    case WIFI_MOD_CLASS_OFDM:
      return (modClassAnswer == WIFI_MOD_CLASS_OFDM);
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
      return true;
    default:
      NS_FATAL_ERROR ("Modulation class not defined");
      return false;
    }
}

WifiMode
WifiRemoteStationManager::GetControlAnswerMode (Mac48Address address, WifiMode reqMode)
{
  NS_LOG_FUNCTION (this << address << reqMode);
  WifiMode mode = GetDefaultMode ();
  bool found = false;

  if (HasDmgSupported ())
    {
      /**
       * Rules for selecting a control response rate from IEEE 802.11ad-2012,
       * Section 9.7.5a Multirate support for DMG STAs:
       */
      WifiMode thismode;
      /* We start from SC PHY Rates, This is for transmitting an ACK frame or a BA frame */
      for (uint32_t idx = 0; idx < m_wifiPhy->GetNModes (); idx++)
        {
          thismode = m_wifiPhy->GetMode (idx);
          if (thismode.IsMandatory () && (thismode.GetDataRate () <= reqMode.GetDataRate ()))
            {
              mode = thismode;
              found = true;
            }
          else
            {
              break;
            }
        }
    }
  else
    {
      /**
       * The standard has relatively unambiguous rules for selecting a
       * control response rate (the below is quoted from IEEE 802.11-2012,
       * Section 9.7):
       *
       *   To allow the transmitting STA to calculate the contents of the
       *   Duration/ID field, a STA responding to a received frame shall
       *   transmit its Control Response frame (either CTS or ACK), other
       *   than the BlockAck control frame, at the highest rate in the
       *   BSSBasicRateSet parameter that is less than or equal to the
       *   rate of the immediately previous frame in the frame exchange
       *   sequence (as defined in Annex G) and that is of the same
       *   modulation class (see Section 9.7.8) as the received frame...
       */
      //First, search the BSS Basic Rate set
      for (WifiModeListIterator i = m_bssBasicRateSet.begin (); i != m_bssBasicRateSet.end (); i++)
        {
          if ((!found || i->IsHigherDataRate (mode))
              && (!i->IsHigherDataRate (reqMode))
              && (IsAllowedControlAnswerModulationClass (reqMode.GetModulationClass (), i->GetModulationClass ())))
            {
              mode = *i;
              //We've found a potentially-suitable transmit rate, but we
              //need to continue and consider all the basic rates before
              //we can be sure we've got the right one.
              found = true;
            }
        }
      if (HasHtSupported () || HasVhtSupported () || HasHeSupported ())
        {
          if (!found)
            {
              mode = GetDefaultMcs ();
              for (WifiModeListIterator i = m_bssBasicMcsSet.begin (); i != m_bssBasicMcsSet.end (); i++)
                {
                  if ((!found || i->IsHigherDataRate (mode))
                      && (!i->IsHigherDataRate (reqMode))
                      && (i->GetModulationClass () == reqMode.GetModulationClass ()))
                    {
                      mode = *i;
                      //We've found a potentially-suitable transmit rate, but we
                      //need to continue and consider all the basic rates before
                      //we can be sure we've got the right one.
                      found = true;
                    }
                }
            }
        }
      //If we found a suitable rate in the BSSBasicRateSet, then we are
      //done and can return that mode.
      if (found)
        {
          NS_LOG_DEBUG ("WifiRemoteStationManager::GetControlAnswerMode returning " << mode);
          return mode;
        }

      /**
       * If no suitable basic rate was found, we search the mandatory
       * rates. The standard (IEEE 802.11-2007, Section 9.6) says:
       *
       *   ...If no rate contained in the BSSBasicRateSet parameter meets
       *   these conditions, then the control frame sent in response to a
       *   received frame shall be transmitted at the highest mandatory
       *   rate of the PHY that is less than or equal to the rate of the
       *   received frame, and that is of the same modulation class as the
       *   received frame. In addition, the Control Response frame shall
       *   be sent using the same PHY options as the received frame,
       *   unless they conflict with the requirement to use the
       *   BSSBasicRateSet parameter.
       *
       * \todo Note that we're ignoring the last sentence for now, because
       * there is not yet any manipulation here of PHY options.
       */
  	  for (uint8_t idx = 0; idx < m_wifiPhy->GetNModes (); idx++)
        {
          WifiMode thismode = m_wifiPhy->GetMode (idx);
          /* If the rate:
           *
           *  - is a mandatory rate for the PHY, and
           *  - is equal to or faster than our current best choice, and
           *  - is less than or equal to the rate of the received frame, and
           *  - is of the same modulation class as the received frame
           *
           * ...then it's our best choice so far.
           */
          if (thismode.IsMandatory ()
              && (!found || thismode.IsHigherDataRate (mode))
              && (!thismode.IsHigherDataRate (reqMode))
              && (IsAllowedControlAnswerModulationClass (reqMode.GetModulationClass (), thismode.GetModulationClass ())))
            {
              mode = thismode;
              //As above; we've found a potentially-suitable transmit
              //rate, but we need to continue and consider all the
              //mandatory rates before we can be sure we've got the right one.
              found = true;
            }
        }
      if (HasHtSupported () || HasVhtSupported () || HasHeSupported ())
        {
          for (uint32_t idx = 0; idx < m_wifiPhy->GetNMcs (); idx++)
            {
              WifiMode thismode = m_wifiPhy->GetMcs (idx);
              if (thismode.IsMandatory ()
                  && (!found || thismode.IsHigherDataRate (mode))
                  && (!thismode.IsHigherCodeRate (reqMode))
                  && (thismode.GetModulationClass () == reqMode.GetModulationClass ()))
                {
                  mode = thismode;
                  //As above; we've found a potentially-suitable transmit
                  //rate, but we need to continue and consider all the
                  //mandatory rates before we can be sure we've got the right one.
                  found = true;
                }

            }
        }
    }

  /**
   * If we still haven't found a suitable rate for the response then
   * someone has messed up the simulation config. This probably means
   * that the WifiPhyStandard is not set correctly, or that a rate that
   * is not supported by the PHY has been explicitly requested in a
   * WifiRemoteStationManager (or descendant) configuration.
   *
   * Either way, it is serious - we can either disobey the standard or
   * fail, and I have chosen to do the latter...
   */
  if (!found)
    {
      NS_FATAL_ERROR ("Can't find response rate for " << reqMode);
    }

  NS_LOG_DEBUG ("WifiRemoteStationManager::GetControlAnswerMode returning " << mode);
  return mode;
}

WifiTxVector
WifiRemoteStationManager::GetCtsTxVector (Mac48Address address, WifiMode rtsMode)
{
  NS_ASSERT (!address.IsGroup ());
  WifiMode ctsMode = GetControlAnswerMode (address, rtsMode);
  WifiTxVector v;
  v.SetMode (ctsMode);
  v.SetPreambleType (GetPreambleForTransmission (ctsMode, address));
  v.SetTxPowerLevel (DoGetCtsTxPowerLevel (address, ctsMode));
  v.SetChannelWidth (GetChannelWidthForTransmission (ctsMode, DoGetCtsTxChannelWidth (address, ctsMode)));
  v.SetGuardInterval (DoGetCtsTxGuardInterval (address, ctsMode));
  v.SetNss (DoGetCtsTxNss (address, ctsMode));
  v.SetNess (DoGetCtsTxNess (address, ctsMode));
  v.SetStbc (m_wifiPhy->GetStbc ());
  return v;
}

WifiTxVector
WifiRemoteStationManager::GetAckTxVector (Mac48Address address, WifiMode dataMode)
{
  NS_ASSERT (!address.IsGroup ());
  WifiMode ackMode = GetControlAnswerMode (address, dataMode);
  WifiTxVector v;
  v.SetMode (ackMode);
  v.SetPreambleType (GetPreambleForTransmission (ackMode, address));
  v.SetTxPowerLevel (DoGetAckTxPowerLevel (address, ackMode));
  v.SetChannelWidth (GetChannelWidthForTransmission (ackMode, DoGetAckTxChannelWidth (address, ackMode)));
  v.SetGuardInterval (DoGetAckTxGuardInterval (address, ackMode));
  v.SetNss (DoGetAckTxNss (address, ackMode));
  v.SetNess (DoGetAckTxNess (address, ackMode));
  v.SetStbc (m_wifiPhy->GetStbc ());
  return v;
}

WifiTxVector
WifiRemoteStationManager::GetDmgControlTxVector (void)
{
  WifiTxVector v;
  v.SetMode (m_wifiPhy->GetMode (0)); // DMG Control Modulation Class (MCS0)
  v.SetTxPowerLevel (m_defaultTxPowerLevel);
  v.SetPreambleType (WIFI_PREAMBLE_LONG);
  v.SetChannelWidth (m_wifiPhy->GetChannelWidth ());
  return v;
}

WifiTxVector
WifiRemoteStationManager::GetDmgLowestScVector (void)
{
  WifiTxVector v;
  v.SetMode (m_wifiPhy->GetMode (1)); // DMG SC Modulation Class (MCS1)
  v.SetTxPowerLevel (m_defaultTxPowerLevel);
  v.SetPreambleType (WIFI_PREAMBLE_LONG);
  v.SetChannelWidth (m_wifiPhy->GetChannelWidth ());
  return v;
}

WifiTxVector
WifiRemoteStationManager::GetBlockAckTxVector (Mac48Address address, WifiMode blockAckReqMode)
{
  NS_ASSERT (!address.IsGroup ());
  WifiMode blockAckMode = GetControlAnswerMode (address, blockAckReqMode);
  WifiTxVector v;
  v.SetMode (blockAckMode);
  v.SetPreambleType (GetPreambleForTransmission (blockAckMode, address));
  v.SetTxPowerLevel (DoGetBlockAckTxPowerLevel (address, blockAckMode));
  v.SetChannelWidth (GetChannelWidthForTransmission (blockAckMode, DoGetBlockAckTxChannelWidth (address, blockAckMode)));
  v.SetGuardInterval (DoGetBlockAckTxGuardInterval (address, blockAckMode));
  v.SetNss (DoGetBlockAckTxNss (address, blockAckMode));
  v.SetNess (DoGetBlockAckTxNess (address, blockAckMode));
  v.SetStbc (m_wifiPhy->GetStbc ());
  return v;
}

uint8_t
WifiRemoteStationManager::DoGetCtsTxPowerLevel (Mac48Address address, WifiMode ctsMode)
{
  return m_defaultTxPowerLevel;
}

uint16_t
WifiRemoteStationManager::DoGetCtsTxChannelWidth (Mac48Address address, WifiMode ctsMode)
{
  return m_wifiPhy->GetChannelWidth ();
}

uint16_t
WifiRemoteStationManager::DoGetCtsTxGuardInterval (Mac48Address address, WifiMode ctsMode)
{
  return ConvertGuardIntervalToNanoSeconds (ctsMode, m_wifiPhy->GetShortGuardInterval (), m_wifiPhy->GetGuardInterval ());
}

uint8_t
WifiRemoteStationManager::DoGetCtsTxNss (Mac48Address address, WifiMode ctsMode)
{
  return 1;
}

uint8_t
WifiRemoteStationManager::DoGetCtsTxNess (Mac48Address address, WifiMode ctsMode)
{
  return 0;
}

uint8_t
WifiRemoteStationManager::DoGetAckTxPowerLevel (Mac48Address address, WifiMode ackMode)
{
  return m_defaultTxPowerLevel;
}

uint16_t
WifiRemoteStationManager::DoGetAckTxChannelWidth (Mac48Address address, WifiMode ctsMode)
{
  return m_wifiPhy->GetChannelWidth ();
}

uint16_t
WifiRemoteStationManager::DoGetAckTxGuardInterval (Mac48Address address, WifiMode ackMode)
{
  return ConvertGuardIntervalToNanoSeconds (ackMode, m_wifiPhy->GetShortGuardInterval (), m_wifiPhy->GetGuardInterval ());
}

uint8_t
WifiRemoteStationManager::DoGetAckTxNss (Mac48Address address, WifiMode ackMode)
{
  return 1;
}

uint8_t
WifiRemoteStationManager::DoGetAckTxNess (Mac48Address address, WifiMode ackMode)
{
  return 0;
}

uint8_t
WifiRemoteStationManager::DoGetBlockAckTxPowerLevel (Mac48Address address, WifiMode blockAckMode)
{
  return m_defaultTxPowerLevel;
}

uint16_t
WifiRemoteStationManager::DoGetBlockAckTxChannelWidth (Mac48Address address, WifiMode ctsMode)
{
  return m_wifiPhy->GetChannelWidth ();
}

uint16_t
WifiRemoteStationManager::DoGetBlockAckTxGuardInterval (Mac48Address address, WifiMode blockAckMode)
{
  return ConvertGuardIntervalToNanoSeconds (blockAckMode, m_wifiPhy->GetShortGuardInterval (), m_wifiPhy->GetGuardInterval ());
}

uint8_t
WifiRemoteStationManager::DoGetBlockAckTxNss (Mac48Address address, WifiMode blockAckMode)
{
  return 1;
}

uint8_t
WifiRemoteStationManager::DoGetBlockAckTxNess (Mac48Address address, WifiMode blockAckMode)
{
  return 0;
}

uint8_t
WifiRemoteStationManager::GetDefaultTxPowerLevel (void) const
{
  return m_defaultTxPowerLevel;
}

WifiRemoteStationInfo
WifiRemoteStationManager::GetInfo (Mac48Address address)
{
  WifiRemoteStationState *state = LookupState (address);
  return state->m_info;
}

WifiRemoteStationState *
WifiRemoteStationManager::LookupState (Mac48Address address) const
{
  NS_LOG_FUNCTION (this << address);
  for (StationStates::const_iterator i = m_states.begin (); i != m_states.end (); i++)
    {
      if ((*i)->m_address == address)
        {
          NS_LOG_DEBUG ("WifiRemoteStationManager::LookupState returning existing state");
          return (*i);
        }
    }
  WifiRemoteStationState *state = new WifiRemoteStationState ();
  state->m_state = WifiRemoteStationState::BRAND_NEW;
  state->m_address = address;
  state->m_operationalRateSet.push_back (GetDefaultMode ());
  state->m_operationalMcsSet.push_back (GetDefaultMcs ());
  state->m_channelWidth = m_wifiPhy->GetChannelWidth ();
  state->m_shortGuardInterval = m_wifiPhy->GetShortGuardInterval ();
  state->m_guardInterval = m_wifiPhy->GetGuardInterval ().GetNanoSeconds ();
  state->m_greenfield = m_wifiPhy->GetGreenfield ();
  state->m_streams = 1;
  state->m_ness = 0;
  state->m_aggregation = false;
  state->m_stbc = false;
  state->m_ldpc = false;
  state->m_qosSupported = false;
  state->m_htSupported = false;
  state->m_vhtSupported = false;
  state->m_heSupported = false;
  state->m_dmgSupported = false;
  const_cast<WifiRemoteStationManager *> (this)->m_states.push_back (state);
  NS_LOG_DEBUG ("WifiRemoteStationManager::LookupState returning new state");
  return state;
}

WifiRemoteStation *
WifiRemoteStationManager::Lookup (Mac48Address address, const WifiMacHeader *header) const
{
  uint8_t tid;
  if (header->IsQosData ())
    {
      tid = header->GetQosTid ();
    }
  else
    {
      tid = 0;
    }
  return Lookup (address, tid);
}

WifiRemoteStation *
WifiRemoteStationManager::Lookup (Mac48Address address, uint8_t tid) const
{
  NS_LOG_FUNCTION (this << address << +tid);
  for (Stations::const_iterator i = m_stations.begin (); i != m_stations.end (); i++)
    {
      if ((*i)->m_tid == tid
          && (*i)->m_state->m_address == address)
        {
          return (*i);
        }
    }
  WifiRemoteStationState *state = LookupState (address);

  WifiRemoteStation *station = DoCreateStation ();
  station->m_state = state;
  station->m_tid = tid;
  station->m_ssrc = 0;
  station->m_slrc = 0;
  const_cast<WifiRemoteStationManager *> (this)->m_stations.push_back (station);
  return station;
}

void
WifiRemoteStationManager::SetQosSupport (Mac48Address from, bool qosSupported)
{
  NS_LOG_FUNCTION (this << from << qosSupported);
  WifiRemoteStationState *state;
  state = LookupState (from);
  state->m_qosSupported = qosSupported;
}

void
WifiRemoteStationManager::AddStationHtCapabilities (Mac48Address from, Ptr<HtCapabilities> htCapabilities)
{
  //Used by all stations to record HT capabilities of remote stations
  NS_LOG_FUNCTION (this << from << htCapabilities);
  WifiRemoteStationState *state;
  state = LookupState (from);
  state->m_shortGuardInterval = htCapabilities->GetShortGuardInterval20 ();
  if (htCapabilities->GetSupportedChannelWidth () == 1)
    {
      state->m_channelWidth = 40;
    }
  else
    {
      state->m_channelWidth = 20;
    }
  SetQosSupport (from, true);
  state->m_greenfield = htCapabilities->GetGreenfield ();
  state->m_streams = htCapabilities->GetRxHighestSupportedAntennas ();
  for (uint8_t j = 0; j < m_wifiPhy->GetNMcs (); j++)
    {
      WifiMode mcs = m_wifiPhy->GetMcs (j);
      if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HT && htCapabilities->IsSupportedMcs (mcs.GetMcsValue ()))
        {
          AddSupportedMcs (from, mcs);
        }
    }
  state->m_htSupported = true;
}

void
WifiRemoteStationManager::AddStationVhtCapabilities (Mac48Address from, Ptr<VhtCapabilities> vhtCapabilities)
{
  //Used by all stations to record VHT capabilities of remote stations
  NS_LOG_FUNCTION (this << from << vhtCapabilities);
  WifiRemoteStationState *state;
  state = LookupState (from);
  if (vhtCapabilities->GetSupportedChannelWidthSet () == 1)
    {
      state->m_channelWidth = 160;
    }
  else
    {
      state->m_channelWidth = 80;
    }
  //This is a workaround to enable users to force a 20 or 40 MHz channel for a VHT-compliant device,
  //since IEEE 802.11ac standard says that 20, 40 and 80 MHz channels are mandatory.
  if (m_wifiPhy->GetChannelWidth () < state->m_channelWidth)
    {
      state->m_channelWidth = m_wifiPhy->GetChannelWidth ();
    }
  state->m_ldpc = (vhtCapabilities->GetRxLdpc () != 0);
  state->m_stbc = (vhtCapabilities->GetTxStbc () != 0 && vhtCapabilities->GetRxStbc () != 0);
  for (uint8_t i = 1; i <= m_wifiPhy->GetMaxSupportedTxSpatialStreams (); i++)
    {
      for (uint8_t j = 0; j < m_wifiPhy->GetNMcs (); j++)
        {
          WifiMode mcs = m_wifiPhy->GetMcs (j);
          if (mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT && vhtCapabilities->IsSupportedMcs (mcs.GetMcsValue (), i))
            {
              AddSupportedMcs (from, mcs);
            }
        }
    }
  state->m_vhtSupported = true;
}

void
WifiRemoteStationManager::AddStationDmgCapabilities (Mac48Address from, Ptr<DmgCapabilities> dmgCapabilities)
{
  //Used by all stations to record DMG capabilities of remote stations
  NS_LOG_FUNCTION (this << from << dmgCapabilities);
  WifiRemoteStationState *state;
  state = LookupState (from);
  state->m_qosSupported = true;
  state->m_dmgSupported = true;
}

void
WifiRemoteStationManager::AddStationHeCapabilities (Mac48Address from, Ptr<HeCapabilities> heCapabilities)
{
  //Used by all stations to record HE capabilities of remote stations
  NS_LOG_FUNCTION (this << from << heCapabilities);
  WifiRemoteStationState *state;
  state = LookupState (from);
  if (Is5Ghz (m_wifiPhy->GetFrequency ()))
    {
      if (heCapabilities->GetChannelWidthSet () & 0x04)
        {
          state->m_channelWidth = 160;
        }
      else if (heCapabilities->GetChannelWidthSet () & 0x02)
        {
          state->m_channelWidth = 80;
        }
      //For other cases at 5 GHz, the supported channel width is set by the VHT capabilities
    }
  else if (Is2_4Ghz (m_wifiPhy->GetFrequency ()))
    {
      if (heCapabilities->GetChannelWidthSet () & 0x01)
        {
          state->m_channelWidth = 40;
        }
      else
        {
          state->m_channelWidth = 20;
        }
    }
  if (heCapabilities->GetHeLtfAndGiForHePpdus () >= 2)
    {
      state->m_guardInterval = 800;
    }
  else if (heCapabilities->GetHeLtfAndGiForHePpdus () == 1)
    {
      state->m_guardInterval = 1600;
    }
  else
    {
      state->m_guardInterval = 3200;
    }
  state->m_heSupported = true;
  SetQosSupport (from, true);
}

bool
WifiRemoteStationManager::GetGreenfieldSupported (Mac48Address address) const
{
  return LookupState (address)->m_greenfield;
}

WifiMode
WifiRemoteStationManager::GetDefaultMode (void) const
{
  return m_defaultTxMode;
}

WifiMode
WifiRemoteStationManager::GetDefaultMcs (void) const
{
  return m_defaultTxMcs;
}

void
WifiRemoteStationManager::Reset (void)
{
  NS_LOG_FUNCTION (this);
  for (StationStates::const_iterator i = m_states.begin (); i != m_states.end (); i++)
    {
      delete (*i);
    }
  m_states.clear ();
  for (Stations::const_iterator i = m_stations.begin (); i != m_stations.end (); i++)
    {
      delete (*i);
    }
  m_stations.clear ();
  m_bssBasicRateSet.clear ();
  m_bssBasicMcsSet.clear ();
}

void
WifiRemoteStationManager::AddBasicMode (WifiMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT || mode.GetModulationClass () == WIFI_MOD_CLASS_VHT || mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      NS_FATAL_ERROR ("It is not allowed to add a (V)HT rate in the BSSBasicRateSet!");
    }
  for (uint8_t i = 0; i < GetNBasicModes (); i++)
    {
      if (GetBasicMode (i) == mode)
        {
          return;
        }
    }
  m_bssBasicRateSet.push_back (mode);
}

uint8_t
WifiRemoteStationManager::GetNBasicModes (void) const
{
  return m_bssBasicRateSet.size ();
}

WifiMode
WifiRemoteStationManager::GetBasicMode (uint8_t i) const
{
  NS_ASSERT (i < GetNBasicModes ());
  return m_bssBasicRateSet[i];
}

uint32_t
WifiRemoteStationManager::GetNNonErpBasicModes (void) const
{
  uint32_t size = 0;
  for (WifiModeListIterator i = m_bssBasicRateSet.begin (); i != m_bssBasicRateSet.end (); i++)
    {
      if (i->GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
        {
          continue;
        }
      size++;
    }
  return size;
}

WifiMode
WifiRemoteStationManager::GetNonErpBasicMode (uint8_t i) const
{
  NS_ASSERT (i < GetNNonErpBasicModes ());
  uint32_t index = 0;
  bool found = false;
  for (WifiModeListIterator j = m_bssBasicRateSet.begin (); j != m_bssBasicRateSet.end (); )
    {
      if (i == index)
        {
          found = true;
        }
      if (j->GetModulationClass () != WIFI_MOD_CLASS_ERP_OFDM)
        {
          if (found)
            {
              break;
            }
        }
      index++;
      j++;
    }
  return m_bssBasicRateSet[index];
}

void
WifiRemoteStationManager::AddBasicMcs (WifiMode mcs)
{
  NS_LOG_FUNCTION (this << +mcs.GetMcsValue ());
  for (uint8_t i = 0; i < GetNBasicMcs (); i++)
    {
      if (GetBasicMcs (i) == mcs)
        {
          return;
        }
    }
  m_bssBasicMcsSet.push_back (mcs);
}

uint8_t
WifiRemoteStationManager::GetNBasicMcs (void) const
{
  return m_bssBasicMcsSet.size ();
}

WifiMode
WifiRemoteStationManager::GetBasicMcs (uint8_t i) const
{
  NS_ASSERT (i < GetNBasicMcs ());
  return m_bssBasicMcsSet[i];
}

WifiMode
WifiRemoteStationManager::GetNonUnicastMode (void) const
{
  if (m_nonUnicastMode == WifiMode ())
    {
      if (GetNBasicModes () > 0)
        {
          return GetBasicMode (0);
        }
      else
        {
          return GetDefaultMode ();
        }
    }
  else
    {
      return m_nonUnicastMode;
    }
}

bool
WifiRemoteStationManager::DoNeedRts (WifiRemoteStation *station,
                                     Ptr<const Packet> packet, bool normally)
{
  return normally;
}

bool
WifiRemoteStationManager::DoNeedRtsRetransmission (WifiRemoteStation *station,
                                                   Ptr<const Packet> packet, bool normally)
{
  return normally;
}

bool
WifiRemoteStationManager::DoNeedDataRetransmission (WifiRemoteStation *station,
                                                    Ptr<const Packet> packet, bool normally)
{
  return normally;
}

bool
WifiRemoteStationManager::DoNeedFragmentation (WifiRemoteStation *station,
                                               Ptr<const Packet> packet, bool normally)
{
  return normally;
}

void
WifiRemoteStationManager::DoReportAmpduTxStatus (WifiRemoteStation *station, uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus, double rxSnr, double dataSnr)
{
  NS_LOG_DEBUG ("DoReportAmpduTxStatus received but the manager does not handle A-MPDUs!");
}

WifiMode
WifiRemoteStationManager::GetSupported (const WifiRemoteStation *station, uint8_t i) const
{
  NS_ASSERT (i < GetNSupported (station));
  return station->m_state->m_operationalRateSet[i];
}

WifiMode
WifiRemoteStationManager::GetMcsSupported (const WifiRemoteStation *station, uint8_t i) const
{
  NS_ASSERT (i < GetNMcsSupported (station));
  return station->m_state->m_operationalMcsSet[i];
}

WifiMode
WifiRemoteStationManager::GetNonErpSupported (const WifiRemoteStation *station, uint8_t i) const
{
  NS_ASSERT (i < GetNNonErpSupported (station));
  //IEEE 802.11g standard defines that if the protection mechanism is enabled, Rts, Cts and Cts-To-Self
  //frames should select a rate in the BSSBasicRateSet that corresponds to an 802.11b basic rate.
  //This is a implemented here to avoid changes in every RAA, but should maybe be moved in case it breaks standard rules.
  uint32_t index = 0;
  bool found = false;
  for (WifiModeListIterator j = station->m_state->m_operationalRateSet.begin (); j != station->m_state->m_operationalRateSet.end (); )
    {
      if (i == index)
        {
          found = true;
        }
      if (j->GetModulationClass () != WIFI_MOD_CLASS_ERP_OFDM)
        {
          if (found)
            {
              break;
            }
        }
      index++;
      j++;
    }
  return station->m_state->m_operationalRateSet[index];
}

Mac48Address
WifiRemoteStationManager::GetAddress (const WifiRemoteStation *station) const
{
  return station->m_state->m_address;
}

uint16_t
WifiRemoteStationManager::GetChannelWidth (const WifiRemoteStation *station) const
{
  return station->m_state->m_channelWidth;
}

bool
WifiRemoteStationManager::GetShortGuardInterval (const WifiRemoteStation *station) const
{
  return station->m_state->m_shortGuardInterval;
}

uint16_t
WifiRemoteStationManager::GetGuardInterval (const WifiRemoteStation *station) const
{
  return station->m_state->m_guardInterval;
}

bool
WifiRemoteStationManager::GetGreenfield (const WifiRemoteStation *station) const
{
  return station->m_state->m_greenfield;
}

bool
WifiRemoteStationManager::GetAggregation (const WifiRemoteStation *station) const
{
  return station->m_state->m_aggregation;
}

uint8_t
WifiRemoteStationManager::GetNumberOfSupportedStreams (const WifiRemoteStation *station) const
{
  return station->m_state->m_streams;
}

uint32_t
WifiRemoteStationManager::GetNess (const WifiRemoteStation *station) const
{
  return station->m_state->m_ness;
}

Ptr<WifiPhy>
WifiRemoteStationManager::GetPhy (void) const
{
  return m_wifiPhy;
}

Ptr<WifiMac>
WifiRemoteStationManager::GetMac (void) const
{
  return m_wifiMac;
}

void
WifiRemoteStationManager::RegisterTxOkCallback (Callback<void, Mac48Address> callback)
{
  m_txCallbackOk = callback;
}

void
WifiRemoteStationManager::RegisterRxOkCallback (Callback<void, Mac48Address> callback)
{
  m_rxCallbackOk = callback;
}

StationStates*
WifiRemoteStationManager::GetStationStates (void)
{
  return &m_states;
}

uint32_t
WifiRemoteStationManager::GetNAssociatedStation (void) const
{
  return m_states.size ();
}

double
WifiRemoteStationManager::GetRxSnr (void) const
{
  return m_rxSnr;
}

uint8_t
WifiRemoteStationManager::GetNSupported (const WifiRemoteStation *station) const
{
  return station->m_state->m_operationalRateSet.size ();
}

bool
WifiRemoteStationManager::GetQosSupported (const WifiRemoteStation *station) const
{
  return station->m_state->m_qosSupported;
}

bool
WifiRemoteStationManager::GetHtSupported (const WifiRemoteStation *station) const
{
  return station->m_state->m_htSupported;
}

bool
WifiRemoteStationManager::GetVhtSupported (const WifiRemoteStation *station) const
{
  return station->m_state->m_vhtSupported;
}

bool
WifiRemoteStationManager::GetHeSupported (const WifiRemoteStation *station) const
{
  return station->m_state->m_heSupported;
}

uint8_t
WifiRemoteStationManager::GetNMcsSupported (const WifiRemoteStation *station) const
{
  return station->m_state->m_operationalMcsSet.size ();
}

uint32_t
WifiRemoteStationManager::GetNNonErpSupported (const WifiRemoteStation *station) const
{
  uint32_t size = 0;
  for (WifiModeListIterator i = station->m_state->m_operationalRateSet.begin (); i != station->m_state->m_operationalRateSet.end (); i++)
    {
      if (i->GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
        {
          continue;
        }
      size++;
    }
  return size;
}

uint16_t
WifiRemoteStationManager::GetChannelWidthSupported (Mac48Address address) const
{
  return LookupState (address)->m_channelWidth;
}

bool
WifiRemoteStationManager::GetShortGuardInterval (Mac48Address address) const
{
  return LookupState (address)->m_shortGuardInterval;
}

uint8_t
WifiRemoteStationManager::GetNumberOfSupportedStreams (Mac48Address address) const
{
  return LookupState (address)->m_streams;
}

uint8_t
WifiRemoteStationManager::GetNMcsSupported (Mac48Address address) const
{
  return LookupState (address)->m_operationalMcsSet.size ();
}

bool
WifiRemoteStationManager::GetHtSupported (Mac48Address address) const
{
  return LookupState (address)->m_htSupported;
}

bool
WifiRemoteStationManager::GetVhtSupported (Mac48Address address) const
{
  return LookupState (address)->m_vhtSupported;
}

void
WifiRemoteStationManager::SetDefaultTxPowerLevel (uint8_t txPower)
{
  m_defaultTxPowerLevel = txPower;
}

uint8_t
WifiRemoteStationManager::GetNumberOfAntennas (void)
{
  return m_wifiPhy->GetNumberOfAntennas ();
}

uint8_t
WifiRemoteStationManager::GetMaxNumberOfTransmitStreams (void)
{
  return m_wifiPhy->GetMaxSupportedTxSpatialStreams ();
}

WifiPreamble
WifiRemoteStationManager::GetPreambleForTransmission (WifiMode mode, Mac48Address dest)
{
  NS_LOG_FUNCTION (this << mode << dest);
  WifiPreamble preamble;
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      preamble = WIFI_PREAMBLE_HE_SU;
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      preamble = WIFI_PREAMBLE_VHT;
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT && m_wifiPhy->GetGreenfield () && GetGreenfieldSupported (dest) && !GetUseGreenfieldProtection ())
    {
      //If protection for greenfield is used we go for HT_MF preamble which is the default protection for GF format defined in the standard.
      preamble = WIFI_PREAMBLE_HT_GF;
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
    {
      preamble = WIFI_PREAMBLE_HT_MF;
    }
  else if (GetShortPreambleEnabled ())
    {
      preamble = WIFI_PREAMBLE_SHORT;
    }
  else
    {
      preamble = WIFI_PREAMBLE_LONG;
    }
  NS_LOG_DEBUG ("selected preamble=" << preamble);
  return preamble;
}

WifiRemoteStation::~WifiRemoteStation ()
{
  NS_LOG_FUNCTION (this);
}

WifiRemoteStationInfo::WifiRemoteStationInfo ()
  : m_memoryTime (Seconds (1.0)),
    m_lastUpdate (Seconds (0.0)),
    m_failAvg (0.0)
{
  NS_LOG_FUNCTION (this);
}

double
WifiRemoteStationInfo::CalculateAveragingCoefficient ()
{
  double retval = std::exp (static_cast<double> (m_lastUpdate.GetMicroSeconds () - Simulator::Now ().GetMicroSeconds ()) / m_memoryTime.GetMicroSeconds ());
  m_lastUpdate = Simulator::Now ();
  return retval;
}

void
WifiRemoteStationInfo::NotifyTxSuccess (uint32_t retryCounter)
{
  double coefficient = CalculateAveragingCoefficient ();
  m_failAvg = static_cast<double> (retryCounter) / (1 + retryCounter) * (1 - coefficient) + coefficient * m_failAvg;
}

void
WifiRemoteStationInfo::NotifyTxFailed ()
{
  double coefficient = CalculateAveragingCoefficient ();
  m_failAvg = (1 - coefficient) + coefficient * m_failAvg;
}

double
WifiRemoteStationInfo::GetFrameErrorRate () const
{
  return m_failAvg;
}

} //namespace ns3
