// This code slightly follows the conventions of, but is not derived from:
//      EHTERSHIELD_H library for Arduino etherShield
//      Copyright (c) 2008 Xing Yu.  All right reserved. (this is LGPL v2.1)
// It is however derived from the enc28j60 and ip code (which is GPL v2)
//      Author: Pascal Stang
//      Modified by: Guido Socher
//      DHCP code: Andrew Lindsay
// Hence: GPL V2
//
// 2010-05-19 <jc@wippler.nl>
//
//
// PIN Connections (Using Arduino UNO):
//   VCC -   3.3V
//   GND -    GND
//   SCK - Pin 13
//   SO  - Pin 12
//   SI  - Pin 11
//   CS  - Pin  8
//
#define __PROG_TYPES_COMPAT__

#ifndef EtherCard_h
#define EtherCard_h


#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#define WRITE_RESULT size_t
#define WRITE_RETURN return 1;
#else
#include <WProgram.h> // Arduino 0022
#define WRITE_RESULT void
#define WRITE_RETURN
#endif

#include <avr/pgmspace.h>
#include "enc28j60.h"
#include "net.h"

/** This type definition defines the structure of a UDP server event handler callback funtion */
typedef void (*UdpServerCallback)(
    uint16_t dest_port,    ///< Port the packet was sent to
    uint8_t src_ip[4],    ///< IP address of the sender
    const char *data,   ///< UDP payload data
    uint16_t len);        ///< Length of the payload data

/** This structure describes the structure of memory used within the ENC28J60 network interface. */
typedef struct {
    uint8_t count;     ///< Number of allocated pages
    uint8_t first;     ///< First allocated page
    uint8_t last;      ///< Last allocated page
} StashHeader;

/** This class provides access to the memory within the ENC28J60 network interface. */
class Stash : public /*Stream*/ Print, private StashHeader {
    uint8_t curr;      //!< Current page
    uint8_t offs;      //!< Current offset in page

    typedef struct {
        union {
            uint8_t bytes[64];
            uint16_t words[32];
            struct {
                StashHeader head;
                uint8_t filler[59];
                uint8_t tail;
                uint8_t next;
            };
        };
        uint8_t bnum;
    } Block;

    static uint8_t allocBlock ();
    static void freeBlock (uint8_t block);
    static uint8_t fetchByte (uint8_t blk, uint8_t off);

    static Block bufs[2];
    static uint8_t map[256/8];

public:
    static void initMap (uint8_t last);
    static void load (uint8_t idx, uint8_t blk);
    static uint8_t freeCount ();

    Stash () : curr (0) { first = 0; }
    Stash (uint8_t fd) { open(fd); }

    uint8_t create ();
    uint8_t open (uint8_t blk);
    void save ();
    void release ();

    void put (char c);
    char get ();
    uint16_t size ();

    virtual WRITE_RESULT write(uint8_t b) { put(b); WRITE_RETURN }

    // virtual int available() {
    //   if (curr != last)
    //     return 1;
    //   load(1, last);
    //   return offs < bufs[1].tail;
    // }
    // virtual int read() {
    //   return available() ? get() : -1;
    // }
    // virtual int peek() {
    //   return available() ? bufs[1].bytes[offs] : -1;
    // }
    // virtual void flush() {
    //   curr = last;
    //   offs = 63;
    // }

    static void prepare (PGM_P fmt, ...);
    static uint16_t length ();
    static void extract (uint16_t offset, uint16_t count, void* buf);
    static void cleanup ();

    friend void dumpBlock (const char* msg, uint8_t idx); // optional
    friend void dumpStash (const char* msg, void* ptr);   // optional
};

/** This class populates network send / recieve buffers. */
class BufferFiller : public Print {
    uint8_t *start, *ptr;
public:
    BufferFiller () {}
    BufferFiller (uint8_t* buf) : start (buf), ptr (buf) {}

    void emit_p (PGM_P fmt, ...);
    void emit_raw (const char* s, uint16_t n) { memcpy(ptr, s, n); ptr += n; }
    void emit_raw_p (PGM_P p, uint16_t n) { memcpy_P(ptr, p, n); ptr += n; }

    uint8_t* buffer () const { return start; }
    uint16_t position () const { return ptr - start; }

    virtual WRITE_RESULT write (uint8_t v) { *ptr++ = v; WRITE_RETURN }
};

