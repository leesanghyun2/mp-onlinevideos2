/*
    Copyright (C) 2007-2010 Team MediaPortal
    http://www.team-mediaportal.com

    This file is part of MediaPortal 2

    MediaPortal 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MediaPortal 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaPortal 2.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StdAfx.h"

#include "CurlInstance.h"
#include "Logger.h"
#include <librtmp/log.h>

CCurlInstance::CCurlInstance(CLogger *logger, TCHAR *url, TCHAR *protocolName)
{
  this->logger = logger;
  this->url = Duplicate(url);
  this->protocolName = Duplicate(protocolName);
  this->curl = NULL;
  this->hCurlWorkerThread = NULL;
  this->dwCurlWorkerThreadId = 0;
  this->curlWorkerErrorCode = CURLE_OK;
  this->writeCallback = NULL;
  this->writeData = NULL;
  this->state = CURL_STATE_CREATED;

  this->rtmpApp = RTMP_APP_DEFAULT;
  this->rtmpArbitraryData = RTMP_ARBITRARY_DATA_DEFAULT;
  this->rtmpBuffer = RTMP_BUFFER_DEFAULT;
  this->rtmpFlashVersion = RTMP_FLASH_VER_DEFAULT;
  this->rtmpJtv = RTMP_JTV_DEFAULT;
  this->rtmpLive = RTMP_LIVE_DEFAULT;
  this->rtmpPageUrl = RTMP_PAGE_URL_DEFAULT;
  this->rtmpPlaylist = RTMP_PLAYLIST_DEFAULT;
  this->rtmpPlayPath = RTMP_PLAY_PATH_DEFAULT;
  this->rtmpReceiveDataTimeout = RTMP_RECEIVE_DATA_TIMEOUT_DEFAULT;
  this->rtmpStart = RTMP_START_DEFAULT;
  this->rtmpStop = RTMP_STOP_DEFAULT;
  this->rtmpSubscribe = RTMP_SUBSCRIBE_DEFAULT;
  this->rtmpSwfAge = RTMP_SWF_AGE_DEFAULT;
  this->rtmpSwfUrl = RTMP_SWF_URL_DEFAULT;
  this->rtmpSwfVerify = RTMP_SWF_VERIFY_DEFAULT;
  this->rtmpTcUrl = RTMP_TC_URL_DEFAULT;
  this->rtmpToken = RTMP_TOKEN_DEFAULT;
}


CCurlInstance::~CCurlInstance(void)
{
  this->DestroyCurlWorker();

  if (this->curl != NULL)
  {
    curl_easy_cleanup(this->curl);
    this->curl = NULL;
  }

  FREE_MEM(this->url);
  FREE_MEM(this->protocolName);
  FREE_MEM(this->rtmpApp);
  FREE_MEM(this->rtmpArbitraryData);
  FREE_MEM(this->rtmpFlashVersion);
  FREE_MEM(this->rtmpJtv);
  FREE_MEM(this->rtmpPageUrl);
  FREE_MEM(this->rtmpPlayPath);
  FREE_MEM(this->rtmpSubscribe);
  FREE_MEM(this->rtmpSwfUrl);
  FREE_MEM(this->rtmpTcUrl);
  FREE_MEM(this->rtmpToken);
}

CURL *CCurlInstance::GetCurlHandle(void)
{
  return this->curl;
}

CURLcode CCurlInstance::GetErrorCode(void)
{
  return this->curlWorkerErrorCode;
}

bool CCurlInstance::Initialize(void)
{
  bool result = (this->logger != NULL) && (this->url != NULL) && (this->protocolName != NULL);

  if (result)
  {
    this->curl = curl_easy_init();
    result = (this->curl != NULL);

    CURLcode errorCode = CURLE_OK;
    if (this->rtmpReceiveDataTimeout != UINT_MAX)
    {
      errorCode = curl_easy_setopt(this->curl, CURLOPT_CONNECTTIMEOUT, (long)(this->rtmpReceiveDataTimeout / 1000));
      if (errorCode != CURLE_OK)
      {
        this->ReportCurlErrorMessage(LOGGER_ERROR, this->protocolName, METHOD_OPEN_CONNECTION_NAME, _T("error while setting connection timeout"), errorCode);
        result = false;
      }
    }

    if (errorCode == CURLE_OK)
    {
      // librtmp needs url in specific format

      TCHAR *connectionString = Duplicate(this->url);

      if (this->rtmpApp != RTMP_APP_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_APP, this->rtmpApp, true);
      }
      if (this->rtmpApp != RTMP_ARBITRARY_DATA_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, this->rtmpArbitraryData);
      }
      if (this->rtmpBuffer != RTMP_BUFFER_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_BUFFER, this->rtmpBuffer);
      }
      if (this->rtmpFlashVersion != RTMP_FLASH_VER_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_FLASHVER, this->rtmpFlashVersion, true);
      }
      if (this->rtmpJtv != RTMP_JTV_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_JTV, this->rtmpJtv, true);
      }
      if (this->rtmpLive != RTMP_LIVE_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_LIVE, this->rtmpLive ? _T("1") : _T("0"), true);
      }
      if (this->rtmpPageUrl != RTMP_PAGE_URL_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_PAGE_URL, this->rtmpPageUrl, true);
      }
      if (this->rtmpPlaylist != RTMP_PLAYLIST_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_PLAYLIST, this->rtmpPlaylist ? _T("1") : _T("0"), true);
      }
      if (this->rtmpPlayPath != RTMP_PLAY_PATH_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_PLAY_PATH, this->rtmpPlayPath, true);
      }
      if (this->rtmpReceiveDataTimeout != RTMP_RECEIVE_DATA_TIMEOUT_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_RECEIVE_DATA_TIMEOUT, this->rtmpReceiveDataTimeout);
      }
      if (this->rtmpStart != RTMP_START_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_START, this->rtmpStart);
      }
      if (this->rtmpStop != RTMP_STOP_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_STOP, this->rtmpStop);
      }
      if (this->rtmpSubscribe != RTMP_SUBSCRIBE_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_SUBSCRIBE, this->rtmpSubscribe, true);
      }
      if (this->rtmpSwfAge != RTMP_SWF_AGE_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_SWF_AGE, this->rtmpSwfAge);
      }
      if (this->rtmpSwfUrl != RTMP_SWF_URL_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_SWF_URL, this->rtmpSwfUrl, true);
      }
      if (this->rtmpSwfVerify != RTMP_SWF_VERIFY_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_SWF_VERIFY, this->rtmpSwfVerify ? _T("1") : _T("0"), true);
      }
      if (this->rtmpTcUrl != RTMP_TC_URL_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_TC_URL, this->rtmpTcUrl, true);
      }
      if (this->rtmpToken != RTMP_TOKEN_DEFAULT)
      {
        this->AddToRtmpConnectionString(&connectionString, RTMP_TOKEN_TOKEN, this->rtmpToken, true);
      }

      this->logger->Log(LOGGER_VERBOSE, _T("%s: %s: librtmp connection string: %s"), this->protocolName, METHOD_OPEN_CONNECTION_NAME, connectionString);
      
      char *curlUrl = ConvertToMultiByte(connectionString);
      errorCode = curl_easy_setopt(this->curl, CURLOPT_URL, curlUrl);
      if (errorCode != CURLE_OK)
      {
        this->ReportCurlErrorMessage(LOGGER_ERROR, this->protocolName, METHOD_OPEN_CONNECTION_NAME, _T("error while setting url"), errorCode);
        result = false;
      }
      FREE_MEM(curlUrl);
      FREE_MEM(connectionString);
    }

    if (errorCode == CURLE_OK)
    {
      errorCode = curl_easy_setopt(this->curl, CURLOPT_RTMP_LOG_CALLBACK, &CCurlInstance::RtmpLogCallback);
      if (errorCode != CURLE_OK)
      {
        this->ReportCurlErrorMessage(LOGGER_ERROR, this->protocolName, METHOD_OPEN_CONNECTION_NAME, _T("error while setting RTMP protocol log callback"), errorCode);
        result = false;
      }
    }

    if (errorCode == CURLE_OK)
    {
      errorCode = curl_easy_setopt(this->curl, CURLOPT_RTMP_LOG_USERDATA, this);
      if (errorCode != CURLE_OK)
      {
        this->ReportCurlErrorMessage(LOGGER_ERROR, this->protocolName, METHOD_OPEN_CONNECTION_NAME, _T("error while setting RTMP protocol log callback user data"), errorCode);
        result = false;
      }
    }

    if (errorCode == CURLE_OK)
    {
      //errorCode = curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, this->writeCallback);
      errorCode = curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, CCurlInstance::CurlReceiveData);
      if (errorCode != CURLE_OK)
      {
        this->ReportCurlErrorMessage(LOGGER_ERROR, this->protocolName, METHOD_OPEN_CONNECTION_NAME, _T("error while setting write callback"), errorCode);
        result = false;
      }
    }

    if (errorCode == CURLE_OK)
    {
      //errorCode = curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, this->writeData);
      errorCode = curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, this);
      if (errorCode != CURLE_OK)
      {
        this->ReportCurlErrorMessage(LOGGER_ERROR, this->protocolName, METHOD_OPEN_CONNECTION_NAME, _T("error while setting write callback data"), errorCode);
        result = false;
      }
    }

    /*if (errorCode == CURLE_OK)
    {
      TCHAR *range = FormatString((this->endStreamTime <= this->startStreamTime) ? _T("%llu-") : _T("%llu-%llu"), this->startStreamTime, this->endStreamTime);
      this->logger->Log(LOGGER_VERBOSE, _T("%s: %s: requesting range: %s"), this->protocolName, METHOD_OPEN_CONNECTION_NAME, range);
      char *curlRange = ConvertToMultiByte(range);
      errorCode = curl_easy_setopt(this->curl, CURLOPT_RANGE, curlRange);
      if (errorCode != CURLE_OK)
      {
        this->ReportCurlErrorMessage(LOGGER_ERROR, this->protocolName, METHOD_OPEN_CONNECTION_NAME, _T("error while setting range"), errorCode);
        result = false;
      }
      FREE_MEM(curlRange);
      FREE_MEM(range);
    }*/
  }

  if (result)
  {
    this->state = CURL_STATE_INITIALIZED;
  }

  return result;
}

