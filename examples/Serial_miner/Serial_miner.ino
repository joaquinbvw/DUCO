#include <DUCO.h>

DUCO duco = DUCO();

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(100);
  while(!Serial); // For Arduino Leonardo or any board with the ATmega32U4
  Serial.flush();
  duco.get_DUCOID();
}

void loop()
{
  if(duco.recv_job(Serial)) {
    duco.handle(Serial);
  }
}