/** This class provides the main interface to a ENC28J60 based network interface card and is the class most users will use. */
class EtherCard : public Ethernet {
public:
    static uint8_t mymac[6];  ///< MAC address
    static uint8_t myip[4];   ///< IP address
    static uint8_t netmask[4]; ///< Netmask
    static uint8_t broadcastip[4]; ///< Subnet broadcast address
    static uint8_t gwip[4];   ///< Gateway
    static uint8_t dhcpip[4]; ///< DHCP server IP address
    static uint8_t dnsip[4];  ///< DNS server IP address
    static uint8_t hisip[4];  ///< DNS lookup result
    static uint16_t hisport;  ///< TCP port to connect to (default 80)
    static bool using_dhcp;   ///< True if using DHCP
    static bool persist_tcp_connection; ///< True to break connections on first packet received

    // EtherCard.cpp
    /**   @brief  Initialise the network interface
    *     @param  size Size of data buffer
    *     @param  macaddr Hardware address to assign to the network interface (6 bytes)
    *     @param  csPin Arduino pin number connected to chip select. Default = 8
    *     @return <i>uint8_t</i> Firmware version or zero on failure.
    */
    static uint8_t begin (const uint16_t size, const uint8_t* macaddr,
                          uint8_t csPin =8);

    /**   @brief  Configure network interface with static IP
    *     @param  my_ip IP address (4 bytes). 0 for no change.
    *     @param  gw_ip Gateway address (4 bytes). 0 for no change. Default = 0
    *     @param  dns_ip DNS address (4 bytes). 0 for no change. Default = 0
    *     @param  mask Subnet mask (4 bytes). 0 for no change. Default = 0
    *     @return <i>bool</i> Returns true on success - actually always true
    */
    static bool staticSetup (const uint8_t* my_ip,
                             const uint8_t* gw_ip = 0,
                             const uint8_t* dns_ip = 0,
                             const uint8_t* mask = 0);

    // tcpip.cpp
    /**   @brief  Does not seem to be implemented
    *     @todo   Remove declaration or impelement
    */
    static void initIp (uint8_t *myip, uint16_t wwwp);

    /**   @brief  Sends a UDP packet to the IP address of last processed recieved packet
    *     @param  data Pointer to data payload
    *     @param  len Size of data payload (max 220)
    *     @param  port Destination IP port
    */
    static void makeUdpReply (char *data, uint8_t len, uint16_t port);

    /**   @brief  Parse recieved data
    *     @param  plen Size of data to parse (e.g. return value of packetRecieve()).
    *     @return <i>uint16_t</i> Position of TCP data in data buffer
    *     @note   Data buffer is shared by recieve and transmit functions
    */
    static uint16_t packetLoop (uint16_t plen);

    /**   @brief
    *     @todo   Document accept
    */
    static uint16_t accept (uint16_t port, uint16_t plen);

    /**   @brief
    *     @todo   Document httpServerReply
    */
    static void httpServerReply (uint16_t dlen);

    /**   @brief
    *     @todo   Document httpServerReply_with_flags
    */
    static void httpServerReply_with_flags (uint16_t dlen , uint8_t flags);

    /**   @brief
    *     @todo   Document httpServerReplyAck
    */
    static void httpServerReplyAck ();

    /**   @brief  Set the gateway address
    *     @param  gwipaddr Gateway address (4 bytes)
    */
    static void setGwIp (const uint8_t *gwipaddr);

    /**   @brief  Updates the broadcast address based on current IP address and subnet mask
    */
    static void updateBroadcastAddress();

    /**   @brief  Check if got gateway hardware address (ARP lookup)
    *     @return <i>unit8_t</i> True if gateway found
    */
    static uint8_t clientWaitingGw ();

    /**   @brief
    *     @todo   Document clientTcpReq
    *     @return <i>unit8_t</i>
    */
    static uint8_t clientTcpReq (uint8_t (*r)(uint8_t,uint8_t,uint16_t,uint16_t),
                                 uint16_t (*d)(uint8_t),uint16_t port);

    /**   @brief
    *     @todo   Document browseUrl
    */
    static void browseUrl (prog_char *urlbuf, const char *urlbuf_varpart,
                           prog_char *hoststr, const prog_char *additionalheaderline,
                           void (*callback)(uint8_t,uint16_t,uint16_t));

