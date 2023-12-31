/** 
 * @file llpanelmaininventory.cpp
 * @brief Implementation of llpanelmaininventory.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpanelmaininventory.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llavataractions.h"
#include "llcombobox.h"
#include "lldndbutton.h"
#include "lleconomy.h"
#include "llfilepicker.h"
#include "llfloaterinventory.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llfiltereditor.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterreg.h"
#include "llmenubutton.h"
#include "lloutfitobserver.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollcontainer.h"
#include "llsdserialize.h"
#include "llsdparam.h"
#include "llspinctrl.h"
#include "lltoggleablemenu.h"
#include "lltooldraganddrop.h"
#include "llviewermenu.h"
#include "llviewertexturelist.h"
#include "llsidepanelinventory.h"
#include "llfolderview.h"
#include "llradiogroup.h"
#include "pvdata.h"

// <polarity> fix Major FPS drop by disabling filters.xml
// const std::string FILTERS_FILENAME("filters.xml");

static LLPanelInjector<LLPanelMainInventory> t_inventory("panel_main_inventory");

void on_file_loaded_for_save(BOOL success, 
							 LLViewerFetchedTexture *src_vi,
							 LLImageRaw* src, 
							 LLImageRaw* aux_src, 
							 S32 discard_level,
							 BOOL final,
							 void* userdata);

///----------------------------------------------------------------------------
/// LLFloaterInventoryFinder
///----------------------------------------------------------------------------

class LLFloaterInventoryFinder : public LLFloater
{
public:
	LLFloaterInventoryFinder( LLPanelMainInventory* inventory_view);
	virtual void draw();
	/*virtual*/	BOOL	postBuild();
	void changeFilter(LLInventoryFilter* filter);
	void updateElementsFromFilter();
	BOOL getCheckShowEmpty();
	BOOL getCheckSinceLogoff();
	U32 getDateSearchDirection();

	static void onTimeAgo(LLUICtrl*, void *);
	static void onCloseBtn(void* user_data);
	static void selectAllTypes(void* user_data);
	static void selectNoTypes(void* user_data);
private:
	LLPanelMainInventory*	mPanelMainInventory;
	LLSpinCtrl*			mSpinSinceDays;
	LLSpinCtrl*			mSpinSinceHours;
	LLInventoryFilter*	mFilter;
	LLSidepanelInventory * mSidepanelInventory;
};

///----------------------------------------------------------------------------
/// LLPanelMainInventory
///----------------------------------------------------------------------------

LLPanelMainInventory::LLPanelMainInventory(const LLPanel::Params& p)
	: LLPanel(p),
	  mActivePanel(NULL),
	  mSavedFolderState(NULL),
	  mFilterText(""),
	  mMenuGearDefault(NULL),
	  mMenuAddHandle(),
	  mNeedUploadCost(true)
{
	// Menu Callbacks (non contex menus)
	mCommitCallbackRegistrar.add("Inventory.DoToSelected", boost::bind(&LLPanelMainInventory::doToSelected, this, _2));
	mCommitCallbackRegistrar.add("Inventory.CloseAllFolders", boost::bind(&LLPanelMainInventory::closeAllFolders, this));
	mCommitCallbackRegistrar.add("Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLFolderType::FT_TRASH));
	mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND));
	mCommitCallbackRegistrar.add("Inventory.DoCreate", boost::bind(&LLPanelMainInventory::doCreate, this, _2));
 	//mCommitCallbackRegistrar.add("Inventory.NewWindow", boost::bind(&LLPanelMainInventory::newWindow, this));
	mCommitCallbackRegistrar.add("Inventory.ShowFilters", boost::bind(&LLPanelMainInventory::toggleFindOptions, this));
	mCommitCallbackRegistrar.add("Inventory.ResetFilters", boost::bind(&LLPanelMainInventory::resetFilters, this));
	mCommitCallbackRegistrar.add("Inventory.SetSortBy", boost::bind(&LLPanelMainInventory::setSortBy, this, _2));
	mCommitCallbackRegistrar.add("Inventory.Share",  boost::bind(&LLAvatarActions::shareWithAvatars, this));

	// <FS:Zi> Filter Links Menu
	mCommitCallbackRegistrar.add("Inventory.FilterLinks.Set", boost::bind(&LLPanelMainInventory::onFilterLinksChecked, this, _2));
	mEnableCallbackRegistrar.add("Inventory.FilterLinks.Check", boost::bind(&LLPanelMainInventory::isFilterLinksChecked, this, _2));
	// </FS:Zi> Filter Links Menu

	// <FS:Zi> Extended Inventory Search
	mCommitCallbackRegistrar.add("Inventory.SearchTarget.Set", boost::bind(&LLPanelMainInventory::onSearchTargetChecked, this, _2));
	mEnableCallbackRegistrar.add("Inventory.SearchTarget.Check", boost::bind(&LLPanelMainInventory::isSearchTargetChecked, this, _2));
	// </FS:Zi> Extended Inventory Search
	// <FS:Zi> Sort By menu handlers
	// we set up our own handlers here because the gear menu handlers are only set up
	// later in the code, so our XML based menus can't reach them yet.
	mCommitCallbackRegistrar.add("Inventory.SortBy.Set", boost::bind(&LLPanelMainInventory::setSortBy, this, _2));
	mEnableCallbackRegistrar.add("Inventory.SortBy.Check", boost::bind(&LLPanelMainInventory::isSortByChecked, this, _2));
	// </FS:Zi> Sort By menu handlers
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
	// <FS:Zi> Filter dropdown
	// create name-to-number mapping for the dropdown filter
	mFilterMap["filter_type_animations"]	= 0x01 << LLInventoryType::IT_ANIMATION;
	mFilterMap["filter_type_calling_cards"] = 0x01 << LLInventoryType::IT_CALLINGCARD;
	mFilterMap["filter_type_clothing"]	= 0x01 << LLInventoryType::IT_WEARABLE;
	mFilterMap["filter_type_gestures"]	= 0x01 << LLInventoryType::IT_GESTURE;
	mFilterMap["filter_type_landmarks"]	 = 0x01 << LLInventoryType::IT_LANDMARK;
	mFilterMap["filter_type_notecards"]	 = 0x01 << LLInventoryType::IT_NOTECARD;
	mFilterMap["filter_type_objects"]	 = 0x01 << LLInventoryType::IT_OBJECT;
	mFilterMap["filter_type_scripts"]	 = 0x01 << LLInventoryType::IT_LSL;
	mFilterMap["filter_type_sounds"]		= 0x01 << LLInventoryType::IT_SOUND;
	mFilterMap["filter_type_textures"]	= 0x01 << LLInventoryType::IT_TEXTURE;
	mFilterMap["filter_type_snapshots"]	 = 0x01 << LLInventoryType::IT_SNAPSHOT;
	mFilterMap["filter_type_meshes"]		= 0x01 << LLInventoryType::IT_MESH;
	// initialize empty filter mask
	mFilterMask = 0;
	// add filter bits to the mask
	for (std::map<std::string, U64>::iterator it = mFilterMap.begin() ; it != mFilterMap.end(); ++it)
	{
		mFilterMask |= (*it).second;
	}
	// </FS:Zi> Filter dropdown
}

