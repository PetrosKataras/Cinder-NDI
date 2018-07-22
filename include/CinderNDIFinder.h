#pragma once

#include <memory>
#include <string>
#include "Processing.NDI.Lib.h"
#include "cinder/app/AppBase.h"

class CinderNDIFinder;
using CinderNDIFinderPtr = std::unique_ptr<CinderNDIFinder>;

using NDIFinderPtr = NDIlib_find_instance_t;
using NDISource = NDIlib_source_t;

class CinderNDIFinder {
public:
	struct Description {
		bool	mShowLocalSources{ true };	
		std::string 	mGroups;
		std::string		mExtraIPs;
	};
	struct CinderNDISource {
		std::string name;
	};
	using ConnectedNDISources = std::vector<CinderNDISource>;

	CinderNDIFinder( const Description dscr );
	~CinderNDIFinder();

	ci::signals::Signal<void( const NDISource& )>& 	getSignalNDISourceAdded();
	ci::signals::Signal<void( std::string )>& 	getSignalNDISourceRemoved();
private:
	void										update();
private:
	NDIFinderPtr 								mNDIFinder;
	ci::signals::Connection 					mAppConnectionUpdate;
	ci::signals::Signal<void( const NDISource& )>	mNDISourceAdded;
	ci::signals::Signal<void( std::string )>	mNDISourceRemoved;
	ConnectedNDISources							mConnectedNDISources;
};
