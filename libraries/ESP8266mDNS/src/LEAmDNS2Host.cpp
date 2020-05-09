/*
    LEAmDNS2Host.cpp

    License (MIT license):
      Permission is hereby granted, free of charge, to any person obtaining a copy
      of this software and associated documentation files (the "Software"), to deal
      in the Software without restriction, including without limitation the rights
      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
      copies of the Software, and to permit persons to whom the Software is
      furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included in
      all copies or substantial portions of the Software.

      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
      THE SOFTWARE.

*/

#include "ESP8266mDNS.h"
#include "LEAmDNS2Host.h"

#ifdef MDNS_IPV4_SUPPORT
#include <lwip/igmp.h>
#endif
#ifdef MDNS_IPV6_SUPPORT
#include <lwip/mld6.h>
#endif


namespace   // anonymous
{

/*
    strrstr (static)

    Backwards search for p_pcPattern in p_pcString
    Based on: https://stackoverflow.com/a/1634398/2778898

*/
const char* strrstr(const char*__restrict p_pcString,
                    const char*__restrict p_pcPattern)
{
    const char* pcResult = 0;

    size_t      stStringLength = (p_pcString ? strlen(p_pcString) : 0);
    size_t      stPatternLength = (p_pcPattern ? strlen(p_pcPattern) : 0);

    if ((stStringLength) &&
            (stPatternLength) &&
            (stPatternLength <= stStringLength))
    {
        // Pattern is shorter or has the same length than the string
        for (const char* s = (p_pcString + stStringLength - stPatternLength); s >= p_pcString; --s)
        {
            if (0 == strncmp(s, p_pcPattern, stPatternLength))
            {
                pcResult = s;
                break;
            }
        }
    }
    return pcResult;
}


} // anonymous


namespace esp8266
{


namespace experimental
{


/*

    HELPERS

*/

/*
    clsLEAmDNS2_Host::indexDomainName (static)

    Increments the given domain 'p_pcDomainName' by appending a delimiter and an index number.

    If the given domain name already has a numeric index (after the given delimiter), this index
    is incremented. If not, the delimiter and index '2' is added.

    If 'p_pcDomainName' is empty (==0), the given default name 'p_pcDefaultDomainName' is used,
    if no default is given, 'ESP8266' is used.

*/
//static
const char* clsLEAMDNSHost::indexDomainName(const char* p_pcDomainName,
        const char* p_pcDivider /*= "-"*/,
        const char* p_pcDefaultDomainName /*= 0*/)
{
    static char acResultDomainName[clsConsts::stDomainLabelMaxLength];
    *acResultDomainName = 0;

    // Ensure a divider exists; use '-' as default
    const char* pcDivider = (p_pcDivider ? : "-");

    if (p_pcDomainName)
    {
        // Given domain
        const char* pFoundDivider = strrstr(p_pcDomainName, pcDivider);
        if (pFoundDivider)          // maybe already extended
        {
            char*         pEnd = 0;
            unsigned long ulIndex = strtoul((pFoundDivider + strlen(pcDivider)), &pEnd, 10);
            if ((ulIndex) &&
                    ((pEnd - p_pcDomainName) == (ptrdiff_t)strlen(p_pcDomainName)) &&
                    (!*pEnd))
            {
                // Valid (old) index found
                char    acIndexBuffer[16];
                sprintf(acIndexBuffer, "%lu", (++ulIndex));
                //size_t  stLength = ((pFoundDivider - p_pcDomainName + strlen(pcDivider)) + strlen(acIndexBuffer) + 1);

                memcpy(acResultDomainName, p_pcDomainName, (pFoundDivider - p_pcDomainName + strlen(pcDivider)));
                acResultDomainName[pFoundDivider - p_pcDomainName + strlen(pcDivider)] = 0;
                strcat(acResultDomainName, acIndexBuffer);
            }
            else
            {
                pFoundDivider = 0;  // Flag the need to (base) extend the hostname
            }
        }

        if (!pFoundDivider)
        {
            // not yet extended (or failed to increment extension) -> start indexing
            //size_t    stLength = strlen(p_pcDomainName) + (strlen(pcDivider) + 1 + 1);   // Name + Divider + '2' + '\0'
            sprintf(acResultDomainName, "%s%s2", p_pcDomainName, pcDivider);
        }
    }
    else
    {
        // No given domain, use base or default
        const char* cpcDefaultName = (p_pcDefaultDomainName ? : "ESP8266");
        size_t      stLength = strlen(cpcDefaultName) + 1;   // '\0'
        strncpy(acResultDomainName, cpcDefaultName, stLength);
    }
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("[mDNS] indexDomainName: From '%s' to '%s'\n"), (p_pcDomainName ? : ""), acResultDomainName););
    return acResultDomainName;
}


/*
    clsLEAmDNS2_Host::setStationHostName (static)

    Sets the staion hostname

*/
// static
bool clsLEAMDNSHost::setNetIfHostName(netif* p_pNetIf,
                                      const char* p_pcHostName)
{
    if ((p_pNetIf) &&
            (p_pcHostName))
    {
        netif_set_hostname(p_pNetIf, (char*)p_pcHostName);  // LWIP 1.x forces 'char*' instead of 'const char*'
        DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("[mDNS] setNetIfHostName host name: %s!\n"), p_pcHostName););
    }
    return true;
}


/**
    clsLEAmDNS2_Host::sm_pBackbone

*/
clsLEAMDNSHost::clsBackbone* clsLEAMDNSHost::clsBackbone::sm_pBackbone  = 0;

/**
    Consts::...

*/
const char* clsLEAMDNSHost::clsConsts::pcLocal                        = "local";
const char* clsLEAMDNSHost::clsConsts::pcServices                     = "services";
const char* clsLEAMDNSHost::clsConsts::pcDNSSD                        = "dns-sd";
const char* clsLEAMDNSHost::clsConsts::pcUDP                          = "udp";
//const char* clsLEAMDNSHost::clsConsts::pcTCP                          = "tcp";