BOOL LLPanelMainInventory::postBuild()
{
	gInventory.addObserver(this);
	
	// <FS:Zi> Inventory Collapse and Expand Buttons
	mCollapseBtn = getChild<LLButton>("collapse_btn");
	mCollapseBtn->setClickedCallback(boost::bind(&LLPanelMainInventory::onCollapseButtonClicked, this));
	mExpandBtn = getChild<LLButton>("expand_btn");
	mExpandBtn->setClickedCallback(boost::bind(&LLPanelMainInventory::onExpandButtonClicked, this));
	// </FS:Zi> Inventory Collapse and Expand Buttons
	mItemcountText=getChild<LLTextBox>("ItemcountText");
	mFilterTabs = getChild<LLTabContainer>("inventory filter tabs");
	mFilterTabs->setCommitCallback(boost::bind(&LLPanelMainInventory::onFilterSelected, this));
	
    mCounterCtrl = getChild<LLUICtrl>("ItemcountText");
    
	//panel->getFilter().markDefault();

	// Set up the default inv. panel/filter settings.
	mActivePanel = getChild<LLInventoryPanel>("All Items");
	if (mActivePanel)
	{
		// "All Items" is the previous only view, so it gets the InventorySortOrder
		mActivePanel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER));
		mActivePanel->setFilterLinks(LLInventoryFilter::FILTERLINK_INCLUDE_LINKS);
		mActivePanel->getFilter().markDefault();
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		mActivePanel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, mActivePanel, _1, _2));
		mResortActivePanel = true;
	}
	LLInventoryPanel* recent_items_panel = getChild<LLInventoryPanel>("Recent Items");
	if (recent_items_panel)
	{
		// assign default values until we will be sure that we have setting to restore
		recent_items_panel->setSinceLogoff(TRUE);
		recent_items_panel->setSortOrder(LLInventoryFilter::SO_DATE);
		recent_items_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		// <FS:Ansariel> FIRE-2629 / FIRE-3256: Hide links by default in recent inventory panel
		recent_items_panel->setFilterLinks(LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS);
		LLInventoryFilter& recent_filter = recent_items_panel->getFilter();
		recent_filter.setFilterObjectTypes(recent_filter.getFilterObjectTypes() & ~(0x1 << LLInventoryType::IT_CATEGORY));
		recent_filter.markDefault();
		recent_items_panel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, recent_items_panel, _1, _2));
	}

	// <FS:ND> Bring back worn items panel.
	LLInventoryPanel* worn_items_panel = getChild<LLInventoryPanel>("Worn Items");
	if (worn_items_panel)
	{
		worn_items_panel->setWorn(TRUE);
		worn_items_panel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER));
		worn_items_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		LLInventoryFilter& worn_filter = worn_items_panel->getFilter();
		worn_filter.setFilterObjectTypes(0xffffffffffffffffULL & ~(0x1 << LLInventoryType::IT_GESTURE | 0x1 << LLInventoryType::IT_CATEGORY));
		worn_filter.markDefault();

		// <FS:ND> Do not go all crazy and recurse through the whole inventory
		//		worn_items_panel->openAllFolders();
		if( worn_items_panel->getRootFolder() )
		{
			worn_items_panel->getRootFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_NO);
			worn_items_panel->getRootFolder()->arrangeAll();
		}
		// </FS:ND>

		worn_items_panel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, worn_items_panel, _1, _2));
	}
	// </FS:ND>

	// Now load the stored settings from disk, if available.
	// <polarity/> Disabled for performance reasons. Seriously.
#if 0
	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILTERS_FILENAME);
	LL_INFOS() << "LLPanelMainInventory::init: reading from " << filterSaveName.str() << LL_ENDL;
	llifstream file(filterSaveName.str());
	LLSD savedFilterState;
	if (file.is_open())
	{
		LLSDSerialize::fromXML(savedFilterState, file);
		file.close();

		// Load the persistent "Recent Items" settings.
		// Note that the "All Items" settings do not persist.
		if(recent_items_panel)
		{
			if(savedFilterState.has(recent_items_panel->getFilter().getName()))
			{
				LLSD recent_items = savedFilterState.get(
					recent_items_panel->getFilter().getName());
				LLInventoryFilter::Params p;
				LLParamSDParser parser;
				parser.readSD(recent_items, p);
				recent_items_panel->getFilter().fromParams(p);
				recent_items_panel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::RECENTITEMS_SORT_ORDER));
			}
		}

	}
#endif
// </polarity>
	mFilterEditor = getChild<LLFilterEditor>("inventory search editor");
	if (mFilterEditor)
	{
		mFilterEditor->setCommitCallback(boost::bind(&LLPanelMainInventory::onFilterEdit, this, _2));
	}
	// <FS:Zi> Filter dropdown
	mFilterComboBox = getChild<LLComboBox>("filter_combo_box");
	if (mFilterComboBox)
	{
		mFilterComboBox->setCommitCallback(boost::bind(&LLPanelMainInventory::onFilterTypeSelected, this, _2));
	}
	// </FS:Zi> Filter dropdown
	
	// <polarity> Search separator dropdown
	mSeparatorComboBox = getChild<LLComboBox>("search_separator_combo_box");
	if(mSeparatorComboBox)
	{
		mSeparatorComboBox->setCommitCallback(boost::bind(&LLPanelMainInventory::onSeparatorSelected, this, _2));
	}
	// </polarity>

	mGearMenuButton = getChild<LLMenuButton>("options_gear_btn");

	initListCommandsHandlers();

	// *TODO:Get the cost info from the server
	const std::string upload_cost("10");

	LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
	if (menu)
	{
		menu->getChild<LLMenuItemGL>("Upload Image")->setLabelArg("[COST]", upload_cost);
		menu->getChild<LLMenuItemGL>("Upload Sound")->setLabelArg("[COST]", upload_cost);
		menu->getChild<LLMenuItemGL>("Upload Animation")->setLabelArg("[COST]", upload_cost);
		menu->getChild<LLMenuItemGL>("Bulk Upload")->setLabelArg("[COST]", upload_cost);
	}

	// Trigger callback for focus received so we can deselect items in inbox/outbox
	LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLPanelMainInventory::onFocusReceived, this));

	return TRUE;
}

