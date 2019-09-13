#include <Artnet_old.h>

Artnet_old::Artnet_old() {}

void Artnet_old::begin(WiFiUDP *udp)
{
	Udp = udp;
	setDefault();
}

void Artnet_old::setDefault() {
	ArtPollReply.subH = 0; // net
	ArtPollReply.sub  = 0; // sub

	uint8_t swin[4]  = {0x00,0x01,0x02,0x03};
	uint8_t swout[4] = {0x00,0x01,0x02,0x03};
	for(uint8_t i = 0; i < 4; i++)
	{
		ArtPollReply.swout[i] = swout[i];
		ArtPollReply.swin[i] = swin[i];
	}

	sprintf((char *)id, "Art-Net");
	memcpy(ArtPollReply.id, id, sizeof(ArtPollReply.id));
	memcpy(ArtPollReply.ip, node_ip_address, sizeof(ArtPollReply.ip));

	ArtPollReply.opCode = ART_POLL_REPLY;
	ArtPollReply.port =  ART_NET_PORT;

	memset(ArtPollReply.goodinput,  0x08, 4);
	memset(ArtPollReply.goodoutput,  0x80, 4);
	memset(ArtPollReply.porttypes,  0xc0, 4);

	uint8_t shortname [18];
	uint8_t longname [64];
	sprintf((char *)shortname, "ESP32 Artnet");
	sprintf((char *)longname, "ESP32 - Art-Net");
	memcpy(ArtPollReply.shortname, shortname, sizeof(shortname));
	memcpy(ArtPollReply.longname, longname, sizeof(longname));

	ArtPollReply.etsaman[0] = 0;
	ArtPollReply.etsaman[1] = 0;
	ArtPollReply.verH       = 1;
	ArtPollReply.ver        = 0;
	ArtPollReply.oemH       = 0;
	ArtPollReply.oem        = 0xFF;
	ArtPollReply.ubea       = 0;
	ArtPollReply.status     = 0xd2;
	ArtPollReply.swvideo    = 0;
	ArtPollReply.swmacro    = 0;
	ArtPollReply.swremote   = 0;
	ArtPollReply.style      = 0;

	ArtPollReply.numbportsH = 0;
	ArtPollReply.numbports  = 4;
	ArtPollReply.status2    = 0x08;

	sprintf((char *)ArtPollReply.nodereport, "%i DMX output universes active.", ArtPollReply.numbports);
}

void Artnet_old::setBroadcast(byte bc[]) {
	//sets the broadcast address
	broadcast = bc;
}

uint16_t Artnet_old::read(uint8_t *artnetPacket, uint16_t packetSize) {
	IPAddress local_ip;
	uint8_t *swin;
	uint8_t *swout;

	remoteIP = Udp->remoteIP();
	if (packetSize <= MAX_BUFFER_ARTNET && packetSize > 0)
	{
		// Check that packetID is "Art-Net" else ignore
		// for (byte i = 0 ; i < 8 ; i++)
		// {
		// 	if (artnetPacket[i] != ART_NET_ID[i])
		// 	return 0;
		// }

		opcode = artnetPacket[8] | artnetPacket[9] << 8;

		switch (opcode) {

			case ART_DMX:
				sequence = artnetPacket[12];
				incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
				dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;

				if (artDmxCallback)
					(*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START, remoteIP);

				return ART_DMX;

			case ART_POLL:
				//fill the reply struct, and then send it to the network's broadcast address
				Serial.print("Art-Net POLL from ");
				Serial.print(remoteIP);
				Serial.print(" broadcast addr: ");
				Serial.println(broadcast);

				#if !defined(ARDUINO_SAMD_ZERO) && !defined(ESP8266) && !defined(ESP32)
					local_ip = Ethernet.localIP();
				#else
					local_ip = WiFi.localIP();
				#endif

				node_ip_address[0] = local_ip[0];
				node_ip_address[1] = local_ip[1];
				node_ip_address[2] = local_ip[2];
				node_ip_address[3] = local_ip[3];

				ArtPollReply.bindip[0] = node_ip_address[0];
				ArtPollReply.bindip[1] = node_ip_address[1];
				ArtPollReply.bindip[2] = node_ip_address[2];
				ArtPollReply.bindip[3] = node_ip_address[3];

				Udp->beginPacket(remoteIP, ART_NET_PORT);//send the packet to the broadcast address
					Udp->write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
				Udp->endPacket();

				return ART_POLL;

			case ART_SYNC:
				if (artSyncCallback) (*artSyncCallback)(remoteIP);
				return ART_SYNC;

			case ART_ADDR:
				Serial.print("Art-Net ADRESS");

				if (artnetPacket[12] & 0x80)
					ArtPollReply.subH = artnetPacket[12] & 0x7F;

				if (artnetPacket[104] & 0x80)
					ArtPollReply.sub = artnetPacket[104] & 0x0F;

				swin  = artnetPacket + 96;
				swout = artnetPacket + 100;

				for(uint8_t i = 0; i < 4; i++) {
					if (swin[i] & 0x80)
						ArtPollReply.swin[i] = swin[i] & 0x0F;
					if (swout[i] & 0x80)
						ArtPollReply.swout[i] = swout[i] & 0x0F;
				}

				if (artnetPacket[14])
					memcpy(ArtPollReply.shortname, artnetPacket + 14, 18);

				if (artnetPacket[32])
					memcpy(ArtPollReply.longname, artnetPacket + 32, 64);

				Udp->beginPacket(broadcast, ART_NET_PORT); //send the packet to the broadcast address
					Udp->write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
				Udp->endPacket();

				return ART_ADDR;

			default :
				Serial.printf("opcode unknow 0x%x\n", opcode);
				return 0;
		}
	}
	else
		return 0;
}

void Artnet_old::printPacketHeader()
{
	Serial.print("packet size = ");
	Serial.print(packetSize);
	Serial.print("\topcode = ");
	Serial.print(opcode, HEX);
	Serial.print("\tuniverse number = ");
	Serial.print(incomingUniverse);
	Serial.print("\tdata length = ");
	Serial.print(dmxDataLength);
	Serial.print("\tsequence n0. = ");
	Serial.println(sequence);
}

void Artnet_old::printPacketContent()
{
	for (uint16_t i = ART_DMX_START ; i < dmxDataLength ; i++){
		Serial.print(artnetPacket[i], DEC);
		Serial.print("  ");
	}
	Serial.println('\n');
}