#ifdef MDNS_IPV4_SUPPORT
const char* clsLEAMDNSHost::clsConsts::pcReverseIPv4Domain            = "in-addr";
#endif
#ifdef MDNS_IPV6_SUPPORT
const char* clsLEAMDNSHost::clsConsts::pcReverseIPv6Domain            = "ip6";
#endif
const char* clsLEAMDNSHost::clsConsts::pcReverseTopDomain             = "arpa";


/*
    clsLEAmDNS2_Host::clsLEAmDNS2_Host constructor

*/
clsLEAMDNSHost::clsLEAMDNSHost(void)
    :   m_pNetIf(0),
        m_NetIfState(static_cast<typeNetIfState>(enuNetIfState::None)),
        m_pUDPContext(0),
        m_pcHostName(0),
        m_pcDefaultInstanceName(0),
        m_ProbeInformation()
{
}

/*
    clsLEAmDNS2_Host::~clsLEAmDNS2_Host destructor

*/
clsLEAMDNSHost::~clsLEAMDNSHost(void)
{
    close();
}

/*

    INIT

*/

/*
    clsLEAmDNS2_Host::begin (hostname, netif, probe_callback)

    Creates a new mDNS host (adding the netif to the multicast groups),
    sets up the instance data (hostname, ...) and starts the probing process

*/
bool clsLEAMDNSHost::begin(const char* p_pcHostName,
                           netif* p_pNetIf,
                           clsLEAMDNSHost::fnProbeResultCallback p_fnCallback /*= 0*/)
{
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s begin(%s, netif: %u)\n"), _DH(), (p_pcHostName ? : "_"), (p_pNetIf ? netif_get_index(p_pNetIf) : 0)););

    bool    bResult = false;

    if (!((bResult = ((setHostName(p_pcHostName)) &&
                      //can be nullptr! ((m_pNetIf = p_pNetIf)) &&
                      (_joinMulticastGroups()) &&
                      (p_fnCallback ? setProbeResultCallback(p_fnCallback) : true) &&
                      ((m_pUDPContext = _allocBackbone())) &&
                      (restart())))))
    {
        DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s begin: FAILED for '%s'!\n"), _DH(), (p_pcHostName ? : "-")););
    }
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s begin: %s to init with hostname %s!\n"), _DH(), (bResult ? "Succeeded" : "FAILED"), (p_pcHostName ? : "-")););
    return bResult;
}

/*
    clsLEAmDNS2_Host::begin (hostname, probe_callback)

*/
bool clsLEAMDNSHost::begin(const char* p_pcHostName,
                           clsLEAMDNSHost::fnProbeResultCallback p_fnCallback /*= 0*/)
{
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s begin(%s)\n"), _DH(), (p_pcHostName ? : "_")););

    return begin(p_pcHostName, (WiFiMode_t)wifi_get_opmode(), p_fnCallback);
}

/*
    clsLEAmDNS2_Host::begin (hostname, WiFiMode, probe_callback)

*/
bool clsLEAMDNSHost::begin(const char* p_pcHostName,
                           WiFiMode_t p_WiFiMode,
                           clsLEAMDNSHost::fnProbeResultCallback p_fnCallback /*= 0*/)
{
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s begin(%s, opmode: %u)\n"), _DH(), (p_pcHostName ? : "_"), (uint32_t)p_WiFiMode););

    bool bResult = false;

    if (p_WiFiMode == WIFI_STA || p_WiFiMode == WIFI_AP)
    {
        bResult = begin(p_pcHostName, netif_get_by_index(p_WiFiMode), p_fnCallback);
    }
    else
    {
        DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s begin: FAILED for WiFi mode '%u'! Only 'WIFI_STA' or 'WIFI_AP' is allowed (HostName: %s)!\n"), _DH(), (bResult ? "Succeeded" : "FAILED"), p_WiFiMode, (p_pcHostName ? : "-")););
    }
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s begin: %s for WiFi mode '%u' (HostName: %s)!\n"), _DH(), (bResult ? "Succeeded" : "FAILED"), p_WiFiMode, (p_pcHostName ? : "-")););
    return bResult;
}

/*
    clsLEAmDNS2_Host::close

*/
bool clsLEAMDNSHost::close(void)
{
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s close\n"), _DH()););

    m_pUDPContext = 0;
    return ((_leaveMulticastGroups()) &&
            (_releaseBackbone()));
}


/*

    HOSTNAME

*/

/*
    clsLEAmDNS2_Host::setHostName

*/
bool clsLEAMDNSHost::setHostName(const char* p_pcHostName)
{
    bool    bResult;
    if ((bResult = _allocHostName(p_pcHostName)))
    {
        m_ProbeInformation.clear(false);
        m_ProbeInformation.m_ProbingStatus = clsProbeInformation_Base::enuProbingStatus::ReadyToStart;

        // Replace 'auto-set' service names
        for (clsService* pService : m_Services)
        {
            if ((pService->m_bAutoName) &&
                    (!m_pcDefaultInstanceName))
            {
                if (!((bResult = pService->setInstanceName(p_pcHostName))))
                {
                    break;
                }
            }
        }
    }
    return bResult;
}

/*
    clsLEAmDNS2_Host::indexHostName

*/
bool clsLEAMDNSHost::indexHostName(void)
{
    return setHostName(clsLEAMDNSHost::indexDomainName(hostName(), "-", 0));
}

/*
    clsLEAmDNS2_Host::hostName

*/
const char* clsLEAMDNSHost::hostName(void) const
{
    return m_pcHostName;
}

/*
    clsLEAmDNS2_Host::setProbeResultCallback

*/
bool clsLEAMDNSHost::setProbeResultCallback(clsLEAMDNSHost::fnProbeResultCallback p_fnCallback)
{
    m_ProbeInformation.m_fnProbeResultCallback = p_fnCallback;
    return true;
}

/*
    clsLEAmDNS2_Host::probeStatus

*/
bool clsLEAMDNSHost::probeStatus(void) const
{
    return (clsProbeInformation_Base::enuProbingStatus::DoneFinally == m_ProbeInformation.m_ProbingStatus);
}


