#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <errno.h>
#include <WS2tcpip.h>
#include <string.h>
#include <vector>
#include <map>

#include "Session.h"
#include "ErrCommon.h"
#include "RingBuffer.h"
#include "Packet.h"
#include "protocol.h"

using namespace std;