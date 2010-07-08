/**
 * @file llwearableitemslist.cpp
 * @brief A flat list of wearable items.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llwearableitemslist.h"

#include "lliconctrl.h"
#include "llmenugl.h" // for LLContextMenu

#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "lltransutil.h"
#include "llviewerattachmenu.h"
#include "llvoavatarself.h"

class LLFindOutfitItems : public LLInventoryCollectFunctor
{
public:
	LLFindOutfitItems() {}
	virtual ~LLFindOutfitItems() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
};

bool LLFindOutfitItems::operator()(LLInventoryCategory* cat,
								   LLInventoryItem* item)
{
	if(item)
	{
		if((item->getType() == LLAssetType::AT_CLOTHING)
		   || (item->getType() == LLAssetType::AT_BODYPART)
		   || (item->getType() == LLAssetType::AT_OBJECT))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void LLPanelWearableListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLPanelInventoryListItemBase::onMouseEnter(x, y, mask);
	setWidgetsVisible(true);
	reshapeWidgets();
}

void LLPanelWearableListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLPanelInventoryListItemBase::onMouseLeave(x, y, mask);
	setWidgetsVisible(false);
	reshapeWidgets();
}

LLPanelWearableListItem::LLPanelWearableListItem(LLViewerInventoryItem* item)
: LLPanelInventoryListItemBase(item)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// static
LLPanelWearableOutfitItem* LLPanelWearableOutfitItem::create(LLViewerInventoryItem* item,
															 bool worn_indication_enabled)
{
	LLPanelWearableOutfitItem* list_item = NULL;
	if (item)
	{
		list_item = new LLPanelWearableOutfitItem(item, worn_indication_enabled);
		list_item->init();
	}
	return list_item;
}

LLPanelWearableOutfitItem::LLPanelWearableOutfitItem(LLViewerInventoryItem* item,
													 bool worn_indication_enabled)
: LLPanelInventoryListItemBase(item)
, mWornIndicationEnabled(worn_indication_enabled)
{
}

// virtual
void LLPanelWearableOutfitItem::updateItem(const std::string& name,
										   EItemState item_state)
{
	std::string search_label = name;

	if (mWornIndicationEnabled && get_is_item_worn(mInventoryItemUUID))
	{
		search_label += LLTrans::getString("worn");
		item_state = IS_WORN;
	}

	LLPanelInventoryListItemBase::updateItem(search_label, item_state);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// static
LLPanelClothingListItem* LLPanelClothingListItem::create(LLViewerInventoryItem* item)
{
	LLPanelClothingListItem* list_item = NULL;
	if(item)
	{
		list_item = new LLPanelClothingListItem(item);
		list_item->init();
	}
	return list_item;
}

LLPanelClothingListItem::LLPanelClothingListItem(LLViewerInventoryItem* item)
 : LLPanelDeletableWearableListItem(item)
{
}

LLPanelClothingListItem::~LLPanelClothingListItem()
{
}

void LLPanelClothingListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_clothing_list_item.xml");
}

BOOL LLPanelClothingListItem::postBuild()
{
	LLPanelDeletableWearableListItem::postBuild();

	addWidgetToRightSide("btn_move_up");
	addWidgetToRightSide("btn_move_down");
	addWidgetToRightSide("btn_lock");
	addWidgetToRightSide("btn_edit_panel");

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// static
LLPanelBodyPartsListItem* LLPanelBodyPartsListItem::create(LLViewerInventoryItem* item)
{
	LLPanelBodyPartsListItem* list_item = NULL;
	if(item)
	{
		list_item = new LLPanelBodyPartsListItem(item);
		list_item->init();
	}
	return list_item;
}

LLPanelBodyPartsListItem::LLPanelBodyPartsListItem(LLViewerInventoryItem* item)
: LLPanelWearableListItem(item)
{
}

LLPanelBodyPartsListItem::~LLPanelBodyPartsListItem()
{
}

void LLPanelBodyPartsListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_body_parts_list_item.xml");
}

BOOL LLPanelBodyPartsListItem::postBuild()
{
	LLPanelInventoryListItemBase::postBuild();

	addWidgetToRightSide("btn_lock");
	addWidgetToRightSide("btn_edit_panel");

	return TRUE;
}


// static
LLPanelDeletableWearableListItem* LLPanelDeletableWearableListItem::create(LLViewerInventoryItem* item)
{
	LLPanelDeletableWearableListItem* list_item = NULL;
	if(item)
	{
		list_item = new LLPanelDeletableWearableListItem(item);
		list_item->init();
	}
	return list_item;
}

LLPanelDeletableWearableListItem::LLPanelDeletableWearableListItem(LLViewerInventoryItem* item)
: LLPanelWearableListItem(item)
{
}

void LLPanelDeletableWearableListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_deletable_wearable_list_item.xml");
}

BOOL LLPanelDeletableWearableListItem::postBuild()
{
	LLPanelWearableListItem::postBuild();

	addWidgetToLeftSide("btn_delete");

	LLButton* delete_btn = getChild<LLButton>("btn_delete");
	// Reserve space for 'delete' button event if it is invisible.
	setLeftWidgetsWidth(delete_btn->getRect().mRight);

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}


// static
LLPanelAttachmentListItem* LLPanelAttachmentListItem::create(LLViewerInventoryItem* item)
{
	LLPanelAttachmentListItem* list_item = NULL;
	if(item)
	{
		list_item = new LLPanelAttachmentListItem(item);
		list_item->init();
	}
	return list_item;
}

void LLPanelAttachmentListItem::updateItem(const std::string& name,
										   EItemState item_state)
{
	std::string title_joint = name;

	LLViewerInventoryItem* inv_item = getItem();
	if (inv_item && isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(inv_item->getLinkedUUID()))
	{
		std::string joint = LLTrans::getString(gAgentAvatarp->getAttachedPointName(inv_item->getLinkedUUID()));
		title_joint =  title_joint + " (" + joint + ")";
	}

	LLPanelInventoryListItemBase::updateItem(title_joint, item_state);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelDummyClothingListItem* LLPanelDummyClothingListItem::create(LLWearableType::EType w_type)
{
	LLPanelDummyClothingListItem* list_item = new LLPanelDummyClothingListItem(w_type);
	list_item->init();
	return list_item;
}

BOOL LLPanelDummyClothingListItem::postBuild()
{
	LLIconCtrl* icon = getChild<LLIconCtrl>("item_icon");
	setIconCtrl(icon);
	setTitleCtrl(getChild<LLTextBox>("item_name"));

	addWidgetToRightSide("btn_add_panel");

	setIconImage(LLInventoryIcon::getIcon(LLAssetType::AT_CLOTHING, LLInventoryType::IT_NONE, mWearableType, FALSE));
	updateItem(wearableTypeToString(mWearableType));

	// Make it look loke clothing item - reserve space for 'delete' button
	setLeftWidgetsWidth(icon->getRect().mLeft);

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

LLWearableType::EType LLPanelDummyClothingListItem::getWearableType() const
{
	return mWearableType;
}

LLPanelDummyClothingListItem::LLPanelDummyClothingListItem(LLWearableType::EType w_type)
 : LLPanelWearableListItem(NULL)
 , mWearableType(w_type)
{
}

void LLPanelDummyClothingListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_dummy_clothing_list_item.xml");
}

typedef std::map<LLWearableType::EType, std::string> clothing_to_string_map_t;

clothing_to_string_map_t init_clothing_string_map()
{
	clothing_to_string_map_t w_map;
	w_map.insert(std::make_pair(LLWearableType::WT_SHIRT, "shirt_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_PANTS, "pants_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_SHOES, "shoes_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_SOCKS, "socks_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_JACKET, "jacket_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_GLOVES, "gloves_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_UNDERSHIRT, "undershirt_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_UNDERPANTS, "underpants_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_SKIRT, "skirt_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_ALPHA, "alpha_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_TATTOO, "tattoo_not_worn"));
	return w_map;
}

std::string LLPanelDummyClothingListItem::wearableTypeToString(LLWearableType::EType w_type)
{
	static const clothing_to_string_map_t w_map = init_clothing_string_map();
	static const std::string invalid_str = LLTrans::getString("invalid_not_worn");
	
	std::string type_str = invalid_str;
	clothing_to_string_map_t::const_iterator it = w_map.find(w_type);
	if(w_map.end() != it)
	{
		type_str = LLTrans::getString(it->second);
	}
	return type_str;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*virtual*/
