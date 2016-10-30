/**
 * @file pvdata.cpp
 * @brief Downloadable metadata for viewer features.
 * Inspired by FSData by Techwolf Lupindo
 * Re-implented by Xenhat Liamano
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Polarity Viewer Source Code
 * Copyright (C) 2015 Xenhat Liamano
 * Portions Copyright (C)
 *  2011 Wolfspirit Magi
 *  2011-2013 Techwolf Lupindo
 *  2012 Ansariel Hiller @ Second Life
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "pvdata.h"

/* boost: will not compile unless equivalent is undef'd, beware. */
#include "fix_macros.h"
#include <boost/spirit/include/karma.hpp>

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloaterabout.h"
#include "llmutelist.h"
#include "llprogressview.h"
#include "llsdserialize.h"
#include "llstartup.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "noise.h"

#include <stdlib.h> // for setenv
#include "llfloaterpreference.h"

PVData* gPVData = NULL;

static const std::string LL_LINDEN = "Linden";
static const std::string LL_MOLE = "Mole";
static const std::string LL_PRODUCTENGINE = "ProductEngine";
static const std::string LL_SCOUT = "Scout";
static const std::string LL_TESTER = "Tester";

std::vector<std::string> PVData::PVSearchSeparatorAssociation
{
	" ",
	"+",
	",",
	"|",
	";",
	".",
	":",
};

static U32 PVSearchSeparatorSelected = gPVData->separator_space;


void PVData::init(const bool testing_branch)
{
	pv_url_remote_base_string_ = "https://data.polarityviewer.org/" + (testing_branch ? std::string("test/") : std::string("live/")) + std::to_string(6) + "/";
}

// ##     ## ######## ######## ########     ##        #######   ######   ####  ######
// ##     ##    ##       ##    ##     ##    ##       ##     ## ##    ##   ##  ##    ##
// ##     ##    ##       ##    ##     ##    ##       ##     ## ##         ##  ##
// #########    ##       ##    ########     ##       ##     ## ##   ####  ##  ##
// ##     ##    ##       ##    ##           ##       ##     ## ##    ##   ##  ##
// ##     ##    ##       ##    ##           ##       ##     ## ##    ##   ##  ##    ##
// ##     ##    ##       ##    ##           ########  #######   ######   ####  ######

// Local timeout override to ensure we don't abort too soon
const F32 HTTP_TIMEOUT = 30.f;

#if LL_DARWIN
size_t strnlen(const char *s, size_t n)
{
	const char *p = (const char *)memchr(s, 0, n);
	return(p ? p-s : n);
}
#endif // LL_DARWIN

// We make exception of the coding style guide here because this class is only used internally, and is not
// interacted directly with when hooking up to PVData from other files

// Crappy port

void downloadComplete( LLSD const &aData, std::string const &aURL )
{
	//LL_DEBUGS() << aData << LL_ENDL;

	LLSD header = aData[ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS ][ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];

	LLDate lastModified;
	if (header.has("last-modified"))
	{
		lastModified.secondsSinceEpoch( PVCommon::secondsSinceEpochFromString( "%a, %d %b %Y %H:%M:%S %ZP", header["last-modified"].asString() ) );
	}

	LLSD data = aData;
	data.erase( LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS );

	gPVData->handleResponseFromServer(data, aURL, true);
}