/*

    SERVICES

*/

/*
    clsLEAmDNS2_Host::setDefaultInstanceName

*/
bool clsLEAMDNSHost::setDefaultInstanceName(const char* p_pcDefaultInstanceName)
{
    bool    bResult;
    if ((bResult = _allocDefaultInstanceName(p_pcDefaultInstanceName)))
    {
        // Replace 'auto-set' service names
        for (clsService* pService : m_Services)
        {
            if (pService->m_bAutoName)
            {
                if (!((bResult = pService->setInstanceName(p_pcDefaultInstanceName))))
                {
                    break;
                }
            }
        }
    }
    return bResult;
}

/*
    clsLEAmDNS2_Host::defaultInstanceName

*/
const char* clsLEAMDNSHost::defaultInstanceName(void) const
{
    return m_pcDefaultInstanceName;
}

/*
    clsLEAmDNS2_Host::addService

*/
clsLEAMDNSHost::clsService* clsLEAMDNSHost::addService(const char* p_pcInstanceName,
        const char* p_pcType,
        const char* p_pcProtocol,
        uint16_t p_u16Port,
        clsLEAMDNSHost::clsService::fnProbeResultCallback p_fnCallback /*= 0*/)
{
    clsService* pService = 0;

    if (!((pService = findService(_instanceName(p_pcInstanceName), p_pcType, p_pcProtocol, p_u16Port))))
    {
        // Not already used
        if ((pService = new clsService))
        {
            if ((pService->setInstanceName(_instanceName(p_pcInstanceName))) &&
                    (pService->setType(p_pcType)) &&
                    (pService->setProtocol(p_pcProtocol)) &&
                    (pService->setPort(p_u16Port)) &&
                    (p_fnCallback ? pService->setProbeResultCallback(p_fnCallback) : true))
            {
                m_Services.push_back(pService);
            }
            else
            {
                delete pService;
                pService = 0;
            }
        }
    }
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s addService: %s to add service '%s.%s.%s.local'!\n"), _DH(pService), (pService ? "Succeeded" : "FAILED"), _instanceName(p_pcInstanceName, false), (p_pcType ? : ""), (p_pcProtocol ? : "")););
    DEBUG_EX_ERR(if (!pService) DEBUG_OUTPUT.printf_P(PSTR("%s addService: FAILED to add service '%s.%s.%s.local'!\n"), _DH(pService), _instanceName(p_pcInstanceName, false), (p_pcType ? : ""), (p_pcProtocol ? : "")););
    return pService;
}

/*
    clsLEAmDNS2_Host::removeService

*/
bool clsLEAMDNSHost::removeService(clsLEAMDNSHost::clsService* p_pService)
{
    bool    bResult = false;

    if ((bResult = ((p_pService) &&
                    (m_Services.end() != std::find(m_Services.begin(), m_Services.end(), p_pService)) &&
                    (_announceService(*p_pService, false)))))
    {
        m_Services.remove(p_pService);
        delete p_pService;
    }
    DEBUG_EX_ERR(if (!bResult) DEBUG_OUTPUT.printf_P(PSTR("%s _removeService: FAILED!\n"), _DH(p_pService)););
    return bResult;
}

/*
    clsLEAmDNS2_Host::findService (const)

*/
const clsLEAMDNSHost::clsService* clsLEAMDNSHost::findService(const char* p_pcInstanceName,
        const char* p_pcType,
        const char* p_pcProtocol,
        uint16_t p_u16Port/*= (uint16_t)(-1)*/) const
{
    clsService* pFoundService = 0;

    for (clsService* pService : m_Services)
    {
        if ((0 == strcmp(pService->instanceName(), _instanceName(p_pcInstanceName))) &&
                (0 == strcmp(pService->type(), p_pcType)) &&
                (0 == strcmp(pService->protocol(), p_pcProtocol)) &&
                (((uint16_t)(-1) == p_u16Port) ||
                 (pService->port() == p_u16Port)))
        {
            pFoundService = pService;
            break;
        }
    }
    return pFoundService;
}

/*
    clsLEAmDNS2_Host::findService

*/
clsLEAMDNSHost::clsService* clsLEAMDNSHost::findService(const char* p_pcInstanceName,
        const char* p_pcType,
        const char* p_pcProtocol,
        uint16_t p_u16Port /*= (uint16_t)(-1)*/)
{
    return (clsService*)((const clsLEAMDNSHost*)this)->findService(p_pcInstanceName, p_pcType, p_pcProtocol, p_u16Port);
}

/*
	clsLEAMDNSHost::services

*/
const clsLEAMDNSHost::clsService::list& clsLEAMDNSHost::services(void) const
{
    return m_Services;
}


/*

    QUERIES

*/

/*
    clsLEAmDNS2_Host::queryService

*/
clsLEAMDNSHost::clsQuery::clsAnswerAccessor::vector clsLEAMDNSHost::queryService(const char* p_pcService,
        const char* p_pcProtocol,
        const uint16_t p_u16Timeout)
{
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s queryService '_%s._%s.local'\n"), _DH(), p_pcService, p_pcProtocol););

    clsQuery*    pQuery = 0;
    if ((p_pcService) && (*p_pcService) &&
            (p_pcProtocol) && (*p_pcProtocol) &&
            (p_u16Timeout) &&
            ((pQuery = _allocQuery(clsQuery::enuQueryType::Service))) &&
            (_buildDomainForService(p_pcService, p_pcProtocol, pQuery->m_Domain)))
    {
        if ((_removeLegacyQuery()) &&
                ((pQuery->m_bStaticQuery = true)) &&
                (_sendQuery(*pQuery)))
        {
            // Wait for answers to arrive
            DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s queryService: Waiting %u ms for answers...\n"), _DH(), p_u16Timeout););
            delay(p_u16Timeout);

            // All answers should have arrived by now -> stop adding new answers
            pQuery->m_bAwaitingAnswers = false;
        }
        else
        {
            // FAILED to send query
            _removeQuery(pQuery);
        }
    }
    else
    {
        DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s queryService: INVALID input data!\n"), _DH()););
    }
    return ((pQuery)
            ? pQuery->answerAccessors()
            : clsQuery::clsAnswerAccessor::vector());
}

