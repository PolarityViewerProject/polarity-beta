/**
 * @file fsareasearch.h
 * @brief Floater to search and list objects in view or is known to the viewer.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (c) 2012 Techwolf Lupindo
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
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#ifndef FS_AREASEARCH_H
#define FS_AREASEARCH_H

#include "llfloater.h"
#include "llsingleton.h"
#include "llframetimer.h"
#include "llsaleinfo.h"
#include "llcategory.h"
#include "llpermissions.h"
#include "llviewerobject.h"
#include <boost/regex.hpp>
#include "llscrolllistcolumn.h"

class LLTextBox;
class LLViewerRegion;
class LLCheckBoxCtrl;
class LLLineEditor;
class LLTabContainer;
class LLContextMenu;
class LLSpinCtrl;
class LLComboBox;

class FSPanelAreaSearchList;
class FSPanelAreaSearchFind;
class FSPanelAreaSearchFilter;
class FSPanelAreaSearchAdvanced;
class FSPanelAreaSearchOptions;
class FSScrollListCtrl;

struct FSObjectProperties
{
	LLUUID id;
	bool listed;
	std::string name;
	std::string description;
	std::string touch_name;
	std::string sit_name;
	LLUUID creator_id;
	LLUUID owner_id;
	LLUUID group_id;
	LLUUID ownership_id;
	BOOL group_owned; // used in LL function that uses BOOL instead of bool
	U64 creation_date;
	U32 base_mask, owner_mask, group_mask, everyone_mask, next_owner_mask;
	LLSaleInfo sale_info;
	LLCategory category;
	LLUUID last_owner_id;
	LLAggregatePermissions ag_perms;
	LLAggregatePermissions ag_texture_perms;
	LLAggregatePermissions ag_texture_perms_owner;
	LLPermissions permissions;
	uuid_vec_t texture_ids;
	bool name_requested;
	U32 local_id;
	U64 region_handle;

	typedef enum e_object_properties_request
	{
		NEED,
		SENT,
		FINISHED,
		FAILED
	} EObjectPropertiesRequest;

	EObjectPropertiesRequest request;

	FSObjectProperties() :
		listed(false),
		group_owned(0),
		creation_date(0),
		base_mask(0),
		owner_mask(0),
		group_mask(0),
		everyone_mask(0),
		next_owner_mask(0),
		name_requested(false),
		local_id(0),
		region_handle(0),
		request(NEED)
	{
	}
};

//---------------------------------------------------------------------
// Main class for area search
// holds the search engine and main floater
// --------------------------------------------------------------------
class FSAreaSearch : public LLSingleton<FSAreaSearch>, public LLFloater
{
	LOG_CLASS(FSAreaSearch);
public:
	FSAreaSearch(const LLSD&);
	virtual ~FSAreaSearch();

	/*virtual*/
	BOOL postBuild() override;
	void draw() override;
	void onOpen(const LLSD& key) override;

	void callbackLoadFullName(const LLUUID& id, const std::string& full_name);
	void processObjectProperties(LLMessageSystem* msg);
	void updateObjectCosts(const LLUUID& object_id, F32 object_cost, F32 link_cost, F32 physics_cost, F32 link_physics_cost) const;
	static void idle(void* user_data);

	void checkRegion();
	void refreshList(bool cache_clear);
	void onCommitLine();
	void clearSearchText();
	void onButtonClickedSearch();
	void onCommitCheckboxRegex();
	bool isSearchableObject(LLViewerObject* objectp, LLViewerRegion* our_region) const;

	std::map<LLUUID, FSObjectProperties> mObjectDetails;

	FSPanelAreaSearchAdvanced* getPanelAdvanced() const
	{
		return mPanelAdvanced;
	}

	FSPanelAreaSearchList* getPanelList() const
	{
		return mPanelList;
	}

	void setFilterForSale(bool b)
	{
		mFilterForSale = b;
	}

	void setFilterLocked(bool b)
	{
		mFilterLocked = b;
	}

	void setFilterPhysical(bool b)
	{
		mFilterPhysical = b;
	}

	void setFilterTemporary(bool b)
	{
		mFilterTemporary = b;
	}

	void setFilterPhantom(bool b)
	{
		mFilterPhantom = b;
	}

	void setFilterAttachment(bool b)
	{
		mFilterAttachment = b;
	}

	void setFilterMoaP(bool b)
	{
		mFilterMoaP = b;
	}

	void setRegexSearch(bool b)
	{
		mRegexSearch = b;
	}

	void setBeacons(bool b)
	{
		mBeacons = b;
	}

	void setExcludeAttachment(bool b)
	{
		mExcludeAttachment = b;
	}

	void setExcludetemporary(bool b)
	{
		mExcludeTemporary = b;
	}

	void setExcludePhysics(bool b)
	{
		mExcludePhysics = b;
	}

	void setExcludeChildPrims(bool b)
	{
		mExcludeChildPrims = b;
	}

	void setExcludeNeighborRegions(bool b)
	{
		mExcludeNeighborRegions = b;
	}

	void setFilterForSaleMin(S32 s)
	{
		mFilterForSaleMin = s;
	}

	void setFilterForSaleMax(S32 s)
	{
		mFilterForSaleMax = s;
	}

	void setFilterClickAction(bool b)
	{
		mFilterClickAction = b;
	}

	void setFilterClickActionType(U8 u)
	{
		mFilterClickActionType = u;
	}

	void setFilterDistance(bool b)
	{
		mFilterDistance = b;
	}

	void setFilterDistanceMin(S32 s)
	{
		mFilterDistanceMin = s;
	}

	void setFilterDistanceMax(S32 s)
	{
		mFilterDistanceMax = s;
	}

	void setFilterPermCopy(bool b)
	{
		mFilterPermCopy = b;
	}

	void setFilterPermModify(bool b)
	{
		mFilterPermModify = b;
	}

	void setFilterPermTransfer(bool b)
	{
		mFilterPermTransfer = b;
	}

	void setFilterAgentParcelOnly(bool b)
	{
		mFilterAgentParcelOnly = b;
	}

	bool isActive() const
	{
		return mActive;
	}

