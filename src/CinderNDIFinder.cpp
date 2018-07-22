#include "CinderNDIFinder.h"

CinderNDIFinder::CinderNDIFinder( const Description dscr )
{
	if( ! NDIlib_initialize() ) {
		throw std::runtime_error( "Cannot run NDI on this machine. Probably unsupported CPU." );
	}
	// Create the NDIFinder instance based on the current description
	NDIlib_find_create_t findDscr{ dscr.mShowLocalSources, dscr.mGroups.c_str(), dscr.mExtraIPs.c_str() };
	mNDIFinder = NDIlib_find_create_v2( &findDscr );
	if( ! mNDIFinder ) {
		throw std::runtime_error( "Cannot create NDI Finder. NDIlib_find_create_v2 returned nullptr" );
	}
	// Connect to the application update loop for polling the NDIFinder
	mAppConnectionUpdate = ci::app::AppBase::get()->getSignalUpdate().connect( 
		std::bind( &CinderNDIFinder::update, this )
	);
}

CinderNDIFinder::~CinderNDIFinder()
{
	if( mNDIFinder ) {
		NDIlib_find_destroy( mNDIFinder );
		mNDIFinder = nullptr;
	}
	// No issue with multiple calls or still active objects.
	NDIlib_destroy();
	mAppConnectionUpdate.disconnect();
	mConnectedNDISources.clear();
}

void CinderNDIFinder::update()
{
	auto numCurrentlyConnectedSources = mConnectedNDISources.size();
	uint32_t currentSources = 0;
	const auto* sources = NDIlib_find_get_current_sources( mNDIFinder, &currentSources );	
	// Check to see if NDI source have changed.
	if( numCurrentlyConnectedSources != currentSources ) {
		// NDI sources have been removed.
		if( currentSources < numCurrentlyConnectedSources ) {
			// If all sources have disconnected, flush the local copy of connections.
			if( currentSources == 0 ) {
				auto sourceIt = mConnectedNDISources.begin();
				while( sourceIt != mConnectedNDISources.end() ) {
					mNDISourceRemoved.emit( sourceIt->name );
					sourceIt = mConnectedNDISources.erase( sourceIt );
				}
			}
			else { // Otherwise check to see which sources have been disconnected and update local connections copy.
				for( auto sourceIt = mConnectedNDISources.begin(); sourceIt != mConnectedNDISources.end(); ) {
					bool exists = false;
					for( size_t sourceIndex = 0; sourceIndex < currentSources; ++sourceIndex ) {
						if( sources[ sourceIndex ].p_ndi_name == sourceIt->name ) {
							exists = true;
						}	
					}
					if( ! exists ) {
						mNDISourceRemoved.emit( sourceIt->name );
						sourceIt = mConnectedNDISources.erase( sourceIt );
					}
					else {
						++sourceIt;
					}
				}	
			}
		}
		else { // NDI sources have been added.
			for( size_t sourceIndex = 0; sourceIndex < currentSources; ++sourceIndex ) {
				// Add this source in our local connections copy, only if it does not already exists.
				auto sourceIt = std::find_if( mConnectedNDISources.begin()
								, mConnectedNDISources.end()
								, [ & ] ( const CinderNDISource& source ) -> bool {
									if( sources[ sourceIndex ].p_ndi_name == source.name )
										return true;
									else return false;
								}
							);
				if( sourceIt == mConnectedNDISources.end() ) {
					mConnectedNDISources.emplace_back( CinderNDISource{ sources[ sourceIndex ].p_ndi_name } );	
					mNDISourceAdded.emit( sources[ sourceIndex ] );
				}
			}
		}
	}
}

ci::signals::Signal<void( const NDISource& )>& CinderNDIFinder::getSignalNDISourceAdded()
{
	return mNDISourceAdded;
}

ci::signals::Signal<void( std::string )>& CinderNDIFinder::getSignalNDISourceRemoved()
{
	return mNDISourceRemoved;
}