/*
    clsLEAmDNS2_Host::queryHost

*/
clsLEAMDNSHost::clsQuery::clsAnswerAccessor::vector clsLEAMDNSHost::queryHost(const char* p_pcHostName,
        const uint16_t p_u16Timeout)
{
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s queryHost '%s.local'\n"), _DH(), p_pcHostName););

    clsQuery*    pQuery = 0;
    if ((p_pcHostName) && (*p_pcHostName) &&
            (p_u16Timeout) &&
            ((pQuery = _allocQuery(clsQuery::enuQueryType::Host))) &&
            (_buildDomainForHost(p_pcHostName, pQuery->m_Domain)))
    {
        if ((_removeLegacyQuery()) &&
                ((pQuery->m_bStaticQuery = true)) &&
                (_sendQuery(*pQuery)))
        {
            // Wait for answers to arrive
            DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s queryHost: Waiting %u ms for answers...\n"), _DH(), p_u16Timeout););
            delay(p_u16Timeout);

            // All answers should have arrived by now -> stop adding new answers
            pQuery->m_bAwaitingAnswers = false;
        }
        else
        {
            // FAILED to send query
            _removeQuery(pQuery);
        }
    }
    else
    {
        DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s queryHost: INVALID input data!\n"), _DH()););
    }
    return ((pQuery)
            ? pQuery->answerAccessors()
            : clsQuery::clsAnswerAccessor::vector());
}

/*
    clsLEAmDNS2_Host::removeQuery

*/
bool clsLEAMDNSHost::removeQuery(void)
{
    return _removeLegacyQuery();
}

/*
    clsLEAmDNS2_Host::hasQuery

*/
bool clsLEAMDNSHost::hasQuery(void)
{
    return (0 != _findLegacyQuery());
}

/*
    clsLEAmDNS2_Host::getQuery

*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::getQuery(void)
{
    return _findLegacyQuery();
}

/*
    clsLEAmDNS2_Host::installServiceQuery (answer)

*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::installServiceQuery(const char* p_pcService,
        const char* p_pcProtocol,
        clsLEAMDNSHost::clsQuery::QueryCallbackAnswerFn p_fnCallbackAnswer)
{
    clsQuery*   pQuery = 0;
    if ((pQuery = _installServiceQuery(p_pcService, p_pcProtocol)))
    {
        pQuery->m_fnCallbackAnswer = p_fnCallbackAnswer;
    }
    return pQuery;
}

/*
    clsLEAmDNS2_Host::installServiceQuery (accessor)

*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::installServiceQuery(const char* p_pcService,
        const char* p_pcProtocol,
        clsLEAMDNSHost::clsQuery::QueryCallbackAccessorFn p_fnCallbackAccessor)
{
    clsQuery*   pQuery = 0;
    if ((pQuery = _installServiceQuery(p_pcService, p_pcProtocol)))
    {
        pQuery->m_fnCallbackAccessor = p_fnCallbackAccessor;
    }
    return pQuery;
}

/*
    clsLEAmDNS2_Host::installHostQuery (answer)
*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::installHostQuery(const char* p_pcHostName,
        clsLEAMDNSHost::clsQuery::QueryCallbackAnswerFn p_fnCallbackAnswer)
{
    clsQuery*   pQuery = 0;
    if ((p_pcHostName) && (*p_pcHostName))
    {
        clsRRDomain    domain;
        if ((pQuery = ((_buildDomainForHost(p_pcHostName, domain))
                       ? _installDomainQuery(domain, clsQuery::enuQueryType::Host)
                       : 0)))
        {
            pQuery->m_fnCallbackAnswer = p_fnCallbackAnswer;
        }
    }
    return pQuery;
}

/*
    clsLEAmDNS2_Host::installHostQuery (accessor)
*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::installHostQuery(const char* p_pcHostName,
        clsLEAMDNSHost::clsQuery::QueryCallbackAccessorFn p_fnCallbackAccessor)
{
    clsQuery*   pQuery = 0;
    if ((p_pcHostName) && (*p_pcHostName))
    {
        clsRRDomain    domain;
        if ((pQuery = ((_buildDomainForHost(p_pcHostName, domain))
                       ? _installDomainQuery(domain, clsQuery::enuQueryType::Host)
                       : 0)))
        {
            pQuery->m_fnCallbackAccessor = p_fnCallbackAccessor;
        }
    }
    return pQuery;
}

/*
    clsLEAmDNS2_Host::removeQuery
*/
bool clsLEAMDNSHost::removeQuery(clsLEAMDNSHost::clsQuery* p_pMDNSQuery)
{
    bool    bResult = ((p_pMDNSQuery) &&
                       (_removeQuery(p_pMDNSQuery)));
    DEBUG_EX_ERR(if (!bResult) DEBUG_OUTPUT.printf_P(PSTR("%s removeQuery: FAILED!\n"), _DH()););
    return bResult;
}


/*
    PROCESSING
*/

/*
    clsLEAmDNS2_Host::update
*/
bool clsLEAMDNSHost::update(void)
{
    bool    bResult = false;

    //if (clsBackbone::sm_pBackbone->setDelayUDPProcessing(true))
    //{
    bResult = ((_checkNetIfState()) &&      // Any changes in the netif state?
               (_updateProbeStatus()) &&    // Probing and announcing
               (_checkQueryCache()));

    //    clsBackbone::sm_pBackbone->setDelayUDPProcessing(false);
    //}
    DEBUG_EX_ERR(if (!bResult) DEBUG_OUTPUT.printf_P(PSTR("%s update: FAILED (Not connected?)!\n"), _DH()););

    return bResult;
}

