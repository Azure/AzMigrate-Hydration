### Enable Replication/Reprotect Failures for VMs with multiple disks

Due to a recent code change, we have observed enable replication failures on some replication jobs which attempted to protect high number of disks at once.
This can occur in the race condition where initial resync of 2 or more disks reach the service concurrently, causing a parallel update to a backend entity.
While a retry should sinficantly reduces the likelihood of race condition, for VMs which contain very high numnber of disks, we recommend customers to enable protection on the VM, and replicate the data disks once the VM is in protected state to reduce chances of the race condition. Since the failure is immediate, retrying the job after few minutes is the favoured mitigation to resolve this issue.

Please run the MultiDiskEnableReplicationFix.ps1 to add disks to an already protected VM, if  the issue reoccurs in retry. The script resolves the race condition by calling replication on the disks in a sequential manner.

#### Update (7th June, 2025): Azure Site Recovery has started the rollout of the fix.
#### Update (12th June, 2025): :white_check_mark: Azure Site Recovery has completed the rollout of the fix in all the public regions.

### Deployment status for the service side fix.

| Region               | Deployment Status     |
|----------------------|------------------------|
| Australia Central    | :white_check_mark: Completed              |
| Australia Central 2  | :white_check_mark: Completed              |
| Australia East       | :white_check_mark: Completed              |
| Australia Southeast  | :white_check_mark: Completed              |
| Austria East         | :white_check_mark: Completed              |
| Brazil South         | :white_check_mark: Completed              |
| Brazil Southeast     | :white_check_mark: Completed              |
| Canada Central       | :white_check_mark: Completed              |
| Canada East          | :white_check_mark: Completed              |
| Central India        | :white_check_mark: Completed              |
| Central US           | :white_check_mark: Completed              |
| Chile Central        | :white_check_mark: Completed           |
| East Asia            | :white_check_mark: Completed            |
| East US              | :white_check_mark: Completed              |
| East US 2            | :white_check_mark: Completed              |
| France Central       | :white_check_mark: Completed              |
| France South         | :white_check_mark: Completed              |
| Germany North        | :white_check_mark: Completed              |
| Germany West Central | :white_check_mark: Completed              |
| Indonesia Central    | :white_check_mark: Completed           |
| Israel Central       | :white_check_mark: Completed           |
| Italy North          | :white_check_mark: Completed           |
| Japan East           | :white_check_mark: Completed              |
| Japan West           | :white_check_mark: Completed              |
| Korea Central        | :white_check_mark: Completed              |
| Korea South          | :white_check_mark: Completed              |
| Malaysia West        | :white_check_mark: Completed           |
| Mexico Central       | :white_check_mark: Completed           |
| New Zealand North    | :white_check_mark: Completed           |
| North Central US     | :white_check_mark: Completed              |
| North Europe         | :white_check_mark: Completed             |
| Norway East          | :white_check_mark: Completed              |
| Norway West          | :white_check_mark: Completed              |
| Poland Central       | :white_check_mark: Completed           |
| Qatar Central        | :white_check_mark: Completed           |
| South Africa North   | :white_check_mark: Completed              |
| South Africa West    | :white_check_mark: Completed              |
| South Central US     | :white_check_mark: Completed              |
| South India          | :white_check_mark: Completed              |
| Southeast Asia       | :white_check_mark: Completed             |
| Spain Central        | :white_check_mark: Completed           |
| Sweden Central       | :white_check_mark: Completed              |
| Sweden South         | :white_check_mark: Completed              |
| Switzerland North    | :white_check_mark: Completed              |
| Switzerland West     | :white_check_mark: Completed              |
| UAE Central          | :white_check_mark: Completed              |
| UAE North            | :white_check_mark: Completed              |
| UK South             | :white_check_mark: Completed              |
| UK West              | :white_check_mark: Completed              |
| West Central US      | :white_check_mark: Completed              |
| West Europe          | :white_check_mark: Completed              |
| West India           | :white_check_mark: Completed              |
| West US              | :white_check_mark: Completed              |
| West US 2            | :white_check_mark: Completed              |
| West US 3            | :white_check_mark: Completed              |