    /**   @brief
    *     @todo   Document browseUrl
    */
    static void browseUrl (prog_char *urlbuf, const char *urlbuf_varpart,
                           prog_char *hoststr,
                           void (*callback)(uint8_t,uint16_t,uint16_t));

    /**   @brief
    *     @todo   Document httpPost
    */
    static void httpPost (prog_char *urlbuf, prog_char *hoststr,
                          prog_char *additionalheaderline, const char *postval,
                          void (*callback)(uint8_t,uint16_t,uint16_t));

    /**   @brief
    *     @todo   Document ntpRequest
    */
    static void ntpRequest (uint8_t *ntpip,uint8_t srcport);

    /**   @brief  Process network time protocol response
    *     @param  time Pointer to integer to hold result
    *     @param  dstport_l Destination port to expect response. Set to zero to accept on any port
    *     @return <i>uint8_t</i> True (1) on success
    */
    static uint8_t ntpProcessAnswer (uint32_t *time, uint8_t dstport_l);

    /**   @brief  Prepare a UDP message for transmission
    *     @param  sport Source port
    *     @param  dip Pointer to 4 byte destination IP address
    *     @param  dport Destination port
    */
    static void udpPrepare (uint16_t sport, const uint8_t *dip, uint16_t dport);

    /**   @brief  Transmit UDP packet
    *     @param  len Size of payload
    */
    static void udpTransmit (uint16_t len);

    /**   @brief  Sends a UDP packet
    *     @param  data Pointer to data
    *     @param  len Size of payload (maximum 220 octets / bytes)
    *     @param  sport Source port
    *     @param  dip Pointer to 4 byte destination IP address
    *     @param  dport Destination port
    */
    static void sendUdp (const char *data, uint8_t len, uint16_t sport,
                         const uint8_t *dip, uint16_t dport);
    /**   @brief  Resister the function to handle ping events
    *     @param  cb Pointer to function
    */
    static void registerPingCallback (void (*cb)(uint8_t*));

    /**   @brief  Send ping
    *     @param  destip Ponter to 4 byte destination IP address
    */
    static void clientIcmpRequest (const uint8_t *destip);

    /**   @brief  Check if for response
    *     @param  ip_monitoredhost Pointer to 4 byte IP address of host to check
    *     @return <i>uint8_t</i> True (1) if ping response from specified host
    */
    static uint8_t packetLoopIcmpCheckReply (const uint8_t *ip_monitoredhost);

    /**   @brief  Send a wake on lan message
    *     @param  wolmac Pointer to 6 byte hardware (MAC) address of host to send message to
    */
    static void sendWol (uint8_t *wolmac);

    // new stash-based API
    /**   @brief
    *     @todo Document tcpSend
    */
    static uint8_t tcpSend ();

    /**   @brief
    *     @todo Document tcpReply
    */
    static const char* tcpReply (uint8_t fd);

    /**   @brief
    *     @todo Document persistTcpConnection
    */
    static void persistTcpConnection(bool persist);

    //udpserver.cpp
    /**   @brief  Register function to handle incomint UDP events
    *     @param  callback Function to handle event
    *     @param  port Port to listen on
    */
    static void udpServerListenOnPort(UdpServerCallback callback, uint16_t port);

    /**   @brief  Pause listing on UDP port
    *     @brief  port Port to pause
    */
    static void udpServerPauseListenOnPort(uint16_t port);

    /**   @brief  Resume listing on UDP port
    *     @brief  port Port to pause
    */
    static void udpServerResumeListenOnPort(uint16_t port);

    /**   @brief  Check if UDP server is listening on any ports
    *     @return <i>bool</i> True if listening on any ports
    */
    static bool udpServerListening();                        //called by tcpip, in packetLoop

    /**   @brief  Passes packet to UDP Server
    *     @param  len Not used
    *     @return <i>bool</i> True if packet processed
    */
    static bool udpServerHasProcessedPacket(uint16_t len);    //called by tcpip, in packetLoop

    // dhcp.cpp
    /**   @brief  Update DHCP state
    *     @param  len Length of recieved data packet
    */
    static void DhcpStateMachine(uint16_t len);

    /**   @brief Not implemented
    *     @todo Implement dhcpStartTime or remove declaration
    */
    static uint32_t dhcpStartTime ();

    /**   @brief Not implemented
    *     @todo Implement dhcpLeaseTime or remove declaration
    */
    static uint32_t dhcpLeaseTime ();