/*
    clsLEAmDNS2_Host::announce
*/
bool clsLEAMDNSHost::announce(bool p_bAnnounce /*= true*/,
                              bool p_bIncludeServices /*= true*/)
{
    return _announce(p_bAnnounce, p_bIncludeServices);
}

/*
    clsLEAmDNS2_Host::announceService
*/
bool clsLEAMDNSHost::announceService(clsService* p_pService,
                                     bool p_bAnnounce /*= true*/)
{
    return _announceService(*p_pService, p_bAnnounce);
}

/*
    clsLEAmDNS2_Host::restart
*/
bool clsLEAMDNSHost::restart(void)
{
    return (_resetProbeStatus(true));   // Stop and restart probing
}



/*
    P R O T E C T E D
*/

/*

    BACKBONE

*/

/*
    clsLEAmDNS2_Host::_allocBackbone

*/
UdpContext* clsLEAMDNSHost::_allocBackbone(void)
{
    UdpContext* pUDPContext = 0;

    if (!clsBackbone::sm_pBackbone)
    {
        // Not yet created
        clsBackbone::sm_pBackbone = new clsBackbone;
        DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _allocBackbone: Created backbone.\n"), _DH()););

        if ((clsBackbone::sm_pBackbone) &&
                (!clsBackbone::sm_pBackbone->init()))
        {
            DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s _allocBackbone: FAILED to init backbone!\n"), _DH()););

            delete clsBackbone::sm_pBackbone;
            clsBackbone::sm_pBackbone = 0;
        }
    }
    if (clsBackbone::sm_pBackbone)
    {
        pUDPContext = clsBackbone::sm_pBackbone->addHost(this);
    }
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _allocBackbone: %s to add host to backbone.\n"), _DH(), (pUDPContext ? "Succeeded" : "FAILED")););
    return pUDPContext;
}

/*
    clsLEAmDNS2_Host::_releaseBackbone

*/
bool clsLEAMDNSHost::_releaseBackbone(void)
{
    bool    bResult = false;

    if ((clsBackbone::sm_pBackbone) &&
            ((bResult = clsBackbone::sm_pBackbone->removeHost(this))) &&
            (0 == clsBackbone::sm_pBackbone->hostCount()))
    {
        delete clsBackbone::sm_pBackbone;
        clsBackbone::sm_pBackbone = 0;
        DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _releaseBackbone: Released backbone."), _DH()););
    }
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _releaseBackbone: %s to remove host from backbone."), _DH(), (bResult ? "Succeeded" : "FAILED")););
    return bResult;
}


/*

    MULTICAST GROUPS

*/

/*
    clsLEAmDNS2_Host::_joinMulticastGroups
*/
bool clsLEAMDNSHost::_joinMulticastGroups(void)
{
    bool    bResult = false;

    // Join multicast group(s)
    if (m_pNetIf)
    {
        bResult = _joinMulticastGroups(m_pNetIf);
    }
    else
    {
        for (netif* p_netif = netif_list; p_netif; p_netif = p_netif->next)
        {
            if (netif_is_up(p_netif) && _joinMulticastGroups(p_netif))
            {
                bResult = true;
            }
        }
    }
    return bResult;
}
    
/*
    clsLEAmDNS2_Host::_joinMulticastGroups
*/
bool clsLEAMDNSHost::_joinMulticastGroups(netif* p_pNetIf)
{
    bool    bResult = false;

    // Join multicast group(s)
    if (p_pNetIf)
    {
        bResult = true;

#ifdef MDNS_IPV4_SUPPORT
        ip_addr_t   multicast_addr_V4 = DNS_MQUERY_IPV4_GROUP_INIT;
        if (!(p_pNetIf->flags & NETIF_FLAG_IGMP))
        {
            DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s _createHost: Setting flag: flags & NETIF_FLAG_IGMP\n"), _DH()););
            p_pNetIf->flags |= NETIF_FLAG_IGMP;

            if (ERR_OK != igmp_start(p_pNetIf))
            {
                DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s _createHost: igmp_start FAILED!\n"), _DH()););
            }
        }

        bResult = ((bResult) &&
#if LWIP_VERSION_MAJOR == 1
                   (ERR_OK == igmp_joingroup(ip_2_ip4(&p_pNetIf->ip_addr), ip_2_ip4(&multicast_addr_V4))));
#else
                   (ERR_OK == igmp_joingroup_netif(p_pNetIf, ip_2_ip4(&multicast_addr_V4))));
#endif
        DEBUG_EX_ERR(if (!bResult) DEBUG_OUTPUT.printf_P(PSTR("%s _createHost: igmp_joingroup_netif(%s) FAILED!\n"), _DH(), IPAddress(multicast_addr_V4).toString().c_str()););
#endif

#ifdef MDNS_IPV6_SUPPORT
        ip_addr_t   multicast_addr_V6 = DNS_MQUERY_IPV6_GROUP_INIT;
        bResult = ((bResult) &&
                   (ERR_OK == mld6_joingroup_netif(p_pNetIf, ip_2_ip6(&multicast_addr_V6))));
        DEBUG_EX_ERR(if (!bResult) DEBUG_OUTPUT.printf_P(PSTR("%s _createHost: mld6_joingroup_netif FAILED!\n"), _DH()););
#endif
    }
    return bResult;
}

/*
    clsLEAmDNS2_Host::_leaveMulticastGroups
*/
bool clsLEAMDNSHost::_leaveMulticastGroups(void)
{
    bool    bResult = false;

    if (m_pNetIf)
    {
        bResult = _leaveMulticastGroups(m_pNetIf);
    }
    else
    {
        for (netif* p_netif = netif_list; p_netif; p_netif = p_netif->next)
        {
            if (_leaveMulticastGroups(p_netif))
            {
                bResult = true;
            }
        }
    }
    return bResult;
}

