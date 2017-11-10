/*
  ESP8266WebServerSecure.cpp - Dead simple HTTPS web-server.
  Supports only one simultaneous client, knows how to handle GET and POST.

  Copyright (c) 2017 Earle F. Philhower, III. All rights reserved.

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
  Modified 8 May 2015 by Hristo Gochkov (proper post and file upload handling)
*/


#include <Arduino.h>
#include <libb64/cencode.h>
#include "WiFiServer.h"
#include "WiFiClient.h"
#include "ESP8266WebServerSecure.h"


ESP8266WebServerSecure::ESP8266WebServerSecure(IPAddress addr, int port) : _serverSecure(addr, port)
{
}

ESP8266WebServerSecure::ESP8266WebServerSecure(int port) : _serverSecure(port)
{
}

void ESP8266WebServerSecure::setServerKeyAndCert_P(const uint8_t *key, int keyLen, const uint8_t *cert, int certLen)
{
    _serverSecure.setServerKeyAndCert_P(key, keyLen, cert, certLen);
}

void ESP8266WebServerSecure::setServerKeyAndCert(const uint8_t *key, int keyLen, const uint8_t *cert, int certLen)
{
    _serverSecure.setServerKeyAndCert(key, keyLen, cert, certLen);
}

ESP8266WebServerSecure::~ESP8266WebServerSecure() {
  if (_currentHeaders)
    delete[]_currentHeaders;
  _headerKeysCount = 0;
  RequestHandler* handler = _firstHandler;
  while (handler) {
    RequestHandler* next = handler->next();
    delete handler;
    handler = next;
  }
  close();
}

// We need to basically cut-n-paste these from WebServer because of the problem
// of object slicing. The class uses assignment operators like "WiFiClient x=y;"
// When this happens, even if "y" is a WiFiClientSecure, the main class is 
// already compiled down into code which will only copy the WiFiClient superclass
// and not the extra bits for our own class (since when it was compiled it needed
// to know the size of memory to allocate on the stack for this local variable
// there's not realy anything else it could do).

void ESP8266WebServerSecure::begin() {
  _currentStatus = HC_NONE;
  _serverSecure.begin();
  if(!_headerKeysCount)
    collectHeaders(0, 0);
}

void ESP8266WebServerSecure::handleClient() {
  if (_currentStatus == HC_NONE) {
    _currentClientSecure = _serverSecure.available();
    if (!_currentClientSecure) {
      return;
    }

#ifdef DEBUG_ESP_HTTP_SERVER
    DEBUG_OUTPUT.println("New client secure");
#endif

    _currentStatus = HC_WAIT_READ;
    _statusChange = millis();
  }

  if (!_currentClientSecure.connected()) {
    _currentClientSecure.stop();
    _currentClientSecure = WiFiClientSecure();
    _currentStatus = HC_NONE;
    return;
  }

  // Wait for data from client to become available
  if (_currentStatus == HC_WAIT_READ) {
    if (!_currentClientSecure.available()) {
      if (millis() - _statusChange > HTTP_MAX_DATA_WAIT) {
        _currentClientSecure.stop();
        _currentClientSecure = WiFiClientSecure();
        _currentStatus = HC_NONE;
      }
      yield();
      return;
    }
    if (!_parseRequest(_currentClientSecure)) {
      _currentClientSecure.stop();
      _currentClientSecure = WiFiClientSecure();
      _currentStatus = HC_NONE;
      return;
    }
    _contentLength = CONTENT_LENGTH_NOT_SET;
    _handleRequest();

    if (!_currentClientSecure.connected()) {
      _currentClientSecure.stop();
      _currentClientSecure = WiFiClientSecure();
      _currentStatus = HC_NONE;
      return;
    } else {
      _currentStatus = HC_WAIT_CLOSE;
      _statusChange = millis();
      return;
    }
  }

  if (_currentStatus == HC_WAIT_CLOSE) {
    if (millis() - _statusChange > HTTP_MAX_CLOSE_WAIT) {
      _currentClientSecure.stop();
      _currentClientSecure = WiFiClientSecure();
      _currentStatus = HC_NONE;
    } else {
      yield();
      return;
    }
  }
}

void ESP8266WebServerSecure::close() {
  _currentClientSecure.stop();
  _serverSecure.close();
}