bool LLWearableItemNameComparator::doCompare(const LLPanelInventoryListItemBase* wearable_item1, const LLPanelInventoryListItemBase* wearable_item2) const
{
	std::string name1 = wearable_item1->getItemName();
	std::string name2 = wearable_item2->getItemName();

	LLStringUtil::toUpper(name1);
	LLStringUtil::toUpper(name2);

	return name1 < name2;
}

/*virtual*/
bool LLWearableItemTypeNameComparator::doCompare(const LLPanelInventoryListItemBase* wearable_item1, const LLPanelInventoryListItemBase* wearable_item2) const
{
	const LLAssetType::EType item_type1 = wearable_item1->getType();
	const LLAssetType::EType item_type2 = wearable_item2->getType();

	LLWearableItemTypeNameComparator::ETypeListOrder item_type_order1 = getTypeListOrder(item_type1);
	LLWearableItemTypeNameComparator::ETypeListOrder item_type_order2 = getTypeListOrder(item_type2);

	if (item_type_order1 != item_type_order2)
	{
		// If items are of different asset types we can compare them
		// by types order in the list.
		return item_type_order1 < item_type_order2;
	}

	if (item_type_order1 & TLO_SORTABLE_BY_NAME)
	{
		// If both items are of the same asset type except AT_CLOTHING and AT_BODYPART
		// we can compare them by name.
		return LLWearableItemNameComparator::doCompare(wearable_item1, wearable_item2);
	}

	const LLWearableType::EType item_wearable_type1 = wearable_item1->getWearableType();
	const LLWearableType::EType item_wearable_type2 = wearable_item2->getWearableType();

	if (item_wearable_type1 != item_wearable_type2)
	{
		// If items are of different clothing types they are compared
		// by clothing types order determined in LLWearableType::EType.
		return item_wearable_type1 < item_wearable_type2;
	}
	else
	{
		// If both items are of the same clothing type they are compared
		// by description and place in reverse order i.e. outer layer item
		// on top.
		return wearable_item1->getDescription() > wearable_item2->getDescription();
	}
}