/*
    clsLEAmDNS2_Host::_leaveMulticastGroups
*/
bool clsLEAMDNSHost::_leaveMulticastGroups(netif* p_pNetif)
{
    bool    bResult = false;

    if (p_pNetIf)
    {
        bResult = true;
        /*  _resetProbeStatus(false);   // Stop probing

            _releaseQueries();
            _releaseServices();
            _releaseHostName();*/

        // Leave multicast group(s)
#ifdef MDNS_IPV4_SUPPORT
        ip_addr_t   multicast_addr_V4 = DNS_MQUERY_IPV4_GROUP_INIT;
#if LWIP_VERSION_MAJOR == 1
        if (ERR_OK != igmp_leavegroup(ip_2_ip4(&p_pNetIf.ip_addr), ip_2_ip4(&multicast_addr_V4)))
#else
        if (ERR_OK != igmp_leavegroup_netif(p_pNetIf, ip_2_ip4(&multicast_addr_V4)))
#endif
        {
            DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("\n")););
        }
#endif
#ifdef MDNS_IPV6_SUPPORT
        ip_addr_t   multicast_addr_V6 = DNS_MQUERY_IPV6_GROUP_INIT;
        if (ERR_OK != mld6_leavegroup_netif(p_pNetIf, ip_2_ip6(&multicast_addr_V6)/*&(multicast_addr_V6.u_addr.ip6)*/))
        {
            DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("\n")););
        }
#endif
    }
    return bResult;
}


/*
    NETIF
*/

/*
    clsLEAmDNS2_Host::_getNetIfState

    Returns the current netif state.

*/
clsLEAMDNSHost::typeNetIfState clsLEAMDNSHost::_getNetIfState(void) const
{
    typeNetIfState  curNetIfState = static_cast<typeNetIfState>(enuNetIfState::None);

    if ((m_pNetIf) &&
            (netif_is_up(m_pNetIf)))
    {
        curNetIfState |= static_cast<typeNetIfState>(enuNetIfState::IsUp);

        // Check if netif link is up
        if ((netif_is_link_up(m_pNetIf)) &&
                ((m_pNetIf != netif_get_by_index(WIFI_STA)) ||
                 (STATION_GOT_IP == wifi_station_get_connect_status())))
        {
            curNetIfState |= static_cast<typeNetIfState>(enuNetIfState::LinkIsUp);
        }

#ifdef MDNS_IPV4_SUPPORT
        // Check for IPv4 address
        if (_getResponderIPAddress(enuIPProtocolType::V4).isSet())
        {
            curNetIfState |= static_cast<typeNetIfState>(enuNetIfState::IPv4);
        }
#endif
#ifdef MDNS_IPV6_SUPPORT
        // Check for IPv6 address
        if (_getResponderIPAddress(enuIPProtocolType::V6).isSet())
        {
            curNetIfState |= static_cast<typeNetIfState>(enuNetIfState::IPv6);
        }
#endif
    }
    return curNetIfState;
}

/*
    clsLEAmDNS2_Host::_checkNetIfState

    Checks the netif state.
    If eg. a new address appears, the announcing is restarted.

*/
bool clsLEAMDNSHost::_checkNetIfState(void)
{
    typeNetIfState  curNetIfState;
    if (m_NetIfState != ((curNetIfState = _getNetIfState())))
    {
        // Some state change happened
        DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: DID CHANGE NETIF STATE\n\n"), _DH()););
        DEBUG_EX_INFO(
            if ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IsUp)) != (m_NetIfState & static_cast<typeNetIfState>(enuNetIfState::IsUp))) DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: Netif is up: %s\n"), _DH(), ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IsUp)) ? "YES" : "NO"));
            if ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkIsUp)) != (m_NetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkIsUp))) DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: Netif link is up: %s\n"), _DH(), ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkIsUp)) ? "YES" : "NO"));
#ifdef MDNS_IPV4_SUPPORT
                if ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv4)) != (m_NetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv4)))
            {
                DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: IPv4 address is set: %s\n"), _DH(), ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv4)) ? "YES" : "NO"));
                    if (curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv4))
                    {
                        DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: IPv4 address: %s\n"), _DH(), _getResponderIPAddress(enuIPProtocolType::V4).toString().c_str());
                    }
                }
#endif
#ifdef MDNS_IPV6_SUPPORT
        if ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv6)) != (m_NetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv6)))
    {
        DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: IPv6 address is set: %s\n"), _DH(), ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv6)) ? "YES" : "NO"));
            if (curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPv6))
            {
                DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: IPv6 address: %s\n"), _DH(), _getResponderIPAddress(enuIPProtocolType::V6).toString().c_str());
            }
        }
#endif
        );
        if ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkMask)) != (m_NetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkMask)))
        {
            // Link came up or down
            DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: Link state changed -> restarting\n"), _DH()););
            restart();
        }
        else if (curNetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkIsUp))
        {
            // Link is up (unchanged)
            if ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPMask)) != (m_NetIfState & static_cast<typeNetIfState>(enuNetIfState::IPMask)))
            {
                // IP state changed
                // TODO: If just a new IP address was added, a simple re-announcement should be enough
                DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: IP state changed -> restarting\n"), _DH()););
                restart();
            }
        }
        /*  if (enuProbingStatus::Done == m_HostProbeInformation.m_ProbingStatus) {
            // Probing is done, prepare to (re)announce host
            DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: Preparing to (re)announce host.\n")););
            //m_HostProbeInformation.m_ProbingStatus = enuProbingStatus::Done;
            m_HostProbeInformation.m_u8SentCount = 0;
            m_HostProbeInformation.m_Timeout.reset(MDNS_ANNOUNCE_DELAY);
            }*/
        m_NetIfState = curNetIfState;
    }

    bool    bResult = ((curNetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkMask)) &&    // Continue if Link is UP
                       (curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPMask)));       // AND has any IP
    DEBUG_EX_INFO(if (!bResult) DEBUG_OUTPUT.printf_P(PSTR("%s _checkNetIfState: Link is DOWN(%s) or NO IP address(%s)!\n"), _DH(), (curNetIfState & static_cast<typeNetIfState>(enuNetIfState::LinkMask) ? "NO" : "YES"), (curNetIfState & static_cast<typeNetIfState>(enuNetIfState::IPMask) ? "NO" : "YES")););
    return bResult;
}