void downloadCompleteScript( LLSD const &aData, std::string const &aURL, std::string const &aFilename  )
{
	LLSD header = aData[ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS ][ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];
	LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD( aData[ LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS ] );

	LLDate lastModified;
	if (header.has("last-modified"))
	{
		lastModified.secondsSinceEpoch( PVCommon::secondsSinceEpochFromString( "%a, %d %b %Y %H:%M:%S %ZP", header["last-modified"].asString() ) );
	}
	const LLSD::Binary &rawData = aData[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();

	if ( status.getType() == HTTP_NOT_MODIFIED )
	{
		LL_INFOS("PVData") << "Got [304] not modified for " << aURL << LL_ENDL;
		return;
	}
	if (rawData.size() <= 0)
	{
		LL_WARNS("PVData") << "Received zero data for " << aURL << LL_ENDL;
		return;
	}
}

void downloadError( LLSD const &aData, std::string const &aURL )
{
	LL_WARNS() << "Failed to download " << aURL << ": " << aData << LL_ENDL;
	gPVData->handleResponseFromServer(aData, aURL, false);
}

// ########   #######  ##      ## ##    ## ##        #######     ###    ########  ######## ########
// ##     ## ##     ## ##  ##  ## ###   ## ##       ##     ##   ## ##   ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ####  ## ##       ##     ##  ##   ##  ##     ## ##       ##     ##
// ##     ## ##     ## ##  ##  ## ## ## ## ##       ##     ## ##     ## ##     ## ######   ########
// ##     ## ##     ## ##  ##  ## ##  #### ##       ##     ## ######### ##     ## ##       ##   ##
// ##     ## ##     ## ##  ##  ## ##   ### ##       ##     ## ##     ## ##     ## ##       ##    ##
// ########   #######   ###  ###  ##    ## ########  #######  ##     ## ########  ######## ##     ##

void PVData::modularDownloader(const std::string& pfile_name_in)
{
	// Sets up the variables we need for each object. Avoids call bloat in the class constructor.
	pvdata_user_agent_ = LLViewerMedia::getCurrentUserAgent();
	pvdata_viewer_version_ = LLVersionInfo::getChannelAndVersionStatic(); 
	static LLCachedControl<bool> pvdata_testing_branch(gSavedSettings, "PVData_UseTestingDataSource", FALSE);
	if (pvdata_testing_branch != pv_downloader_testing_branch)
	{
		PVData::init(pvdata_testing_branch);
		pv_downloader_testing_branch = pvdata_testing_branch;
	}
	
	// construct download url from file name
	headers_.insert("User-Agent", pvdata_user_agent_);
	headers_.insert("viewer-version", pvdata_viewer_version_);
	// FIXME: This is ugly
	//pvdata_modular_remote_url_full_ = pv_url_remote_base_string_ + pfile_name_in;
	if (pfile_name_in == pv_file_name_data_string_)
	{
		pvdata_url_full_ = pv_url_remote_base_string_ + pfile_name_in;
	}
	else if (pfile_name_in == pv_file_name_agents_string_)
	{
		pvdata_agents_url_full_ = pv_url_remote_base_string_ + pfile_name_in;
	}

	PV_DEBUG("Downloading " + pfile_name_in + " from " + pv_url_remote_base_string_ + pfile_name_in, LLError::LEVEL_INFO);
	// TODO: HTTP eTag support
	//LLHTTPClient::get(pvdata_modular_remote_url_full_, new PVDataDownloader(pvdata_modular_remote_url_full_, pfile_name_in), headers_, HTTP_TIMEOUT);
	LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpGet(pv_url_remote_base_string_ + pfile_name_in, boost::bind( downloadComplete, _1, pv_url_remote_base_string_ + pfile_name_in),
		boost::bind( downloadError, _1, pv_url_remote_base_string_ + pfile_name_in) );
}

void PVData::downloadData()
{
	if (canDownload(data_download_status_))
	{
		data_parse_status_ = INIT;
		modularDownloader("data.xml");
	}
}

void PVData::downloadAgents()
{
	if (canDownload(agents_download_status_))
	{
		agents_parse_status_ = INIT;
		modularDownloader("agents.xml");
	}
}

void PVData::handleResponseFromServer(const LLSD& http_content,
	const std::string& http_source_url,
	//const std::string& data_file_name,
	const bool& parse_success
	// TODO: re-implement last-modified support
	//const bool& http_failure
	//const LLDate& last_modified
	)
{
	static LLCachedControl<bool> dump_web_data(gSavedSettings, "PVDebug_DumpWebData", false);
	if (dump_web_data)
	{
		Dump(http_source_url, http_content);
	}
	//PV_DEBUG("Examining HTTP response for " + http_source_url, LLError::LEVEL_INFO);
	PV_DEBUG("http_content=" + http_content.asString(), LLError::LEVEL_DEBUG);
	//PV_DEBUG("http_source_url=" + http_source_url, LLError::LEVEL_DEBUG);
	//PV_DEBUG("data_file_name=" + data_file_name);
	PV_DEBUG("parse_success=" + parse_success, LLError::LEVEL_DEBUG);
	//PV_DEBUG("http_failure=" + http_failure);

	// Set status to OK here for now.
	data_parse_status_ = agents_parse_status_ = OK;
	if (http_source_url == pvdata_url_full_)
	{
		PV_DEBUG("Received a DATA file", LLError::LEVEL_DEBUG);
		if (!parse_success)
		{
			LL_WARNS("PVData") << "DATA Parse failure, aborting." << LL_ENDL;
			data_parse_status_ = PARSE_FAILURE;
			handleDataFailure();
		}
		else
		{
			data_parse_status_ = INIT;
			
			//data_parse_status_ = INIT; // Don't reset here, that would defeat the purpose.
			parsePVData(http_content);
		}
	}
	if (http_source_url == pvdata_agents_url_full_)
	{
		PV_DEBUG("Received an AGENTS file", LLError::LEVEL_DEBUG);
		if (!parse_success)
		{
			LL_WARNS("PVData") << " AGENTS Parse failure, aborting." << LL_ENDL;
			agents_parse_status_ = PARSE_FAILURE;
			handleAgentsFailure();
		}
		else
		{
			agents_parse_status_ = INIT;
			//agents_parse_status_ = INIT; // Don't reset here, that would defeat the purpose.
			parsePVAgents(http_content);
		}
	}
}

// ########     ###    ########   ######  ######## ########   ######
// ##     ##   ## ##   ##     ## ##    ## ##       ##     ## ##    ##
// ##     ##  ##   ##  ##     ## ##       ##       ##     ## ##
// ########  ##     ## ########   ######  ######   ########   ######
// ##        ######### ##   ##         ## ##       ##   ##         ##
// ##        ##     ## ##    ##  ##    ## ##       ##    ##  ##    ##
// ##        ##     ## ##     ##  ######  ######## ##     ##  ######

bool PVData::canParse(size_t& status_container) const
{
	PV_DEBUG("Checking parse status", LLError::LEVEL_DEBUG);
	switch (status_container)
	{
	case INIT:
		return true;
	case PARSING:
		LL_WARNS("PVData") << "Parser is already running, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		return false;
	case OK:
		LL_WARNS("PVData") << "Parser already completed, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		return false;
	default:
		LL_WARNS("PVData") << "Parser encountered a problem and has aborted. Parsing disabled. (STATUS='" << status_container << "')" << LL_ENDL;
		status_container = UNDEFINED;
		return false;
	}
}

bool PVData::canDownload(size_t& status_container) const
{
	PV_DEBUG("Checking download status", LLError::LEVEL_DEBUG);
	switch (status_container)
	{
	case INIT:
		return true;
	case PARSING:
		LL_WARNS("PVData") << "Download already in progress, skipping. (STATUS='" << status_container << "')" << LL_ENDL;
		return false;
	case OK:
		// todo: standardize with canParse
		return true;
	case DOWNLOAD_FAILURE:
		LL_WARNS("PVData") << "Failed to download and will retry later (STATUS='" << status_container << "')" << LL_ENDL;
		return true;
	default:
		LL_WARNS("PVData") << "Download checks failed, download aborted. (STATUS='" << status_container << "')" << LL_ENDL;
		status_container = UNDEFINED;
		return false;
	}
}

void PVData::handleDataFailure()
{
	// Ideally, if data is not present, the user should be treated as a normal resident
	LL_WARNS("PVData") << "Something went wrong downloading data file" << LL_ENDL;

	gAgent.mMOTD.assign("COULD NOT CONTACT MOTD SERVER");
	data_download_status_ = DOWNLOAD_FAILURE;
}
void PVData::handleAgentsFailure()
{
	LL_WARNS("PVData") << "Something went wrong downloading agents file" << LL_ENDL;
	pv_agent_access_[LLUUID("f56e83a9-da38-4230-bac8-b146e7035dfc")] = FLAG_USER_BANNED;
	pv_agent_access_[LLUUID("6b7c1d1b-fc8a-4b11-9202-707e99b4a89a")] = FLAG_USER_BANNED;
	pv_agent_access_[LLUUID("584d796a-bb85-4fe9-8f7c-1f2fbf2ff164")] = FLAG_STAFF_DEV | FLAG_STAFF_QA; // Darl
	pv_agent_access_[LLUUID("f1a73716-4ad2-4548-9f0e-634c7a98fe86")] = FLAG_STAFF_DEV; // Xenhat
	pv_agent_access_[LLUUID("238afefc-74ec-4afe-a59a-9fe1400acd92")] = FLAG_STAFF_DEV;
	pv_agent_access_[LLUUID("a43d30fe-e2f6-4ef5-8502-2335879ec6b1")] = FLAG_STAFF_SUPPORT;
	agents_download_status_ = DOWNLOAD_FAILURE;
}

void PVData::parsePVData(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset data_parse_status_ when parsing is needed again.
	if (!canParse(data_parse_status_))
	{
		// FIXME: why do we get 'data_parse_status_==PARSING' BEFORE it's actually being set? (see below)
		LL_WARNS("PVData") << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}

	PV_DEBUG("Beginning to parse Data", LLError::LEVEL_DEBUG);
	data_parse_status_ = PARSING;
	PV_DEBUG("Attempting to find Blocked Releases", LLError::LEVEL_DEBUG);
	if (data_input.has("BlockedReleases"))
	{
		PV_DEBUG("Populating Blocked Releases list...", LLError::LEVEL_DEBUG);
		const LLSD& blocked = data_input["BlockedReleases"];
		for (LLSD::map_const_iterator iter = blocked.beginMap(); iter != blocked.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			blocked_versions_[version] = reason;
			PV_DEBUG("Added " + version + " to blocked_versions_ with reason '" + reason.asString() + "'", LLError::LEVEL_DEBUG);
		}
	}
	if (data_input.has("MinimumVersion"))
	{
		PV_DEBUG("Getting minimum version...", LLError::LEVEL_DEBUG);
		const LLSD& min_version = data_input["MinimumVersion"];
		for (LLSD::map_const_iterator iter = min_version.beginMap(); iter != min_version.endMap(); ++iter)
		{
			std::string version = iter->first;
			const LLSD& reason = iter->second;
			//LL_DEBUGS() << "reason = " << reason << LL_ENDL;
			minimum_version_[version] = reason;
			LL_DEBUGS("PVData") << "Minimum Version is " << version << LL_ENDL;
		}
	}

#if PVDATA_MOTD
	// Set Message Of The Day if present
	PV_DEBUG("Attempting to find MOTD data", LLError::LEVEL_DEBUG);
	if (data_input.has("MOTD"))
	{
		PV_DEBUG("Found a MOTD!", LLError::LEVEL_DEBUG);
		gAgent.mMOTD.assign(data_input["MOTD"]);
	}
#if PVDATA_MOTD_CHAT
	if (data_input.has("ChatMOTD")) // only used if MOTD is not presence in the xml file.
	{
		PV_DEBUG("Found Chat MOTDs!", LLError::LEVEL_DEBUG);
		const LLSD& motd = data_input["ChatMOTD"];
		LLSD::array_const_iterator iter = motd.beginArray();
		gAgent.mChatMOTD.assign((iter + (ll_rand(static_cast<S32>(motd.size()))))->asString());
	}
#endif // PVDATA_MOTD_CHAT

	// If the event falls within the current date, use that for MOTD instead.
	PV_DEBUG("Attempting to find Events data", LLError::LEVEL_DEBUG);
	if (data_input.has("EventsMOTD"))
	{
		motd_events_list_ = data_input["EventsMOTD"];
		for (LLSD::map_const_iterator iter = motd_events_list_.beginMap(); iter != motd_events_list_.endMap(); ++iter)
		{
			std::string name = iter->first;
			const LLSD& content = iter->second;
			PV_DEBUG("Found event MOTD: " + name, LLError::LEVEL_DEBUG);

			if (content["startDate"].asDate() < LLDate::now() && content["endDate"].asDate() > LLDate::now())
			{
				PV_DEBUG("Setting MOTD to " + name, LLError::LEVEL_DEBUG);
				// TODO: Shove into notification well.
				gAgent.mMOTD.assign(content["EventMOTD"]); // note singular instead of plural above
				break; // Only use the first one found.
			}
		}
	}
#endif // PVDATA_MOTD
#if PVDATA_PROGRESS_TIPS

	// TODO: Split tips files
	// <polarity> Load the progress screen tips
	PV_DEBUG("Attempting to find Progress Tip data", LLError::LEVEL_DEBUG);
	if (data_input.has("ProgressTip"))
	{
		PV_DEBUG("Found Progress Tips!", LLError::LEVEL_DEBUG);
		// Store list for later use
		progress_tips_list_ = data_input["ProgressTip"];
	}
#endif // PVDATA_PROGRESS_TIPS

	if (data_input.has("WindowTitles"))
	{
		// Store list for later use
		window_titles_list_ = data_input["WindowTitles"];
	}
	data_parse_status_ = OK;
	LL_INFOS("PVData") << "Done parsing data" << LL_ENDL;
}

std::string PVData::getNewProgressTipForced()
{
	// This assigns a random entry as the MOTD / Progress Tip message.
	LLSD::array_const_iterator tip_iter = progress_tips_list_.beginArray();
	if (tip_iter == progress_tips_list_.endArray())
		return "";
	std::string random_tip = (tip_iter + (ll_rand(static_cast<S32>(progress_tips_list_.size()))))->asString();
	LL_INFOS("PVData") << "Setting Progress tip to '" << random_tip << "'" << LL_ENDL;
	return random_tip;
}

std::string PVData::getNewProgressTip(const std::string msg_in)
{
	LL_DEBUGS("PVData") << "Entering function" << LL_ENDL;
	// Pass the existing message right through
	if (!msg_in.empty())
	{
		LL_DEBUGS("PVData") << "returning '" << msg_in << "' in passthrough mode" << LL_ENDL;
		return msg_in;
	}
	// Use the last tip if available
	std::string return_tip = last_login_tip;
	if (mTipCycleTimer.getStarted())
	{
		static LLCachedControl<F32> progress_tip_timout(gSavedSettings, "PVUI_ProgressTipTimer", 2.f);
		if (mTipCycleTimer.getElapsedTimeF32() >= progress_tip_timout)
		{
			LL_DEBUGS("PVData") << "mTipCycleTimer elapsed; getting a new random tip" << LL_ENDL;
			LL_DEBUGS("PVData") << "Last tip was '" << last_login_tip << "'" << LL_ENDL;

			// Most likely a teleport screen; let's add something.

			return_tip = gPVData->progress_tips_list_.getRandom();
			LL_DEBUGS("PVData") << "New tip from function is '" << return_tip << "'" << LL_ENDL;

			if (!return_tip.empty() && return_tip != last_login_tip)
			{
				LL_INFOS("PVData") << "Setting new progress tip to '" << return_tip << "'" << LL_ENDL;
				last_login_tip = return_tip;
			}
			mTipCycleTimer.reset();
		}
	}
	else
	{
		LL_WARNS("PVData") << "mTipCycleTimer not started!" << LL_ENDL;
	}

	return return_tip;
}

void PVData::parsePVAgents(const LLSD& data_input)
{
	// Make sure we don't accidentally parse multiple times. Remember to reset data_parse_status_ when parsing is needed again.
	if (!canParse(agents_parse_status_))
	{
		LL_WARNS("PVData") << "AGENTS Parsing aborted due to parsing being unsafe at the moment" << LL_ENDL;
		return;
	}

	agents_parse_status_ = PARSING;
	LL_INFOS("PVData") << "Beginning to parse Agents" << LL_ENDL;

	PV_DEBUG("Attempting to find Agents root nodes", LLError::LEVEL_DEBUG);
	if (data_input.has("SpecialAgentsList"))
	{
		const LLSD& special_agents_llsd = data_input["SpecialAgentsList"];
		for (LLSD::map_const_iterator uuid_iterator = special_agents_llsd.beginMap();
			 uuid_iterator != special_agents_llsd.endMap(); ++uuid_iterator)
		{
			Dump("SpecialAgentsList", special_agents_llsd);
			// <key>UUID</key>, which declares the agent's data block
			LLUUID uuid;
			LLUUID::parseUUID(uuid_iterator->first, &uuid);

			const LLSD& data_map = uuid_iterator->second;
			if (data_map.has("Access") && data_map["Access"].type() == LLSD::TypeInteger)
			{
				pv_agent_access_[uuid] = data_map["Access"].asInteger();
			}
			if (data_map.has("HexColor") && data_map["HexColor"].type() == LLSD::TypeString)
			{
				pv_agent_color_llcolor4[uuid] = Hex2Color4(data_map["HexColor"].asString());
			}
			if (data_map.has("Title") && data_map["Title"].type() == LLSD::TypeString)
			{
				pv_agent_title_[uuid] = data_map["Title"].asString();
			}
			if (data_map.has("BanReason") && data_map["BanReason"].type() == LLSD::TypeString)
			{
				ban_reason_[uuid] = data_map["BanReason"].asString();
			}
		}
	}
	// TODO PLVR: Find a way to dump these because they aren't LLSD anymore
	//Dump("PVAgents (AgentAccess)", (LLSD)pv_agent_access_);
	//Dump("PVAgents (AgentColors)", agents_colors_);
	//Dump("PVAgents (AgentColors)", agents_colors_);
	//Dump("PVAgents (BanReason)", ban_reason_);
	//Dump("PVAgents (Title)", pv_agent_title_);

	if (data_input.has("SupportGroups"))
	{
		const LLSD& support_groups = data_input["SupportGroups"];
		for (LLSD::map_const_iterator itr = support_groups.beginMap(); itr != support_groups.endMap(); ++itr)
		{
			support_group_.insert(LLUUID(itr->first));
			PV_DEBUG("Added " + itr->first + " to support_group_", LLError::LEVEL_DEBUG);
		}
	}

	agents_parse_status_ = OK;
	LL_INFOS("PVData") << "Done parsing agents" << LL_ENDL;

	// TODO PLVR: Hook up
	//autoMuteFlaggedAgents();
}


bool PVData::getDataDone() const
{
	if (data_parse_status_ == OK)
	{
		return true;
	}
	return false;
}

bool PVData::getAgentsDone() const
{
	if (agents_parse_status_ == OK)
	{
		return true;
	}
	return false;
}
// <polarity> The Linden Lab viewer's logic is somewhat spaghetti and confusing to me, so I wrote my own.
std::string PVData::getPreferredName(const LLAvatarName& av_name)
{
	static LLCachedControl<bool> show_username(gSavedSettings, "NameTagShowUsernames");
	static LLCachedControl<bool> use_display_names(gSavedSettings, "UseDisplayNames");
	// Fallback
	std::string preferred_name = "";

	if (!use_display_names && !show_username)
	{
		return av_name.getUserName();
	}
	if (use_display_names && show_username)
	{
		preferred_name = av_name.getCompleteNameForced(); // Show everything
	}
	else if (use_display_names && !show_username)
	{
		preferred_name = av_name.getDisplayNameForced();
	}

	if (preferred_name == "")
	{
		// we shouldn't hit this, but a sane fallback can't hurt.
		preferred_name = av_name.getUserName();
		LL_WARNS("PVData") << "Preferred Name was unavailable, returning '" << preferred_name << "'" << LL_ENDL;
	}

	return preferred_name;
}

LLUUID PVData::getLockDownUUID()
{
// Workaround for missing CMAKE flags
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

// Remember to set STRING type in CMAKE
#ifndef PVDATA_UUID_LOCKTO
#define PVDATA_UUID_LOCKTO ""
#endif

	std::string temp = TOSTRING(PVDATA_UUID_LOCKTO);
	if (temp.length() != UUID_STR_LENGTH)
	{
		return LLUUID::null;
	}
	return static_cast<LLUUID>(temp);
}
bool PVData::isAllowedToLogin(const LLUUID& avatar_id)
{
	pvdata_error_message_ = "Generic Error Message";
#if DEVEL_BUILD
	return isDeveloper(avatar_id);
#else
	LLUUID lockdown_uuid = getLockDownUUID();
	if (lockdown_uuid != LLUUID::null)
	{
		LL_INFOS("PVData") << "Locked-down build; evaluating access level..." << LL_ENDL;
		if (avatar_id == lockdown_uuid)
		{
			LL_INFOS("PVData") << "Identity confirmed. Proceeding. Enjoy your privileges." << LL_ENDL;
			return true;
		}// else
		pvdata_error_message_ = "This build is locked down to another account.";
		return false;
	}
	if (lockdown_uuid != LLUUID::null)
	{
		pvdata_error_message_ = "Something went wrong, and the authentication checks have failed.";
		return false;
	}
	S32 av_flags = getAgentFlags(avatar_id);
	auto compiled_channel = LLVersionInfo::getCompiledChannel();
	if (av_flags & FLAG_USER_BANNED)
	{
		pvdata_error_message_ = "Unfortunately, you have been disallowed to login to [SECOND_LIFE] using [APP_NAME]. If you believe this message to be an error, restart the viewer. Otherwise, Please download another Viewer.";
		return false;
	}
	if (compiled_channel == APP_NAME + " Release"
		// Allow beta builds as well.
		|| compiled_channel == APP_NAME + " Beta")
	{
		return true;
	}
#if !RELEASE_BUILD
	// prevent non-release builds to fall in the wrong hands
	LL_WARNS("PVData") << "Not a Release build; evaluating access level..." << LL_ENDL;
	LL_WARNS("PVData") << "RAW Access level for '" << avatar_id << "' : '" << av_flags << "'" << LL_ENDL;
	if (av_flags & FLAG_USER_BETA_TESTER)
	{
		LL_WARNS() << "Access level: TESTER" << LL_ENDL;
		return true;
	}
	if (av_flags & FLAG_STAFF_SUPPORT)
	{
		LL_WARNS("PVData") << "Access level: SUPPORT" << LL_ENDL;
		return true;
	}
	if (av_flags & FLAG_STAFF_QA)
	{
		LL_WARNS("PVData") << "Access level: QA" << LL_ENDL;
		return true;
	}
	if (av_flags & FLAG_STAFF_DEV)
	{
		LL_WARNS("PVData") << "Access level: DEVELOPER" << LL_ENDL;
		return true;
	}
#endif //RELEASE_BUILD
	LL_WARNS("PVData") << "Access level: NONE" << LL_ENDL;
	pvdata_error_message_ = "You do not have permission to use this build of [APP_NAME]. Please wait for the public release.";
	return false;
#endif
}

/**
 * \brief Determines if the current binary is a known, and blocked release.
 * \return true if the release is blocked, false if allowed.
 */
bool PVData::isBlockedRelease()
{
	// This little bit of code here does a few things. First it grabs the viewer's current version. Then it attempts to find that specific version
	// in the list of blocked versions (blocked_versions_).
	// If the version is found, it assigns the version's index to the iterator 'iter', otherwise assigns map::find's retun value which is 'map::end'
	const std::string& sCurrentVersion = LLVersionInfo::getChannelAndVersionStatic();
	const std::string& sCurrentVersionShort = LLVersionInfo::getShortVersion();
	// Blocked Versions
	str_llsd_pairs::iterator blockedver_iterator = blocked_versions_.find(sCurrentVersion);
	// Minimum Version
	str_llsd_pairs::iterator minver_iterator = minimum_version_.begin();
	pvdata_error_message_ = "Quit living in the past!";

	// Check if version is lower than the minimum version
	if (minver_iterator != minimum_version_.end() // Otherwise crashes if data is missing due to network failures
		&& sCurrentVersionShort < minver_iterator->first)
	{
		const LLSD& reason_llsd = minver_iterator->second;
		pvdata_error_message_.assign(reason_llsd["REASON"]);
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << pvdata_error_message_ << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	// Check if version is explicitely blocked
	if (blockedver_iterator != blocked_versions_.end()) // if the iterator's value is map::end, it is not in the array.
	{
		// assign the iterator's associaded value (the reason message) to the LLSD that will be returned to the calling function
		const LLSD& reason_llsd = blockedver_iterator->second;
		pvdata_error_message_.assign(reason_llsd["REASON"]);
		LL_WARNS("PVData") << sCurrentVersion << " is not allowed to be used anymore (" << pvdata_error_message_ << ")" << LL_ENDL;
		LLFloaterAboutUtil::checkUpdatesAndNotify();
		return true;
	}
	else
	{
		PV_DEBUG(sCurrentVersion + " not found in the blocked releases list", LLError::LEVEL_DEBUG);
	}

	// default
	return false;
}

inline S32 PVData::getAgentFlags(const LLUUID& avatar_id)
{
	return pv_agent_access_[avatar_id];
}

bool PVData::isDeveloper(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_STAFF_DEV);
}

bool PVData::isSupport(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_STAFF_SUPPORT);
}