TCHAR *CCurlInstance::GetCurlErrorMessage(CURLcode errorCode)
{
  const char *error = curl_easy_strerror(errorCode);
  TCHAR *result = NULL;
  result = ConvertToUnicodeA(error);

  // there is no need to free error message

  return result;
}

void CCurlInstance::ReportCurlErrorMessage(unsigned int logLevel, const TCHAR *protocolName, const TCHAR *functionName, const TCHAR *message, CURLcode errorCode)
{
  TCHAR *curlError = this->GetCurlErrorMessage(errorCode);

  this->logger->Log(logLevel, METHOD_CURL_ERROR_MESSAGE, protocolName, functionName, (message == NULL) ? _T("libcurl error") : message, curlError);

  FREE_MEM(curlError);
}

HRESULT CCurlInstance::CreateCurlWorker(void)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, this->protocolName, METHOD_CREATE_CURL_WORKER_NAME);

  // clear curl error code
  this->curlWorkerErrorCode = CURLE_OK;

  this->hCurlWorkerThread = CreateThread( 
    NULL,                                   // default security attributes
    0,                                      // use default stack size  
    &CCurlInstance::CurlWorker,             // thread function name
    this,                                   // argument to thread function 
    0,                                      // use default creation flags 
    &dwCurlWorkerThreadId);                 // returns the thread identifier

  if (this->hCurlWorkerThread == NULL)
  {
    // thread not created
    result = HRESULT_FROM_WIN32(GetLastError());
    this->logger->Log(LOGGER_ERROR, _T("%s: %s: CreateThread() error: 0x%08X"), this->protocolName, METHOD_CREATE_CURL_WORKER_NAME, result);
  }

  this->logger->Log(LOGGER_INFO, (SUCCEEDED(result)) ? METHOD_END_FORMAT : METHOD_END_FAIL_HRESULT_FORMAT, this->protocolName, METHOD_CREATE_CURL_WORKER_NAME, result);
  return result;
}

