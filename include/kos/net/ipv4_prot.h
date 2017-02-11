/* MIT License
 *
 * Copyright (c) 2017 GrieferAtWork
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __KOS_NET_IPV4_PROT_H__
#define __KOS_NET_IPV4_PROT_H__ 1

/* Auto-generate by: make_ipv4_prot.dee. */
/* Based on table found here: https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers */

#define IPV4_PROT_HOPOPT                        0x00 /* HOPOPT: IPv6 Hop-by-Hop Option 	RFC 2460. */
#define IPV4_PROT_ICMP                          0x01 /* ICMP: Internet Control Message Protocol 	RFC 792. */
#define IPV4_PROT_IGMP                          0x02 /* IGMP: Internet Group Management Protocol 	RFC 1112. */
#define IPV4_PROT_GGP                           0x03 /* GGP: Gateway-to-Gateway Protocol 	RFC 823. */
#define IPV4_PROT_IP_IN_IP                      0x04 /* IP-in-IP: IP in IP (encapsulation) 	RFC 2003. */
#define IPV4_PROT_ST                            0x05 /* ST: Internet Stream Protocol 	RFC 1190, RFC 1819. */
#define IPV4_PROT_TCP                           0x06 /* TCP: Transmission Control Protocol 	RFC 793. */
#define IPV4_PROT_CBT                           0x07 /* CBT: Core-based trees 	RFC 2189. */
#define IPV4_PROT_EGP                           0x08 /* EGP: Exterior Gateway Protocol 	RFC 888. */
#define IPV4_PROT_IGP                           0x09 /* IGP: Interior Gateway Protocol (any private interior gateway (used by Cisco for their IGRP)). */
#define IPV4_PROT_BBN_RCC_MON                   0x0A /* BBN-RCC-MON: BBN RCC Monitoring. */
#define IPV4_PROT_NVP_II                        0x0B /* NVP-II: Network Voice Protocol 	RFC 741. */
#define IPV4_PROT_PUP                           0x0C /* PUP: Xerox PUP. */
#define IPV4_PROT_ARGUS                         0x0D /* ARGUS: ARGUS. */
#define IPV4_PROT_EMCON                         0x0E /* EMCON: EMCON. */
#define IPV4_PROT_XNET                          0x0F /* XNET: Cross Net Debugger 	IEN 158. */
#define IPV4_PROT_CHAOS                         0x10 /* CHAOS: Chaos. */
#define IPV4_PROT_UDP                           0x11 /* UDP: User Datagram Protocol 	RFC 768. */
#define IPV4_PROT_MUX                           0x12 /* MUX: Multiplexing 	IEN 90. */
#define IPV4_PROT_DCN_MEAS                      0x13 /* DCN-MEAS: DCN Measurement Subsystems. */
#define IPV4_PROT_HMP                           0x14 /* HMP: Host Monitoring Protocol 	RFC 869. */
#define IPV4_PROT_PRM                           0x15 /* PRM: Packet Radio Measurement. */
#define IPV4_PROT_XNS_IDP                       0x16 /* XNS-IDP: XEROX NS IDP. */
#define IPV4_PROT_TRUNK_1                       0x17 /* TRUNK-1: Trunk-1. */
#define IPV4_PROT_TRUNK_2                       0x18 /* TRUNK-2: Trunk-2. */
#define IPV4_PROT_LEAF_1                        0x19 /* LEAF-1: Leaf-1. */
#define IPV4_PROT_LEAF_2                        0x1A /* LEAF-2: Leaf-2. */
#define IPV4_PROT_RDP                           0x1B /* RDP: Reliable Datagram Protocol 	RFC 908. */
#define IPV4_PROT_IRTP                          0x1C /* IRTP: Internet Reliable Transaction Protocol 	RFC 938. */
#define IPV4_PROT_ISO_TP4                       0x1D /* ISO-TP4: ISO Transport Protocol Class 4 	RFC 905. */
#define IPV4_PROT_NETBLT                        0x1E /* NETBLT: Bulk Data Transfer Protocol 	RFC 998. */
#define IPV4_PROT_MFE_NSP                       0x1F /* MFE-NSP: MFE Network Services Protocol. */
#define IPV4_PROT_MERIT_INP                     0x20 /* MERIT-INP: MERIT Internodal Protocol. */
#define IPV4_PROT_DCCP                          0x21 /* DCCP: Datagram Congestion Control Protocol 	RFC 4340. */
#define IPV4_PROT_3PC                           0x22 /* 3PC: Third Party Connect Protocol. */
#define IPV4_PROT_IDPR                          0x23 /* IDPR: Inter-Domain Policy Routing Protocol 	RFC 1479. */
#define IPV4_PROT_XTP                           0x24 /* XTP: Xpress Transport Protocol. */
#define IPV4_PROT_DDP                           0x25 /* DDP: Datagram Delivery Protocol. */
#define IPV4_PROT_IDPR_CMTP                     0x26 /* IDPR-CMTP: IDPR Control Message Transport Protocol. */
#define IPV4_PROT_TPPP                          0x27 /* TP++: TP++ Transport Protocol. */
#define IPV4_PROT_IL                            0x28 /* IL: IL Transport Protocol. */
#define IPV4_PROT_IPV6                          0x29 /* IPv6: IPv6 Encapsulation 	RFC 2473. */
#define IPV4_PROT_SDRP                          0x2A /* SDRP: Source Demand Routing Protocol 	RFC 1940. */
#define IPV4_PROT_IPV6_ROUTE                    0x2B /* IPv6-Route: Routing Header for IPv6 	RFC 2460. */
#define IPV4_PROT_IPV6_FRAG                     0x2C /* IPv6-Frag: Fragment Header for IPv6 	RFC 2460. */
#define IPV4_PROT_IDRP                          0x2D /* IDRP: Inter-Domain Routing Protocol. */
#define IPV4_PROT_RSVP                          0x2E /* RSVP: Resource Reservation Protocol 	RFC 2205. */
#define IPV4_PROT_GRE                           0x2F /* GRE: Generic Routing Encapsulation 	RFC 2784, RFC 2890. */
#define IPV4_PROT_DSR                           0x30 /* DSR: Dynamic Source Routing Protocol 	RFC 4728. */
#define IPV4_PROT_BNA                           0x31 /* BNA: Burroughs Network Architecture. */
#define IPV4_PROT_ESP                           0x32 /* ESP: Encapsulating Security Payload 	RFC 4303. */
#define IPV4_PROT_AH                            0x33 /* AH: Authentication Header 	RFC 4302. */
#define IPV4_PROT_I_NLSP                        0x34 /* I-NLSP: Integrated Net Layer Security Protocol 	TUBA. */
#define IPV4_PROT_SWIPE                         0x35 /* SWIPE: SwIPe 	IP with Encryption. */
#define IPV4_PROT_NARP                          0x36 /* NARP: NBMA Address Resolution Protocol 	RFC 1735. */
#define IPV4_PROT_MOBILE                        0x37 /* MOBILE: IP Mobility (Min Encap) 	RFC 2004. */
#define IPV4_PROT_TLSP                          0x38 /* TLSP: Transport Layer Security Protocol (using Kryptonet key management). */
#define IPV4_PROT_SKIP                          0x39 /* SKIP: Simple Key-Management for Internet Protocol 	RFC 2356. */
#define IPV4_PROT_IPV6_ICMP                     0x3A /* IPv6-ICMP: ICMP for IPv6 	RFC 4443, RFC 4884. */
#define IPV4_PROT_IPV6_NONXT                    0x3B /* IPv6-NoNxt: No Next Header for IPv6 	RFC 2460. */
#define IPV4_PROT_IPV6_OPTS                     0x3C /* IPv6-Opts: Destination Options for IPv6 	RFC 2460. */
#define IPV4_PROT_ANY_HOST_INTERNAL_PROTOCOL    0x3D /* Any host internal protocol. */
#define IPV4_PROT_CFTP                          0x3E /* CFTP: CFTP. */
#define IPV4_PROT_ANY_LOCAL_NETWORK             0x3F /* Any local network. */
#define IPV4_PROT_SAT_EXPAK                     0x40 /* SAT-EXPAK: SATNET and Backroom EXPAK. */
#define IPV4_PROT_KRYPTOLAN                     0x41 /* KRYPTOLAN: Kryptolan. */
#define IPV4_PROT_RVD                           0x42 /* RVD: MIT Remote Virtual Disk Protocol. */
#define IPV4_PROT_IPPC                          0x43 /* IPPC: Internet Pluribus Packet Core. */
#define IPV4_PROT_ANY_DISTRIBUTED_FILE_SYSTEM   0x44 /* Any distributed file system. */
#define IPV4_PROT_SAT_MON                       0x45 /* SAT-MON: SATNET Monitoring. */
#define IPV4_PROT_VISA                          0x46 /* VISA: VISA Protocol. */
#define IPV4_PROT_IPCU                          0x47 /* IPCU: Internet Packet Core Utility. */
#define IPV4_PROT_CPNX                          0x48 /* CPNX: Computer Protocol Network Executive. */
#define IPV4_PROT_CPHB                          0x49 /* CPHB: Computer Protocol Heart Beat. */
#define IPV4_PROT_WSN                           0x4A /* WSN: Wang Span Network. */
#define IPV4_PROT_PVP                           0x4B /* PVP: Packet Video Protocol. */
#define IPV4_PROT_BR_SAT_MON                    0x4C /* BR-SAT-MON: Backroom SATNET Monitoring. */
#define IPV4_PROT_SUN_ND                        0x4D /* SUN-ND: SUN ND PROTOCOL-Temporary. */
#define IPV4_PROT_WB_MON                        0x4E /* WB-MON: WIDEBAND Monitoring. */
#define IPV4_PROT_WB_EXPAK                      0x4F /* WB-EXPAK: WIDEBAND EXPAK. */
#define IPV4_PROT_ISO_IP                        0x50 /* ISO-IP: International Organization for Standardization Internet Protocol. */
#define IPV4_PROT_VMTP                          0x51 /* VMTP: Versatile Message Transaction Protocol 	RFC 1045. */
#define IPV4_PROT_SECURE_VMTP                   0x52 /* SECURE-VMTP: Secure Versatile Message Transaction Protocol 	RFC 1045. */
#define IPV4_PROT_VINES                         0x53 /* VINES: VINES. */
#define IPV4_PROT_TTP                           0x54 /* TTP: TTP. */
#define IPV4_PROT_IPTM                          0x54 /* IPTM: Internet Protocol Traffic Manager. */
#define IPV4_PROT_NSFNET_IGP                    0x55 /* NSFNET-IGP: NSFNET-IGP. */
#define IPV4_PROT_DGP                           0x56 /* DGP: Dissimilar Gateway Protocol. */
#define IPV4_PROT_TCF                           0x57 /* TCF: TCF. */
#define IPV4_PROT_EIGRP                         0x58 /* EIGRP: EIGRP. */
#define IPV4_PROT_OSPF                          0x59 /* OSPF: Open Shortest Path First 	RFC 1583. */
#define IPV4_PROT_SPRITE_RPC                    0x5A /* Sprite-RPC: Sprite RPC Protocol. */
#define IPV4_PROT_LARP                          0x5B /* LARP: Locus Address Resolution Protocol. */
#define IPV4_PROT_MTP                           0x5C /* MTP: Multicast Transport Protocol. */
#define IPV4_PROT_AX_25                         0x5D /* AX.25: AX.25. */
#define IPV4_PROT_OS                            0x5E /* OS: KA9Q NOS compatible IP over IP tunneling. */
#define IPV4_PROT_MICP                          0x5F /* MICP: Mobile Internetworking Control Protocol. */
#define IPV4_PROT_SCC_SP                        0x60 /* SCC-SP: Semaphore Communications Sec. Pro. */
#define IPV4_PROT_ETHERIP                       0x61 /* ETHERIP: Ethernet-within-IP Encapsulation 	RFC 3378. */
#define IPV4_PROT_ENCAP                         0x62 /* ENCAP: Encapsulation Header 	RFC 1241. */
#define IPV4_PROT_ANY_PRIVATE_ENCRYPTION_SCHEME 0x63 /* Any private encryption scheme. */
#define IPV4_PROT_GMTP                          0x64 /* GMTP: GMTP. */
#define IPV4_PROT_IFMP                          0x65 /* IFMP: Ipsilon Flow Management Protocol. */
#define IPV4_PROT_PNNI                          0x66 /* PNNI: PNNI over IP. */
#define IPV4_PROT_PIM                           0x67 /* PIM: Protocol Independent Multicast. */
#define IPV4_PROT_ARIS                          0x68 /* ARIS: IBM's ARIS (Aggregate Route IP Switching) Protocol. */
#define IPV4_PROT_SCPS                          0x69 /* SCPS: SCPS (Space Communications Protocol Standards) 	SCPS-TP[2]. */
#define IPV4_PROT_QNX                           0x6A /* QNX: QNX. */
#define IPV4_PROT_A_N                           0x6B /* A/N: Active Networks. */
#define IPV4_PROT_IPCOMP                        0x6C /* IPComp: IP Payload Compression Protocol 	RFC 3173. */
#define IPV4_PROT_SNP                           0x6D /* SNP: Sitara Networks Protocol. */
#define IPV4_PROT_COMPAQ_PEER                   0x6E /* Compaq-Peer: Compaq Peer Protocol. */
#define IPV4_PROT_IPX_IN_IP                     0x6F /* IPX-in-IP: IPX in IP. */
#define IPV4_PROT_VRRP                          0x70 /* VRRP: Virtual Router Redundancy Protocol, Common Address Redundancy Protocol (not IANA assigned) 	VRRP:RFC 3768. */
#define IPV4_PROT_PGM                           0x71 /* PGM: PGM Reliable Transport Protocol 	RFC 3208. */
#define IPV4_PROT_ANY_0_HOP_PROTOCOL            0x72 /* Any 0-hop protocol. */
#define IPV4_PROT_L2TP                          0x73 /* L2TP: Layer Two Tunneling Protocol Version 3 	RFC 3931. */
#define IPV4_PROT_DDX                           0x74 /* DDX: D-II Data Exchange (DDX). */
#define IPV4_PROT_IATP                          0x75 /* IATP: Interactive Agent Transfer Protocol. */
#define IPV4_PROT_STP                           0x76 /* STP: Schedule Transfer Protocol. */
#define IPV4_PROT_SRP                           0x77 /* SRP: SpectraLink Radio Protocol. */
#define IPV4_PROT_UTI                           0x78 /* UTI: Universal Transport Interface Protocol. */
#define IPV4_PROT_SMP                           0x79 /* SMP: Simple Message Protocol. */
#define IPV4_PROT_SM                            0x7A /* SM: Simple Multicast Protocol 	draft-perlman-simple-multicast-03. */
#define IPV4_PROT_PTP                           0x7B /* PTP: Performance Transparency Protocol. */
#define IPV4_PROT_IS_IS_OVER_IPV4               0x7C /* IS-IS over IPv4: Intermediate System to Intermediate System (IS-IS) Protocol over IPv4 	RFC 1142 and RFC 1195. */
#define IPV4_PROT_FIRE                          0x7D /* FIRE: Flexible Intra-AS Routing Environment. */
#define IPV4_PROT_CRTP                          0x7E /* CRTP: Combat Radio Transport Protocol. */
#define IPV4_PROT_CRUDP                         0x7F /* CRUDP: Combat Radio User Datagram. */
#define IPV4_PROT_SSCOPMCE                      0x80 /* SSCOPMCE: Service-Specific Connection-Oriented Protocol in a Multilink and Connectionless Environment 	ITU-T Q.2111 (1999). */
#define IPV4_PROT_IPLT                          0x81 /* IPLT. */
#define IPV4_PROT_SPS                           0x82 /* SPS: Secure Packet Shield. */
#define IPV4_PROT_PIPE                          0x83 /* PIPE: Private IP Encapsulation within IP 	Expired I-D draft-petri-mobileip-pipe-00.txt. */
#define IPV4_PROT_SCTP                          0x84 /* SCTP: Stream Control Transmission Protocol. */
#define IPV4_PROT_FC                            0x85 /* FC: Fibre Channel. */
#define IPV4_PROT_RSVP_E2E_IGNORE               0x86 /* RSVP-E2E-IGNORE: Reservation Protocol (RSVP) End-to-End Ignore 	RFC 3175. */
#define IPV4_PROT_MOBILITY_HEADER               0x87 /* Mobility Header: Mobility Extension Header for IPv6 	RFC 6275. */
#define IPV4_PROT_UDPLITE                       0x88 /* UDPLite: Lightweight User Datagram Protocol 	RFC 3828. */
#define IPV4_PROT_MPLS_IN_IP                    0x89 /* MPLS-in-IP: Multiprotocol Label Switching Encapsulated in IP 	RFC 4023. */
#define IPV4_PROT_MANET                         0x8A /* manet: MANET Protocols 	RFC 5498. */
#define IPV4_PROT_HIP                           0x8B /* HIP: Host Identity Protocol 	RFC 5201. */
#define IPV4_PROT_SHIM6                         0x8C /* Shim6: Site Multihoming by IPv6 Intermediation 	RFC 5533. */
#define IPV4_PROT_WESP                          0x8D /* WESP: Wrapped Encapsulating Security Payload 	RFC 5840. */
#define IPV4_PROT_ROHC                          0x8E /* ROHC: Robust Header Compression 	RFC 5856. */
#define IPV4_PROT_RESERVED_FOR_EXTRA            0xFF /* Reserved for extra. */

#endif /* !__KOS_NET_IPV4_PROT_H__ */