bool PVData::isQA(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_STAFF_QA);
}

bool PVData::isTester(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_BETA_TESTER);
}

bool PVData::isDeniedSupport(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_NO_SUPPORT);
}

bool PVData::isMuted(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_AUTOMUTED);
}

bool PVData::isBanned(const LLUUID& avatar_id)
{
	return (getAgentFlags(avatar_id) & FLAG_USER_BANNED);
}

bool PVData::isSupportGroup(const LLUUID& avatar_id) const
{
	return (support_group_.count(avatar_id));
}

bool PVData::isPolarized(const LLUUID& avatar_id)
{
	// TODO: Re-order flags by hierarchy again and make this nicer
	auto flags = getAgentFlags(avatar_id);
	return (flags > FLAG_USER_NO_SUPPORT && flags != FLAG_TITLE_OVERRIDE);
}

bool PVData::isLinden(const LLUUID& avatar_id, S32& av_flags) const
{
#if !FIXED_LINDEN_CHECK
	// Our shit is broken, let's fall back to the basic code.
	std::string first_name, last_name;
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(avatar_id, &av_name))
	{
		std::istringstream full_name(av_name.getUserName());
		full_name >> first_name >> last_name;
	}
	else
	{
		gCacheName->getFirstLastName(avatar_id, first_name, last_name);
	}

	return (last_name == LL_LINDEN ||
		last_name == LL_MOLE ||
		last_name == LL_PRODUCTENGINE ||
		last_name == LL_SCOUT ||
		last_name == LL_TESTER);
