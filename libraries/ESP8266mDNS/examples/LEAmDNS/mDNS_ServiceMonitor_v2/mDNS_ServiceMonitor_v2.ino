/*
  ESP8266 mDNS Responder Service Monitor

  This example demonstrates two features of the LEA MDNSResponder:
  1. The host and service domain negotiation process that ensures
     the uniqueness of the finally choosen host and service domain name.
  2. The dynamic MDNS service lookup/query feature.

  A list of 'HTTP' services in the local network is created and kept up to date.
  In addition to this, a (very simple) HTTP server is set up on port 80
  and announced as a service.

  The ESP itself is initially announced to clients as 'esp8266.local', if this host domain
  is already used in the local network, another host domain is negociated. Keep an
  eye to the serial output to learn the final host domain for the HTTP service.
  The service itself is is announced as 'host domain'._http._tcp.local.
  The HTTP server delivers a short greeting and the current  list of other 'HTTP' services (not updated).
  The web server code is taken nearly 1:1 from the 'mDNS_Web_Server.ino' example.
  Point your browser to 'host domain'.local to see this web page.

  Instructions:
  - Update WiFi SSID and password as necessary.
  - Flash the sketch to the ESP8266 board
  - Install host software:
    - For Linux, install Avahi (http://avahi.org/).
    - For Windows, install Bonjour (http://www.apple.com/support/bonjour/).
    - For Mac OSX and iOS support is built in through Bonjour already.
  - Use a browser like 'Safari' to see the page at http://'host domain'.local.

*/

#ifndef STASSID
#define STASSID "ssid"
#define STAPSK "psk"
#endif

#ifndef APSSID
#define APSSID "esp8266"
//#define APPSK "psk"
#endif

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/*
   Include the MDNSResponder (the library needs to be included also)
   As LEA MDNSResponder is experimantal in the ESP8266 environment currently, the
   legacy MDNSResponder is defaulted in th include file.
   There are two ways to access LEA MDNSResponder:
   1. Prepend every declaration and call to global declarations or functions with the namespace, like:
      'LEAmDNS:MDNSResponder::hMDNSService  hMDNSService;'
      This way is used in the example. But be careful, if the namespace declaration is missing
      somewhere, the call might go to the legacy implementation...
   2. Open 'ESP8266mDNS.h' and set LEAmDNS to default.

*/
#define MDNS2_EXPERIMENTAL
#include <ESP8266mDNS.h>

/*
   Global defines and vars
*/

#define SERVICE_PORT                           80                                  // HTTP port

char*                                          pcHostDomain            = 0;        // Negociated host domain
bool                                           bHostDomainConfirmed    = false;    // Flags the confirmation of the host domain
MDNSResponder::clsService*                     hMDNSService            = 0;        // The handle of the http service in the MDNS responder
MDNSResponder::clsQuery*                       hMDNSServiceQuery       = 0;        // The handle of the 'http.tcp' service query in the MDNS responder

const String                                   cstrNoHTTPServices      = "Currently no 'http.tcp' services in the local network!<br/>";
String                                         strHTTPServices         = cstrNoHTTPServices;

// HTTP server at port 'SERVICE_PORT' will respond to HTTP requests
ESP8266WebServer                               server(SERVICE_PORT);


/*
   setStationHostname
*/
bool setStationHostname(const char* p_pcHostname) {

  if (p_pcHostname) {
    WiFi.hostname(p_pcHostname);
    Serial.printf("setStationHostname: Station hostname is set to '%s'\n", p_pcHostname);
    return true;
  }
  return false;
}


void MDNSServiceQueryCallback(const MDNSResponder::clsQuery& p_Query,
                              const MDNSResponder::clsQuery::clsAnswer& p_Answer,
                              MDNSResponder::clsQuery::clsAnswer::typeQueryAnswerType p_QueryAnswerTypeFlags,
                              bool p_bSetContent) {
  (void)p_Query;

  String answerInfo;
  switch (p_QueryAnswerTypeFlags) {
    case static_cast<MDNSResponder::clsQuery::clsAnswer::typeQueryAnswerType>(MDNSResponder::clsQuery::clsAnswer::enuQueryAnswerType::ServiceDomain):
      answerInfo = "ServiceDomain " + String(p_Answer.m_ServiceDomain.c_str());
      break;

    case static_cast<MDNSResponder::clsQuery::clsAnswer::typeQueryAnswerType>(MDNSResponder::clsQuery::clsAnswer::enuQueryAnswerType::HostDomainPort):
      answerInfo = "HostDomainAndPort " + String(p_Answer.m_HostDomain.c_str()) + ":" + String(p_Answer.m_u16Port);
      break;
    case static_cast<MDNSResponder::clsQuery::clsAnswer::typeQueryAnswerType>(MDNSResponder::clsQuery::clsAnswer::enuQueryAnswerType::IPv4Address):
      answerInfo = "IP4Address ";
      for (auto ip : p_Answer.m_IPv4Addresses) {
        answerInfo += "- " + ip->m_IPAddress.toString();
      };
      break;
    case static_cast<MDNSResponder::clsQuery::clsAnswer::typeQueryAnswerType>(MDNSResponder::clsQuery::clsAnswer::enuQueryAnswerType::Txts):
      answerInfo = "TXT ";
      for (auto kv : p_Answer.m_Txts.m_Txts) {
        answerInfo += "\nkv : " + String(kv->m_pcKey) + " : " + String(kv->m_pcValue);
      }
      break;
    default :
      answerInfo = "Unknown Answertype " + String(p_QueryAnswerTypeFlags);

  }
  Serial.printf("Answer %s %s\n", answerInfo.c_str(), p_bSetContent ? "Modified" : "Deleted");
}

