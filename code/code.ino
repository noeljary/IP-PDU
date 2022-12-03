#include <EEPROM.h>
#include <EtherCard.h>

// ethernet interface mac address
static byte mymac[] = {0x74, 0x69, 0x69, 0x2D, 0x30, 0x31};

byte Ethernet::buffer[500];

byte relay_loc    = 0;
byte relay_status = 0x00;
byte relay_pins[] = {A5, A4, A3, A2, A1, A0, 4, 3};

void setup () {
  // Set Relay Pins as OUTPUTS
  for(byte i = 0; i < sizeof(relay_pins); i++) {
    pinMode(relay_pins[i], OUTPUT);
  }

  relay_status = getDefStates();
  setRelayStates(relay_status);

  Serial.begin(57600);

  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if(ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0) {
    Serial.println("Failed to access Ethernet controller");
  }

  if(!ether.dhcpSetup()) {
    Serial.println("DHCP failed");
  }

  ether.printIp("My IP: ", ether.myip);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
}

void loop () {
  word pos = ether.packetLoop(ether.packetReceive());
  // check if valid tcp data is received
  if(pos) {
    char* data = (char *) Ethernet::buffer + pos;
    if(strncmp("GET /", data, 5) == 0) {
      ether.httpServerReplyAck();

      byte ctr = 0;
      char* tok = strtok(data, " ");
      while(tok != NULL) {
        if(ctr == 1) {
          processUpdate(tok);
          break;
        }
        tok = strtok(NULL, " ");
        ctr++;
      }

      ether.httpServerReply_with_flags(0, TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
    }
  }
  

}

byte processUpdate(char* command) {
  byte relay = 0;
  byte state = 0;
  byte ctr   = 0;

  char* op = strtok(command, "/");
  while(op != NULL) {
    ctr++;

    if(ctr == 1) {
      relay = atoi(op);
    } else if(ctr == 2) {
      state = atoi(op);
    } else if(ctr == 3) {
      break;
    }

    op = strtok(NULL, "/");
  }

  if(ctr == 2) {
    if(relay > 0 && relay <= sizeof(relay_pins)) {
      setRelayState(relay, state);
      return getRelayStates();
    }
  } else if(ctr == 3) {
    if(strcmp("toggle", op) == 0) {
      toggleRelay(relay, state);
      return getRelayStates();
    } else if(strcmp("default", op) == 0) {
      setDefaultState(relay, state);
      return getDefStates();
    }
  }
}

void setDefaultState(byte relay, byte state) {
  byte def_status = getDefStates();
  def_status = def_status & ~(1 << relay - 1) | (state << relay - 1);
  EEPROM.update(relay_loc, def_status);
}

void setRelayState(byte relay, byte state) {
  // Toggle Relay Bit to New State
  relay_status = relay_status & ~(1 << relay - 1) | (state << relay - 1);
  setRelayStates(relay_status);
}

void setRelayStates(byte states) {
  // Loop Through and Set Relay Pin States
  for(byte i = 0; i < sizeof(relay_pins); i++) {
    digitalWrite(relay_pins[i], (states >> i) & 0x01);
  }
}

void toggleRelay(byte relay, byte wait) {
  if(getRelayState(relay)) {
    setRelayState(relay, 0);
    delay(wait * 1000);
    setRelayState(relay, 1);
  }
}

byte getDefState(byte relay) {
  return getDefStates() >> --relay & 0x01;
}

byte getDefStates() {
  byte def_status;
  EEPROM.get(relay_loc, def_status);
  return def_status;
}

byte getRelayState(byte relay) {
  return getRelayStates() >> --relay & 0x01;
}

byte getRelayStates() {
  return relay_status;
}
