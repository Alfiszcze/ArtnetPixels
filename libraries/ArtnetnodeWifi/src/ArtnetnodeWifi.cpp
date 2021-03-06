/*

Copyright (c) Charles Yarnold charlesyarnold@gmail.com 2015

Copyright (c) 2016 Stephan Ruloff
https://github.com/rstephan

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, under version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <ArtnetnodeWifi.h>

ArtnetnodeWifi::ArtnetnodeWifi()
{
  // Initalise DMXOutput array
  for (int i = 0; i < DMX_MAX_OUTPUTS; i++) {
    DMXOutputs[i][0] = -1;
    DMXOutputs[i][1] = -1;
    DMXOutputs[i][2] = 0;
  }

  // Start DMX tick clock
  msSinceDMXSend = 0;

  // Init DMX buffers
  for (int i = 0; i < DMX_MAX_OUTPUTS; i++) {
    memset(DMXBuffer[i], 0, sizeof(DMXBuffer[i]));
  }
}

/**
@retval 0 Ok
*/
uint8_t ArtnetnodeWifi::begin(void)
{
  byte mac[WL_MAC_ADDR_LENGTH];

  Udp.begin(ARTNET_PORT);
  localIP = WiFi.localIP();
  localMask = WiFi.subnetMask();
  localBroadcast = IPAddress(localIP | ~localMask);

  WiFi.macAddress(mac);
  PollReplyPacket.setMac(mac);
  PollReplyPacket.setIP(localIP);
  PollReplyPacket.canDHCP(true);
  PollReplyPacket.isDHCP(true);
  
  return 0;
}

uint8_t ArtnetnodeWifi::setShortName(const char name[])
{
  PollReplyPacket.setShortName(name);
}

uint8_t ArtnetnodeWifi::setLongName(const char name[])
{
  PollReplyPacket.setLongName(name);
}

uint8_t ArtnetnodeWifi::setName(const char name[])
{
  PollReplyPacket.setShortName(name);
  PollReplyPacket.setLongName(name);
}

uint8_t ArtnetnodeWifi::setStartingUniverse(uint16_t startingUniverse)
{
  PollReplyPacket.setStartingUniverse(startingUniverse);
}

uint16_t ArtnetnodeWifi::read()
{
  packetSize = Udp.parsePacket();

  if (packetSize <= ARTNET_MAX_BUFFER && packetSize > 0) {
    Udp.read(artnetPacket, ARTNET_MAX_BUFFER);

    // Check packetID equals "Art-Net"
    for (int i = 0 ; i < 9 ; i++) {
      if (artnetPacket[i] != ARTNET_ID[i]) {
        return 0;
      }
    }

    opcode = artnetPacket[8] | artnetPacket[9] << 8;

    switch (opcode) {
    case OpDmx:
      return handleDMX(0);
    case OpPoll:
      return handlePollRequest();
    case OpNzs:
      return handleDMX(1);
    }
  }

  return 0;
}

bool ArtnetnodeWifi::isBroadcast()
{
  if(Udp.remoteIP() == localBroadcast){
    return true;
  }
  else{
    return false;
  }
}

uint16_t ArtnetnodeWifi::handleDMX(uint8_t nzs)
{
  if (isBroadcast()) {
    return 0;
  } else {
    // Get universe
    uint16_t universe = artnetPacket[14] | artnetPacket[15] << 8;

    // Get DMX frame length
    uint16_t dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;
    
    // Sequence
    uint8_t sequence = artnetPacket[12];

    if (artDmxCallback) {
      (*artDmxCallback)(universe, dmxDataLength, sequence, artnetPacket + ART_DMX_START);
    }

    for(int a = 0; a < DMX_MAX_OUTPUTS; a++){
      if(DMXOutputs[a][1] == universe){
        for (int i = 0 ; i < DMX_MAX_BUFFER ; i++){
          if(i < dmxDataLength){
            DMXBuffer[a][i] = artnetPacket[i+ARTNET_DMX_START_LOC];
          }
          else{
            DMXBuffer[a][i] = 0;
          }
        }
      }
    }

    return OpDmx;
  }
}

uint16_t ArtnetnodeWifi::handlePollRequest()
{
  if (1 || isBroadcast()) {
    Udp.beginPacket(localBroadcast, ARTNET_PORT);
    Udp.write(PollReplyPacket.printPacket(), sizeof(PollReplyPacket.packet));
    Udp.endPacket();

    return OpPoll;
  } else{
    return 0;
  }
}

void ArtnetnodeWifi::enableDMX()
{
  DMXOutputStatus = true;
}

void ArtnetnodeWifi::disableDMX()
{
  DMXOutputStatus = false;
}

void ArtnetnodeWifi::enableDMXOutput(uint8_t outputID){
  DMXOutputs[outputID][2] = 1;

  int numEnabled = 0;
  for(int i = 0; i < DMX_MAX_OUTPUTS; i++){
    if(DMXOutputs[i][2]==1){
      if(numEnabled<4){
        numEnabled++;
      }
    }
  }
  PollReplyPacket.setNumPorts(numEnabled);
  PollReplyPacket.setOutputEnabled(outputID);
}

void ArtnetnodeWifi::disableDMXOutput(uint8_t outputID)
{
  DMXOutputs[outputID][2] = 0;

  int numEnabled = 0;
  for(int i = 0; i < DMX_MAX_OUTPUTS; i++){
    if(DMXOutputs[i][2]==1){
      if(numEnabled<4){
        numEnabled++;
      }
    }
  }
  PollReplyPacket.setNumPorts(numEnabled);
  PollReplyPacket.setOutputDisabled(outputID);
}

uint8_t ArtnetnodeWifi::setDMXOutput(uint8_t outputID, uint8_t uartNum, uint16_t attachedUniverse)
{
  // Validate input
  if(outputID>-1 && uartNum>-1 && attachedUniverse>-1){
    DMXOutputs[outputID][0] = uartNum;
    DMXOutputs[outputID][1] = attachedUniverse;
    DMXOutputs[outputID][2] = 0;
    PollReplyPacket.setSwOut(outputID, attachedUniverse);
    return 1;
  }
  else{
    return 0;
  }
}

void ArtnetnodeWifi::tickDMX(uint32_t time)
{
  msSinceDMXSend += time;
  if(msSinceDMXSend > DMX_MS_BETWEEN_TICKS){
    sendDMX();
    msSinceDMXSend = 0;
  }
}