/*
    PROCESSING
*/

/*
    clsLEAmDNS2_Host::_processUDPInput
*/
bool clsLEAMDNSHost::_processUDPInput(void)
{
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _processUDPInput\n"), _DH()););

    bool    bResult = _parseMessage();

    /*   bResult = ((_checkNetIfState()) &&      // Any changes in the netif state?
                  (_parseMessage()));*/
    DEBUG_EX_ERR(if (!bResult) DEBUG_OUTPUT.printf_P(PSTR("%s processUDPInput: FAILED!\n"), _DH()););

    return bResult;
}


/*
    DOMAIN NAMES
*/

/*
    clsLEAmDNS2_Host::_allocDomainName
*/
bool clsLEAMDNSHost::_allocDomainName(const char* p_pcNewDomainName,
                                      char*& p_rpcDomainName)
{
    bool    bResult = false;

    _releaseDomainName(p_rpcDomainName);

    size_t  stLength = 0;
    if ((p_pcNewDomainName) &&
            (clsConsts::stDomainLabelMaxLength >= (stLength = strlen(p_pcNewDomainName))))   // char max size for a single label
    {
        // Copy in hostname characters as lowercase
        if ((bResult = (0 != (p_rpcDomainName = new char[stLength + 1]))))
        {
#ifdef MDNS_FORCE_LOWERCASE_HOSTNAME
            size_t i = 0;
            for (; i < stLength; ++i)
            {
                p_rpcDomainName[i] = (isupper(p_pcNewDomainName[i]) ? tolower(p_pcNewDomainName[i]) : p_pcNewDomainName[i]);
            }
            p_rpcDomainName[i] = 0;
#else
            strncpy(p_rpcDomainName, p_pcNewDomainName, (stLength + 1));
#endif
        }
    }
    return bResult;
}

/*
    clsLEAmDNS2_Host::_releaseDomainName
*/
bool clsLEAMDNSHost::_releaseDomainName(char*& p_rpcDomainName)
{
    bool    bResult;
    if ((bResult = (0 != p_rpcDomainName)))
    {
        delete[] p_rpcDomainName;
        p_rpcDomainName = 0;
    }
    return bResult;
}

/*
    clsLEAmDNS2_Host::_allocHostName
*/
bool clsLEAMDNSHost::_allocHostName(const char* p_pcHostName)
{
    //DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _allocHostName (%s)\n"), _DH(), p_pcHostName););
    return _allocDomainName(p_pcHostName, m_pcHostName);
}

/*
    clsLEAmDNS2_Host::_releaseHostName
*/
bool clsLEAMDNSHost::_releaseHostName(void)
{
    return _releaseDomainName(m_pcHostName);
}

/*
    clsLEAmDNS2_Host::_allocDefaultInstanceName
*/
bool clsLEAMDNSHost::_allocDefaultInstanceName(const char* p_pcDefaultInstanceName)
{
    //DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _allocDefaultInstanceName (%s)\n"), _DH(), p_pcDefaultInstanceName););
    return _allocDomainName(p_pcDefaultInstanceName, m_pcDefaultInstanceName);
}

/*
    clsLEAmDNS2_Host::_releaseDefaultInstanceName
*/
bool clsLEAMDNSHost::_releaseDefaultInstanceName(void)
{
    return _releaseDomainName(m_pcDefaultInstanceName);
}

/*
    clsLEAmDNS2_Host::_instanceName
*/
const char* clsLEAMDNSHost::_instanceName(const char* p_pcInstanceName,
        bool p_bReturnZero /*= true*/) const
{
    return (p_pcInstanceName ? : (m_pcDefaultInstanceName ? : (m_pcHostName ? : (p_bReturnZero ? 0 : "-"))));
}


/*
    SERVICE TXT
*/

/*
    clsLEAmDNS2_Host::_collectServiceTxts
*/
bool clsLEAMDNSHost::_collectServiceTxts(clsLEAMDNSHost::clsService& p_rService)
{
    if (p_rService.m_fnTxtCallback)
    {
        p_rService.m_fnTxtCallback(p_rService);
    }
    return true;
}

/*
    clsLEAmDNS2_Host::_releaseTempServiceTxts
*/
bool clsLEAMDNSHost::_releaseTempServiceTxts(clsLEAMDNSHost::clsService& p_rService)
{
    return (p_rService.m_Txts.removeTempTxts());
}


/*

    QUERIES

*/

/*
    MDNSResponder::_allocQuery

*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::_allocQuery(clsLEAMDNSHost::clsQuery::enuQueryType p_QueryType)
{
    clsQuery*    pQuery = new clsQuery(p_QueryType);
    if (pQuery)
    {
        // Link to query list
        m_Queries.push_back(pQuery);
    }
    return pQuery;
}

/*
    MDNSResponder:clsHost:::_removeQuery

*/
bool clsLEAMDNSHost::_removeQuery(clsLEAMDNSHost::clsQuery* p_pQuery)
{
    bool    bResult = false;

    clsQuery::list::iterator   it(p_pQuery
                                  ? std::find(m_Queries.begin(), m_Queries.end(), p_pQuery)
                                  : m_Queries.end());
    if (m_Queries.end() != it)
    {
        m_Queries.erase(it);
        delete p_pQuery;

        bResult = true;
    }
    else
    {
        DEBUG_EX_ERR(DEBUG_OUTPUT.printf_P(PSTR("%s _releaseQuery: INVALID query!"), _DH()););
    }
    return bResult;
}

/*
    clsLEAmDNS2_Host::_removeLegacyQuery

*/
bool clsLEAMDNSHost::_removeLegacyQuery(void)
{
    clsQuery*    pLegacyQuery = 0;
    return (((pLegacyQuery = _findLegacyQuery()))
            ? _removeQuery(pLegacyQuery)
            : true);
}