HRESULT CCurlInstance::DestroyCurlWorker(void)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, this->protocolName, METHOD_DESTROY_CURL_WORKER_NAME);

  // wait for the receive data worker thread to exit      
  if (this->hCurlWorkerThread != NULL)
  {
    if (WaitForSingleObject(this->hCurlWorkerThread, 1000) == WAIT_TIMEOUT)
    {
      // thread didn't exit, kill it now
      this->logger->Log(LOGGER_INFO, METHOD_MESSAGE_FORMAT, this->protocolName, METHOD_DESTROY_CURL_WORKER_NAME, _T("thread didn't exit, terminating thread"));
      TerminateThread(this->hCurlWorkerThread, 0);
    }
  }

  this->hCurlWorkerThread = NULL;

  this->logger->Log(LOGGER_INFO, (SUCCEEDED(result)) ? METHOD_END_FORMAT : METHOD_END_FAIL_HRESULT_FORMAT, this->protocolName, METHOD_DESTROY_CURL_WORKER_NAME, result);
  return result;
}

DWORD WINAPI CCurlInstance::CurlWorker(LPVOID lpParam)
{
  CCurlInstance *caller = (CCurlInstance *)lpParam;
  caller->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, caller->protocolName, METHOD_CURL_WORKER_NAME);

  // on next line will be stopped processing of code - until something happens
  caller->curlWorkerErrorCode = curl_easy_perform(caller->curl);

  caller->state = CURL_STATE_RECEIVED_ALL_DATA;

  if ((caller->curlWorkerErrorCode != CURLE_OK) && (caller->curlWorkerErrorCode != CURLE_WRITE_ERROR))
  {
    caller->ReportCurlErrorMessage(LOGGER_ERROR, caller->protocolName, METHOD_CURL_WORKER_NAME, _T("error while receiving data"), caller->curlWorkerErrorCode);
  }

  caller->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, caller->protocolName, METHOD_CURL_WORKER_NAME);
  return S_OK;
}