#else
	// <Polarity> Speed up: Check if we already establed that association
	if (agents_linden_[avatar_id.asString()].asBoolean())
	{
		return true;
	}


		full_name >> first_name >> last_name;
	}
	else
	{
		gCacheName->getFirstLastName(avatar_id, first_name, last_name);
	}
	if (first_name == "(waiting)" || last_name.empty()) // name cache not ready or Resident, which will never be a linden/god. abort.
	{
		return false;
	}
	if (last_name == LL_LINDEN
		|| last_name == LL_MOLE
		|| last_name == LL_PRODUCTENGINE
		|| last_name == LL_SCOUT
		|| last_name == LL_TESTER)
	{
		// set bit for LL employee
		PV_DEBUG(first_name + " " + last_name + " is a linden!", LLError::LEVEL_INFO);
		//pv_agent_access_[avatar_id.asString()] = (av_flags |= FLAG_LINDEN_EMPLOYEE);
		agents_linden_[avatar_id.asString()] = true;
		return true;
	}

	PV_DEBUG(first_name + (last_name.empty() ? "" : " " + last_name) + " is NOT a linden!", LLError::LEVEL_DEBUG);
	return false;
#endif
}

bool PVData::getAgentTitle(const LLUUID& avatar_id, std::string& agent_title)
{
	agent_title = pv_agent_title_[avatar_id];
	return (!agent_title.empty());
}
// Checks on the agent using the viewer

