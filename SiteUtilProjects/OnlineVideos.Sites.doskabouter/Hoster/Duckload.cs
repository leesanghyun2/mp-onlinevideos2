﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using OnlineVideos.Hoster.Base;
using OnlineVideos.Sites;
using System.Text.RegularExpressions;
using System.Net;

namespace OnlineVideos.Hoster
{
    public class Duckload : HosterBase
    {
        public override string getHosterUrl()
        {
            return "Duckload.com";
        }

        public override string getVideoUrls(string url)
        {
            if (url.Contains(".html"))
            {
                url = "http://www.duckload.com/play/" + Regex.Match(url, "divx/(?<id>[^.]+).html").Groups["id"].Value;

            }
            CookieContainer cc = new CookieContainer();
            string page = SiteUtilBase.GetWebData(url, cc);
            System.Threading.Thread.Sleep(10001);
            page = SiteUtilBase.GetWebDataFromPost(url, "secret=&next=true", cc, url);

            if (!string.IsNullOrEmpty(page))
            {
                //Divx
                Match n = Regex.Match(page, @"src=""(?<url>[^""]+)""\stype=""video/divx""");
                if (n.Success)
                {
                    videoType = VideoType.divx;
                    return n.Groups["url"].Value;
                }
                //Flv
                else if (page.Contains("duckloadplayer.swf"))
                {
                    Match o = Regex.Match(page, @"duckloadplayer.swf\?id=(?<id>[^&]+)&");
                    if (o.Success)
                    {
                        videoType = VideoType.flv;
                        page = SiteUtilBase.GetWebData("http://flash.duckload.com/video//video_api.php?showTopBar=undefined&cookie=undefined&id=" + o.Groups["id"].Value, cc);
                        string ident = Regex.Match(page, @"""ident"":\s""(?<ident>[^""]+)"",").Groups["ident"].Value;
                        string link = Regex.Match(page, @"""link"":\s""(?<link>[^""]+)""").Groups["link"].Value.Replace("\\/", "/");
                        return String.Format("http://dl{0}.duckload.com/{1}", ident, link);
                    }

                }
            }
            return String.Empty;
        }
    }
}