// static
LLWearableItemTypeNameComparator::ETypeListOrder LLWearableItemTypeNameComparator::getTypeListOrder(LLAssetType::EType item_type)
{
	switch (item_type)
	{
	case LLAssetType::AT_OBJECT:
		return TLO_ATTACHMENT;

	case LLAssetType::AT_CLOTHING:
		return TLO_CLOTHING;

	case LLAssetType::AT_BODYPART:
		return TLO_BODYPART;

	default:
		return TLO_UNKNOWN;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static const LLWearableItemTypeNameComparator WEARABLE_TYPE_NAME_COMPARATOR;

static const LLDefaultChildRegistry::Register<LLWearableItemsList> r("wearable_items_list");

LLWearableItemsList::Params::Params()
:	standalone("standalone", true)
,	worn_indication_enabled("worn_indication_enabled", true)
{}

LLWearableItemsList::LLWearableItemsList(const LLWearableItemsList::Params& p)
:	LLInventoryItemsList(p)
{
	setComparator(&WEARABLE_TYPE_NAME_COMPARATOR);
	mIsStandalone = p.standalone;
	if (mIsStandalone)
	{
		// Use built-in context menu.
		setRightMouseDownCallback(boost::bind(&LLWearableItemsList::onRightClick, this, _2, _3));
	}
	mWornIndicationEnabled = p.worn_indication_enabled;
}

// virtual
LLWearableItemsList::~LLWearableItemsList()
{}

// virtual
void LLWearableItemsList::addNewItem(LLViewerInventoryItem* item, bool rearrange /*= true*/)
{
	if (!item)
	{
		llwarns << "No inventory item. Couldn't create flat list item." << llendl;
		llassert(item != NULL);
	}

	LLPanelWearableOutfitItem *list_item = LLPanelWearableOutfitItem::create(item, mWornIndicationEnabled);
	if (!list_item)
		return;

	bool is_item_added = addItem(list_item, item->getUUID(), ADD_BOTTOM, rearrange);
	if (!is_item_added)
	{
		llwarns << "Couldn't add flat list item." << llendl;
		llassert(is_item_added);
	}
}

void LLWearableItemsList::updateList(const LLUUID& category_id)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;

	LLFindOutfitItems collector = LLFindOutfitItems();
	// collectDescendentsIf takes non-const reference:
	gInventory.collectDescendentsIf(
		category_id,
		cat_array,
		item_array,
		LLInventoryModel::EXCLUDE_TRASH,
		collector);

	refreshList(item_array);
}

void LLWearableItemsList::updateChangedItems(const LLInventoryModel::changed_items_t& changed_items_uuids)
{
	// nothing to update
	if (changed_items_uuids.empty()) return;

	typedef std::vector<LLPanel*> item_panel_list_t;

	item_panel_list_t items;
	getItems(items);

	for (item_panel_list_t::iterator items_iter = items.begin();
			items_iter != items.end();
			++items_iter)
	{
		LLPanelInventoryListItemBase* item = dynamic_cast<LLPanelInventoryListItemBase*>(*items_iter);
		if (!item) continue;

		LLViewerInventoryItem* inv_item = item->getItem();
		if (!inv_item) continue;

		LLUUID linked_uuid = inv_item->getLinkedUUID();

		for (LLInventoryModel::changed_items_t::const_iterator iter = changed_items_uuids.begin();
				iter != changed_items_uuids.end();
				++iter)
		{
			if (linked_uuid == *iter)
			{
				item->setNeedsRefresh(true);
				break;
			}
		}
	}
}

void LLWearableItemsList::onRightClick(S32 x, S32 y)
{
	uuid_vec_t selected_uuids;

	getSelectedUUIDs(selected_uuids);
	if (selected_uuids.empty())
	{
		return;
	}

	ContextMenu::instance().show(this, selected_uuids, x, y);
}

//////////////////////////////////////////////////////////////////////////
/// ContextMenu
//////////////////////////////////////////////////////////////////////////

LLWearableItemsList::ContextMenu::ContextMenu()
:	mParent(NULL)
{
}

void LLWearableItemsList::ContextMenu::show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y)
{
	mParent = dynamic_cast<LLWearableItemsList*>(spawning_view);
	LLListContextMenu::show(spawning_view, uuids, x, y);
	mParent = NULL; // to avoid dereferencing an invalid pointer
}