    /**   @brief Not implemented
    *     @todo Implement dhcpLease or remove declaration
    */
    static bool dhcpLease ();

    /**   @brief  Configure network interface with DHCP
    *     @return <i>bool</i> True if DHCP successful
    *     @note   Blocks until DHCP complete or timeout after 60 seconds
    */
    static bool dhcpSetup ();

    // dns.cpp
    /**   @brief  Perform DNS lookup
    *     @param  name Host name to lookup
    *     @param  fromRam Set true to look up cached name. Default = false
    *     @return <i>bool</i> True on success.
    *     @note   Result is stored in <i>hisip</i> member
    */
    static bool dnsLookup (const prog_char* name, bool fromRam =false);

    // webutil.cpp
    /**   @brief  Copies an IP address
    *     @param  dst Pointer to the 4 byte destination
    *     @param  src Pointer to the 4 byte source
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    */
    static void copyIp (uint8_t *dst, const uint8_t *src);

    /**   @brief  Copies a hardware address
    *     @param  dst Pointer to the 6 byte destination
    *     @param  src Pointer to the 6 byte destination
    *     @note   There is no check of source or destination size. Ensure both are 6 bytes
    */
    static void copyMac (uint8_t *dst, const uint8_t *src);

    /**   @brief  Output to serial port in dotted decimal IP format
    *     @param  buf Pointer to 4 byte IP address
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    */
    static void printIp (const uint8_t *buf);

    /**   @brief  Output message and IP address to serial port in dotted decimal IP format
    *     @param  msg Pointer to null terminated string
    *     @param  buf Pointer to 4 byte IP address
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    */
    static void printIp (const char* msg, const uint8_t *buf);

    /**   @brief  Output Flash String Helper and IP address to serial port in dotted decimal IP format
    *     @param  ifsh Pointer to Flash String Helper
    *     @param  buf Pointer to 4 byte IP address
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    *     @todo   What is a FlashStringHelper?
    */
    static void printIp (const __FlashStringHelper *ifsh, const uint8_t *buf);

    /**   @brief  Search for a string of the form key=value in a string that looks like q?xyz=abc&uvw=defgh HTTP/1.1\\r\\n
    *     @param  str Pointer to the null terminated string to search
    *     @param  strbuf Pointer to buffer to hold null terminated result string
    *     @param  maxlen Maximum length of result
    *     @param  key Pointer to null terminated string holding the key to search for
    *     @return <i>unit_t</i> Length of the value. 0 if not found
    *     @note   Ensure strbuf has memory allocated of at least maxlen + 1 (to accomodate result plus terminating null)
    */
    static uint8_t findKeyVal(const char *str,char *strbuf,
                              uint8_t maxlen, const char *key);

    /**   @brief  Decode a URL string e.g "hello%20joe" or "hello+joe" becomes "hello joe"
    *     @param  urlbuf Pointer to the null terminated URL
    *     @note   urlbuf is modified
    */
    static void urlDecode(char *urlbuf);

    /**   @brief  Encode a URL, replacing illegal charaters like ' '
    *     @param  str Pointer to the null terminated string to encode
    *     @param  urlbuf Pointer to a buffer to contain the null terminated encoded URL
    *     @note   There must be enough space in urlbuf. In the worst case that is 3 times the length of str
    */
    static  void urlEncode(char *str,char *urlbuf);

    /**   @brief  Convert an IP address from dotted decimal formated string to 4 bytes
    *     @param  bytestr Pointer to the 4 byte array to store IP address
    *     @param  str Pointer to string to parse
    *     @return <i>uint8_t</i> 0 on success
    */
    static uint8_t parseIp(uint8_t *bytestr,char *str);

    /**   @brief  Convert a byte array to a human readable display string
    *     @param  resultstr Pointer to a buffer to hold the resulting null terminated string
    *     @param  bytestr Pointer to the byte array containing the address to convert
    *     @param  len Length of the array (4 for IP address, 6 for hardware (MAC) address)
    *     @param  separator Delimiter character (typically '.' for IP address and ':' for hardware (MAC) address)
    *     @param  base Base for numerical representation (typically 10 for IP address and 16 for hardware (MAC) address
    */
    static void makeNetStr(char *resultstr,uint8_t *bytestr,uint8_t len,
                           char separator,uint8_t base);
};

extern EtherCard ether; //!< Global presentation of EtherCard class

#endif