// Destroys the object
LLPanelMainInventory::~LLPanelMainInventory( void )
{
	// Save the filters state.
	LLSD filterRoot;
	LLInventoryPanel* all_items_panel = getChild<LLInventoryPanel>("All Items");
	if (all_items_panel)
	{
		LLSD filterState;
		LLInventoryPanel::InventoryState p;
		all_items_panel->getFilter().toParams(p.filter);
		all_items_panel->getRootViewModel().getSorter().toParams(p.sort);
		if (p.validateBlock(false))
		{
			LLParamSDParser().writeSD(filterState, p);
			filterRoot[all_items_panel->getName()] = filterState;
		}
	}

	LLInventoryPanel* panel = findChild<LLInventoryPanel>("Recent Items");
	if (panel)
	{
		LLSD filterState;
		LLInventoryPanel::InventoryState p;
		panel->getFilter().toParams(p.filter);
		panel->getRootViewModel().getSorter().toParams(p.sort);
		if (p.validateBlock(false))
		{
			LLParamSDParser().writeSD(filterState, p);
			filterRoot[panel->getName()] = filterState;
		}
	}

	// <polarity> Disabled filters.xml logic for performance reasons
#if 0
	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILTERS_FILENAME);
	llofstream filtersFile(filterSaveName.str());
	if(!LLSDSerialize::toPrettyXML(filterRoot, filtersFile))
	{
		LL_WARNS() << "Could not write to filters save file " << filterSaveName << LL_ENDL;
	}
	else
    {
		filtersFile.close();
    }
#endif
    
	gInventory.removeObserver(this);
	delete mSavedFolderState;
}

void LLPanelMainInventory::startSearch()
{
	// this forces focus to line editor portion of search editor
	if (mFilterEditor)
	{
		mFilterEditor->focusFirstItem(TRUE);
	}
}

BOOL LLPanelMainInventory::handleKeyHere(KEY key, MASK mask)
{
	LLFolderView* root_folder = mActivePanel ? mActivePanel->getRootFolder() : NULL;
	if (root_folder)
	{
		// first check for user accepting current search results
		if (mFilterEditor 
			&& mFilterEditor->hasFocus()
		    && (key == KEY_RETURN 
		    	|| key == KEY_DOWN)
		    && mask == MASK_NONE)
		{
			// move focus to inventory proper
			mActivePanel->setFocus(TRUE);
			root_folder->scrollToShowSelection();
			return TRUE;
		}

		if (mActivePanel->hasFocus() && key == KEY_UP)
		{
			startSearch();
		}
	}

	return LLPanel::handleKeyHere(key, mask);

}

//----------------------------------------------------------------------------
// menu callbacks

void LLPanelMainInventory::doToSelected(const LLSD& userdata)
{
	getPanel()->doToSelected(userdata);
}

void LLPanelMainInventory::closeAllFolders()
{
	getPanel()->getRootFolder()->closeAllFolders();
}

void LLPanelMainInventory::newWindow()
{
	static S32 instance_num = 0;
	instance_num = (instance_num + 1) % S32_MAX;

	if (!gAgentCamera.cameraMouselook())
	{
		LLFloaterReg::showTypedInstance<LLFloaterSidePanelContainer>("inventory", LLSD(instance_num));
	}
}

void LLPanelMainInventory::doCreate(const LLSD& userdata)
{
	reset_inventory_filter();
	menu_create_inventory_item(getPanel(), NULL, userdata);
}

void LLPanelMainInventory::resetFilters()
{
	LLFloaterInventoryFinder *finder = getFinder();
	getActivePanel()->getFilter().resetDefault();
	if (finder)
	{
		finder->updateElementsFromFilter();
	}

	setFilterTextFromFilter();
}

void LLPanelMainInventory::setSortBy(const LLSD& userdata)
{
	U32 sort_order_mask = getActivePanel()->getSortOrder();
	std::string sort_type = userdata.asString();
	if (sort_type == "name")
	{
		sort_order_mask &= ~LLInventoryFilter::SO_DATE;
	}
	else if (sort_type == "date")
	{
		sort_order_mask |= LLInventoryFilter::SO_DATE;
	}
	else if (sort_type == "foldersalwaysbyname")
	{
		if ( sort_order_mask & LLInventoryFilter::SO_FOLDERS_BY_NAME )
		{
			sort_order_mask &= ~LLInventoryFilter::SO_FOLDERS_BY_NAME;
		}
		else
		{
			sort_order_mask |= LLInventoryFilter::SO_FOLDERS_BY_NAME;
		}
	}
	else if (sort_type == "systemfolderstotop")
	{
		if ( sort_order_mask & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP )
		{
			sort_order_mask &= ~LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
		}
		else
		{
			sort_order_mask |= LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
		}
	}

	getActivePanel()->setSortOrder(sort_order_mask);
    if ("Recent Items" == getActivePanel()->getName())
    {
        gSavedSettings.setU32("RecentItemsSortOrder", sort_order_mask);
    }
    else
    {
        gSavedSettings.setU32("InventorySortOrder", sort_order_mask);
	}
}
BOOL LLPanelMainInventory::isSortByChecked(const LLSD& userdata)
{
	U32 sort_order_mask = getActivePanel()->getSortOrder();
	const std::string command_name = userdata.asString();
	if (command_name == "name")
	{
		return !(sort_order_mask & LLInventoryFilter::SO_DATE);
	}
	if (command_name == "date")
	{
		return (sort_order_mask & LLInventoryFilter::SO_DATE);
	}
	if (command_name == "foldersalwaysbyname")
	{
		return (sort_order_mask & LLInventoryFilter::SO_FOLDERS_BY_NAME);
	}
	if (command_name == "systemfolderstotop")
	{
		return (sort_order_mask & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP);
	}
	return FALSE;
}
// ## Zi: Sort By menu handlers

// static
BOOL LLPanelMainInventory::filtersVisible(void* user_data)
{
	LLPanelMainInventory* self = (LLPanelMainInventory*)user_data;
	if(!self) return FALSE;

	return self->getFinder() != NULL;
}

void LLPanelMainInventory::onClearSearch()
{
	BOOL initially_active = FALSE;
	LLFloater *finder = getFinder();
	if (mActivePanel)
	{
		initially_active = mActivePanel->getFilter().isNotDefault();
		mActivePanel->setFilterSubString(LLStringUtil::null);
		// <FS:Ansariel>
		//mActivePanel->setFilterTypes(0xffffffffffffffffULL);
		if (mActivePanel->getName() == "Recent Items" || mActivePanel->getName() == "Worn Items")
		{
			mActivePanel->getFilter().resetDefault();
		}
		else
		{
		mActivePanel->setFilterTypes(0xffffffffffffffffULL);
		}
		// </FS:Ansariel>
		// ## Zi: Filter Links Menu
		// We don't do this anymore, we have a menu option for it now. -Zi
		// mActivePanel->setFilterLinks(LLInventoryFilter::FILTERLINK_INCLUDE_LINKS);
		// <FS:Zi> make sure the dropdown shows "All Types" once again
		LLInventoryFilter &filter = mActivePanel->getFilter();
		updateFilterDropdown(&filter);
		// </FS:Zi>
	}

	if (finder)
	{
		LLFloaterInventoryFinder::selectAllTypes(finder);
	}

	// re-open folders that were initially open in case filter was active
	if (mActivePanel && (mFilterSubString.size() || initially_active))
	{
		mSavedFolderState->setApply(TRUE);
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		mActivePanel->getRootFolder()->applyFunctorRecursively(opener);
		mActivePanel->getRootFolder()->scrollToShowSelection();
	}
	mFilterSubString = "";
}