unsigned int CCurlInstance::GetReceiveDataTimeout(void)
{
  return this->rtmpReceiveDataTimeout;
}

void CCurlInstance::SetReceivedDataTimeout(unsigned int timeout)
{
  this->rtmpReceiveDataTimeout = timeout;
}

void CCurlInstance::SetWriteCallback(curl_write_callback writeCallback, void *writeData)
{
  this->writeCallback = writeCallback;
  this->writeData = writeData;
}

REFERENCE_TIME CCurlInstance::GetStartStreamTime(void)
{
  return this->startStreamTime;
}

void CCurlInstance::SetStartStreamTime(REFERENCE_TIME startStreamTime)
{
  this->startStreamTime = startStreamTime;
}

REFERENCE_TIME CCurlInstance::GetEndStreamTime(void)
{
  return this->endStreamTime;
}

void CCurlInstance::SetEndStreamTime(REFERENCE_TIME endStreamTime)
{
  this->endStreamTime = endStreamTime;
}

bool CCurlInstance::StartReceivingData(void)
{
  return (this->CreateCurlWorker() == S_OK);
}

unsigned int CCurlInstance::GetCurlState(void)
{
  return this->state;
}

size_t CCurlInstance::CurlReceiveData(char *buffer, size_t size, size_t nmemb, void *userdata)
{
  CCurlInstance *caller = (CCurlInstance *)userdata;
  unsigned int bytesRead = size * nmemb;

  if (bytesRead > 0)
  {
    caller->state = CURL_STATE_RECEIVING_DATA;
  }

  if (caller->writeCallback != NULL)
  {
    // retrun caller write callback value
    return caller->writeCallback(buffer, size, nmemb, caller->writeData);
  }

  // return default value
  return bytesRead;
}

