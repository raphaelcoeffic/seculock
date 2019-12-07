#ifndef _debug_h_
#define _debug_h_

extern bool debugTrace;

#define debugWrite(...) do { if (debugTrace) Serial.write(__VA_ARGS__); } while(0)
#define debugPrint(...) do { if (debugTrace) Serial.print(__VA_ARGS__); } while(0)
#define debugPrintln(...) do { if (debugTrace) Serial.println(__VA_ARGS__); } while(0)

#endif