void LLPanelMainInventory::onFilterEdit(const std::string& search_string )
{
	if (search_string == "")
	{
		onClearSearch();
	}
	if (!mActivePanel)
	{
		return;
	}

	LLInventoryModelBackgroundFetch::instance().start();

	mFilterSubString = search_string;
	if (mActivePanel->getFilterSubString().empty() && mFilterSubString.empty())
	{
			// current filter and new filter empty, do nothing
			return;
	}

	// save current folder open state if no filter currently applied
	if (!mActivePanel->getFilter().isNotDefault())
	{
		mSavedFolderState->setApply(FALSE);
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}

	// set new filter string
	setFilterSubString(mFilterSubString);
}
// ## Zi: Filter dropdown
void LLPanelMainInventory::onFilterTypeSelected(const std::string& filter_type_name)
{
	if (!mActivePanel)
		return;
	// by default enable everything
	U64 filterTypes=~0;
	// get the pointer to the filter subwindow
	LLFloaterInventoryFinder* finder=getFinder();
	// find the filter name in our filter map
	if(mFilterMap.find(filter_type_name)!=mFilterMap.end())
	{
		filterTypes=mFilterMap[filter_type_name];
	}
	// special treatment for "all" filter
	else if(filter_type_name=="filter_type_all")
	{
		// update subwindow if it's open
		if (finder)
			LLFloaterInventoryFinder::selectAllTypes(finder);
	}
	// special treatment for "custom" filter
	else if(filter_type_name=="filter_type_custom")
	{
		// open the subwindow if needed, otherwise just give it focus
		if(!finder)
			toggleFindOptions();
		else
			finder->setFocus(TRUE);
		return;
	}
	// invalid selection (broken XML?)
	else
	{
		LL_WARNS() << "Invalid filter selection: " << filter_type_name << LL_ENDL;
		return;
	}
	mActivePanel->setFilterTypes(filterTypes);
	// update subwindow if it's open
	if(finder)
		finder->updateElementsFromFilter();
}
// reflect state of current filter selection in the dropdown list
void LLPanelMainInventory::updateFilterDropdown(const LLInventoryFilter* filter)
{
	// if we don't have a filter combobox (missing in the skin and failed to create?) do nothing
	if(!mFilterComboBox)
		return;
	// extract filter bits we need to see
	U64 filterTypes=filter->getFilterObjectTypes() & mFilterMask;
	std::string controlName;
	// check if the filter types match our filter mask, meaning "All"
	if(filterTypes==mFilterMask)
		controlName="filter_type_all";
	else
	{
		// find the name of the current filter in our filter map, if exists
		for(std::map<std::string,U64>::iterator i=mFilterMap.begin();i!=mFilterMap.end();i++)
		{
			if((*i).second==filterTypes)
			{
				controlName=(*i).first;
				break;
			}
		}
		// no filter type found in the map, must be a custom filter
		if(controlName.empty())
			controlName="filter_type_custom";
	}
	mFilterComboBox->setValue(controlName);
}
// ## Zi: Filter dropdown

void LLPanelMainInventory::onSeparatorSelected(const std::string& separator_selected)
{
	if (!mActivePanel)
		return;
	LLFloaterInventoryFinder* finder = getFinder();
	if (!separator_selected.empty())
	{
		PVSearchUtil::getInstance()->setSearchSeparator(std::stoi(separator_selected));
		// clear, then restore search string to make the new separator effective
		// TODO: Find a better way to refresh search results
		std::string old_substring = mFilterSubString;
		mActivePanel->setFilterSubString(LLStringUtil::null);
		mActivePanel->setFilterSubString(old_substring);
	}
	// invalid selection (broken XML?)
	else
	{
		LL_WARNS() << "Invalid separator selection: " << separator_selected << LL_ENDL;
		return;
	}
	//mActivePanel->setFilterTypes(filterTypes);
	if (finder)
		finder->updateElementsFromFilter();
}
// reflect state of current filter selection in the dropdown list
void LLPanelMainInventory::updateSeparatorDropdown(const LLInventoryFilter* filter)
{
	if (!mFilterComboBox)
		return;
	// extract filter bits we need to see
	U64 filterTypes = filter->getFilterObjectTypes() & mSeparatorMask;
	std::string controlName;
	// check if the filter types match our filter mask, meaning "All"
	//if (filterTypes == mSeparatorMask)
	//	controlName = "filter_type_all";
	//else
	{
		// find the name of the current filter in our filter map, if exists
		for (std::map<std::string, U64>::iterator i = mFilterMap.begin(); i != mFilterMap.end(); ++i)
		{
			if ((*i).second == filterTypes)
			{
				controlName = (*i).first;
				break;
			}
		}
		// no filter type found in the map, must be a custom filter
		//if (controlName.empty())
		//	controlName = "filter_type_custom";
	}
	mFilterComboBox->setValue(controlName);
}

 //static
 BOOL LLPanelMainInventory::incrementalFind(LLFolderViewItem* first_item, const char *find_text, BOOL backward)
 {
 	LLPanelMainInventory* active_view = NULL;
	
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLPanelMainInventory* iv = dynamic_cast<LLPanelMainInventory*>(*iter);
		if (iv)
		{
			if (gFocusMgr.childHasKeyboardFocus(iv))
			{
				active_view = iv;
				break;
			}
 		}
 	}

 	if (!active_view)
 	{
 		return FALSE;
 	}

 	std::string search_string(find_text);

 	if (search_string.empty())
 	{
 		return FALSE;
 	}

 	if (active_view->getPanel() &&
 		active_view->getPanel()->getRootFolder()->search(first_item, search_string, backward))
 	{
 		return TRUE;
 	}

 	return FALSE;
 }

void LLPanelMainInventory::onFilterSelected()
{
	// Find my index
	mActivePanel = (LLInventoryPanel*)getChild<LLTabContainer>("inventory filter tabs")->getCurrentPanel();

	if (!mActivePanel)
	{
		return;
	}

	setFilterSubString(mFilterSubString);
	LLInventoryFilter& filter = mActivePanel->getFilter();
	LLFloaterInventoryFinder *finder = getFinder();
	if (finder)
	{
		finder->changeFilter(&filter);
	}
	if (filter.isActive())
	{
		// If our filter is active we may be the first thing requiring a fetch so we better start it here.
		LLInventoryModelBackgroundFetch::instance().start();
	}
	updateFilterDropdown(&filter); // ## Zi: Filter dropdown
	setFilterTextFromFilter();
}