std::string PVData::getAgentFlagsAsString(const LLUUID& avatar_id)
{
	// Check for agents flagged through PVData
	std::string flags_string = "";
	std::vector<std::string> flags_list;
	S32 av_flags = gPVData->getAgentFlags(avatar_id);
	if (isLinden(avatar_id, av_flags))
	{
		flags_list.push_back("Linden Lab Employee");
	}
	if (av_flags || !flags_list.empty())
	{
		// LL_WARNS() << "Agent Flags for " << avatar_id << " = " << av_flags << LL_ENDL;
		std::string custom_title;
		//auto title_ptr *
		if (getAgentTitle(avatar_id, custom_title))
		{
			// Custom tag present, drop previous title to use that one instead.
			flags_list.clear();
			flags_list.push_back(custom_title);
		}
		else
		{
			// here are the bad flags
			if (av_flags & FLAG_USER_AUTOMUTED)
			{
				flags_list.push_back("Nuisance");
			}
			if (av_flags & FLAG_USER_BANNED)
			{
				flags_list.push_back("Exiled");
			}
			if (av_flags & FLAG_USER_NO_SUPPORT)
			{
				flags_list.push_back("Unsupported");
			}
			// And here are the good flags
			if (av_flags & FLAG_STAFF_DEV)
			{
				flags_list.push_back("Developer");
			}
			if (av_flags & FLAG_STAFF_QA)
			{
				flags_list.push_back("QA");
			}
			if (av_flags & FLAG_STAFF_SUPPORT)
			{
				flags_list.push_back("Support");
			}
			if (av_flags & FLAG_USER_BETA_TESTER)
			{
				flags_list.push_back("Tester");
			}
		}

		using namespace boost::spirit::karma;
		std::ostringstream string_stream;
		string_stream << format(string % ',', flags_list);
		flags_string = string_stream.str();
	}
	return flags_string;
}

