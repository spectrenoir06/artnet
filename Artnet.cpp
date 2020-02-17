#include <Artnet.h>

Artnet::Artnet() {}

void Artnet::begin()
{
	nodes_nb = 1;
	setDefault(1);
}

void Artnet::begin(uint8_t nodes, uint8_t numbports)
{
	nodes_nb = nodes;
	setDefault(numbports);
}

void Artnet::setDefault(uint8_t numbports) {
	ArtPollReply.subH = 0; // net
	ArtPollReply.sub  = 0; // sub

	uint8_t swin[4]  = {0x00,0x01,0x02,0x03};
	uint8_t swout[4] = {0x00,0x01,0x02,0x03};
	for(uint8_t i = 0; i < 4; i++)
	{
		ArtPollReply.swout[i] = swout[i];
		ArtPollReply.swin[i] = swin[i];
	}

	setIp(WiFi.localIP());

	sprintf((char *)id, "Art-Net");
	memcpy(ArtPollReply.id, id, sizeof(ArtPollReply.id));

	ArtPollReply.opCode = ART_POLL_REPLY;
	ArtPollReply.port =  ART_NET_PORT;


	memset(ArtPollReply.goodinput,  0x08, 4);
	ArtPollReply.goodoutput[0] = 0x80; //memset(ArtPollReply.goodoutput[0],  0x80, 4);
	memset(ArtPollReply.porttypes,  0xc0, 4);

	uint8_t shortname [18];
	uint8_t longname [64];
	bzero(shortname, sizeof(shortname));
	bzero(longname, sizeof(longname));
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
	ArtPollReply.numbports  = numbports;
	ArtPollReply.status2    = 0x0E;

	sprintf((char *)ArtPollReply.nodereport, "%i DMX output universes active.", ArtPollReply.numbports);
}

void Artnet::setBroadcast(byte bc[]) {
	//sets the broadcast address
	broadcast = bc;
}

void Artnet::setIp(IPAddress ip) {
	node_ip_address[0] = ip[0];
	node_ip_address[1] = ip[1];
	node_ip_address[2] = ip[2];
	node_ip_address[3] = ip[3];

	memcpy(ArtPollReply.ip, node_ip_address, sizeof(ArtPollReply.ip));
}

uint16_t Artnet::read(AsyncUDP_bigPacket *packet) {
	uint8_t *swin;
	uint8_t *swout;

	uint8_t sequence;
	uint16_t incomingUniverse;
	uint16_t dmxDataLength;

	uint8_t *artnetPacket	= packet->data();
	uint16_t packetSize		= packet->length();

	if (packetSize <= MAX_BUFFER_ARTNET && packetSize > 0)
	{
		// Check that packetID is "Art-Net" else ignore
		// for (byte i = 0 ; i < 8 ; i++)
		// {
		// 	if (artnetPacket[i] != ART_NET_ID[i])
		// 	return 0;
		// }

		uint16_t opcode = artnetPacket[8] | artnetPacket[9] << 8;

		switch (opcode) {

			case ART_DMX:
				sequence = artnetPacket[12];
				incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
				dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;

				if (artDmxCallback)
					(*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START, packet->remoteIP());

				return ART_DMX;

			case ART_POLL:
				//fill the reply struct, and then send it to the network's broadcast address
				Serial.print("Art-Net POLL from ");
				Serial.println(packet->remoteIP());
				// Serial.print(" broadcast addr: ");
				// Serial.println(broadcast);

				ArtPollReply.bindip[0] = packet->remoteIP()[0];
				ArtPollReply.bindip[1] = packet->remoteIP()[1];
				ArtPollReply.bindip[2] = packet->remoteIP()[2];
				ArtPollReply.bindip[3] = packet->remoteIP()[3];

				for (int i=0; i<nodes_nb;i++){
					ArtPollReply.bindindex = i+1;
					ArtPollReply.sub = i*ArtPollReply.numbports;
					packet->write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
				}
				return ART_POLL;


			case ART_SYNC:
				if (artSyncCallback) (*artSyncCallback)(packet->remoteIP());
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

				packet->write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));

				return ART_ADDR;

			default :
				Serial.printf("opcode unknow 0x%x\n", opcode);
				return 0;
		}
	}
	else
		return 0;
}