const std::string LLPanelMainInventory::getFilterSubString() 
{ 
	return mActivePanel->getFilterSubString(); 
}

void LLPanelMainInventory::setFilterSubString(const std::string& string) 
{ 
	mActivePanel->setFilterSubString(string); 
}

BOOL LLPanelMainInventory::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{
	// Check to see if we are auto scrolling from the last frame
	LLInventoryPanel* panel = (LLInventoryPanel*)this->getActivePanel();
	BOOL needsToScroll = panel->getScrollableContainer()->autoScroll(x, y);
	if(mFilterTabs)
	{
		if(needsToScroll)
		{
			mFilterTabs->startDragAndDropDelayTimer();
		}
	}
	
	BOOL handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

	return handled;
}

// virtual
void LLPanelMainInventory::changed(U32)
{
	updateItemcountText();
}

void LLPanelMainInventory::setFocusFilterEditor()
{
	if(mFilterEditor)
	{
		mFilterEditor->setFocus(true);
	}
}

// virtual
void LLPanelMainInventory::draw()
{
	if (mActivePanel && mFilterEditor)
	{
		mFilterEditor->setText(mFilterSubString);
	}	
	if (mActivePanel && mResortActivePanel)
	{
		// EXP-756: Force resorting of the list the first time we draw the list: 
		// In the case of date sorting, we don't have enough information at initialization time
		// to correctly sort the folders. Later manual resort doesn't do anything as the order value is 
		// set correctly. The workaround is to reset the order to alphabetical (or anything) then to the correct order.
		U32 order = mActivePanel->getSortOrder();
		mActivePanel->setSortOrder(LLInventoryFilter::SO_NAME);
		mActivePanel->setSortOrder(order);
		mResortActivePanel = false;
	}
	LLPanel::draw();
	/// <FS:CR> This really doesn't need updated every frame. changed() handles
	/// it whenever inventory changes.
	//updateItemcountText();
}

void LLPanelMainInventory::updateItemcountText()
{
	if(mItemCount != gInventory.getItemCount())
	{
		mItemCount = gInventory.getItemCount();
		mItemCountString = "";
		LLLocale locale(LLLocale::USER_LOCALE);
		LLResMgr::getInstance()->getIntegerString(mItemCountString, mItemCount);
	}

	LLStringUtil::format_map_t string_args;
	string_args["[ITEM_COUNT]"] = mItemCountString;
	string_args["[FILTER]"] = getFilterText();

	std::string text = "";

	if (LLInventoryModelBackgroundFetch::instance().folderFetchActive())
	{
		text = getString("ItemcountFetching", string_args);
	}
	else if (LLInventoryModelBackgroundFetch::instance().isEverythingFetched())
	{
		text = getString("ItemcountCompleted", string_args);
	}
	else
	{
		text = getString("ItemcountUnknown", string_args);
	}
	
    mCounterCtrl->setValue(text);
}

void LLPanelMainInventory::onFocusReceived()
{
	LLSidepanelInventory *sidepanel_inventory =	LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (!sidepanel_inventory)
	{
		LL_WARNS() << "Could not find Inventory Panel in My Inventory floater" << LL_ENDL;
		return;
	}

	sidepanel_inventory->clearSelections(false, true);
}

void LLPanelMainInventory::setFilterTextFromFilter() 
{ 
	//mFilterText = mActivePanel->getFilter().getFilterText();
	// ## Zi: Filter dropdown
	// this method gets called by the filter subwindow (once every frame), so we update our combo box here
	LLInventoryFilter &filter = mActivePanel->getFilter();
	updateFilterDropdown(&filter);
	mFilterText = filter.getFilterText();
	// ## Zi: Filter dropdown
}

void LLPanelMainInventory::toggleFindOptions()
{
	LLFloater *floater = getFinder();
	if (!floater)
	{
		LLFloaterInventoryFinder * finder = new LLFloaterInventoryFinder(this);
		mFinderHandle = finder->getHandle();
		finder->openFloater();

		LLFloater* parent_floater = gFloaterView->getParentFloater(this);
		if (parent_floater)
			parent_floater->addDependentFloater(mFinderHandle);
		// start background fetch of folders
		LLInventoryModelBackgroundFetch::instance().start();
	}
	else
	{
		floater->closeFloater();
	}
}

void LLPanelMainInventory::setSelectCallback(const LLFolderView::signal_t::slot_type& cb)
{
	getChild<LLInventoryPanel>("All Items")->setSelectCallback(cb);
	getChild<LLInventoryPanel>("Recent Items")->setSelectCallback(cb);
	getChild<LLInventoryPanel>("Worn Items")->setSelectCallback(cb);
}

void LLPanelMainInventory::onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, BOOL user_action)
{
	updateListCommands();
	panel->onSelectionChange(items, user_action);
}

///----------------------------------------------------------------------------
/// LLFloaterInventoryFinder
///----------------------------------------------------------------------------

LLFloaterInventoryFinder* LLPanelMainInventory::getFinder() 
{ 
	return (LLFloaterInventoryFinder*)mFinderHandle.get();
}


LLFloaterInventoryFinder::LLFloaterInventoryFinder(LLPanelMainInventory* inventory_view) :	
	LLFloater(LLSD()),
	mPanelMainInventory(inventory_view),
	mFilter(&inventory_view->getPanel()->getFilter())
{
	buildFromFile("floater_inventory_view_finder.xml");
	updateElementsFromFilter();
}

BOOL LLFloaterInventoryFinder::postBuild()
{
	const LLRect& viewrect = mPanelMainInventory->getRect();
	setRect(LLRect(viewrect.mLeft - getRect().getWidth(), viewrect.mTop, viewrect.mLeft, viewrect.mTop - getRect().getHeight()));

	childSetAction("All", selectAllTypes, this);
	childSetAction("None", selectNoTypes, this);

	mSpinSinceHours = getChild<LLSpinCtrl>("spin_hours_ago");
	childSetCommitCallback("spin_hours_ago", onTimeAgo, this);

	mSpinSinceDays = getChild<LLSpinCtrl>("spin_days_ago");
	childSetCommitCallback("spin_days_ago", onTimeAgo, this);

	childSetAction("Close", onCloseBtn, this);

	updateElementsFromFilter();
	return TRUE;
}
void LLFloaterInventoryFinder::onTimeAgo(LLUICtrl *ctrl, void *user_data)
{
	LLFloaterInventoryFinder *self = (LLFloaterInventoryFinder *)user_data;
	if (!self) return;
	
	if ( self->mSpinSinceDays->get() ||  self->mSpinSinceHours->get() )
	{
		self->getChild<LLUICtrl>("check_since_logoff")->setValue(false);

		U32 days = (U32)self->mSpinSinceDays->get();
		U32 hours = (U32)self->mSpinSinceHours->get();
		if (hours >= 24)
		{
			// Try to handle both cases of spinner clicking and text input in a sensible fashion as best as possible.
			// There is no way to tell if someone has clicked the spinner to get to 24 or input 24 manually, so in
			// this case add to days.  Any value > 24 means they have input the hours manually, so do not add to the
			// current day value.
			if (24 == hours)  // Got to 24 via spinner clicking or text input of 24
			{
				days = days + hours / 24;
			}
			else	// Text input, so do not add to days
			{ 
				days = hours / 24;
			}
			hours = (U32)hours % 24;
			self->mSpinSinceHours->setFocus(false);
			self->mSpinSinceDays->setFocus(false);
			self->mSpinSinceDays->set((F32)days);
			self->mSpinSinceHours->set((F32)hours);
			self->mSpinSinceHours->setFocus(true);
		}
	}
}

