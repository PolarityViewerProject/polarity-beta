/** 
 * @file lllayoutstack.h
 * @author Richard Nelson
 * @brief LLLayout class - dynamic stacking of UI elements
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLLAYOUTSTACK_H
#define LL_LLLAYOUTSTACK_H

#include "llpanel.h"

class LLPanel;

class LLLayoutPanel;

class LLLayoutStack : public LLView, public LLInstanceTracker<LLLayoutStack>
{
public:
	struct LayoutStackRegistry : public LLChildRegistry<LayoutStackRegistry>
	{};

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string>	orientation;
		Optional<S32>			border_size;
		Optional<bool>			animate,
								clip;

		Params();
	};

	typedef LayoutStackRegistry child_registry_t;

	typedef enum e_layout_orientation
	{
		HORIZONTAL,
		VERTICAL
	} ELayoutOrientation;

	virtual ~LLLayoutStack();

	/*virtual*/ void draw();
	/*virtual*/ void removeChild(LLView*);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ bool addChild(LLView* child, S32 tab_group = 0);

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	S32 getMinWidth() const { return mMinWidth; }
	S32 getMinHeight() const { return mMinHeight; }
	
	typedef enum e_animate
	{
		NO_ANIMATE,
		ANIMATE
	} EAnimate;

	void addPanel(LLLayoutPanel* panel, EAnimate animate = NO_ANIMATE);
	void removePanel(LLPanel* panel);
	void collapsePanel(LLPanel* panel, BOOL collapsed = TRUE);
	S32 getNumPanels() { return mPanels.size(); }

	void updatePanelAutoResize(const std::string& panel_name, BOOL auto_resize);
	void setPanelUserResize(const std::string& panel_name, BOOL user_resize);
	
	/**
	 * Gets minimal dimension along layout_stack axis of the specified by name panel.
	 *
	 * @returns true if specified by panel_name internal panel exists, false otherwise.
	 */
	bool getPanelMinSize(const std::string& panel_name, S32* min_dimp);

	/**
	 * Gets maximal dimension along layout_stack axis of the specified by name panel.
	 *
	 * @returns true if specified by panel_name internal panel exists, false otherwise.
	 */
	bool getPanelMaxSize(const std::string& panel_name, S32* max_dim);
	
	void updateLayout(BOOL force_resize = FALSE);
	
	S32 getPanelSpacing() const { return mPanelSpacing; }
	BOOL getAnimate () const { return mAnimate; }
	void setAnimate (BOOL animate) { mAnimate = animate; }
	
	static void updateClass();

protected:
	LLLayoutStack(const Params&);
	friend class LLUICtrlFactory;

private:
	void createResizeBars();
	void calcMinExtents();
	S32 getDefaultHeight(S32 cur_height);
	S32 getDefaultWidth(S32 cur_width);

	const ELayoutOrientation mOrientation;

	typedef std::vector<LLLayoutPanel*> e_panel_list_t;
	e_panel_list_t mPanels;

	LLLayoutPanel* findEmbeddedPanel(LLPanel* panelp) const;
	LLLayoutPanel* findEmbeddedPanelByName(const std::string& name) const;

	S32 mMinWidth;  // calculated by calcMinExtents
	S32 mMinHeight;  // calculated by calcMinExtents
	S32 mPanelSpacing;

	// true if we already applied animation this frame
	bool mAnimatedThisFrame;
	bool mAnimate;
	bool mClip;
}; // end class LLLayoutStack

class LLLayoutPanel : public LLPanel
{
friend LLLayoutStack;
friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<S32>			min_dim,
								max_dim;
		Optional<bool>			user_resize,
								auto_resize;

		Params()
		:	min_dim("min_dim", 0),
			max_dim("max_dim", 0),
			user_resize("user_resize", true),
			auto_resize("auto_resize", true)
		{
			addSynonym(min_dim, "min_width");
			addSynonym(min_dim, "min_height");
			addSynonym(max_dim, "max_width");
			addSynonym(max_dim, "max_height");
		}
	};

	~LLLayoutPanel();
protected:
	LLLayoutPanel(const Params& p)	;

	
	F32 getCollapseFactor(LLLayoutStack::ELayoutOrientation orientation);

	S32 mMinDim;
	S32 mMaxDim;
	BOOL mAutoResize;
	BOOL mUserResize;
	BOOL mCollapsed;
	class LLResizeBar* mResizeBar;
	F32 mVisibleAmt;
	F32 mCollapseAmt;
};


#endif
