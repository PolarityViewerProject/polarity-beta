/**
 * @file _httppolicy.cpp
 * @brief Internal definitions of the Http policy thread
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#include "linden_common.h"

#include "_httppolicy.h"

#include "_httpoprequest.h"
#include "_httpservice.h"
#include "_httplibcurl.h"
#include "_httppolicyclass.h"

#include "lltimer.h"


namespace LLCore
{


// Per-policy-class data for a running system.
// Collection of queues, options and other data
// for a single policy class.
//
// Threading:  accessed only by worker thread
struct HttpPolicy::ClassState
{
public:
	ClassState()
		: mThrottleEnd(0),
		  mThrottleLeft(0L),
		  mRequestCount(0L)
		{}
	
	HttpReadyQueue		mReadyQueue;
	HttpRetryQueue		mRetryQueue;

	HttpPolicyClass		mOptions;
	HttpTime			mThrottleEnd;
	long				mThrottleLeft;
	long				mRequestCount;
};


HttpPolicy::HttpPolicy(HttpService * service)
	: mService(service)
{
	// Create default class
	mClasses.push_back(new ClassState());
}


HttpPolicy::~HttpPolicy()
{
	shutdown();

	for (class_list_t::iterator it(mClasses.begin()); it != mClasses.end(); ++it)
	{
		delete (*it);
	}
	mClasses.clear();
	
	mService = NULL;
}


HttpRequest::policy_t HttpPolicy::createPolicyClass()
{
	const HttpRequest::policy_t policy_class(mClasses.size());
	if (policy_class >= HTTP_POLICY_CLASS_LIMIT)
	{
		return HttpRequest::INVALID_POLICY_ID;
	}
	mClasses.push_back(new ClassState());
	return policy_class;
}


void HttpPolicy::shutdown()
{
	for (int policy_class(0); policy_class < mClasses.size(); ++policy_class)
	{
		ClassState & state(*mClasses[policy_class]);
		
		HttpRetryQueue & retryq(state.mRetryQueue);
		while (! retryq.empty())
		{
			HttpOpRequest * op(retryq.top());
			retryq.pop();
		
			op->cancel();
			op->release();
		}

		HttpReadyQueue & readyq(state.mReadyQueue);
		while (! readyq.empty())
		{
			HttpOpRequest * op(readyq.top());
			readyq.pop();
		
			op->cancel();
			op->release();
		}
	}
}


void HttpPolicy::start()
{}


void HttpPolicy::addOp(HttpOpRequest * op)
{
	const int policy_class(op->mReqPolicy);
	
	op->mPolicyRetries = 0;
	op->mPolicy503Retries = 0;
	mClasses[policy_class]->mReadyQueue.push(op);
}


void HttpPolicy::retryOp(HttpOpRequest * op)
{
	static const HttpTime retry_deltas[] =
		{
			 250000,			// 1st retry in 0.25 S, etc...
			 500000,
			1000000,
			2000000,
			5000000				// ... to every 5.0 S.
		};
	static const int delta_max(int(LL_ARRAY_SIZE(retry_deltas)) - 1);
	static const HttpStatus error_503(503);

	const HttpTime now(totalTime());
	const int policy_class(op->mReqPolicy);
	HttpTime delta(retry_deltas[llclamp(op->mPolicyRetries, 0, delta_max)]);
	bool external_delta(false);

	if (op->mReplyRetryAfter > 0 && op->mReplyRetryAfter < 30)
	{
		delta = op->mReplyRetryAfter * U64L(1000000);
		external_delta = true;
	}
	op->mPolicyRetryAt = now + delta;
	++op->mPolicyRetries;
	if (error_503 == op->mStatus)
	{
		++op->mPolicy503Retries;
	}
	LL_DEBUGS("CoreHttp") << "HTTP request " << static_cast<HttpHandle>(op)
						  << " retry " << op->mPolicyRetries
						  << " scheduled in " << (delta / HttpTime(1000))
						  << " mS (" << (external_delta ? "external" : "internal")
						  << ").  Status:  " << op->mStatus.toHex()
						  << LL_ENDL;
	if (op->mTracing > HTTP_TRACE_OFF)
	{
		LL_INFOS("CoreHttp") << "TRACE, ToRetryQueue, Handle:  "
							 << static_cast<HttpHandle>(op)
							 << ", Delta:  " << (delta / HttpTime(1000))
							 << ", Retries:  " << op->mPolicyRetries
							 << LL_ENDL;
	}
	mClasses[policy_class]->mRetryQueue.push(op);
}


// Attempt to deliver requests to the transport layer.
//
// Tries to find HTTP requests for each policy class with
// available capacity.  Starts with the retry queue first
// looking for requests that have waited long enough then
// moves on to the ready queue.
//
// If all queues are empty, will return an indication that
// the worker thread may sleep hard otherwise will ask for
// normal polling frequency.
//
// Implements a client-side request rate throttle as well.
// This is intended to mimic and predict throttling behavior
// of grid services but that is difficult to do with different
// time bases.  This also represents a rigid coupling between
// viewer and server that makes it hard to change parameters
// and I hope we can make this go away with pipelining.
//
HttpService::ELoopSpeed HttpPolicy::processReadyQueue()
{
	const HttpTime now(totalTime());
	HttpService::ELoopSpeed result(HttpService::REQUEST_SLEEP);
	HttpLibcurl & transport(mService->getTransport());
	
	for (int policy_class(0); policy_class < mClasses.size(); ++policy_class)
	{
		ClassState & state(*mClasses[policy_class]);
		HttpRetryQueue & retryq(state.mRetryQueue);
		HttpReadyQueue & readyq(state.mReadyQueue);

		if (retryq.empty() && readyq.empty())
		{
			continue;
		}
		
		const bool throttle_enabled(state.mOptions.mThrottleRate > 0L);
		const bool throttle_current(throttle_enabled && now < state.mThrottleEnd);

		if (throttle_current && state.mThrottleLeft <= 0)
		{
			// Throttled condition, don't serve this class but don't sleep hard.
			result = HttpService::NORMAL;
			continue;
		}

		int active(transport.getActiveCountInClass(policy_class));
		int needed(state.mOptions.mConnectionLimit - active);		// Expect negatives here

		if (needed > 0)
		{
			// First see if we have any retries...
			while (needed > 0 && ! retryq.empty())
			{
				HttpOpRequest * op(retryq.top());
				if (op->mPolicyRetryAt > now)
					break;
			
				retryq.pop();
				
				op->stageFromReady(mService);
				op->release();

				++state.mRequestCount;
				--needed;
				if (throttle_enabled)
				{
					if (now >= state.mThrottleEnd)
					{
						// Throttle expired, move to next window
						LL_DEBUGS("CoreHttp") << "Throttle expired with " << state.mThrottleLeft
											  << " requests to go and " << state.mRequestCount
											  << " requests issued." << LL_ENDL;
						state.mThrottleLeft = state.mOptions.mThrottleRate;
						state.mThrottleEnd = now + HttpTime(1000000);
					}
					if (--state.mThrottleLeft <= 0)
					{
						goto throttle_on;
					}
				}
			}
			
			// Now go on to the new requests...
			while (needed > 0 && ! readyq.empty())
			{
				HttpOpRequest * op(readyq.top());
				readyq.pop();

				op->stageFromReady(mService);
				op->release();
					
				++state.mRequestCount;
				--needed;
				if (throttle_enabled)
				{
					if (now >= state.mThrottleEnd)
					{
						// Throttle expired, move to next window
						LL_DEBUGS("CoreHttp") << "Throttle expired with " << state.mThrottleLeft
											  << " requests to go and " << state.mRequestCount
											  << " requests issued." << LL_ENDL;
						state.mThrottleLeft = state.mOptions.mThrottleRate;
						state.mThrottleEnd = now + HttpTime(1000000);
					}
					if (--state.mThrottleLeft <= 0)
					{
						goto throttle_on;
					}
				}
			}
		}

	throttle_on:
		
		if (! readyq.empty() || ! retryq.empty())
		{
			// If anything is ready, continue looping...
			result = HttpService::NORMAL;
		}
	} // end foreach policy_class

	return result;
}


bool HttpPolicy::changePriority(HttpHandle handle, HttpRequest::priority_t priority)
{
	for (int policy_class(0); policy_class < mClasses.size(); ++policy_class)
	{
		ClassState & state(*mClasses[policy_class]);
		// We don't scan retry queue because a priority change there
		// is meaningless.  The request will be issued based on retry
		// intervals not priority value, which is now moot.
		
		// Scan ready queue for requests that match policy
		HttpReadyQueue::container_type & c(state.mReadyQueue.get_container());
		for (HttpReadyQueue::container_type::iterator iter(c.begin()); c.end() != iter;)
		{
			HttpReadyQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c.erase(cur);									// All iterators are now invalidated
				op->mReqPriority = priority;
				state.mReadyQueue.push(op);						// Re-insert using adapter class
				return true;
			}
		}
	}
	
	return false;
}


bool HttpPolicy::cancel(HttpHandle handle)
{
	for (int policy_class(0); policy_class < mClasses.size(); ++policy_class)
	{
		ClassState & state(*mClasses[policy_class]);

		// Scan retry queue
		HttpRetryQueue::container_type & c1(state.mRetryQueue.get_container());
		for (HttpRetryQueue::container_type::iterator iter(c1.begin()); c1.end() != iter;)
		{
			HttpRetryQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c1.erase(cur);									// All iterators are now invalidated
				op->cancel();
				op->release();
				return true;
			}
		}
		
		// Scan ready queue
		HttpReadyQueue::container_type & c2(state.mReadyQueue.get_container());
		for (HttpReadyQueue::container_type::iterator iter(c2.begin()); c2.end() != iter;)
		{
			HttpReadyQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c2.erase(cur);									// All iterators are now invalidated
				op->cancel();
				op->release();
				return true;
			}
		}
	}
	
	return false;
}


bool HttpPolicy::stageAfterCompletion(HttpOpRequest * op)
{
	static const HttpStatus cant_connect(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
	static const HttpStatus cant_res_proxy(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_PROXY);
	static const HttpStatus cant_res_host(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_HOST);
	static const HttpStatus send_error(HttpStatus::EXT_CURL_EASY, CURLE_SEND_ERROR);
	static const HttpStatus recv_error(HttpStatus::EXT_CURL_EASY, CURLE_RECV_ERROR);
	static const HttpStatus upload_failed(HttpStatus::EXT_CURL_EASY, CURLE_UPLOAD_FAILED);
	static const HttpStatus op_timedout(HttpStatus::EXT_CURL_EASY, CURLE_OPERATION_TIMEDOUT);
	static const HttpStatus post_error(HttpStatus::EXT_CURL_EASY, CURLE_HTTP_POST_ERROR);

	// Retry or finalize
	if (! op->mStatus)
	{
		// If this failed, we might want to retry.  Have to inspect
		// the status a little more deeply for those reasons worth retrying...
		if (op->mPolicyRetries < op->mPolicyRetryLimit &&
			((op->mStatus.isHttpStatus() && op->mStatus.mType >= 499 && op->mStatus.mType <= 599) ||
			 cant_connect == op->mStatus ||
			 cant_res_proxy == op->mStatus ||
			 cant_res_host == op->mStatus ||
			 send_error == op->mStatus ||
			 recv_error == op->mStatus ||
			 upload_failed == op->mStatus ||
			 op_timedout == op->mStatus ||
			 post_error == op->mStatus))
		{
			// Okay, worth a retry.  We include 499 in this test as
			// it's the old 'who knows?' error from many grid services...
			retryOp(op);
			return true;				// still active/ready
		}
	}

	// This op is done, finalize it delivering it to the reply queue...
	if (! op->mStatus)
	{
		LL_WARNS("CoreHttp") << "HTTP request " << static_cast<HttpHandle>(op)
							 << " failed after " << op->mPolicyRetries
							 << " retries.  Reason:  " << op->mStatus.toString()
							 << " (" << op->mStatus.toHex() << ")"
							 << LL_ENDL;
	}
	else if (op->mPolicyRetries)
	{
		LL_DEBUGS("CoreHttp") << "HTTP request " << static_cast<HttpHandle>(op)
							  << " succeeded on retry " << op->mPolicyRetries << "."
							  << LL_ENDL;
	}

	op->stageFromActive(mService);
	op->release();
	return false;						// not active
}

	
HttpPolicyClass & HttpPolicy::getClassOptions(HttpRequest::policy_t pclass)
{
	llassert_always(pclass >= 0 && pclass < mClasses.size());
	
	return mClasses[pclass]->mOptions;
}


int HttpPolicy::getReadyCount(HttpRequest::policy_t policy_class) const
{
	if (policy_class < mClasses.size())
	{
		return (mClasses[policy_class]->mReadyQueue.size()
				+ mClasses[policy_class]->mRetryQueue.size());
	}
	return 0;
}


}  // end namespace LLCore