private:
	void requestObjectProperties(const std::vector<U32>& request_list, bool select, LLViewerRegion* regionp) const;
	void matchObject(FSObjectProperties& details, LLViewerObject* objectp);
	void getNameFromUUID(LLUUID& id, std::string& name, BOOL group, bool& name_requested);

	void updateCounterText() const;
	bool regexTest(std::string text) const;
	void findObjects();
	void processRequestQueue();

	S32 mRequested;
	bool mRefresh;
	S32 mSearchableObjects;
	bool mActive;
	bool mRequestQueuePause;
	bool mRequestNeedsSent;
	std::map<U64, S32> mRegionRequests;

	std::string mSearchName;
	std::string mSearchDescription;
	std::string mSearchOwner;
	std::string mSearchGroup;
	std::string mSearchCreator;
	std::string mSearchLastOwner;

	bool mRegexSearch;
	boost::regex mRegexSearchName;
	boost::regex mRegexSearchDescription;
	boost::regex mRegexSearchOwner;
	boost::regex mRegexSearchGroup;
	boost::regex mRegexSearchCreator;
	boost::regex mRegexSearchLastOwner;

	LLFrameTimer mLastUpdateTimer;
	LLFrameTimer mLastPropertiesReceivedTimer;

	uuid_vec_t mNamesRequested;

	LLViewerRegion* mLastRegion;

	class FSParcelChangeObserver;
	friend class FSParcelChangeObserver;
	FSParcelChangeObserver* mParcelChangedObserver;

	// Used for checking to see if a floater has been created.
	// Can not be trusted as a singleton pointer, don't use as a pointer.
	FSAreaSearch* mInstance;

	LLTabContainer* mTab;
	FSPanelAreaSearchList* mPanelList;
	FSPanelAreaSearchFind* mPanelFind;
	FSPanelAreaSearchFilter* mPanelFilter;
	FSPanelAreaSearchOptions* mPanelOptions;
	FSPanelAreaSearchAdvanced* mPanelAdvanced;

	bool mBeacons;
	LLColor4 mBeaconColor;
	LLColor4 mBeaconTextColor;

	bool mExcludeAttachment;
	bool mExcludeTemporary;
	bool mExcludePhysics;
	bool mExcludeChildPrims;
	bool mExcludeNeighborRegions;

	bool mFilterLocked;
	bool mFilterPhysical;
	bool mFilterTemporary;
	bool mFilterPhantom;
	bool mFilterAttachment;
	bool mFilterMoaP;

	bool mFilterForSale;
	S32 mFilterForSaleMin;
	S32 mFilterForSaleMax;

	bool mFilterDistance;
	S32 mFilterDistanceMin;
	S32 mFilterDistanceMax;

	bool mFilterClickAction;
	U8 mFilterClickActionType;

	bool mFilterPermCopy;
	bool mFilterPermModify;
	bool mFilterPermTransfer;

	bool mFilterAgentParcelOnly;

