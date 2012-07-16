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

#include "stdafx.h"

#include "MPUrlSourceSplitter_Parser_Asx.h"
#include "VersionInfo.h"
#include "..\LAVSplitter\VersionInfo.h"

// parser implementation name
#ifdef _DEBUG
#define PARSER_IMPLEMENTATION_NAME                                      L"MPUrlSourceSplitter_Parser_Asxd"
#else
#define PARSER_IMPLEMENTATION_NAME                                      L"MPUrlSourceSplitter_Parser_Asx"
#endif

PIPlugin CreatePluginInstance(CParameterCollection *configuration)
{
  return new CMPUrlSourceSplitter_Parser_Asx(configuration);
}

void DestroyPluginInstance(PIPlugin pProtocol)
{
  if (pProtocol != NULL)
  {
    CMPUrlSourceSplitter_Parser_Asx *pClass = (CMPUrlSourceSplitter_Parser_Asx *)pProtocol;
    delete pClass;
  }
}

CMPUrlSourceSplitter_Parser_Asx::CMPUrlSourceSplitter_Parser_Asx(CParameterCollection *configuration)
{
  this->connectionParameters = new CParameterCollection();
  if (configuration != NULL)
  {
    this->connectionParameters->Append(configuration);
  }

  this->logger = new CLogger(this->connectionParameters);
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);

  wchar_t *version = GetVersionInfo(VERSION_INFO_MPURLSOURCESPLITTER_PARSER_ASX, COMPILE_INFO_MPURLSOURCESPLITTER_PARSER_ASX);
  if (version != NULL)
  {
    this->logger->Log(LOGGER_INFO, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME, version);
  }
  FREE_MEM(version);

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);
}

CMPUrlSourceSplitter_Parser_Asx::~CMPUrlSourceSplitter_Parser_Asx()
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);

  delete this->connectionParameters;

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);

  delete this->logger;
  this->logger = NULL;
}

// IParser interface

HRESULT CMPUrlSourceSplitter_Parser_Asx::ClearSession(void)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_CLEAR_SESSION_NAME);

  this->connectionParameters->Clear();

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_CLEAR_SESSION_NAME);
  return S_OK;
}