// virtual
LLContextMenu* LLWearableItemsList::ContextMenu::createMenu()
{
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	const uuid_vec_t& ids = mUUIDs;		// selected items IDs
	LLUUID selected_id = ids.front();	// ID of the first selected item

	functor_t wear = boost::bind(&LLAppearanceMgr::wearItemOnAvatar, LLAppearanceMgr::getInstance(), _1, true, true);
	functor_t add = boost::bind(&LLAppearanceMgr::wearItemOnAvatar, LLAppearanceMgr::getInstance(), _1, true, false);
	functor_t take_off = boost::bind(&LLAppearanceMgr::removeItemFromAvatar, LLAppearanceMgr::getInstance(), _1);

	// Register handlers common for all wearable types.
	registrar.add("Wearable.Wear", boost::bind(handleMultiple, wear, ids));
	registrar.add("Wearable.Add", boost::bind(handleMultiple, add, ids));
	registrar.add("Wearable.Edit", boost::bind(handleMultiple, LLAgentWearables::editWearable, ids));
	registrar.add("Wearable.CreateNew", boost::bind(createNewWearable, selected_id));
	registrar.add("Wearable.ShowOriginal", boost::bind(show_item_original, selected_id));
	registrar.add("Wearable.TakeOffDetach", boost::bind(handleMultiple, take_off, ids));

	// Register handlers for clothing.
	registrar.add("Clothing.TakeOff", boost::bind(handleMultiple, take_off, ids));

	// Register handlers for body parts.

	// Register handlers for attachments.
	registrar.add("Attachment.Detach", boost::bind(handleMultiple, take_off, ids));
	registrar.add("Attachment.Profile", boost::bind(show_item_profile, selected_id));
	registrar.add("Object.Attach", boost::bind(LLViewerAttachMenu::attachObjects, ids, _2));

	// Create the menu.
	LLContextMenu* menu = createFromFile("menu_wearable_list_item.xml");

	// Determine which items should be visible/enabled.
	updateItemsVisibility(menu);

	// Update labels for the items requiring that.
	updateItemsLabels(menu);
	return menu;
}