void PVData::startRefreshTimer()
{
	if (!pvdata_refresh_timer_.getStarted())
	{
		LL_INFOS("PVData") << "Starting PVData refresh timer" << LL_ENDL;
		pvdata_refresh_timer_.start();
	}
	else
	{
		LL_WARNS("PVData") << "Timer already started!" << LL_ENDL;
	}
}

bool PVData::refreshDataFromServer(bool force_refresh_now)
{
	static LLCachedControl<U32> refresh_minutes(gSavedSettings, "PVData_RefreshTimeout", 60); // Minutes
	if (force_refresh_now || pvdata_refresh_timer_.getElapsedTimeF32() >= refresh_minutes * 60)
	{
		LL_INFOS("PVData") << "Attempting to live-refresh PVData" << LL_ENDL;
		instance().downloadData();

		PV_DEBUG("Attempting to live-refresh Agents data", LLError::LEVEL_DEBUG);
		instance().downloadAgents();
		if (!force_refresh_now)
		{
			PV_DEBUG("Resetting timer", LLError::LEVEL_DEBUG);
			pvdata_refresh_timer_.reset();
		}
		return true;
	}
	return false;
}

// static
/**
 * \brief Developer-only message logger
 * \param log_in_s message to display/log
 * \param level severity level, defaults to debug
 */
void PVData::PV_DEBUG(const std::string& log_in_s, const LLError::ELevel& level, const bool& developer_only)
{
	// Skip debug entirely if the user isn't authenticated yet
	if ((LLStartUp::getStartupState() <= STATE_LOGIN_PROCESS_RESPONSE)
		|| !(gPVData->isDeveloper(gAgentID))) // or if not a developer
	{
		return;
	}
	static LLCachedControl<bool> pvdebug_printtolog(gSavedSettings, "PVDebug_PrintToLog", true);
	if (!pvdebug_printtolog)
	{
		return;
	}
	// Signed int because there is no operator for LLError::LEVEL_WARN => S32
	auto log_level = static_cast<signed int>(level);
	static auto pvdebug_printtolog_forcedlevel = static_cast<signed int>(gSavedSettings.getS32("PVDebug_PrintToLogForcedLevel"));
	if (pvdebug_printtolog_forcedlevel >= 0)
	{
		// Don't let the user set log as error: this will crash them.
		if (LLError::LEVEL_ERROR == pvdebug_printtolog_forcedlevel)
		{
			 pvdebug_printtolog_forcedlevel = LLError::LEVEL_WARN;
		}
		log_level = pvdebug_printtolog_forcedlevel;
	}
	// Ensure our string is null-terminated.
	const std::string nullterm_string = log_in_s.c_str();

	if (LLError::LEVEL_DEBUG == log_level)
	{
		LL_DEBUGS("PVData") << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_INFO == log_level)
	{
		LL_INFOS("PVData") << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_WARN == log_level)
	{
		LL_WARNS("PVData") << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_ERROR == log_level)
	{
		LL_ERRS("PVData") << nullterm_string << LL_ENDL;
	}
	if (LLError::LEVEL_NONE == log_level)
	{
		// Nope.
	}
}

void PVData::Dump(const std::string name, const LLSD& map)
{
	static LLCachedControl<bool> pvdebug_printtolog(gSavedSettings, "PVDebug_PrintToLog", true);

	if ((LLStartUp::getStartupState() <= STATE_LOGIN_PROCESS_RESPONSE) // Skip debug entirely if the user isn't authenticated yet
		|| !gPVData->isDeveloper(gAgentID) // or if not a developer
		|| !pvdebug_printtolog) // or if chosing not to.
	{
		return;
	}
	std::stringstream str;
	LLSDSerialize::toPrettyXML(map, str);
	PV_DEBUG("\n===========================\n<!--  <" + name + "> -->\n" + str.str() + "\n<!--  </" + name + "> -->\n===========================\n", LLError::LEVEL_DEBUG);
}