protected:
	static void* createPanelList(void* data);
	static void* createPanelFind(void* data);
	static void* createPanelFilter(void* data);
	static void* createPanelAdvanced(void* data);
	static void* createPanelOptions(void* data);
};


//------------------------------------------------------------
// List panel
// displays the found objects
//------------------------------------------------------------
class FSPanelAreaSearchList
	: public LLPanel
{
	LOG_CLASS(FSPanelAreaSearchList);
	friend class FSAreaSearchMenu;
	friend class FSPanelAreaSearchOptions;

public:
	FSPanelAreaSearchList(FSAreaSearch* pointer);
	virtual ~FSPanelAreaSearchList();

	/*virtual*/
	BOOL postBuild() override;

	void setCounterText() const;
	void setCounterText(LLStringUtil::format_map_t args) const;
	void updateScrollList();
	void updateName(LLUUID id, std::string name) const;
	static void touchObject(LLViewerObject* objectp);

	FSScrollListCtrl* getResultList() const
	{
		return mResultList;
	}

	void updateResultListColumns();

	void setAgentLastPosition(LLVector3d d)
	{
		mAgentLastPosition = d;
	}

	LLVector3d getAgentLastPosition() const
	{
		return mAgentLastPosition;
	}

private:
	void onDoubleClick() const;
	void onClickRefresh() const;
	void buyObject(FSObjectProperties& details, LLViewerObject* objectp) const;
	void onCommitCheckboxBeacons() const;

	bool onContextMenuItemClick(const LLSD& userdata) const;
	bool onContextMenuItemEnable(const LLSD& userdata) const;

	void onColumnVisibilityChecked(const LLSD& userdata);
	bool onEnableColumnVisibilityChecked(const LLSD& userdata);

	LLVector3d mAgentLastPosition;

	FSAreaSearch* mFSAreaSearch;
	LLButton* mRefreshButton;
	FSScrollListCtrl* mResultList;
	LLCheckBoxCtrl* mCheckboxBeacons;
	LLTextBox* mCounterText;

	std::map<std::string, U32> mColumnBits;
	boost::signals2::connection mFSAreaSearchColumnConfigConnection;

	U32 mColumnConfig;
};


//------------------------------------------------------------
// Find panel
// UI for what objects to search for
//------------------------------------------------------------
class FSPanelAreaSearchFind
	: public LLPanel
{
	LOG_CLASS(FSPanelAreaSearchFind);
public:
	FSPanelAreaSearchFind(FSAreaSearch* pointer);
	virtual ~FSPanelAreaSearchFind();

	/*virtual*/
	BOOL postBuild() override;
	/*virtual*/
	BOOL handleKeyHere(KEY key, MASK mask) override;

	LLLineEditor* mNameLineEditor;
	LLLineEditor* mDescriptionLineEditor;
	LLLineEditor* mOwnerLineEditor;
	LLLineEditor* mGroupLineEditor;
	LLLineEditor* mCreatorLineEditor;
	LLLineEditor* mLastOwnerLineEditor;
	LLCheckBoxCtrl* mCheckboxRegex;

private:
	void onButtonClickedClear() const;

	FSAreaSearch* mFSAreaSearch;

	LLButton* mSearchButton;
	LLButton* mClearButton;
};