void LLFloaterInventoryFinder::changeFilter(LLInventoryFilter* filter)
{
	mFilter = filter;
	updateElementsFromFilter();
}

void LLFloaterInventoryFinder::updateElementsFromFilter()
{
	if (!mFilter)
		return;

	// Get data needed for filter display
	U32 filter_types = mFilter->getFilterObjectTypes();
	std::string filter_string = mFilter->getFilterSubString();
	LLInventoryFilter::EFolderShow show_folders = mFilter->getShowFolderState();
	U32 hours = mFilter->getHoursAgo();
	U32 date_search_direction = mFilter->getDateSearchDirection();

	// update the ui elements
	setTitle(mFilter->getName());

	getChild<LLUICtrl>("check_animation")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_ANIMATION));

	getChild<LLUICtrl>("check_calling_card")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_CALLINGCARD));
	getChild<LLUICtrl>("check_clothing")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_WEARABLE));
	getChild<LLUICtrl>("check_gesture")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_GESTURE));
	getChild<LLUICtrl>("check_landmark")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_LANDMARK));
	getChild<LLUICtrl>("check_mesh")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_MESH));
	getChild<LLUICtrl>("check_notecard")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_NOTECARD));
	getChild<LLUICtrl>("check_object")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_OBJECT));
	getChild<LLUICtrl>("check_script")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_LSL));
	getChild<LLUICtrl>("check_sound")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_SOUND));
	getChild<LLUICtrl>("check_texture")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_TEXTURE));
	getChild<LLUICtrl>("check_snapshot")->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_SNAPSHOT));
	getChild<LLUICtrl>("check_show_empty")->setValue(show_folders == LLInventoryFilter::SHOW_ALL_FOLDERS);
	getChild<LLUICtrl>("check_since_logoff")->setValue(mFilter->isSinceLogoff());
	mSpinSinceHours->set((F32)(hours % 24));
	mSpinSinceDays->set((F32)(hours / 24));
	getChild<LLRadioGroup>("date_search_direction")->setSelectedIndex(date_search_direction);
}

void LLFloaterInventoryFinder::draw()
{
	U64 filter = 0xffffffffffffffffULL;
	BOOL filtered_by_all_types = TRUE;

	if (!getChild<LLUICtrl>("check_animation")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_ANIMATION);
		filtered_by_all_types = FALSE;
	}


	if (!getChild<LLUICtrl>("check_calling_card")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_CALLINGCARD);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_clothing")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_WEARABLE);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_gesture")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_GESTURE);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_landmark")->getValue())


	{
		filter &= ~(0x1 << LLInventoryType::IT_LANDMARK);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_mesh")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_MESH);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_notecard")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_NOTECARD);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_object")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_OBJECT);
		filter &= ~(0x1 << LLInventoryType::IT_ATTACHMENT);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_script")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_LSL);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_sound")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_SOUND);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_texture")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_TEXTURE);
		filtered_by_all_types = FALSE;
	}

	if (!getChild<LLUICtrl>("check_snapshot")->getValue())
	{
		filter &= ~(0x1 << LLInventoryType::IT_SNAPSHOT);
		filtered_by_all_types = FALSE;
	}

	if (!filtered_by_all_types || (mPanelMainInventory->getPanel()->getFilter().getFilterTypes() & LLInventoryFilter::FILTERTYPE_DATE))
	{
		// don't include folders in filter, unless I've selected everything or filtering by date
		filter &= ~(0x1 << LLInventoryType::IT_CATEGORY);
	}

	// update the panel, panel will update the filter
	mPanelMainInventory->getPanel()->setShowFolderState(getCheckShowEmpty() ?
		LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mPanelMainInventory->getPanel()->setFilterTypes(filter);
	if (getCheckSinceLogoff())
	{
		mSpinSinceDays->set(0);
		mSpinSinceHours->set(0);
	}
	U32 days = (U32)mSpinSinceDays->get();
	U32 hours = (U32)mSpinSinceHours->get();
	if (hours >= 24)
	{
		days = hours / 24;
		hours = (U32)hours % 24;
		// A UI element that has focus will not display a new value set to it
		mSpinSinceHours->setFocus(false);
		mSpinSinceDays->setFocus(false);
		mSpinSinceDays->set((F32)days);
		mSpinSinceHours->set((F32)hours);
		mSpinSinceHours->setFocus(true);
	}
	hours += days * 24;

	mPanelMainInventory->getPanel()->setHoursAgo(hours);
	mPanelMainInventory->getPanel()->setSinceLogoff(getCheckSinceLogoff());
	mPanelMainInventory->setFilterTextFromFilter();
	mPanelMainInventory->getPanel()->setDateSearchDirection(getDateSearchDirection());

	LLPanel::draw();
}

BOOL LLFloaterInventoryFinder::getCheckShowEmpty()
{
	return getChild<LLUICtrl>("check_show_empty")->getValue();
}

BOOL LLFloaterInventoryFinder::getCheckSinceLogoff()
{
	return getChild<LLUICtrl>("check_since_logoff")->getValue();
}

U32 LLFloaterInventoryFinder::getDateSearchDirection()
{
	return 	getChild<LLRadioGroup>("date_search_direction")->getSelectedIndex();
}

void LLFloaterInventoryFinder::onCloseBtn(void* user_data)
{
	LLFloaterInventoryFinder* finderp = (LLFloaterInventoryFinder*)user_data;
	finderp->closeFloater();
}

// static
void LLFloaterInventoryFinder::selectAllTypes(void* user_data)
{
	LLFloaterInventoryFinder* self = (LLFloaterInventoryFinder*)user_data;
	if(!self) return;

	self->getChild<LLUICtrl>("check_animation")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_calling_card")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_clothing")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_gesture")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_landmark")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_mesh")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_notecard")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_object")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_script")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_sound")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_texture")->setValue(TRUE);
	self->getChild<LLUICtrl>("check_snapshot")->setValue(TRUE);
}

//static
void LLFloaterInventoryFinder::selectNoTypes(void* user_data)
{
	LLFloaterInventoryFinder* self = (LLFloaterInventoryFinder*)user_data;
	if(!self) return;

	self->getChild<LLUICtrl>("check_animation")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_calling_card")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_clothing")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_gesture")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_landmark")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_mesh")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_notecard")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_object")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_script")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_sound")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_texture")->setValue(FALSE);
	self->getChild<LLUICtrl>("check_snapshot")->setValue(FALSE);
}