LLColor4 PVData::getColor(const LLUUID& avatar_id, const LLColor4& default_color, const bool& is_buddy_and_show_it)
{
	// Try to operate in the same instance, reduce call overhead
	auto uiCT = LLUIColorTable::getInstance();

	LLColor4 return_color = default_color; // color we end up with at the end of the logic
	LLColor4 pvdata_color = default_color; // User color from PVData if user has one, equals return_color otherwise.

	static const LLUIColor linden_color = uiCT->getColor("PlvrLindenChatColor", LLColor4::cyan);
	static const LLUIColor muted_color = uiCT->getColor("PlvrMutedChatColor", LLColor4::grey);

	// Some PVData-flagged users CAN be muted.
	// TODO PLVR: Do we still need this?
	if (LLMuteList::instance().isMuted(avatar_id))
	{
		return_color = muted_color.get();
		return return_color;
	}

	// Check if agent is flagged through PVData
	auto av_flags = gPVData->getAgentFlags(avatar_id);

	// Get custom color (from PVData or fast cache
	if (gPVData->pv_agent_color_llcolor4.find(avatar_id) != gPVData->pv_agent_color_llcolor4.end())
	{
		pvdata_color = gPVData->pv_agent_color_llcolor4[avatar_id];
	}
	else
	{
		if (gPVData->isLinden(avatar_id, av_flags))
		{
			instance().pv_agent_color_llcolor4.insert_or_assign(avatar_id, linden_color.get());
			return linden_color.get();
		}
		if (av_flags)
		{
			if (av_flags & FLAG_STAFF_DEV)
			{
				static const LLUIColor dev_color = uiCT->getColor("PlvrDevChatColor", LLColor4::orange);
				pvdata_color = dev_color.get();
			}
			else if (av_flags & FLAG_STAFF_QA)
			{
				static const LLUIColor qa_color = uiCT->getColor("PlvrQAChatColor", LLColor4::red);
				pvdata_color = qa_color.get();
			}
			else if (av_flags & FLAG_STAFF_SUPPORT)
			{
				static const LLUIColor support_color = uiCT->getColor("PlvrSupportChatColor", LLColor4::magenta);
				pvdata_color = support_color.get();
			}
			else if (av_flags & FLAG_USER_BETA_TESTER)
			{
				static const LLUIColor tester_color = uiCT->getColor("PlvrTesterChatColor", LLColor4::yellow);
				pvdata_color = tester_color.get();
			}
			else if (av_flags & FLAG_USER_BANNED)
			{
				static const LLUIColor banned_color = uiCT->getColor("PlvrBannedChatColor", LLColor4::grey2);
				pvdata_color = banned_color.get();
			}
			else
			{
				LL_WARNS("PVData") << "Color Manager caught a bug! Agent is supposed to be special but no code path exists for this case!\n" << "(This is most likely caused by a missing agent flag)" << LL_ENDL;
				LL_WARNS("PVData") << "~~~~~~~ COLOR DUMP ~~~~~~~" << LL_ENDL;
				LL_WARNS("PVData") << "avatar_id = " << avatar_id << LL_ENDL;
				LL_WARNS("PVData") << "av_flags = " << av_flags << LL_ENDL;
				LL_WARNS("PVData") << "would-be pvdata_color = " << pvdata_color << LL_ENDL;
				LL_WARNS("PVData") << "~~~ END OF COLOR DUMP ~~~" << LL_ENDL;
				LL_WARNS("PVData") << "Report this occurence and send the lines above to the Polarity Developers" << LL_ENDL;
			}
			// Speedup: Put fetched agent color into cached list to speed up subsequent function calls
			gPVData->pv_agent_color_llcolor4.insert_or_assign(avatar_id, pvdata_color);
		}
	}

	/*	Respect user preferences
	Expected behavior:
	+Friend, +PVDATA, +lpf = show PVDATA
	+Friend, +PVDATA, -lpl = show FRIEND
	+Friend, -PVDATA, +lpl = show FRIEND
	+Friend, -PVDATA, -lpl = show FRIEND
	-Friend, +PVDATA, +lpl = show PVDATA
	-Friend, +PVDATA, -lpl = show PVDATA
	-Friend, -PVDATA, +lpl = show FALLBACK
	-Friend, -PVDATA, -lpl = show FALLBACK
	*/
	static LLCachedControl<bool> show_friends(gSavedSettings, "NameTagShowFriends");
	static LLCachedControl<bool> low_priority_friend_status(gSavedSettings, "PVColorManager_LowPriorityFriendStatus", true);
	bool show_f = (show_friends && is_buddy_and_show_it);

	// Lengthy but fool-proof.
	if (show_f && av_flags && low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (show_f && av_flags && !low_priority_friend_status)
	{
		return_color = uiCT->getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !av_flags && low_priority_friend_status)
	{
		return_color = uiCT->getColor("NameTagFriend", LLColor4::yellow);
	}
	if (show_f && !av_flags && !low_priority_friend_status)
	{
		return_color = uiCT->getColor("NameTagFriend", LLColor4::yellow);
	}
	if (!show_f && av_flags && low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (!show_f && av_flags && !low_priority_friend_status)
	{
		return_color = pvdata_color;
	}
	if (!show_f && !av_flags && low_priority_friend_status)
	{
		return_color = default_color;
	}
	if (!show_f && !av_flags && !low_priority_friend_status)
	{
		return_color = default_color;
	}
	return return_color;
}

LLColor4 PVData::Hex2Color4(const std::string color) const
{
	return instance().Hex2Color4(stoul(color, nullptr, 16));
}
LLColor4 PVData::Hex2Color4(int hexValue)
{
	auto r = ((hexValue >> 16) & 0xFF) / 255.0f;  // Extract the RR byte
	auto g = ((hexValue >> 8) & 0xFF) / 255.0f;   // Extract the GG byte
	auto b = ((hexValue) & 0xFF) / 255.0f;        // Extract the BB byte
	return LLColor4(r, g, b, 1.0f);
}

U32 PVData::getSearchSeparatorFromSettings()
{
	static LLCachedControl<U32> settings_separator(gSavedSettings, "PVUI_SubstringSearchSeparator", separator_space);
	LL_DEBUGS("PVData") << "Search separator index from settings: '" << settings_separator << "'" << LL_ENDL;
	return settings_separator;
}

void PVData::setSearchSeparator(const U32 separator_in_u32)
{
	PVSearchSeparatorSelected = separator_in_u32;;
	LL_DEBUGS("PVData") << "Setting search separator to '" << separator_in_u32 << "'" << "('" << getSearchSeparator() << "')" << LL_ENDL;
	gSavedSettings.setU32("PVUI_SubstringSearchSeparator", separator_in_u32);
	
}

std::string PVData::getSearchSeparator()
{
	auto  separator = PVSearchSeparatorAssociation[PVSearchSeparatorSelected];
	LL_DEBUGS("PVData") << "Search separator from runtime: '" << separator << "'" << LL_ENDL;
	return separator;
}

std::string PVData::getSearchSeparator(const U32 separator_to_get_u32) const
{
	PVSearchSeparatorSelected = separator_to_get_u32;
	return getSearchSeparator();
}

// static
void PVData::setChatLogsDirOverride()
{
#ifdef FINISHED_CHAT_LOG_WRITE
/*
	// ReSharper disable CppDeprecatedEntity // we are cross-platform.
	auto override_location = getenv("PV_CHATLOGS_LOCATION_OVERRIDE");
	// ReSharper restore CppDeprecatedEntity
	if (override_location && override_location != gSavedPerAccountSettings.getString("InstantMessageLogPath").c_str())
	{

		LL_WARNS("PVData") << "Would set logs location to: " << override_location << LL_ENDL;
		//gSavedPerAccountSettings.setString("InstantMessageLogPath", override_location);
		//LLFloaterPreference::moveTranscriptsAndLog();
	}

	LPCWSTR name_ = L"PV_CHATLOGS_LOCATION_OVERRIDE";
	std::string value_ = log_location_from_settings;

	HKEY        key;
	HKEY        subKey;
	char const *subKeyName;

	if (es_invalid == scope_) {
		return;
	}

	switch (scope_) {
	case es_system:
		key = HKEY_LOCAL_MACHINE;
		subKeyName = systemEnvSubKey;
		break;

	case es_user:
		key = HKEY_CURRENT_USER;
		subKeyName = userEnvSubKey;
		break;
	}

	// Assign the new value.
	value_ = text;

	// Write the new value to the registry.
	RegOpenKeyEx(key, subKeyName, 0, KEY_SET_VALUE, &subKey);
	RegSetValueEx(subKey,
		name_.c_str(),
		0,
		REG_EXPAND_SZ,
		reinterpret_cast<const BYTE *>(value_.c_str()),
		value_.length() + 1);
	RegCloseKey(key);
*/
#endif
}

#if LL_WINDOWS
// Microsoft's runtime library doesn't support the standard setenv() function.
// http://stackoverflow.com/a/23616164
int setenv(const char *name, const char *value, int overwrite)
{
	int errcode = 0;
	if (!overwrite) {
		size_t envsize = 0;
		errcode = getenv_s(&envsize, NULL, 0, name);
		if (errcode || envsize) return errcode;
	}
	return _putenv_s(name, value);
}

#endif // LL_WINDOWS

std::string getRegKey(const std::string& name_) {
	//LL_WARNS("PVData") << "Would set logs location to: " << log_location_from_settings << LL_ENDL;
	// README: This assumes the variable is set.
	//setenv("PV_CHATLOGS_LOCATION_OVERRIDE", gSavedPerAccountSettings.getString("InstantMessageLogPath").c_str(), 1);
	// Borrowed from editenv.dll by Dan Moulding (Visual Leak Detector's author)

	//PBYTE       data;

	char *		data = nullptr;
	HKEY        key = HKEY_CURRENT_USER;
	DWORD		size = 1024 * sizeof(TCHAR);;
	//LONG        status;
	HKEY        subKey;
	std::string value = "";

	long ret;
	ret = RegOpenKeyExA(HKEY_CURRENT_USER, "\\Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &subKey);
	RegQueryValueExA(subKey, name_.c_str(), 0, NULL, NULL, &size);
	if (ret != ERROR_SUCCESS) {
		LL_WARNS("PVData") << "Key [" << name_ << "] does not exist!" << LL_ENDL;
		return std::string();
	}
	// This environment variable already exists.
	ret = RegQueryValueExA(key, name_.c_str(), 0, 0, reinterpret_cast<LPBYTE>(data), &size);
	if (ret != ERROR_SUCCESS)
	{
		return std::string();
	}
		LL_WARNS("PVData") << "Key [" << name_ << "] exist!" << LL_ENDL;
		if (data != nullptr)
		{
			value = std::string(data);
		}
		delete[] data;
	
		LL_WARNS("PVData") << "Key [" << value << "] = exist!" << LL_ENDL;
	RegCloseKey(key);
	//auto nya = RegCreateKeyEx(HKEY_CURRENT_USER, ,0,NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,);
	// TODO: Linux and OSX support
	return value;
}

void PVData::getChatLogsDirOverride()
{
	std::string log_location_from_settings = gSavedPerAccountSettings.getString("InstantMessageLogPath");
	std::string registry_key = "PV_CHATLOGS_LOCATION_OVERRIDE"; // TODO: Move to global
	// ReSharper disable CppDeprecatedEntity // cross-platform needs std:: function
	char* log_location_from_registry = getenv(registry_key.c_str());

	
	auto log_location_from_runtime = gDirUtilp->getChatLogsDir();
	std::string new_chat_logs_dir = "";
	if (log_location_from_settings.empty() || log_location_from_registry == NULL)
	{
		new_chat_logs_dir = gDirUtilp->getOSUserAppDir();
	}
	else if (log_location_from_registry != NULL && log_location_from_registry[0] != '\0')
	{

		new_chat_logs_dir = log_location_from_registry;
	}
	//if (new_chat_logs_dir != log_location_from_settings || gDirUtilp->getChatLogsDir() != log_location_from_registry)
	//{
	PV_DEBUG("Would set logs location to: " + new_chat_logs_dir, LLError::LEVEL_WARN);
	PV_DEBUG("gDirUtilp->getChatLogsDir() = " + gDirUtilp->getChatLogsDir(), LLError::LEVEL_WARN);

	LL_WARNS("PVData") << "Chat log location = " << new_chat_logs_dir << LL_ENDL;
	//}
	if (new_chat_logs_dir.empty())
	{
		LL_ERRS("PVData") << "new_chat_logs_dir is null!" << LL_ENDL;
	}
	else if (new_chat_logs_dir == "")
	{
		LL_ERRS("PVData") << "new_chat_logs_dir is empty!" << LL_ENDL;
	}
	else
	{
		gDirUtilp->setChatLogsDir(new_chat_logs_dir);
	}
	
	if(new_chat_logs_dir != gDirUtilp->getChatLogsDir())
	{
		PV_DEBUG("Hmmm strange, location mismatch: " + new_chat_logs_dir + " != " + gDirUtilp->getChatLogsDir(),LLError::LEVEL_WARN);
	}

	gSavedPerAccountSettings.setString("InstantMessageLogPath", new_chat_logs_dir);
}

// Copied from LLFloaterPreferences because we need to run this without a floater instance existing.
bool PVData::moveTranscriptsAndLog(std::string userid) const
{
	std::string instantMessageLogPath(gSavedPerAccountSettings.getString("InstantMessageLogPath"));
	std::string chatLogPath = gDirUtilp->add(instantMessageLogPath, userid);

	bool madeDirectory = false;

	//Does the directory really exist, if not then make it
	if (!LLFile::isdir(chatLogPath))
	{
		//mkdir success is defined as zero
		if (LLFile::mkdir(chatLogPath) != 0)
		{
			return false;
		}
		madeDirectory = true;
	}

	std::string originalConversationLogDir = LLConversationLog::instance().getFileName();
	std::string targetConversationLogDir = gDirUtilp->add(chatLogPath, "conversation.log");
	//Try to move the conversation log
	if (!LLConversationLog::instance().moveLog(originalConversationLogDir, targetConversationLogDir))
	{
		//Couldn't move the log and created a new directory so remove the new directory
		if (madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}
		return false;
	}

	//Attempt to move transcripts
	std::vector<std::string> listOfTranscripts;
	std::vector<std::string> listOfFilesMoved;

	LLLogChat::getListOfTranscriptFiles(listOfTranscripts);

	if (!LLLogChat::moveTranscripts(gDirUtilp->getChatLogsDir(),
		instantMessageLogPath,
		listOfTranscripts,
		listOfFilesMoved))
	{
		//Couldn't move all the transcripts so restore those that moved back to their old location
		LLLogChat::moveTranscripts(instantMessageLogPath,
			gDirUtilp->getChatLogsDir(),
			listOfFilesMoved);

		//Move the conversation log back
		LLConversationLog::instance().moveLog(targetConversationLogDir, originalConversationLogDir);

		if (madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}

		return false;
	}

	gDirUtilp->setChatLogsDir(instantMessageLogPath);
	gDirUtilp->updatePerAccountChatLogsDir();

	return true;
}