/*
    clsLEAmDNS2_Host::_findLegacyQuery

*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::_findLegacyQuery(void)
{
    clsQuery*   pLegacyQuery = 0;

    for (clsQuery* pQuery : m_Queries)
    {
        if (pQuery->m_bStaticQuery)
        {
            pLegacyQuery = pQuery;
            break;
        }
    }
    return pLegacyQuery;
}

/*
    clsLEAmDNS2_Host::_releaseQueries

*/
bool clsLEAMDNSHost::_releaseQueries(void)
{
    for (clsQuery* pQuery : m_Queries)
    {
        delete pQuery;
    }
    m_Queries.clear();
    return true;
}

/*
    clsLEAmDNS2_Host::_findNextQueryByDomain

*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::_findNextQueryByDomain(const clsLEAMDNSHost::clsRRDomain& p_Domain,
        const clsLEAMDNSHost::clsQuery::enuQueryType p_QueryType,
        const clsQuery* p_pPrevQuery)
{
    clsQuery*    pMatchingQuery = 0;

    clsQuery::list::iterator it(m_Queries.begin());
    if (p_pPrevQuery)
    {
        if (m_Queries.end() != ((it = std::find(m_Queries.begin(), m_Queries.end(), p_pPrevQuery))))
        {
            // Found previous object
            it++;
        }
        DEBUG_EX_ERR(else
        {
            DEBUG_OUTPUT.printf_P(PSTR("%s _findNextQueryByDomain: FAILED to find 'previous' object!\n"), _DH());
        });   // if not prev was found -> 'cancel'
    }

    for (; it != m_Queries.end(); it++)
    {
        if (((clsQuery::enuQueryType::None == p_QueryType) ||
                ((*it)->m_QueryType == p_QueryType)) &&
                (p_Domain == (*it)->m_Domain))
        {
            pMatchingQuery = *it;
            break;
        }
    }
    return pMatchingQuery;
}

/*
    clsLEAmDNS2_Host::_installServiceQuery

*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::_installServiceQuery(const char* p_pcService,
        const char* p_pcProtocol)
{
    clsQuery*   pMDNSQuery = 0;

    if ((p_pcService) && (*p_pcService) &&
            (p_pcProtocol) && (*p_pcProtocol) &&
            ((pMDNSQuery = _allocQuery(clsQuery::enuQueryType::Service))) &&
            (_buildDomainForService(p_pcService, p_pcProtocol, pMDNSQuery->m_Domain)))
    {
        pMDNSQuery->m_bStaticQuery = false;

        if (_sendQuery(*pMDNSQuery))
        {
            pMDNSQuery->m_u8SentCount = 1;
            pMDNSQuery->m_ResendTimeout.reset(clsConsts::u32DynamicQueryResendDelay);
        }
        else
        {
            _removeQuery(pMDNSQuery);
        }
    }
    DEBUG_EX_INFO(DEBUG_OUTPUT.printf_P(PSTR("%s _installServiceQuery: %s for '_%s._%s.local'!\n\n"), _DH(), (pMDNSQuery ? "Succeeded" : "FAILED"), (p_pcService ? : "-"), (p_pcProtocol ? : "-")););
    DEBUG_EX_ERR(if (!pMDNSQuery) DEBUG_OUTPUT.printf_P(PSTR("%s _installServiceQuery: FAILED for '_%s._%s.local'!\n\n"), _DH(), (p_pcService ? : "-"), (p_pcProtocol ? : "-")););
    return pMDNSQuery;
}

/*
    clsLEAmDNS2_Host::_installDomainQuery
*/
clsLEAMDNSHost::clsQuery* clsLEAMDNSHost::_installDomainQuery(clsLEAMDNSHost::clsRRDomain& p_Domain,
        clsLEAMDNSHost::clsQuery::enuQueryType p_QueryType)
{
    clsQuery*    pQuery = 0;

    if ((pQuery = _allocQuery(p_QueryType)))
    {
        pQuery->m_Domain = p_Domain;
        pQuery->m_bStaticQuery = false;

        if (_sendQuery(*pQuery))
        {
            pQuery->m_u8SentCount = 1;
            pQuery->m_ResendTimeout.reset(clsConsts::u32DynamicQueryResendDelay);
        }
        else
        {
            _removeQuery(pQuery);
        }
    }
    DEBUG_EX_INFO(
        DEBUG_OUTPUT.printf_P(PSTR("%s _installDomainQuery: %s for "), (pQuery ? "Succeeded" : "FAILED"), _DH());
        _printRRDomain(p_Domain);
        DEBUG_OUTPUT.println();
    );
    DEBUG_EX_ERR(if (!pQuery)
{
    DEBUG_OUTPUT.printf_P(PSTR("%s _installDomainQuery: FAILED for "), _DH());
        _printRRDomain(p_Domain);
        DEBUG_OUTPUT.println();
    }
                );
    return pQuery;
}

/*
    clsLEAmDNS2_Host::_hasQueriesWaitingForAnswers
*/
bool clsLEAMDNSHost::_hasQueriesWaitingForAnswers(void) const
{
    bool    bOpenQueries = false;

    for (const clsQuery* pQuery : m_Queries)
    {
        if (pQuery->m_bAwaitingAnswers)
        {
            bOpenQueries = true;
            break;
        }
    }
    return bOpenQueries;
}

/*
    clsLEAmDNS2_Host::_executeQueryCallback
*/
bool clsLEAMDNSHost::_executeQueryCallback(const clsQuery& p_Query,
        const clsQuery::clsAnswer& p_Answer,
        clsQuery::clsAnswer::typeQueryAnswerType p_QueryAnswerTypeFlags,
        bool p_bSetContent)
{
    if (p_Query.m_fnCallbackAnswer)
    {
        p_Query.m_fnCallbackAnswer(p_Query, p_Answer, p_QueryAnswerTypeFlags, p_bSetContent);
    }
    if (p_Query.m_fnCallbackAccessor)
    {
        p_Query.m_fnCallbackAccessor(p_Query, clsQuery::clsAnswerAccessor(&p_Answer), p_QueryAnswerTypeFlags, p_bSetContent);
    }
    return true;
}


}   // namespace MDNSImplementation


}   // namespace esp8266




