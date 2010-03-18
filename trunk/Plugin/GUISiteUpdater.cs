﻿using System;
using System.Collections.Generic;
using MediaPortal.GUI.Library;
using MediaPortal.Dialogs;
using MediaPortal.Configuration;

namespace OnlineVideos
{
    public class GUISiteUpdater : GUIWindow
    {
        enum FilterOption { All, Reported, Broken, Working, Updatable };
        enum SortOption { SortByUpdated, SortByName, SortByLanguage_SortByName, SortByLanguage_SortByUpdated };

        [SkinControlAttribute(50)]
        protected GUIListControl GUI_infoList = null;
        [SkinControlAttribute(502)]
        protected GUIButtonControl GUI_btnUpdate = null;
        [SkinControlAttribute(503)]
        protected GUISelectButtonControl GUI_btnFilter = null;
        [SkinControlAttribute(504)]
        protected GUISelectButtonControl GUI_btnSort = null;
        [SkinControlAttribute(505)]
        protected GUIButtonControl GUI_btnFullUpdate = null;

        OnlineVideosWebservice.Site[] onlineSites = null;
        DateTime lastSitesRetrievalTime = DateTime.MinValue;

        public override int GetID
        {
            get { return 4757; }
            set { base.GetID = value; }
        }

        public override bool Init()
        {
            bool result = Load(GUIGraphicsContext.Skin + @"\myonlinevideosUpdater.xml");
            GUIPropertyManager.SetProperty("#OnlineVideos.owner", " ");
            return result;
        }

        #if !MP102
        public override string GetModuleName()
        {
            return OnlineVideoSettings.PLUGIN_NAME;
        }
        #endif

        protected override void OnPageLoad()
        {
            #if MP102
            GUIPropertyManager.SetProperty("#currentmodule", OnlineVideoSettings.PLUGIN_NAME);
            #endif

            Translation.TranslateSkin();

            base.OnPageLoad();

            if (GUI_btnFilter.SubItemCount == 0)
            {
                foreach (string aFilterOption in Enum.GetNames(typeof(FilterOption)))
                {
                    GUIControl.AddItemLabelControl(GetID, GUI_btnFilter.GetID, Translation.Strings[aFilterOption]);
                }
            }
            if (GUI_btnSort.SubItemCount == 0)
            {
                foreach (string aSortOption in Enum.GetNames(typeof(SortOption)))
                {
                    string[] singled = aSortOption.Split('_');
                    for(int i = 0; i<singled.Length;i++) singled[i] = Translation.Strings[singled[i]];                    
                    GUIControl.AddItemLabelControl(GetID, GUI_btnSort.GetID, string.Join(", ", singled));
                }
            }

            GUIPropertyManager.SetProperty("#header.label",
                                           OnlineVideoSettings.Instance.BasicHomeScreenName + ": " + Translation.ManageSites);
            GUIPropertyManager.SetProperty("#header.image",
                                           Config.GetFolder(Config.Dir.Thumbs) + @"\OnlineVideos\Banners\OnlineVideos.png");

            DisplayOnlineSites();
        }

        void DisplayOnlineSites()
        {
            GUIPropertyManager.SetProperty("#OnlineVideos.owner", String.Empty);
            GUIPropertyManager.SetProperty("#OnlineVideos.desc", String.Empty);

            if (DateTime.Now - lastSitesRetrievalTime > TimeSpan.FromMinutes(10)) // only get sites every 10 minutes
            {
                if (!Gui2UtilConnector.Instance.ExecuteInBackgroundAndWait(delegate()
                {
                    OnlineVideosWebservice.OnlineVideosService ws = new OnlineVideosWebservice.OnlineVideosService();
                    onlineSites = ws.GetSitesOverview();
                    lastSitesRetrievalTime = DateTime.Now;
                }, "getting site overview from webservice"))
                {
                    return;
                }
            }

            if (onlineSites == null || onlineSites.Length == 0) return;

            GUIControl.ClearControl(GetID, GUI_infoList.GetID);

            List<OnlineVideosWebservice.Site> filteredsortedSites = new List<OnlineVideos.OnlineVideosWebservice.Site>(onlineSites);
            filteredsortedSites = filteredsortedSites.FindAll(SitePassesFilter);
            filteredsortedSites.Sort(CompareSiteForSort);

            foreach (OnlineVideosWebservice.Site site in filteredsortedSites)
            {
                if (!site.IsAdult || !OnlineVideoSettings.Instance.useAgeConfirmation || OnlineVideoSettings.Instance.ageHasBeenConfirmed)
                {
                    GUIListItem loListItem = new GUIListItem(site.Name);
                    loListItem.TVTag = site;
                    loListItem.Label2 = site.Language;
                    loListItem.Label3 = site.LastUpdated.ToString("dd.MM.yy HH:mm");
                    string image = Config.GetFolder(Config.Dir.Thumbs) + @"\OnlineVideos\Icons\" + site.Name + ".png";
                    if (System.IO.File.Exists(image)) { loListItem.IconImage = image; loListItem.ThumbnailImage = image; }
                    loListItem.PinImage = GUIGraphicsContext.Skin + @"\Media\OnlineVideos\" + site.State.ToString() + ".png";
                    loListItem.OnItemSelected += new MediaPortal.GUI.Library.GUIListItem.ItemSelectedHandler(OnSiteSelected);
                    GUI_infoList.Add(loListItem);
                }
            }

            GUIControl.SelectItemControl(GetID, GUI_infoList.GetID, 0);
        }

