//
// initialsettings.h: aggregates all cx configurator data into single structure
//
#ifndef CONFIGURATORINITIALSETTINGS_H
#define CONFIGURATORINITIALSETTINGS_H

#include "transport_settings.h"
#include "transportprotocols.h"
#include "volumegroupsettings.h"
#include "retentionsettings.h"
#include "prismsettings.h"
#include "sourcesystemconfigsettings.h"


#define INITIAL_SETTINGS_VERSION 2

struct ViableCx
{
    std::string publicip;
    std::string privateip;
    SV_UINT port;

    ViableCx();
    ViableCx(const ViableCx&);
    bool operator==(const ViableCx&) const;
    ViableCx& operator=(const ViableCx&);
};
typedef std::list<ViableCx> ViableCxs;
typedef std::map<std::string, ViableCxs > REMOTECX_MAP;


struct ReachableCx
{
	std::string ip;
	SV_UINT port;

	ReachableCx();
	ReachableCx(const ReachableCx&);
	bool operator==(const ReachableCx&) const;
	ReachableCx& operator=(const ReachableCx&);
};


struct InitialSettings {
	InitialSettings() : timeoutInSecs(0), 
		transportProtocol(TRANSPORT_PROTOCOL_UNKNOWN), transportSecureMode(VOLUME_SETTINGS::SECURE_MODE_NONE) {}

	TRANSPORT_CONNECTION_SETTINGS getTransportSettings(const std::string & deviceName);
	bool shouldThrottleTarget(std::string const& deviceName) const;

	TRANSPORT_CONNECTION_SETTINGS getTransportSettings(void);
	VOLUME_SETTINGS::SECURE_MODE getTransportSecureMode(void);
	TRANSPORT_PROTOCOL getTransportProtocol(void);
	bool shouldRegisterOnDemand(void) const;
    std::string getRequestIdForOnDemandRegistration(void) const;
	std::string getDisksLayoutOption(void) const;
	bool AnyJobsAvailableForProcessing(void) const;
    std::string getCsAddressForAzureComponents(void) const;

    HOST_VOLUME_GROUP_SETTINGS hostVolumeSettings;//r/o,r/w,resync<fast,secure>
	CDPSETTINGS_MAP cdpSettings;

    //
    // if the primary cx is not reachable for "timeout"
    // secs, agent will contact remotecx and start pointing
    // to remotecx if available.
    //
    // For now, there will be one remotecx
    // later on, agents will be able to talk to multiple cx
    // on replication pair (endpoint/volume) basis.
    //
    SV_ULONG timeoutInSecs;
    REMOTECX_MAP remoteCxs;

    PRISM_SETTINGS prismSettings;

	TRANSPORT_CONNECTION_SETTINGS transportSettings;
	TRANSPORT_PROTOCOL transportProtocol;
	/* TODO: remove SECURE_MODE from volume settings
	 * and keep it in transport settings.h 
	 * as secure mode holds for any transport */
	VOLUME_SETTINGS::SECURE_MODE transportSecureMode;

	/* Currently options stores on demand registration request id */
    Options_t options;
	
	InitialSettings(const InitialSettings&);
	bool operator==( InitialSettings const&) const;
	bool strictCompare(InitialSettings const&) const;
    InitialSettings& operator=(const InitialSettings&);
};
bool StrictCompare(const InitialSettings & lhs, const InitialSettings &rhs);

namespace NameSpaceInitialSettings
{
	/* here follows keys*/
	const char ON_DEMAND_REGISTRATION_REQUEST_ID_KEY[] = "on_demand_registration_request_id";
	const char DISKS_LAYOUT_OPTION_KEY[] = "disks_layout_option";
    const char RESOURCE_ID_KEY[] = "resource_id";
	const char ANY_JOBS_TO_BE_PROCESSED_KEY[] = "any_jobs_to_be_processed";
    const char CS_ADDRESS_FOR_AZURE_COMPONENTS[] = "cs_address_for_azure_components";


	/* here follows values*/
};

#define CSJOBS_VERSION 0

/// ------------------------------------------------------------------------
/// Constants for specifying job status.

/// <summary>
/// Default status.
/// </summary>
const std::string CsJobStatusPending = "Pending";

/// <summary>
/// Status represents job is completed.
/// </summary>
const std::string CsJobStatusCompleted = "Completed";

/// <summary>
/// Status represents job is in progress.
/// </summary>
const std::string CsJobStatusInProgress = "InProgress";

/// <summary>
/// Status represents job has failed.
/// </summary>
const std::string CsJobStatusFailed = "Failed";

/// <summary>
/// Status represents job got cancelled.
/// </summary>
const std::string CsJobStatusCancelled = "Cancelled";

/// ------------------------------------------------------------------------

/// ------------------------------------------------------------------------
/// Constants to describe the type of job to be performed.


/// <summary>
/// Update registry hive and certs required for AAD migration.
/// </summary>
const std::string MTPrepareForAadMigrationJob = "MTPrepareForAadMigrationJob";

/// ------------------------------------------------------------------------


/// <summary>
/// Contains information about job context.
/// </summary>
struct CsJobContext
{
	/// <summary>
	/// Gets or sets the activity ID of the job.
	/// </summary>
	std::string ActivityId;

	/// <summary>
	/// Gets or sets the client request ID of the job.
	/// </summary>
	std::string ClientRequestId;
};

struct CsJobError
{
	/// <summary>
	/// Gets error code.
	/// </summary>
	std::string ErrorCode;

	/// <summary>
	/// Gets error message.
	/// </summary>
	std::string Message;

	/// <summary>
	/// Gets possible causes of error.
	/// </summary>
	std::string PossibleCauses;

	/// <summary>
	/// Gets recommended action to resolve error.
	/// </summary>
	std::string RecommendedAction;

	/// <summary>
	/// Gets the parameters for constructing the message.
	/// </summary>
	std::map<std::string, std::string> MessageParams;
};

/// <summary>
/// Defines job input and output for any data plane component.
/// </summary>
struct CsJob
{
	/// <summary>
	/// Gets or sets job identifier.
	/// </summary>
	std::string Id;

	/// <summary>
	/// Gets or sets job type.
	/// </summary>
	std::string JobType;

	/// <summary>
	/// Gets or sets job status.
	/// </summary>
	std::string JobStatus;

	/// <summary>
	/// Gets or sets job input payload.
	/// The payload will be serialized string representation of job input
	/// contract depending on the job type.
	/// </summary>
	std::string InputPayload;

	/// <summary>
	/// Gets or sets job output payload.
	/// The payload will be serialized string representation of job output
	/// contract depending on the job type.
	/// </summary>
	std::string OutputPayload;

	/// <summary>
	/// Gets or sets job errors.
	/// </summary>
	std::list<CsJobError> Errors;

	/// <summary>
	/// Gets or sets the job context.
	/// </summary>
	CsJobContext Context;
};

/// <summary>
/// Input for MTPrepareForAadMigration job.
/// </summary>
struct MTPrepareForAadMigrationInput
{
	/// Base64 encoded certificate for MARS agent registration.
	std::string certData;

	/// Based64 encoded registry hive data to imported for MARS agent consumption.
	std::string registryData;
};

#endif // CONFIGURATORINITIALSETTINGS_H


