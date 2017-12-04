/*
  NTP-TZ-DST
  NetWork Time Protocol - Time Zone - Daylight Saving Time

  This example shows how to read and set time,
  and how to use NTP (set NTP0_OR_LOCAL1 to 0 below)
  or an external RTC (set NTP0_OR_LOCAL1 to 1 below)

  TZ and DST below have to be manually set
  according to your local settings.

  Demo of settimeofday_cb() + sntp_force_request()
  in NTP mode

  This example code is in the public domain.
*/

#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <ESP8266WiFi.h>
#include <coredecls.h>			// settimeofday_cb() / sntp_force_request()

////////////////////////////////////////////////////////

#define SSID            "open"
#define SSIDPWD         ""
#define TZ              1       // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries

#define NTP0_OR_LOCAL1  1       // 0:use NTP  1:fake external RTC
#define RTC_TEST     1510592825 // 1510592825 = Monday 13 November 2017 17:07:05 UTC

////////////////////////////////////////////////////////

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

timeval cbtime;			// time set in callback
bool cbtime_set = false;

void time_is_set (void)
{
  gettimeofday(&cbtime, NULL);
  cbtime_set = true;
  Serial.println("------------------ settimeofday() was called ------------------");
}

void setup() {
  Serial.begin(115200);
  settimeofday_cb(time_is_set);

#if NTP0_OR_LOCAL1
  // local

  ESP.eraseConfig();
  time_t rtc = RTC_TEST;
  timeval tv = { rtc, 0 };
  timezone tz = { TZ_MN + DST_MN, 0 };
  settimeofday(&tv, &tz);

#else // ntp

  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSIDPWD);
  // don't wait, observe time changing when ntp timestamp is received

#endif // ntp
}

// for testing purpose:
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

#define PTM(w) \
  Serial.print(":" #w "="); \
  Serial.print(tm->tm_##w);

void printTm (const char* what, const tm* tm) {
  Serial.print(what);
  PTM(isdst); PTM(yday); PTM(wday);
  PTM(year);  PTM(mon);  PTM(mday);
  PTM(hour);  PTM(min);  PTM(sec);
}

timeval tv;
timespec tp;
time_t now;
uint32_t now_ms, now_us;
int loop_count = 0;

void loop() {

  gettimeofday(&tv, nullptr);
  clock_gettime(0, &tp);
  now = time(nullptr);
  now_ms = millis();
  now_us = micros();

  // localtime / gmtime every second change
  static time_t lastv = 0;
  if (lastv != tv.tv_sec) {
    lastv = tv.tv_sec;
    Serial.println();
    printTm("localtime", localtime(&now));
    Serial.println();
    printTm("gmtime   ", gmtime(&now));
    Serial.println();
    Serial.println();
  }
  
  if (cbtime_set)
  {
    Serial.print("gettimeofday at time-is-set-callback:");
    Serial.print((uint32_t)cbtime.tv_sec);
    Serial.print("/");
    Serial.print((uint32_t)cbtime.tv_usec);
    Serial.println("us");
    Serial.println();
    cbtime_set = false;
  }

  // time from boot
  Serial.print("clock:");
  Serial.print((uint32_t)tp.tv_sec);
  Serial.print("/");
  Serial.print((uint32_t)tp.tv_nsec);
  Serial.print("ns");

  // time from boot
  Serial.print(" millis:");
  Serial.print(now_ms);
  Serial.print(" micros:");
  Serial.print(now_us);

  // EPOCH+tz+dst
  Serial.print(" gtod:");
  Serial.print((uint32_t)tv.tv_sec);
  Serial.print("/");
  Serial.print((uint32_t)tv.tv_usec);
  Serial.print("us");

  // EPOCH+tz+dst
  Serial.print(" time:");
  Serial.print((uint32_t)now);

  // human readable
  Serial.print(" ctime:(UTC+");
  Serial.print((uint32_t)(TZ * 60 + DST_MN));
  Serial.print("mn)");
  Serial.print(ctime(&now));

#if !NTP0_OR_LOCAL1
  // sntp_force_request() demo

  if (++loop_count == 100) // 100=>10secs
  {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    Serial.println("------------------ WIFI OFF ------------------");
  }

  if (loop_count == 130) // 130=>13secs
  {
    Serial.println("------------------ WIFI ON ------------------");

    timeval backtothefuture = { 0, 0 };
    settimeofday(&backtothefuture, nullptr);
    now = time(nullptr);
    Serial.print("reset time after settimeofday(0) (sec): ");
    Serial.println(now);

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, SSIDPWD);
    Serial.print("reconnecting.");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("reconnected");
    loop_count = 0;

    sntp_force_request();
  }

#endif // NTP

  // simple drifting loop
  delay(100);
}