// ## Zi: Inventory Collapse and Expand Buttons
void LLPanelMainInventory::onCollapseButtonClicked()
{
	// <polarity> Don't clear search filter.
	//mFilterEditor->clear();
	//onFilterEdit("");
	getPanel()->closeAllFolders();
}
void LLPanelMainInventory::onExpandButtonClicked()
{
	getPanel()->openAllFolders();
}
// ## Zi: Inventory Collapse and Expand Buttons
//////////////////////////////////////////////////////////////////////////////////
// List Commands                                                                //

void LLPanelMainInventory::initListCommandsHandlers()
{
	childSetAction("trash_btn", boost::bind(&LLPanelMainInventory::onTrashButtonClick, this));
	childSetAction("add_btn", boost::bind(&LLPanelMainInventory::onAddButtonClick, this));

	mTrashButton = getChild<LLDragAndDropButton>("trash_btn");
	mTrashButton->setDragAndDropHandler(boost::bind(&LLPanelMainInventory::handleDragAndDropToTrash, this
			,	_4 // BOOL drop
			,	_5 // EDragAndDropType cargo_type
			,	_7 // EAcceptance* accept
			));

	mCommitCallbackRegistrar.add("Inventory.GearDefault.Custom.Action", boost::bind(&LLPanelMainInventory::onCustomAction, this, _2));
	mEnableCallbackRegistrar.add("Inventory.GearDefault.Check", boost::bind(&LLPanelMainInventory::isActionChecked, this, _2));
	mEnableCallbackRegistrar.add("Inventory.GearDefault.Enable", boost::bind(&LLPanelMainInventory::isActionEnabled, this, _2));
	mMenuGearDefault = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_inventory_gear_default.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mGearMenuButton->setMenu(mMenuGearDefault);
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_inventory_add.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mMenuAddHandle = menu->getHandle();

	// Update the trash button when selected item(s) get worn or taken off.
	LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLPanelMainInventory::updateListCommands, this));
}

void LLPanelMainInventory::updateListCommands()
{
	bool trash_enabled = isActionEnabled("delete");

	mTrashButton->setEnabled(trash_enabled);
}

void LLPanelMainInventory::onAddButtonClick()
{
// Gray out the "New Folder" option when the Recent tab is active as new folders will not be displayed
// unless "Always show folders" is checked in the filter options.
	bool recent_active = ("Recent Items" == mActivePanel->getName());
	LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
	if (menu)
	{
		menu->getChild<LLMenuItemGL>("New Folder")->setEnabled(!recent_active);

		setUploadCostIfNeeded();

		showActionMenu(menu,"add_btn");
	}
}

void LLPanelMainInventory::showActionMenu(LLMenuGL* menu, std::string spawning_view_name)
{
	if (menu)
	{
		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLView* spawning_view = getChild<LLView> (spawning_view_name);
		S32 menu_x, menu_y;
		//show menu in co-ordinates of panel
		spawning_view->localPointToOtherView(0, spawning_view->getRect().getHeight(), &menu_x, &menu_y, this);
		menu_y += menu->getRect().getHeight();
		LLMenuGL::showPopup(this, menu, menu_x, menu_y);
	}
}

void LLPanelMainInventory::onTrashButtonClick()
{
	onClipboardAction("delete");
}

void LLPanelMainInventory::onClipboardAction(const LLSD& userdata)
{
	std::string command_name = userdata.asString();
	getActivePanel()->doToSelected(command_name);
}

void LLPanelMainInventory::saveTexture(const LLSD& userdata)
{
	LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return;
	}
	
	const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
	LLPreviewTexture* preview_texture = LLFloaterReg::showTypedInstance<LLPreviewTexture>("preview_texture", LLSD(item_id), TAKE_FOCUS_YES);
	if (preview_texture)
	{
		preview_texture->openToSave();
	}
}

void LLPanelMainInventory::onCustomAction(const LLSD& userdata)
{
	if (!isActionEnabled(userdata))
		return;

	const std::string command_name = userdata.asString();
	if (command_name == "new_window")
	{
		newWindow();
	}
	if (command_name == "sort_by_name")
	{
		const LLSD arg = "name";
		setSortBy(arg);
	}
	if (command_name == "sort_by_recent")
	{
		const LLSD arg = "date";
		setSortBy(arg);
	}
	if (command_name == "sort_folders_by_name")
	{
		const LLSD arg = "foldersalwaysbyname";
		setSortBy(arg);
	}
	if (command_name == "sort_system_folders_to_top")
	{
		const LLSD arg = "systemfolderstotop";
		setSortBy(arg);
	}
	if (command_name == "show_filters")
	{
		toggleFindOptions();
	}
	if (command_name == "reset_filters")
	{
		resetFilters();
	}
	if (command_name == "close_folders")
	{
		closeAllFolders();
	}
	if (command_name == "empty_trash")
	{
		const std::string notification = "ConfirmEmptyTrash";
		gInventory.emptyFolderType(notification, LLFolderType::FT_TRASH);
	}
	if (command_name == "empty_lostnfound")
	{
		const std::string notification = "ConfirmEmptyLostAndFound";
		gInventory.emptyFolderType(notification, LLFolderType::FT_LOST_AND_FOUND);
	}
	if (command_name == "save_texture")
	{
		saveTexture(userdata);
	}
	// This doesn't currently work, since the viewer can't change an assetID an item.
	if (command_name == "regenerate_link")
	{
		LLInventoryPanel *active_panel = getActivePanel();
		LLFolderViewItem* current_item = active_panel->getRootFolder()->getCurSelectedItem();
		if (!current_item)
		{
			return;
		}
		const LLUUID item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		if (item)
		{
			item->regenerateLink();
		}
		active_panel->setSelection(item_id, TAKE_FOCUS_NO);
	}
	if (command_name == "find_original")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item)
		{
			return;
		}
		static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->performAction(getActivePanel()->getModel(), "goto");
	}

	if (command_name == "find_links")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item)
		{
			return;
		}
		const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
		const std::string &item_name = current_item->getViewModelItem()->getName();
		mFilterSubString = item_name;

		LLInventoryFilter &filter = mActivePanel->getFilter();
		filter.setFindAllLinksMode(item_name, item_id);

		mFilterEditor->setText(item_name);
		mFilterEditor->setFocus(TRUE);
	}
}

void LLPanelMainInventory::onVisibilityChange( BOOL new_visibility )
{
	if(!new_visibility)
	{
		LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
		if (menu)
		{
			menu->setVisible(FALSE);
		}
		getActivePanel()->getRootFolder()->finishRenamingItem();
	}
}

