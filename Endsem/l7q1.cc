#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/qos-utils.h"
#include "ns3/netanim-module.h"
//#include "ns3/nqos-wifi-mac-helper.h"


    //
//   Wifi 10.1.3.0
//  AP
//  *    *    *    *    *    *    *    *    *    *
//  |    |    |    |    |    |    |    |    |    |
// n0   n1   n2   n3   n4   n5   n6   n7   n8   n9
    //       
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int main (int argc, char *argv[])
{
  uint32_t nWifi = 3;


  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (9);
  NodeContainer wifiApNode;
  wifiApNode.Create(1);



  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    //channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    //channel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(5e9));
  YansWifiPhyHelper phy;
  phy.SetErrorRateModel ("ns3::NistErrorRateModel");
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");

  WifiMacHelper mac;
  //mac.SetType ("ns3::QosWifiMacHelper", "QosSupported", BooleanValue (true));
  //NqosWifiMacHelper mac= NqosWifiMacHelper::Default();
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));
  //mac.SetType ("ns3::NqstaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  //mac.SetType ("ns3::NqapWifiMac", "Ssid", SsidValue (ssid));
  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);


// Set up the nodes in a star topology
// MobilityHelper mobility;

// Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
// for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i)
// {
//   // Arrange the nodes in a star topology around the point (0,0)
//   double angle = (double)i/wifiStaNodes.GetN() * 2 * M_PI;
//   positionAlloc->Add (Vector (10.0 * cos(angle), 10.0 * sin(angle), 0.0));
// }

// mobility.SetPositionAllocator (positionAlloc);

// // Set the mobility model to RandomWalk2dMobilityModel with bounds encompassing the star
// mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue (Rectangle (-10, 10, -10, 10)));
// mobility.Install (wifiStaNodes);

// // Position the AP node at the center of the star
// positionAlloc = CreateObject<ListPositionAllocator> ();
// positionAlloc->Add (Vector (0.0, 0.0, 0.0));
// mobility.SetPositionAllocator (positionAlloc);
// mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
// mobility.Install (wifiApNode);





  InternetStackHelper stack;
  //stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;


  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (apDevices);
  Ipv4InterfaceContainer staInterfaces = address.Assign (staDevices);
  

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (staInterfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (8));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AnimationInterface anim("l7q1.xml");

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
