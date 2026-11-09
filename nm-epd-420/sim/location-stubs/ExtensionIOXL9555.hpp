#pragma once
struct ExtensionIOXL9555 {
 int levels[16]={};bool failWrite=false;
 void pinMode(int,int){}
 void digitalWrite(int pin,int level){if(!failWrite)levels[pin]=level;}
 int digitalRead(int pin){return levels[pin];}
};
