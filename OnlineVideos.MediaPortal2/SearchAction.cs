﻿using System;
using MediaPortal.Common;
using MediaPortal.Common.Localization;
using MediaPortal.UI.Presentation.Workflow;

namespace OnlineVideos.MediaPortal2
{
    public class SearchAction : IWorkflowContributor
    {
        protected readonly IResourceString _displayTitle;

        public SearchAction()
        {
            _displayTitle = LocalizationHelper.CreateResourceString("Search");
        }

        #region IWorkflowContributor Member

        public IResourceString DisplayTitle
        {
            get { return _displayTitle; }
        }

        public void Execute()
        {
            IWorkflowManager workflowManager = ServiceRegistration.Get<IWorkflowManager>();
            workflowManager.NavigatePush(Guids.DialogStateSearch);
        }

        public void Initialize()
        {
            
        }

        public bool IsActionEnabled(NavigationContext context)
        {
            if (context.WorkflowModelId == Guids.WorkFlowModel)
            {
                if (!((OnlineVideosWorkflowModel)context.Models[context.WorkflowModelId.Value]).IsExecutingBackgroundTask)
                {
                    if (context.WorkflowState.Name == Guids.WorkflowStateCategoriesName || context.WorkflowState.StateId == Guids.WorkflowStateVideos)
                    {
                        return ((OnlineVideosWorkflowModel)context.Models[context.WorkflowModelId.Value]).SelectedSite.CanSearch;
                    }
                }
            }
            return false;
        }

        public bool IsActionVisible(NavigationContext context)
        {
            return context.WorkflowModelId == Guids.WorkFlowModel;
        }

        public event ContributorStateChangeDelegate StateChanged;

        public void Uninitialize()
        {
            
        }

        #endregion
    }
}