        private void OnSiteSelected(GUIListItem item, GUIControl parent)
        {
            OnlineVideosWebservice.Site site = item.TVTag as OnlineVideosWebservice.Site;
            if (site != null)
            {
                GUIPropertyManager.SetProperty("#OnlineVideos.owner", site.Owner_FK.Substring(0, site.Owner_FK.IndexOf('@')));
                if (!string.IsNullOrEmpty(site.Description)) GUIPropertyManager.SetProperty("#OnlineVideos.desc", site.Description);
                else GUIPropertyManager.SetProperty("#OnlineVideos.desc", String.Empty);
            }
        }

        public override void OnAction(Action action)
        {
            GUI_btnSort.Label = Translation.SortOptions;
            GUI_btnFilter.Label = Translation.Filter;
            base.OnAction(action);
        }

        protected override void OnClicked(int controlId, GUIControl control, Action.ActionType actionType)
        {
            if (control == GUI_btnUpdate)
            {
                DisplayOnlineSites();
                GUIControl.FocusControl(GetID, GUI_infoList.GetID);
            }
            else if (control == GUI_btnSort)
            {
                GUIControl.SelectItemControl(GetID, GUI_btnSort.GetID, GUI_btnSort.SelectedItem);
            }
            else if (control == GUI_btnFilter)
            {
                GUIControl.SelectItemControl(GetID, GUI_btnFilter.GetID, GUI_btnFilter.SelectedItem);
            }
            else if (control == GUI_infoList)
            {
                ShowOptionsForSite(GUI_infoList.SelectedListItem.TVTag as OnlineVideosWebservice.Site);
            }
            else if (control == GUI_btnFullUpdate)
            {
                FullUpdate();
            }

            base.OnClicked(controlId, control, actionType);
        }

        void ShowOptionsForSite(OnlineVideosWebservice.Site site)
        {
            SiteSettings localSite = GetLocalSite(site.Name);

            GUIDialogMenu dlgSel = (GUIDialogMenu)GUIWindowManager.GetWindow((int)GUIWindow.Window.WINDOW_DIALOG_MENU);
            dlgSel.ShowQuickNumbers = false;
            if (dlgSel != null)
            {
                dlgSel.Reset();
                dlgSel.SetHeading(Translation.Actions);

                if (localSite == null)
                {
                    dlgSel.Add(Translation.AddToMySites);
                }
                else
                {
                    if (localSite.LastUpdated < site.LastUpdated)
                    {
                        dlgSel.Add(Translation.UpdateMySite);
                        dlgSel.Add(Translation.UpdateMySiteSkipCategories);                        
                    }
                    dlgSel.Add(Translation.RemoveFromMySites);
                }
            }
            dlgSel.DoModal(GetID);

            if (dlgSel.SelectedLabelText == Translation.AddToMySites)
            {
                
                    SiteSettings newSite = GetRemoteSite(site.Name);
                    if (newSite != null)
                    {
                        OnlineVideoSettings.Instance.SiteSettingsList.Add(newSite);
                        OnlineVideoSettings.Instance.SaveSites();
                        OnlineVideoSettings.Instance.BuildSiteList();
                    }
            }
            else if (dlgSel.SelectedLabelText == Translation.UpdateMySite)
            {
                SiteSettings site2Update = GetRemoteSite(site.Name);
                if (site2Update != null)
                {
                    for (int i = 0; i < OnlineVideoSettings.Instance.SiteSettingsList.Count; i++)
                    {
                        if (OnlineVideoSettings.Instance.SiteSettingsList[i].Name == site2Update.Name)
                        {
                            OnlineVideoSettings.Instance.SiteSettingsList[i] = site2Update;
                            OnlineVideoSettings.Instance.SaveSites();
                            OnlineVideoSettings.Instance.BuildSiteList();
                            break;
                        }
                    }
                }
            }
            else if (dlgSel.SelectedLabelText == Translation.UpdateMySiteSkipCategories)
            {
                SiteSettings siteRemote = GetRemoteSite(site.Name);
                if (siteRemote != null)
                {
                    for (int i = 0; i < OnlineVideoSettings.Instance.SiteSettingsList.Count; i++)
                    {
                        if (OnlineVideoSettings.Instance.SiteSettingsList[i].Name == siteRemote.Name)
                        {
                            siteRemote.Categories = OnlineVideoSettings.Instance.SiteSettingsList[i].Categories;
                            OnlineVideoSettings.Instance.SiteSettingsList[i] = siteRemote;
                            OnlineVideoSettings.Instance.SaveSites();
                            OnlineVideoSettings.Instance.BuildSiteList();
                            break;
                        }
                    }
                }
            }
            else if (dlgSel.SelectedLabelText == Translation.RemoveFromMySites)
            {
                OnlineVideoSettings.Instance.SiteSettingsList.Remove(localSite);
                OnlineVideoSettings.Instance.SaveSites();
                OnlineVideoSettings.Instance.BuildSiteList();
            }
        }