void CCurlInstance::SetRtmpApp(const TCHAR *rtmpApp)
{
  FREE_MEM(this->rtmpApp);
  this->rtmpApp = Duplicate(rtmpApp);
}

void CCurlInstance::SetRtmpTcUrl(const TCHAR *rtmpTcUrl)
{
  FREE_MEM(this->rtmpTcUrl);
  this->rtmpTcUrl = Duplicate(rtmpTcUrl);
}

void CCurlInstance::SetRtmpPageUrl(const TCHAR *rtmpPageUrl)
{
  FREE_MEM(this->rtmpPageUrl);
  this->rtmpPageUrl = Duplicate(rtmpPageUrl);
}

void CCurlInstance::SetRtmpSwfUrl(const TCHAR *rtmpSwfUrl)
{
  FREE_MEM(this->rtmpSwfUrl);
  this->rtmpSwfUrl = Duplicate(rtmpSwfUrl);
}

void CCurlInstance::SetRtmpFlashVersion(const TCHAR *rtmpFlashVersion)
{
  FREE_MEM(this->rtmpFlashVersion);
  this->rtmpFlashVersion = Duplicate(rtmpFlashVersion);
}

void CCurlInstance::SetRtmpArbitraryData(const TCHAR *rtmpArbitraryData)
{
  FREE_MEM(this->rtmpArbitraryData);
  this->rtmpArbitraryData = Duplicate(rtmpArbitraryData);
}

void CCurlInstance::SetRtmpPlayPath(const TCHAR *rtmpPlayPath)
{
  FREE_MEM(this->rtmpPlayPath);
  this->rtmpPlayPath = Duplicate(rtmpPlayPath);
}

void CCurlInstance::SetRtmpPlaylist(bool rtmpPlaylist)
{
  this->rtmpPlaylist = rtmpPlaylist;
}

void CCurlInstance::SetRtmpLive(bool rtmpLive)
{
  this->rtmpLive = rtmpLive;
}

void CCurlInstance::SetRtmpSubscribe(const TCHAR *rtmpSubscribe)
{
  FREE_MEM(this->rtmpSubscribe);
  this->rtmpSubscribe = Duplicate(rtmpSubscribe);
}

void CCurlInstance::SetRtmpStart(unsigned int rtmpStart)
{
  this->rtmpStart = rtmpStart;
}

void CCurlInstance::SetRtmpStop(unsigned int rtmpStop)
{
  this->rtmpStop = this->rtmpStop;
}

void CCurlInstance::SetRtmpBuffer(unsigned int rtmpBuffer)
{
  this->rtmpBuffer = rtmpBuffer;
}

void CCurlInstance::SetRtmpToken(const TCHAR *rtmpToken)
{
  FREE_MEM(this->rtmpToken);
  this->rtmpToken = Duplicate(rtmpToken);
}

void CCurlInstance::SetRtmpJtv(const TCHAR *rtmpJtv)
{
  FREE_MEM(this->rtmpJtv);
  this->rtmpJtv = Duplicate(rtmpJtv);
}

void CCurlInstance::SetRtmpSwfVerify(bool rtmpSwfVerify)
{
  this->rtmpSwfVerify = rtmpSwfVerify;
}

void CCurlInstance::SetRtmpSwfAge(unsigned int rtmpSwfAge)
{
  this->rtmpSwfAge = rtmpSwfAge;
}