ParseResult CMPUrlSourceSplitter_Parser_Asx::ParseMediaPacket(CMediaPacket *mediaPacket)
{
  ParseResult result = ParseResult_NotKnown;
  this->logger->Log(LOGGER_VERBOSE, METHOD_START_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME);

  if (mediaPacket != NULL)
  {
    unsigned int length = mediaPacket->GetBuffer()->GetBufferOccupiedSpace() + 1;
    ALLOC_MEM_DEFINE_SET(buffer, char, length, 0);
    if ((buffer != NULL) && (length > 1))
    {
      mediaPacket->GetBuffer()->CopyFromBuffer(buffer, length - 1, 0, 0);

      char *lowerBuffer = DuplicateA(buffer);
      if (lowerBuffer != NULL)
      {
        size_t length = strlen(lowerBuffer);
        if (length > 0)
        {
          _strlwr_s(lowerBuffer, length + 1);

          if (length > 4)
          {
            // the length of received data should be at least 5 characters '<asx '

            if (strncmp(lowerBuffer, "<asx ", 5) == 0)
            {
              // possible ASX file
              // try to parse

              this->logger->Log(LOGGER_VERBOSE, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, L"ASX stream");
              wchar_t *asxBuffer = ConvertToUnicodeA(buffer);
              if (asxBuffer != NULL)
              {
                this->logger->Log(LOGGER_VERBOSE, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, asxBuffer);
              }
              FREE_MEM(asxBuffer);

              char *entryStartString = strstr(lowerBuffer, "<entry");
              if (entryStartString != NULL)
              {
                char *entryEndString = strstr(entryStartString, "</entry>");
                if (entryEndString != NULL)
                {
                  char *hrefNode = strstr(entryStartString, "href");
                  if (hrefNode != NULL)
                  {
                    hrefNode += 4;
                    unsigned int hrefLength = strlen(hrefNode);
                    // found href attribute in ref node
                    hrefNode = SkipBlanksA(hrefNode, hrefLength);
                    if (strncmp(hrefNode, "=", 1) == 0)
                    {
                      hrefNode++;
                      hrefNode = SkipBlanksA(hrefNode, hrefLength);
                      if (strncmp(hrefNode, "\"", 1) == 0)
                      {
                        // we are on the first ", find second "
                        char *first = hrefNode + 1;
                        char *last = strstr(first, "\"");
                        if ((first != NULL) && (last != NULL))
                        {
                          unsigned int firstIndex = first - lowerBuffer;
                          unsigned int lastIndex = last - lowerBuffer;

                          if (lastIndex > firstIndex)
                          {
                            unsigned int urlLength = lastIndex - firstIndex + 1;
                            ALLOC_MEM_DEFINE_SET(url, char, urlLength, 0);
                            if (url != NULL)
                            {
                              memcpy(url, buffer + firstIndex, urlLength - 1);

                              wchar_t *w_url = ConvertToUnicodeA(url);
                              if (w_url != NULL)
                              {
                                this->logger->Log(LOGGER_VERBOSE, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, w_url);

                                CParameter *urlParameter = new CParameter(PARAMETER_NAME_URL, w_url);
                                if (urlParameter != NULL)
                                {
                                  bool invariant = true;
                                  this->connectionParameters->Remove(PARAMETER_NAME_URL, (void *)&invariant);
                                  this->connectionParameters->Add(urlParameter);

                                  result = ParseResult_Known;
                                }
                              }
                              FREE_MEM(w_url);
                            }
                            FREE_MEM(url);
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          if ((result == ParseResult_NotKnown) && (length > 11))
          {
            // the length of received data should be at least 11 characters '[Reference]'
            if (strncmp(lowerBuffer, "[reference]", 11) == 0)
            {
              // possible special ASX file
              // try to parse

              this->logger->Log(LOGGER_VERBOSE, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, L"special ASX stream");
              wchar_t *asxBuffer = ConvertToUnicodeA(buffer);
              if (asxBuffer != NULL)
              {
                this->logger->Log(LOGGER_VERBOSE, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, asxBuffer);
              }
              FREE_MEM(asxBuffer);

              char *ref1StartString = strstr(lowerBuffer, "ref1=");
              if (ref1StartString != NULL)
              {
                char *ref1EndString1 = strstr(ref1StartString, "\n");
                char *ref1EndString2 = strstr(ref1StartString, "\r");

                char *ref1EndString = NULL;
                if ((ref1EndString1 != NULL) && (ref1EndString2 != NULL))
                {
                  ref1EndString = (ref1EndString1 < ref1EndString2) ? ref1EndString1 : ref1EndString2;
                }
                else if (ref1EndString1 != NULL)
                {
                  ref1EndString = ref1EndString1;
                }
                else if (ref1EndString2 != NULL)
                {
                  ref1EndString = ref1EndString2;
                }

                if (ref1EndString != NULL)
                {
                  char *first = ref1StartString + 5;
                  if (first != NULL)
                  {
                    unsigned int firstIndex = first - lowerBuffer;
                    unsigned int lastIndex = ref1EndString - lowerBuffer;

                    if (lastIndex > firstIndex)
                    {
                      unsigned int urlLength = lastIndex - firstIndex + 1;
                      ALLOC_MEM_DEFINE_SET(url, char, urlLength, 0);
                      if (url != NULL)
                      {
                        memcpy(url, buffer + firstIndex, urlLength - 1);

                        wchar_t *w_url = ConvertToUnicodeA(url);
                        if (w_url != NULL)
                        {
                          wchar_t *replacedUrl = ReplaceString(w_url, L"http://", L"mms://");
                          if (replacedUrl != NULL)
                          {
                            this->logger->Log(LOGGER_VERBOSE, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, replacedUrl);

                            CParameter *urlParameter = new CParameter(PARAMETER_NAME_URL, replacedUrl);
                            if (urlParameter != NULL)
                            {
                              bool invariant = true;
                              this->connectionParameters->Remove(PARAMETER_NAME_URL, (void *)&invariant);
                              this->connectionParameters->Add(urlParameter);

                              result = ParseResult_Known;
                            }
                          }
                          FREE_MEM(replacedUrl);
                        }
                        FREE_MEM(w_url);
                      }
                      FREE_MEM(url);
                    }
                  }
              //    char *hrefNode = strstr(entryStartString, "href");
              //    if (hrefNode != NULL)
              //    {
              //      hrefNode += 4;
              //      unsigned int hrefLength = strlen(hrefNode);
              //      // found href attribute in ref node
              //      hrefNode = SkipBlanks(hrefNode, hrefLength);
              //      if (strncmp(hrefNode, "=", 1) == 0)
              //      {
              //        hrefNode++;
              //        hrefNode = SkipBlanks(hrefNode, hrefLength);
              //        if (strncmp(hrefNode, "\"", 1) == 0)
              //        {
              //          // we are on the first ", find second "
              //          char *first = hrefNode + 1;
              //          char *last = strstr(first, "\"");
              //          if ((first != NULL) && (last != NULL))
              //          {
              //            unsigned int firstIndex = first - lowerBuffer;
              //            unsigned int lastIndex = last - lowerBuffer;

              //            if (lastIndex > firstIndex)
              //            {
              //              unsigned int urlLength = lastIndex - firstIndex + 1;
              //              ALLOC_MEM_DEFINE_SET(url, char, urlLength, 0);
              //              if (url != NULL)
              //              {
              //                memcpy(url, buffer + firstIndex, urlLength - 1);

              //                wchar_t *w_url = ConvertToUnicodeA(url);
              //                if (w_url != NULL)
              //                {
              //                  this->logger->Log(LOGGER_VERBOSE, METHOD_MESSAGE_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, w_url);

              //                  CParameter *urlParameter = new CParameter(PARAMETER_NAME_URL, w_url);
              //                  if (urlParameter != NULL)
              //                  {
              //                    bool invariant = true;
              //                    this->connectionParameters->Remove(PARAMETER_NAME_URL, (void *)&invariant);
              //                    this->connectionParameters->Add(urlParameter);

              //                    result = ParseResult_Known;
              //                  }
              //                }
              //                FREE_MEM(w_url);
              //              }
              //              FREE_MEM(url);
              //            }
              //          }
              //        }
              //      }
              //    }
                }
              }
            }
          }
        }
      }
      FREE_MEM(lowerBuffer);
    }
    FREE_MEM(buffer);
  }

  this->logger->Log(LOGGER_VERBOSE, METHOD_END_INT_FORMAT, PARSER_IMPLEMENTATION_NAME, METHOD_PARSE_MEDIA_PACKET_NAME, result);
  return result;
}

HRESULT CMPUrlSourceSplitter_Parser_Asx::SetConnectionParameters(const CParameterCollection *parameters)
{
  if (parameters != NULL)
  {
    this->connectionParameters->Append((CParameterCollection *)parameters);
  }
  return S_OK;
}

Action CMPUrlSourceSplitter_Parser_Asx::GetAction(void)
{
  return Action_GetNewConnection;
}

HRESULT CMPUrlSourceSplitter_Parser_Asx::GetConnectionParameters(CParameterCollection *parameters)
{
  HRESULT result = (parameters == NULL) ? E_POINTER : S_OK;

  if (SUCCEEDED(result))
  {
    parameters->Append(this->connectionParameters);
  }

  return result;
}

// IPlugin interface

wchar_t *CMPUrlSourceSplitter_Parser_Asx::GetName(void)
{
  return Duplicate(PARSER_NAME);
}

GUID CMPUrlSourceSplitter_Parser_Asx::GetInstanceId(void)
{
  return this->logger->loggerInstance;
}

HRESULT CMPUrlSourceSplitter_Parser_Asx::Initialize(PluginConfiguration *configuration)
{
  if (configuration == NULL)
  {
    return E_POINTER;
  }

  ParserPluginConfiguration *parserPluginConfiguration = (ParserPluginConfiguration *)configuration;

  this->connectionParameters->Clear();
  if (parserPluginConfiguration->configuration != NULL)
  {
    this->logger->SetParameters(configuration->configuration);
    this->connectionParameters->Append(parserPluginConfiguration->configuration);
  }
  this->connectionParameters->LogCollection(this->logger, LOGGER_VERBOSE, PARSER_IMPLEMENTATION_NAME, METHOD_INITIALIZE_NAME);

  return S_OK;
}