﻿using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Text.RegularExpressions;
using OnlineVideos.Sites;
using RssToolkit.Rss;
using System.Text;

namespace OnlineVideos
{
    public enum VideoKind { Other, TvSeries, Movie, MovieTrailer, GameTrailer, MusicVideo, News }

    public class VideoInfo : MarshalByRefObject, System.ComponentModel.INotifyPropertyChanged, ISearchResultItem
    {
        public string Title { get; set; }
        /// <summary>Used as label for the clips retrieved by <see cref="IChoice.getVideoChoices"/></summary>
        public string Title2 { get; set; }
        public string Description { get; set; }
        public string VideoUrl { get; set; }
        public string ImageUrl { get; set; }
        public string SubtitleUrl { get; set; }
        public string SubtitleText { get; set; }
        /// <summary>optional property is used by the <see cref="ImageDownloader"/> to resize the thumbnail after downloading from <see cref="ImageUrl"/> to a given aspect ratio (width/height).</summary>
        public float? ImageForcedAspectRatio { get; set; }
        public string Length { get; set; }
        public string Airdate { get; set; }
        public string StartTime { get; set; }
		object _Other;
        /// <summary>If you have additional data that you need to identify your Video object you can store it here. In order to make it work with Favorites, mark custom classes as [Serializable] and make them public.</summary>
		public object Other 
		{ 
			get { return _Other; } 
			set 
			{
				if (_Other != value)
				{
					_Other = value;
					// propagate a change in the Other object (if it supports PropertyChanged)
					System.ComponentModel.INotifyPropertyChanged notifier = _Other as System.ComponentModel.INotifyPropertyChanged;
					if (notifier != null) notifier.PropertyChanged += (s, e) => NotifyPropertyChanged("Other");
				}
			}
		}
		public string GetOtherAsString() 
        {
            if (Other == null) return "";
            else if (Other is string) return (string)Other;
            else if (Other.GetType().IsSerializable) 
            {
                try
                {
                    string serialized = null;
                    StringBuilder sb = new StringBuilder();
                    new System.Xml.Serialization.XmlSerializer(Other.GetType()).Serialize(new System.IO.StringWriter(sb), Other);
                    serialized = string.Format("Serialized://{0}|{1}", Regex.Replace(Other.GetType().AssemblyQualifiedName, @",\sVersion=[^,]+", ""), sb.ToString());
                    return serialized;
                }
                catch (Exception ex)
                {
                    Log.Warn("Error serializing Other object for Favorites: {0}", ex.Message);
                }
            }
            return Other.ToString();
        }
        public void SetOtherFromString(string other)
        {
            if (!string.IsNullOrEmpty(other))
            {
                if (other.StartsWith("Serialized://"))
                {
                    try
                    {
                        int index1 = "Serialized://".Length;
                        int index2 = other.IndexOf("|", index1, StringComparison.InvariantCulture);
                        string type = other.Substring(index1, index2 - index1);
                        string data = other.Substring(index2 + 1);
                        Type resolvedType = Type.GetType(type);
                        if (resolvedType != null)
                        {
                            Other = new System.Xml.Serialization.XmlSerializer(resolvedType).Deserialize(new System.IO.StringReader(data));
                            return;
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Warn("Error deserializing Other object from Favorites: {0}", ex.Message);
                    }
                }    
            }
            Other = other;
        }
        
		public Dictionary<string, string> PlaybackOptions;

        /// <summary>This property is only used by the <see cref="FavoriteUtil"/> to store the Name of the Site where this Video came from.</summary>
        public string SiteName { get; set; }
        /// <summary>This property is only used by the <see cref="FavoriteUtil"/> to store the Id of Video, so it can be deleted from the DB.</summary>
        public int Id { get; set; }

        /// <summary>If the SiteUtil for this VideoInfo implements <see cref="IChoice"/> setting this to true will show the details view (default), false will play the video</summary>
        public bool HasDetails { get; set; }

        /// <summary>This property is set by the <see cref="ImageDownloader"/> to the file after downloading from <see cref="ImageUrl"/>.</summary>
        public string ThumbnailImage { get; set; }

        public VideoInfo()
        {
            Title = string.Empty;
            Title2 = string.Empty;
            Description = string.Empty;
            VideoUrl = string.Empty;
            ImageUrl = string.Empty;
            Length = string.Empty;
            StartTime = string.Empty;
            SiteName = string.Empty;
            HasDetails = true;
        }

        public void CleanDescriptionAndTitle()
        {
            Description = Utils.PlainTextFromHtml(Description);
            Title = Utils.PlainTextFromHtml(Title);
        }

        public override string ToString()
        {
			return string.Format("Title:{0}\r\nDesc:{1}\r\nVidUrl:{2}\r\nImgUrl:{3}\r\nLength:{4}\r\nAirdate:{5}", Title, Description, VideoUrl, ImageUrl, Length, Airdate);
        }

        /// <summary>
        /// Can be overriden to further resolve the urls of a playbackoption.
        /// By default it only returns the url for the option given as parameter.
        /// </summary>
        /// <param name="option">key from the <see cref="PlaybackOptions"/> to get a playback url for</param>
        /// <returns>url that points to the file that can be played</returns>
        public virtual string GetPlaybackOptionUrl(string option)
        {
            return PlaybackOptions[option];
        }

        /// <summary>
        /// Example: startTime = 02:34:25.00 should result in 9265 seconds
        /// </summary>
        /// <returns></returns>
        public double GetSecondsFromStartTime()
        {
            try
            {
                double hours = 0.0d;
                double minutes = 0.0d;
                double seconds = 0.0d;

                double.TryParse(StartTime.Substring(0, 2), out hours);
                double.TryParse(StartTime.Substring(3, 2), out minutes);
                double.TryParse(StartTime.Substring(6, 2), out seconds);

                seconds += (((hours * 60) + minutes) * 60);

                return seconds;
            }
            catch (Exception ex)
            {
                Log.Warn("Error getting seconds from StartTime ({0}): {1}", StartTime, ex.Message);
                return 0.0d;
            }
        }

		public Dictionary<string, string> GetExtendedProperties()
		{
			IVideoDetails details = Other as IVideoDetails;
			return details == null ? null : details.GetExtendedProperties();
		}

        public static VideoInfo FromRssItem(RssItem rssItem, bool useLink, System.Predicate<string> isPossibleVideo)
        {
            VideoInfo video = new VideoInfo() { PlaybackOptions = new Dictionary<string, string>() };

            // Title - prefer from MediaTitle tag if available
            if (!String.IsNullOrEmpty(rssItem.MediaTitle)) video.Title = rssItem.MediaTitle;
            else video.Title = rssItem.Title;

            // Description - prefer MediaDescription tag if available
            if (!String.IsNullOrEmpty(rssItem.MediaDescription)) video.Description = rssItem.MediaDescription;
            else video.Description = rssItem.Description;

            // Try to find a thumbnail
            if (!string.IsNullOrEmpty(rssItem.GT_Image))
            {
                video.ImageUrl = rssItem.GT_Image;
            }
            else if (rssItem.MediaThumbnails.Count > 0)
            {
                video.ImageUrl = rssItem.MediaThumbnails[0].Url;
            }
            else if (rssItem.MediaContents.Count > 0 && rssItem.MediaContents[0].MediaThumbnails.Count > 0)
            {
                video.ImageUrl = rssItem.MediaContents[0].MediaThumbnails[0].Url;
            }
            else if (rssItem.MediaGroups.Count > 0 && rssItem.MediaGroups[0].MediaThumbnails.Count > 0)
            {
                video.ImageUrl = rssItem.MediaGroups[0].MediaThumbnails[0].Url;
            }
            else if (rssItem.Enclosure != null && rssItem.Enclosure.Type != null && rssItem.Enclosure.Type.ToLower().StartsWith("image"))
            {
                video.ImageUrl = rssItem.Enclosure.Url;
            }

			if (!string.IsNullOrEmpty(rssItem.Blip_Runtime)) video.Length = GetDuration(rssItem.Blip_Runtime);
            if (string.IsNullOrEmpty(video.Length)) video.Length = GetDuration(rssItem.iT_Duration);

            // if we are forced to use the Link of the RssItem, just set the video link
            if (useLink) video.VideoUrl = rssItem.Link;

            //get the video and the length
            if (rssItem.Enclosure != null && rssItem.Enclosure.Url != null && (rssItem.Enclosure.Type == null || !rssItem.Enclosure.Type.ToLower().StartsWith("image")) && (isPossibleVideo(rssItem.Enclosure.Url.Trim()) || useLink))
            {
                video.VideoUrl = useLink ? rssItem.Link : rssItem.Enclosure.Url.Trim();

                if (string.IsNullOrEmpty(video.Length) && !string.IsNullOrEmpty(rssItem.Enclosure.Length))
                {
                    int bytesOrSeconds = 0;
                    if (int.TryParse(rssItem.Enclosure.Length, out bytesOrSeconds))
                    {
                        if (bytesOrSeconds > 18000) // won't be longer than 5 hours if Length is guessed as seconds, so it's bytes
                            video.Length = (bytesOrSeconds / 1024).ToString("N0") + " KB";
                        else
                            video.Length = TimeSpan.FromSeconds(bytesOrSeconds).ToString();
                    }
                    else
                    {
                        video.Length = rssItem.Enclosure.Length;
                    }
                }
            }
            if (rssItem.MediaContents.Count > 0) // try to get the first MediaContent
            {
                foreach (RssItem.MediaContent content in rssItem.MediaContents)
                {
                    if (!useLink && content.Url != null && isPossibleVideo(content.Url.Trim())) AddToPlaybackOption(video.PlaybackOptions, content);
                    if (string.IsNullOrEmpty(video.Length)) video.Length = GetDuration(content.Duration);
                }
            }
            if (rssItem.MediaGroups.Count > 0) // videos might be wrapped in groups, try to get the first MediaContent
            {
                foreach (RssItem.MediaGroup grp in rssItem.MediaGroups)
                {
                    foreach (RssItem.MediaContent content in grp.MediaContents)
                    {
                        if (!useLink && content.Url != null && isPossibleVideo(content.Url.Trim())) AddToPlaybackOption(video.PlaybackOptions, content);
                        if (string.IsNullOrEmpty(video.Length)) video.Length = GetDuration(content.Duration);
                    }
                }
            }

            // PubDate in localized form if possible
            if (!string.IsNullOrEmpty(rssItem.PubDate))
            {
                try
                {
                    video.Airdate = rssItem.PubDateParsed.ToString("g", OnlineVideoSettings.Instance.Locale);
                }
                catch
                {
                    video.Airdate = rssItem.PubDate;
                }
            }

            // if no VideoUrl but PlaybackOptions are set -> put the first option as VideoUrl
            if (string.IsNullOrEmpty(video.VideoUrl) && video.PlaybackOptions.Count > 0)
            {
                var enumer = video.PlaybackOptions.GetEnumerator();
                enumer.MoveNext();
                video.VideoUrl = enumer.Current.Value;
                if (video.PlaybackOptions.Count == 1) video.PlaybackOptions = null; // no need for options with only one url
            }

            if (string.IsNullOrEmpty(video.VideoUrl) && isPossibleVideo(rssItem.Link) && !useLink) video.VideoUrl = rssItem.Link;

            return video;
        }

        static void AddToPlaybackOption(Dictionary<string, string> playbackOptions, RssItem.MediaContent content)
        {
            int sizeInBytes = 0;
            if (!string.IsNullOrEmpty(content.FileSize))
            {
                if (!int.TryParse(content.FileSize, out sizeInBytes))
                {
                    // with . inside the string -> already in KB
                    if (int.TryParse(content.FileSize, System.Globalization.NumberStyles.AllowThousands, null, out sizeInBytes)) sizeInBytes *= 1000;
                }
            }

            if (!playbackOptions.ContainsValue(content.Url.Trim()))
            {
                string baseInfo = string.Format("{0}({1}) | {2}:// | {3}",
                        content.Width > 0 || content.Height > 0 ? 
                            string.Format("{0}x{1} ", content.Width, content.Height) : 
                            "",
                        content.Bitrate != 0 ?
                            content.Bitrate.ToString() + " kbps" :
                            (sizeInBytes != 0 ? (sizeInBytes / 1024).ToString("N0") + " KB" : ""),
                        new Uri(content.Url).Scheme,
                        System.IO.Path.GetExtension(content.Url));
                string info = baseInfo;
                int i = 1;
                while (playbackOptions.ContainsKey(info))
                {
                    info = string.Format("{0} ({1})", baseInfo, i.ToString().PadLeft(2, ' '));
                }
                playbackOptions.Add(info, content.Url.Trim());
            }
        }

        public static string GetDuration(string duration)
        {
            if (!string.IsNullOrEmpty(duration))
            {
                double seconds;
                if (double.TryParse(duration, System.Globalization.NumberStyles.None | System.Globalization.NumberStyles.AllowDecimalPoint, System.Globalization.CultureInfo.CreateSpecificCulture("en-US"), out seconds))
                {
                    return new DateTime(TimeSpan.FromSeconds(seconds).Ticks).ToString("HH:mm:ss");
                }
                else return duration;
            }
            return "";
        }

        public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
        public void NotifyPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null) PropertyChanged(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
        }

        public VideoInfo CloneForPlayList(string videoUrl, bool withPlaybackOptions)
        {
            VideoInfo newVideoInfo = MemberwiseClone(false) as VideoInfo;
            if (withPlaybackOptions)
            {
                if (PlaybackOptions != null) newVideoInfo.PlaybackOptions = new Dictionary<string, string>(PlaybackOptions);
            }
            else
            {
                newVideoInfo.PlaybackOptions = null;
            }
            newVideoInfo.VideoUrl = videoUrl;
            return newVideoInfo;
        }

		#region MarshalByRefObject overrides
		public override object InitializeLifetimeService()
		{
			// In order to have the lease across appdomains live forever, we return null.
			return null;
		}
		#endregion
    }
}