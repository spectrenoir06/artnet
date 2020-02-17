#ifndef ARTNET_H
#define ARTNET_H

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO)
	#include <WiFi101.h>
	#include <WiFiUdp.h>
#elif defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <WiFiUdp.h>
#elif defined(ESP32)
	#include <WiFi.h>
	#include <AsyncUDP_big.h>
#else
	#include <Ethernet.h>
	#include <EthernetUdp.h>
#endif

// UDP specific
#define ART_NET_PORT 6454
// Opcodes
#define ART_POLL 0x2000
#define ART_POLL_REPLY 0x2100
#define ART_DMX 0x5000
#define ART_SYNC 0x5200
#define ART_ADDR 0x6000
// Buffers
#define MAX_BUFFER_ARTNET 530
// Packet
#define ART_NET_ID "Art-Net\0"
#define ART_DMX_START 18

struct artnet_reply_s {
	uint8_t  id[8];
	uint16_t opCode;
	uint8_t  ip[4];
	uint16_t port;
	uint8_t  verH;
	uint8_t  ver;
	uint8_t  subH;
	uint8_t  sub;
	uint8_t  oemH;
	uint8_t  oem;
	uint8_t  ubea;
	uint8_t  status;
	uint8_t  etsaman[2];
	uint8_t  shortname[18];
	uint8_t  longname[64];
	uint8_t  nodereport[64];
	uint8_t  numbportsH;
	uint8_t  numbports;
	uint8_t  porttypes[4];//max of 4 ports per node
	uint8_t  goodinput[4];
	uint8_t  goodoutput[4];
	uint8_t  swin[4];
	uint8_t  swout[4];
	uint8_t  swvideo;
	uint8_t  swmacro;
	uint8_t  swremote;
	uint8_t  sp1;
	uint8_t  sp2;
	uint8_t  sp3;
	uint8_t  style;
	uint8_t  mac[6];
	uint8_t  bindip[4];
	uint8_t  bindindex;
	uint8_t  status2;
	uint8_t  filler[26];
} __attribute__((packed));

class Artnet
{
	public:
		Artnet();

		void begin(WiFiUDP *udp);
		void begin();
		void begin(uint8_t nodes, uint8_t numbports);
		void setBroadcast(byte bc[]);
		uint16_t read(AsyncUDP_bigPacket *packet);
		void setDefault(uint8_t numbports);
		void setIp(IPAddress ip);

		inline void setArtDmxCallback(void (*fptr)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP))
		{
			artDmxCallback = fptr;
		}

		inline void setArtSyncCallback(void (*fptr)(IPAddress remoteIP))
		{
			artSyncCallback = fptr;
		}

	private:
		uint8_t  node_ip_address[4];
		uint8_t  id[8];
		uint8_t nodes_nb;
		#if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
			WiFiUDP *Udp;
		#else
			EthernetUDP Udp;
		#endif
		struct artnet_reply_s ArtPollReply;

		IPAddress broadcast;
		void (*artDmxCallback)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP);
		void (*artSyncCallback)(IPAddress remoteIP);
};

#endif