//------------------------------------------------------------
// Filter panel
// "filter" the list to certion object types.
//------------------------------------------------------------
class FSPanelAreaSearchFilter
	: public LLPanel
{
	LOG_CLASS(FSPanelAreaSearchFilter);
public:
	FSPanelAreaSearchFilter(FSAreaSearch* pointer);
	virtual ~FSPanelAreaSearchFilter();

	/*virtual*/
	BOOL postBuild() override;

private:
	void onCommitCheckbox() const;
	void onCommitSpin() const;
	void onCommitCombo() const;

	FSAreaSearch* mFSAreaSearch;
	LLCheckBoxCtrl* mCheckboxForSale;
	LLCheckBoxCtrl* mCheckboxPhysical;
	LLCheckBoxCtrl* mCheckboxTemporary;
	LLCheckBoxCtrl* mCheckboxLocked;
	LLCheckBoxCtrl* mCheckboxPhantom;
	LLCheckBoxCtrl* mCheckboxMoaP;
	LLCheckBoxCtrl* mCheckboxDistance;
	LLSpinCtrl* mSpinDistanceMinValue;
	LLSpinCtrl* mSpinDistanceMaxValue;
	LLSpinCtrl* mSpinForSaleMinValue;
	LLSpinCtrl* mSpinForSaleMaxValue;
	LLButton* mButtonApply;
	LLComboBox* mComboClickAction;
	LLCheckBoxCtrl* mCheckboxAttachment;
	LLCheckBoxCtrl* mCheckboxExcludeAttachment;
	LLCheckBoxCtrl* mCheckboxExcludePhysics;
	LLCheckBoxCtrl* mCheckboxExcludetemporary;
	LLCheckBoxCtrl* mCheckboxExcludeChildPrim;
	LLCheckBoxCtrl* mCheckboxExcludeNeighborRegions;
	LLCheckBoxCtrl* mCheckboxPermCopy;
	LLCheckBoxCtrl* mCheckboxPermModify;
	LLCheckBoxCtrl* mCheckboxPermTransfer;
	LLCheckBoxCtrl* mCheckboxAgentParcelOnly;
};


//------------------------------------------------------------
// Options panel
//------------------------------------------------------------
class FSPanelAreaSearchOptions
	: public LLPanel
{
	LOG_CLASS(FSPanelAreaSearchOptions);
public:
	FSPanelAreaSearchOptions(FSAreaSearch* pointer);
	virtual ~FSPanelAreaSearchOptions();

	// not used
	//	/*virtual*/ BOOL postBuild();

private:
	void onCommitCheckboxDisplayColumn(const LLSD& userdata) const;
	bool onEnableColumnVisibilityChecked(const LLSD& userdata) const;

	FSAreaSearch* mFSAreaSearch;

	std::map<std::string, LLScrollListColumn::Params> mColumnParms;
};


//------------------------------------------------------------
// Advanced panel
//------------------------------------------------------------
class FSPanelAreaSearchAdvanced
	: public LLPanel
{
	LOG_CLASS(FSPanelAreaSearchAdvanced);
public:
	FSPanelAreaSearchAdvanced(FSAreaSearch* pointer);
	virtual ~FSPanelAreaSearchAdvanced();

	/*virtual*/
	BOOL postBuild() override;

	LLCheckBoxCtrl* mCheckboxClickTouch;
	LLCheckBoxCtrl* mCheckboxClickBuy;

private:
	FSAreaSearch* mFSAreaSearch;
};

#endif // FS_AREASEARCH_H