bool LLPanelMainInventory::isSaveTextureEnabled(const LLSD& userdata)
{
	LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (current_item) 
	{
		LLViewerInventoryItem *inv_item = dynamic_cast<LLViewerInventoryItem*>(static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getInventoryObject());
		if(inv_item)
		{
			bool can_save = inv_item->checkPermissionsSet(PERM_ITEM_UNRESTRICTED);
			LLInventoryType::EType curr_type = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getInventoryType();
			return can_save && (curr_type == LLInventoryType::IT_TEXTURE || curr_type == LLInventoryType::IT_SNAPSHOT);
		}
	}
	return false;
}

BOOL LLPanelMainInventory::isActionEnabled(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "delete")
	{
		return getActivePanel()->isSelectionRemovable();
	}
	if (command_name == "save_texture")
	{
		return isSaveTextureEnabled(userdata);
	}
	if (command_name == "find_original")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item) return FALSE;
		const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
		const LLViewerInventoryItem *item = gInventory.getItem(item_id);
		if (item && item->getIsLinkType() && !item->getIsBrokenLink())
		{
			return TRUE;
		}
		return FALSE;
	}

	if (command_name == "find_links")
	{
		LLFolderView* root = getActivePanel()->getRootFolder();
		std::set<LLFolderViewItem*> selection_set = root->getSelectionList();
		if (selection_set.size() != 1) return FALSE;
		LLFolderViewItem* current_item = root->getCurSelectedItem();
		if (!current_item) return FALSE;
		const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
		const LLInventoryObject *obj = gInventory.getObject(item_id);
		if (obj && !obj->getIsLinkType() && LLAssetType::lookupCanLink(obj->getType()))
		{
			return TRUE;
		}
		return FALSE;
	}
	// This doesn't currently work, since the viewer can't change an assetID an item.
	if (command_name == "regenerate_link")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item) return FALSE;
		const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
		const LLViewerInventoryItem *item = gInventory.getItem(item_id);
		if (item && item->getIsBrokenLink())
		{
			return TRUE;
		}
		return FALSE;
	}

	if (command_name == "share")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item) return FALSE;
		LLSidepanelInventory* parent = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
		return parent ? parent->canShare() : FALSE;
	}

	return TRUE;
}

BOOL LLPanelMainInventory::isActionChecked(const LLSD& userdata)
{
	U32 sort_order_mask = getActivePanel()->getSortOrder();
	const std::string command_name = userdata.asString();
	if (command_name == "sort_by_name")
	{
		return ~sort_order_mask & LLInventoryFilter::SO_DATE;
	}

	if (command_name == "sort_by_recent")
	{
		return sort_order_mask & LLInventoryFilter::SO_DATE;
	}

	if (command_name == "sort_folders_by_name")
	{
		return sort_order_mask & LLInventoryFilter::SO_FOLDERS_BY_NAME;
	}

	if (command_name == "sort_system_folders_to_top")
	{
		return sort_order_mask & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
	}

	return FALSE;
}

// ## Zi: Filter Links Menu
void LLPanelMainInventory::onFilterLinksChecked(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "show_links")
	{
		getActivePanel()->setFilterLinks(LLInventoryFilter::FILTERLINK_INCLUDE_LINKS);
	}

	if (command_name == "only_links")
	{
		getActivePanel()->setFilterLinks(LLInventoryFilter::FILTERLINK_ONLY_LINKS);
	}

	if (command_name == "hide_links")
	{
		getActivePanel()->setFilterLinks(LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS);
	}
}

BOOL LLPanelMainInventory::isFilterLinksChecked(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "show_links")
	{
		return (getActivePanel()->getFilter().getFilterLinks() == LLInventoryFilter::FILTERLINK_INCLUDE_LINKS);
	}

	if (command_name == "only_links")
	{
		return (getActivePanel()->getFilter().getFilterLinks() == LLInventoryFilter::FILTERLINK_ONLY_LINKS);
	}

	if (command_name == "hide_links")
	{
		return (getActivePanel()->getFilter().getFilterLinks() == LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS);
	}

	return FALSE;
}
// ## Zi: Filter Links Menu

// ## Zi: Extended Inventory Search
void LLPanelMainInventory::onSearchTargetChecked(const LLSD& userdata)
{
	getActivePanel()->setFilterSubStringTarget(userdata.asString());
	resetFilters();
}

LLInventoryFilter::EFilterSubstringTarget LLPanelMainInventory::getSearchTarget() const
{
	return getActivePanel()->getFilterSubStringTarget();
}

BOOL LLPanelMainInventory::isSearchTargetChecked(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "name")
	{
		return (getSearchTarget()==LLInventoryFilter::SUBST_TARGET_NAME);
	}

	if (command_name == "creator")
	{
		return (getSearchTarget()==LLInventoryFilter::SUBST_TARGET_CREATOR);
	}

	if (command_name == "description")
	{
		return (getSearchTarget()==LLInventoryFilter::SUBST_TARGET_DESCRIPTION);
	}

	if (command_name == "uuid")
	{
		return (getSearchTarget()==LLInventoryFilter::SUBST_TARGET_UUID);
	}

	if (command_name == "all")
	{
		return (getSearchTarget()==LLInventoryFilter::SUBST_TARGET_ALL);
	}
	return FALSE;
}
// ## Zi: Extended Inventory Search

bool LLPanelMainInventory::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept)
{
	*accept = ACCEPT_NO;

	const bool is_enabled = isActionEnabled("delete");
	if (is_enabled) *accept = ACCEPT_YES_MULTI;

	if (is_enabled && drop)
	{
		onClipboardAction("delete");
	}
	return true;
}

void LLPanelMainInventory::setUploadCostIfNeeded()
{
	// *NOTE dzaporozhan
	// Upload cost is set in process_economy_data() (llviewermessage.cpp). But since we
	// have two instances of Inventory panel at the moment(and two instances of context menu),
	// call to gMenuHolder->childSetLabelArg() sets upload cost only for one of the instances.

	LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
	if(mNeedUploadCost && menu)
	{
		LLMenuItemBranchGL* upload_menu = menu->findChild<LLMenuItemBranchGL>("upload");
		if(upload_menu)
		{
			S32 upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
			std::string cost_str;

			// getPriceUpload() returns -1 if no data available yet.
			if(upload_cost >= 0)
			{
				mNeedUploadCost = false;
				cost_str = llformat("%d", upload_cost);
			}
			else
			{
				cost_str = llformat("%d", gSavedSettings.getU32("DefaultUploadCost"));
			}

			upload_menu->getChild<LLView>("Upload Image")->setLabelArg("[COST]", cost_str);
			upload_menu->getChild<LLView>("Upload Sound")->setLabelArg("[COST]", cost_str);
			upload_menu->getChild<LLView>("Upload Animation")->setLabelArg("[COST]", cost_str);
			upload_menu->getChild<LLView>("Bulk Upload")->setLabelArg("[COST]", cost_str);
		}
	}
}

// List Commands                                                              //
////////////////////////////////////////////////////////////////////////////////
