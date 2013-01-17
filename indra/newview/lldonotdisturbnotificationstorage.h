/** 
* @file   lldonotdisturbnotificationstorage.h
* @brief  Header file for lldonotdisturbnotificationstorage
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLDONOTDISTURBNOTIFICATIONSTORAGE_H
#define LL_LLDONOTDISTURBNOTIFICATIONSTORAGE_H

#include "llerror.h"
#include "lleventtimer.h"
#include "llnotifications.h"
#include "llnotificationstorage.h"
#include "llsingleton.h"

class LLSD;

class LLDoNotDisturbNotificationStorageTimer : public LLEventTimer
{
public:
    LLDoNotDisturbNotificationStorageTimer();
    ~LLDoNotDisturbNotificationStorageTimer();

public:
    void startTimer();
    void stopTimer();
    bool isRunning();
    BOOL tick();
};

class LLDoNotDisturbNotificationStorage : public LLSingleton<LLDoNotDisturbNotificationStorage>, public LLNotificationStorage
{
	LOG_CLASS(LLDoNotDisturbNotificationStorage);
public:
	LLDoNotDisturbNotificationStorage();
	~LLDoNotDisturbNotificationStorage();

	void initialize();
    bool getDirty();
    void resetDirty();
	void saveNotifications();
	void loadNotifications();
    void updateNotifications();
    void removeIMNotification(const LLUUID& session_id);

protected:

private:
    bool mDirty;
    LLDoNotDisturbNotificationStorageTimer mTimer;

	LLNotificationChannelPtr getCommunicationChannel() const;
	bool                     onChannelChanged(const LLSD& pPayload);
};

#endif // LL_LLDONOTDISTURBNOTIFICATIONSTORAGE_H