        void FullUpdate()
        {
            // update and add all sites and dlls, so the user has everything the server currently has
        }

        bool SitePassesFilter(OnlineVideosWebservice.Site site)
        {
            FilterOption fo = (FilterOption)GUI_btnFilter.SelectedItem;
            switch (fo)
            {
                case FilterOption.Working:
                    return site.State == OnlineVideos.OnlineVideosWebservice.SiteState.Working;
                case FilterOption.Reported:
                    return site.State == OnlineVideos.OnlineVideosWebservice.SiteState.Reported;
                case FilterOption.Broken:
                    return site.State == OnlineVideos.OnlineVideosWebservice.SiteState.Broken;
                case FilterOption.Updatable:
                    foreach (SiteSettings localSite in OnlineVideoSettings.Instance.SiteSettingsList)
                    {
                        if (localSite.Name == site.Name)
                        {
                            if (localSite.LastUpdated < site.LastUpdated) return true;
                            else return false;
                        }
                    }
                    return false;
                default: return true;
            }
        }

        int CompareSiteForSort(OnlineVideosWebservice.Site site1, OnlineVideosWebservice.Site site2)
        {
            SortOption so = (SortOption)GUI_btnSort.SelectedItem;
            switch (so)
            {
                case SortOption.SortByUpdated:
                    return site2.LastUpdated.CompareTo(site1.LastUpdated);
                case SortOption.SortByName:
                    return site1.Name.CompareTo(site2.Name);
                case SortOption.SortByLanguage_SortByUpdated:
                    int langCompResult = site1.Language.CompareTo(site2.Language);
                    if (langCompResult == 0)
                    {
                        return site2.LastUpdated.CompareTo(site1.LastUpdated);
                    }
                    else return langCompResult;
                case SortOption.SortByLanguage_SortByName:
                    int langCompResult2 = site1.Language.CompareTo(site2.Language);
                    if (langCompResult2 == 0)
                    {
                        return site1.Name.CompareTo(site2.Name);
                    }
                    else return langCompResult2;
            }
            return 0;
        }

        SiteSettings GetLocalSite(string name)
        {
            foreach (SiteSettings localSite in OnlineVideoSettings.Instance.SiteSettingsList)
            {
                if (localSite.Name == name) return localSite;
            }
            return null;
        }

        SiteSettings GetRemoteSite(string name)
        {
            string siteXml = "";
            OnlineVideosWebservice.OnlineVideosService ws = null;
            if (Gui2UtilConnector.Instance.ExecuteInBackgroundAndWait(delegate()
            {
                ws = new OnlineVideos.OnlineVideosWebservice.OnlineVideosService();
                siteXml = ws.GetSiteXml(name);
            }, "getting site xml from webservice"))
            {
                if (siteXml.Length > 0)
                {
                    IList<SiteSettings> sitesFromWeb = Utils.SiteSettingsFromXml(siteXml);
                    if (sitesFromWeb != null && sitesFromWeb.Count > 0)
                    {
                        DownloadImages(sitesFromWeb[0].Name, ws);
                        return sitesFromWeb[0];
                    }
                }
            }
            return null;
        }

        static void DownloadImages(string siteName, OnlineVideosWebservice.OnlineVideosService ws)
        {
            try
            {
                byte[] icon = null;
                if (Gui2UtilConnector.Instance.ExecuteInBackgroundAndWait(delegate()
                {
                    icon = ws.GetSiteIcon(siteName);
                }, "getting site icon from webservice"))
                {
                    if (icon != null && icon.Length > 0)
                    {
                        System.IO.File.WriteAllBytes(Config.GetFolder(Config.Dir.Thumbs) + @"\OnlineVideos\Icons\" + siteName + ".png", icon);
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex.ToString());
            }
            try
            {
                byte[] banner = null;
                if (Gui2UtilConnector.Instance.ExecuteInBackgroundAndWait(delegate()
                {
                    banner = ws.GetSiteBanner(siteName);
                }, "getting site banner from webservice"))
                {
                    if (banner != null && banner.Length > 0)
                    {
                        System.IO.File.WriteAllBytes(Config.GetFolder(Config.Dir.Thumbs) + @"\OnlineVideos\Banners\" + siteName + ".png", banner);
                    }
                }                
            }
            catch (Exception ex)
            {
                Log.Error(ex.ToString());
            }
        }
    }
}