void LLWearableItemsList::ContextMenu::updateItemsVisibility(LLContextMenu* menu)
{
	if (!menu)
	{
		llwarns << "Invalid menu" << llendl;
		return;
	}

	const uuid_vec_t& ids = mUUIDs;	// selected items IDs
	U32 mask = 0;					// mask of selected items' types
	U32 n_items = ids.size();		// number of selected items
	U32 n_worn = 0;					// number of worn items among the selected ones
	U32 n_already_worn = 0;			// number of items worn of same type as selected items
	U32 n_links = 0;				// number of links among the selected items
	U32 n_editable = 0;				// number of editable items among the selected ones

	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLUUID id = *it;
		LLViewerInventoryItem* item = gInventory.getItem(id);

		if (!item)
		{
			llwarns << "Invalid item" << llendl;
			// *NOTE: the logic below may not work in this case
			continue;
		}

		updateMask(mask, item->getType());

		const LLWearableType::EType wearable_type = item->getWearableType();
		const bool is_link = item->getIsLinkType();
		const bool is_worn = get_is_item_worn(id);
		const bool is_editable = gAgentWearables.isWearableModifiable(id);
		const bool is_already_worn = gAgentWearables.selfHasWearable(wearable_type);
		if (is_worn)
		{
			++n_worn;
		}
		if (is_editable)
		{
			++n_editable;
		}
		if (is_link)
		{
			++n_links;
		}
		if (is_already_worn)
		{
			++n_already_worn;
		}
	} // for

	bool standalone = mParent ? mParent->isStandalone() : false;

	// *TODO: eliminate multiple traversals over the menu items
	setMenuItemVisible(menu, "wear_wear", 			n_already_worn == 0);
	setMenuItemEnabled(menu, "wear_wear", 			n_already_worn == 0);
	setMenuItemVisible(menu, "wear_add",			mask == MASK_CLOTHING && n_worn == 0 && n_already_worn != 0);
	setMenuItemEnabled(menu, "wear_add",			n_items == 1 && canAddWearable(ids.front()) && n_already_worn != 0);
	setMenuItemVisible(menu, "wear_replace",		n_worn == 0 && n_already_worn != 0);
	//visible only when one item selected and this item is worn
	setMenuItemVisible(menu, "edit",				!standalone && mask & (MASK_CLOTHING|MASK_BODYPART) && n_worn == n_items && n_worn == 1);
	setMenuItemEnabled(menu, "edit",				n_editable == 1 && n_worn == 1 && n_items == 1);
	setMenuItemVisible(menu, "create_new",			mask & (MASK_CLOTHING|MASK_BODYPART) && n_items == 1);
	setMenuItemVisible(menu, "show_original",		!standalone);
	setMenuItemEnabled(menu, "show_original",		n_items == 1 && n_links == n_items);
	setMenuItemVisible(menu, "take_off",			mask == MASK_CLOTHING && n_worn == n_items);
	setMenuItemVisible(menu, "detach",				mask == MASK_ATTACHMENT && n_worn == n_items);
	setMenuItemVisible(menu, "take_off_or_detach",	mask == (MASK_ATTACHMENT|MASK_CLOTHING));
	setMenuItemEnabled(menu, "take_off_or_detach",	n_worn == n_items);
	setMenuItemVisible(menu, "object_profile",		!standalone);
	setMenuItemEnabled(menu, "object_profile",		n_items == 1);
	setMenuItemVisible(menu, "--no options--", 		FALSE);
	setMenuItemEnabled(menu, "--no options--",		FALSE);

	// Populate or hide the "Attach to..." / "Attach to HUD..." submenus.
	if (mask == MASK_ATTACHMENT && n_worn == 0)
	{
		LLViewerAttachMenu::populateMenus("wearable_attach_to", "wearable_attach_to_hud");
	}
	else
	{
		setMenuItemVisible(menu, "wearable_attach_to",			false);
		setMenuItemVisible(menu, "wearable_attach_to_hud",		false);
	}

	if (mask & MASK_UNKNOWN)
	{
		llwarns << "Non-wearable items passed." << llendl;
	}

	U32 num_visible_items = 0;
	for (U32 menu_item_index = 0; menu_item_index < menu->getItemCount(); ++menu_item_index)
	{
		const LLMenuItemGL* menu_item = menu->getItem(menu_item_index);
		if (menu_item && menu_item->getVisible())
		{
			num_visible_items++;
		}
	}
	if (num_visible_items == 0)
	{
		setMenuItemVisible(menu, "--no options--", TRUE);
	}
}