void CCurlInstance::RtmpLogCallback(RTMP *r, int level, const char *format, va_list vl)
{
  CCurlInstance *caller = (CCurlInstance *)r->m_logUserData;

  int length = _vscprintf(format, vl) + 1;
  ALLOC_MEM_DEFINE_SET(buffer, char, length, 0);
  if (buffer != NULL)
  {
    vsprintf_s(buffer, length, format, vl);
  }

  // convert buffer to TCHAR
  TCHAR *convertedBuffer = ConvertToUnicodeA(buffer);

  int loggerLevel = LOGGER_NONE;

  switch (level)
  {
  case RTMP_LOGCRIT:
  case RTMP_LOGERROR:
    loggerLevel = LOGGER_ERROR;
    break;
  case RTMP_LOGWARNING:
    loggerLevel = LOGGER_WARNING;
    break;
  case RTMP_LOGINFO:
    loggerLevel = LOGGER_INFO;
    break;
  case RTMP_LOGDEBUG:
    loggerLevel = LOGGER_VERBOSE;
    break;
  case RTMP_LOGDEBUG2:
    loggerLevel = LOGGER_DATA;
    break;
  default:
    loggerLevel = LOGGER_NONE;
    break;
  }

  caller->logger->Log(loggerLevel, _T("%s: %s: %s"), caller->protocolName, _T("RtmpLogCallback()"), convertedBuffer);

  FREE_MEM(convertedBuffer);
  FREE_MEM(buffer);
}

TCHAR *CCurlInstance::EncodeString(const TCHAR *string)
{
  TCHAR *replacedString = ReplaceString(string, _T("\\"), _T("\\5c"));
  TCHAR *replacedString2 = ReplaceString(replacedString, _T(" "), _T("\\20"));
  FREE_MEM(replacedString);
  return replacedString2;
}

TCHAR *CCurlInstance::CreateRtmpParameter(const TCHAR *name, const TCHAR *value)
{
  if ((name == NULL) || (value == NULL))
  {
    return NULL;
  }
  else
  {
    return FormatString(_T("%s=%s"), name, value);
  }
}

TCHAR *CCurlInstance::CreateRtmpEncodedParameter(const TCHAR *name, const TCHAR *value)
{
  TCHAR *result = NULL;
  TCHAR *encodedValue = this->EncodeString(value);
  if (encodedValue != NULL)
  {
    result = this->CreateRtmpParameter(name, encodedValue);
  }
  FREE_MEM(encodedValue);

  return result;
}

bool CCurlInstance::AddToRtmpConnectionString(TCHAR **connectionString, const TCHAR *name, unsigned int value)
{
  TCHAR *formattedValue = FormatString(_T("%u"), value);
  bool result = this->AddToRtmpConnectionString(connectionString, name, formattedValue, false);
  FREE_MEM(formattedValue);
  return result;
}

bool CCurlInstance::AddToRtmpConnectionString(TCHAR **connectionString, const TCHAR *name, const TCHAR *value, bool encode)
{
  if ((connectionString == NULL) || (*connectionString == NULL) || (name == NULL) || (value == NULL))
  {
    return false;
  }

  TCHAR *temp = (encode) ? this->CreateRtmpEncodedParameter(name, value) : this->CreateRtmpParameter(name, value);
  bool result = this->AddToRtmpConnectionString(connectionString, temp);
  FREE_MEM(temp);

  return result;
}

bool CCurlInstance::AddToRtmpConnectionString(TCHAR **connectionString, const TCHAR *string)
{
  if ((connectionString == NULL) || (*connectionString == NULL) || (string == NULL))
  {
    return false;
  }

  TCHAR *temp = FormatString(_T("%s %s"), *connectionString, string);
  FREE_MEM(*connectionString);

  *connectionString = temp;

  return (*connectionString != NULL);
}