/*
   MDNSServiceProbeResultCallback
   Probe result callback for Services
*/

void serviceProbeResult(const char* p_pcInstanceName,
                        bool p_bProbeResult) {
  Serial.printf("serviceProbeResult: Service %s probe %s\n", p_pcInstanceName, (p_bProbeResult ? "succeeded." : "failed!"));
}

void serviceAddService(MDNSResponder::clsService& p_rMDNSService) {
  Serial.printf("user callback: serviceAddService called, addServiceTxt() called\n");

  // Add some '_http._tcp' protocol specific MDNS service TXT items
  // See: http://www.dns-sd.org/txtrecords.html#http
  p_rMDNSService.addServiceTxt("user", "");
  p_rMDNSService.addServiceTxt("password", "");
  p_rMDNSService.addServiceTxt("path", "/");
}

/*
   MDNSHostAddServicesCallback

   Callback in which "unattended?" services can be added
   This callback can be called several times.

   Obtained service pointer inside this callback from previous calls
   will have been onvalidated prior to subsequent calls.
*/

void hostAddServices(clsLEAMDNSHost& p_rMDNSHost) {

  // Add a 'http.tcp' service to port 'SERVICE_PORT', using the host domain as instance domain
  hMDNSService = MDNS.addService(0, "http", "tcp", SERVICE_PORT, serviceProbeResult, serviceAddService);

  if (hMDNSService)  {
    Serial.printf("user callback: service http.tcp successfully added, subsequent callback will add serviceTxt\n");
  } else {
    Serial.printf("user callback: service http.tcp: adding FAILED!\n");
  }

  // Install dynamic 'http.tcp' service query
  hMDNSServiceQuery = MDNS.installServiceQuery("http", "tcp", MDNSServiceQueryCallback);
  if (hMDNSServiceQuery) {
    Serial.printf("MDNSProbeResultCallback: Service query for 'http.tcp' services installed.\n");
  } else {
    Serial.printf("MDNSProbeResultCallback: FAILED to install service query for 'http.tcp' services!\n");
  }
}

/*
   MDNSHostProbeResultCallback

   Probe result callback for the host domain.
   If the domain is free, the host domain is set and the http service is
   added.
   If the domain is already used, a new name is created and the probing is
   restarted via p_pMDNSResponder->setHostname().

*/

void hostProbeResult(const char* p_pcDomainName, bool p_bProbeResult) {

  Serial.printf("MDNSHostProbeResultCallback: Host domain '%s.local' is %s\n", p_pcDomainName, (p_bProbeResult ? "free" : "already USED!"));

  if (true == p_bProbeResult) {
    // Set station hostname
    setStationHostname(pcHostDomain);

    if (!bHostDomainConfirmed) {
      // Hostname free -> setup clock service
      bHostDomainConfirmed = true;

    }
  } else {
    // Change hostname, use '-' as divider between base name and index
    const char* tryName = MDNSResponder::indexDomainName(p_pcDomainName, "-", 0);
    Serial.printf("mDNSHost::ProbeResultCallback: try with hostname '%s'\n", tryName);
    MDNS.setHostName(tryName);
  }
}

/*
   HTTP request function (not found is handled by server)
*/
void handleHTTPRequest() {
  Serial.println("");
  Serial.println("HTTP Request");

  IPAddress ip = server.client().localIP();
  String ipStr = ip.toString();
  String s = "<!DOCTYPE HTML>\r\n<html><h3><head>Hello from ";
  s += WiFi.hostname() + ".local at " + server.client().localIP().toString() + "</h3></head>";
  s += "<br/><h4>Local HTTP services are :</h4>";
  s += "<ol>";
  /*
    for (auto info :  MDNS.answerInfo(hMDNSServiceQuery)) {
    s += "<li>";
    s += info.serviceDomain();
    if (info.hostDomainAvailable()) {
      s += "<br/>Hostname: ";
      s += String(info.hostDomain());
      s += (info.hostPortAvailable()) ? (":" + String(info.hostPort())) : "";
    }
    if (info.IP4AddressAvailable()) {
      s += "<br/>IP4:";
      for (auto ip : info.IP4Adresses()) {
        s += " " + ip.toString();
      }
    }
    if (info.txtAvailable()) {
      s += "<br/>TXT:<br/>";
      for (auto kv : info.keyValues()) {
        s += "\t" + String(kv.first) + " : " + String(kv.second) + "<br/>";
      }
    }
    s += "</li>";

    }
  */
  s += "</ol><br/>";

  Serial.println("Sending 200");
  server.send(200, "text/html", s);
  Serial.println("Done with request");
}

/*
   setup
*/
void setup(void) {
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  // Connect to WiFi network
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(APSSID);
  WiFi.begin(STASSID, STAPSK);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(STASSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup HTTP server
  server.on("/", handleHTTPRequest);

  if (MDNS.begin("mDNSv2-monitor", hostProbeResult, hostAddServices)) {
    Serial.println("mDNSv-monitor started");
  } else {
    Serial.println("FAILED to start mDNSv2-monitor");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("MDNS responder started");

  // Start HTTP server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  // Check if a request has come in
  server.handleClient();
  // Allow MDNS processing
  MDNS.update();

  static bool AP_started = false;
  if (!AP_started && millis() > START_AP_AFTER_MS) {
    AP_started = true;
    Serial.printf("Starting AP...\n");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(APSSID, APPSK);
    Serial.printf("AP started...(%s:%s, %s)\n",
                  WiFi.softAPSSID().c_str(),
                  WiFi.softAPPSK().c_str(),
                  WiFi.softAPIP().toString().c_str());
  }
}
