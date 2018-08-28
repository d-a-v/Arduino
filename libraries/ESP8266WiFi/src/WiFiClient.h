/*
  WiFiClient.h - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified by Ivan Grokhotkov, December 2014 - esp8266 support
*/

#ifndef wificlient_h
#define wificlient_h
#include <memory>
#include "Arduino.h"
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"
#include "include/slist.h"

#ifndef TCP_MSS
#define TCP_MSS 1460 // lwip1.4
#endif

#define WIFICLIENT_MAX_PACKET_SIZE TCP_MSS
#define WIFICLIENT_MAX_FLUSH_WAIT_MS 100

#define TCP_DEFAULT_KEEPALIVE_IDLE_SEC          7200 // 2 hours
#define TCP_DEFAULT_KEEPALIVE_INTERVAL_SEC      75   // 75 sec
#define TCP_DEFAULT_KEEPALIVE_COUNT             9    // fault after 9 failures

class ClientContext;
class WiFiServer;

class WiFiClient : public Client, public SList<WiFiClient> {
protected:
  WiFiClient(ClientContext* client);

public:
  WiFiClient();
  virtual ~WiFiClient();
  WiFiClient(const WiFiClient&);
  WiFiClient& operator=(const WiFiClient&);

  uint8_t status();
  virtual int connect(IPAddress ip, uint16_t port);
  virtual int connect(const char *host, uint16_t port);
  virtual int connect(const String host, uint16_t port);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual size_t write_P(PGM_P buf, size_t size);
  size_t write(Stream& stream);

  // This one is deprecated, use write(Stream& instead)
  size_t write(Stream& stream, size_t unitSize) __attribute__ ((deprecated));

  virtual int available();
  virtual int read();
  virtual int read(uint8_t *buf, size_t size);
  virtual int peek();
  virtual size_t peekBytes(uint8_t *buffer, size_t length);
  size_t peekBytes(char *buffer, size_t length) {
    return peekBytes((uint8_t *) buffer, length);
  }
  virtual void flush();
  virtual void stop();
  virtual uint8_t connected();
  virtual operator bool();

  IPAddress remoteIP();
  uint16_t  remotePort();
  IPAddress localIP();
  uint16_t  localPort();
  bool getNoDelay() const;
  void setNoDelay(bool nodelay);
  bool getSync() const;
  void setSync(bool sync);

  static void setLocalPortStart(uint16_t port) { _localPort = port; }

  size_t availableForWrite();

  friend class WiFiServer;

  using Print::write;

  static void stopAll();
  static void stopAllExcept(WiFiClient * c);

  void     keepAlive (uint16_t idle_sec = TCP_DEFAULT_KEEPALIVE_IDLE_SEC, uint16_t intv_sec = TCP_DEFAULT_KEEPALIVE_INTERVAL_SEC, uint8_t count = TCP_DEFAULT_KEEPALIVE_COUNT);
  bool     isKeepAliveEnabled () const;
  uint16_t getKeepAliveIdle () const;
  uint16_t getKeepAliveInterval () const;
  uint8_t  getKeepAliveCount () const;
  void     disableKeepAlive () { keepAlive(0, 0, 0); }

  static void setDefaultNoDelay (bool noDelay) { _defaultNoDelay = noDelay; }
  static void setDefaultSync    (bool sync)    { _defaultSync = sync; }
  static bool getDefaultNoDelay ()             { return _defaultNoDelay; }
  static bool getDefaultSync    ()             { return _defaultSync; }

protected:

  static int8_t _s_connected(void* arg, void* tpcb, int8_t err);
  static void _s_err(void* arg, int8_t err);

  int8_t _connected(void* tpcb, int8_t err);
  void _err(int8_t err);

  ClientContext* _client;
  static uint16_t _localPort;

  static bool _defaultNoDelay;
  static bool _defaultSync;
};

#endif