void LLWearableItemsList::ContextMenu::updateItemsLabels(LLContextMenu* menu)
{
	llassert(menu);
	if (!menu) return;

	// Set proper label for the "Create new <WEARABLE_TYPE>" menu item.
	LLViewerInventoryItem* item = gInventory.getLinkedItem(mUUIDs.back());
	if (!item || !item->isWearableType()) return;

	LLWearableType::EType w_type = item->getWearableType();
	std::string new_label = LLTrans::getString("create_new_" + LLWearableType::getTypeName(w_type));

	LLMenuItemGL* menu_item = menu->getChild<LLMenuItemGL>("create_new");
	menu_item->setLabel(new_label);
}

// We need this method to convert non-zero BOOL values to exactly 1 (TRUE).
// Otherwise code relying on a BOOL value being TRUE may fail
// (I experienced a weird assert in LLView::drawChildren() because of that.
// static
void LLWearableItemsList::ContextMenu::setMenuItemVisible(LLContextMenu* menu, const std::string& name, bool val)
{
	menu->setItemVisible(name, val);
}

// static
void LLWearableItemsList::ContextMenu::setMenuItemEnabled(LLContextMenu* menu, const std::string& name, bool val)
{
	menu->setItemEnabled(name, val);
}

// static
void LLWearableItemsList::ContextMenu::updateMask(U32& mask, LLAssetType::EType at)
{
	if (at == LLAssetType::AT_CLOTHING)
	{
		mask |= MASK_CLOTHING;
	}
	else if (at == LLAssetType::AT_BODYPART)
	{
		mask |= MASK_BODYPART;
	}
	else if (at == LLAssetType::AT_OBJECT)
	{
		mask |= MASK_ATTACHMENT;
	}
	else
	{
		mask |= MASK_UNKNOWN;
	}
}

// static
void LLWearableItemsList::ContextMenu::createNewWearable(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = gInventory.getLinkedItem(item_id);
	if (!item || !item->isWearableType()) return;

	LLAgentWearables::createWearable(item->getWearableType(), true);
}

// Can we wear another wearable of the given item's wearable type?
// static
bool LLWearableItemsList::ContextMenu::canAddWearable(const LLUUID& item_id)
{
	// TODO: investigate wearables may not be loaded at this point EXT-8231

	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (!item || item->getType() != LLAssetType::AT_CLOTHING)
	{
		return false;
	}

	U32 wearable_count = gAgentWearables.getWearableCount(item->getWearableType());
	return wearable_count < LLAgentWearables::MAX_CLOTHING_PER_TYPE;
}

// EOF
