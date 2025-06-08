### Enable Replication/Reprotect Failures for VMs with multiple disks

Due to a recent code change, we have observed enable replication failures on some replication jobs which attempted to protect high number of disks at once.
This can occur in the race condition where initial resync of 2 or more disks reach the service concurrently, causing a parallel update to a backend entity.
While a retry should sinficantly reduces the likelihood of race condition, for VMs which contain very high numnber of disks, we recommend customers to enable protection on the VM, and replicate the data disks once the VM is in protected state to reduce chances of the race condition. Since the failure is immediate, retrying the job after few minutes is the favoured mitigation to resolve this issue.

Please run the MultiDiskEnableReplicationFix.ps1 to add disks to an already protected VM, if  the issue reoccurs in retry. The script resolves the race condition by calling replication on the disks in a sequential manner.

#### Update (7th June, 2025): Azure Site Recovery has started the rollout of the fix.

### Deployment status for the service side fix.

| Region               | Deployment Status     |
|----------------------|------------------------|
| Australia Central    | Not Completed              |
| Australia Central 2  | Not Completed              |
| Australia East       | Not Completed              |
| Australia Southeast  | Not Completed              |
| Austria East         | Not Completed              |
| Brazil South         | Not Completed              |
| Brazil Southeast     | Not Completed              |
| Canada Central       | Not Completed              |
| Canada East          | Not Completed              |
| Central India        | Not Completed              |
| Central US           | Not Completed              |
| Chile Central        | Not Completed           |
| East Asia            | :heavy_check_mark: 7th June, 2025             |
| East US              | Not Completed              |
| East US 2            | Not Completed              |
| France Central       | Not Completed              |
| France South         | Not Completed              |
| Germany North        | Not Completed              |
| Germany West Central | Not Completed              |
| Indonesia Central    | Not Completed           |
| Israel Central       | Not Completed           |
| Italy North          | Not Completed           |
| Japan East           | Not Completed              |
| Japan West           | Not Completed              |
| Korea Central        | Not Completed              |
| Korea South          | Not Completed              |
| Malaysia West        | Not Completed           |
| Mexico Central       | Not Completed           |
| New Zealand North    | Not Completed           |
| North Central US     | Not Completed              |
| North Europe         | Not Completed              |
| Norway East          | Not Completed              |
| Norway West          | Not Completed              |
| Poland Central       | Not Completed           |
| Qatar Central        | Not Completed           |
| South Africa North   | Not Completed              |
| South Africa West    | Not Completed              |
| South Central US     | Not Completed              |
| South India          | Not Completed              |
| Southeast Asia       | :heavy_check_mark: 8th June, 2025              |
| Spain Central        | Not Completed           |
| Sweden Central       | Not Completed              |
| Sweden South         | Not Completed              |
| Switzerland North    | Not Completed              |
| Switzerland West     | Not Completed              |
| UAE Central          | Not Completed              |
| UAE North            | Not Completed              |
| UK South             | Not Completed              |
| UK West              | Not Completed              |
| West Central US      | Not Completed              |
| West Europe          | Not Completed              |
| West India           | Not Completed              |
| West US              | Not Completed              |
| West US 2            | Not Completed              |
| West US 3            | Not Completed              |

