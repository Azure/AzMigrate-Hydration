### Enable Replication/Reprotect Failures for VMs with multiple disks

Due to a recent code change, we have observed enable replication failures on some replication jobs which attempted to protect high number of disks at once.
This can occur in the race condition where initial resync of 2 or more disks reach the service concurrently, causing a parallel update to a backend entity.
While a retry should sinficantly reduces the likelihood of race condition, for VMs which contain very high numnber of disks, we recommend customers to enable protection on the VM, and replicate the data disks once the VM is in protected state to reduce chances of the race condition. Since the failure is immediate, retrying the job after few minutes is the favoured mitigation to resolve this issue.

Please run the MultiDiskEnableReplicationFix.ps1 to add disks to an already protected VM, if  the issue reoccurs in retry. The script resolves the race condition by calling replication on the disks in a sequential manner.

#### Update (7th June, 2025): Azure Site Recovery has started the rollout of the fix.
#### Update (13th June, 2025): :white_check_mark: Azure Site Recovery has completed the rollout of the fix in all the public regions.

### Deployment status for the service side fix.

| Region               | Deployment Status     |
|----------------------|------------------------|
| Australia Central    | :white_check_mark: Completed (10th June, 2025)                 |
| Australia Central 2  | :white_check_mark: Completed (10th June, 2025)                 |
| Australia East       | :white_check_mark: Completed (10th June, 2025)             |
| Australia Southeast  | :white_check_mark: Completed (10th June, 2025)                |
| Austria East         | :white_check_mark: Completed (13th June, 2025)              |
| Brazil South         | :white_check_mark: Completed (10th June, 2025)                  |
| Brazil Southeast     | :white_check_mark: Completed (11th June, 2025)                |
| Canada Central       | :white_check_mark: Completed (10th June, 2025)              |
| Canada East          | :white_check_mark: Completed (10th June, 2025)          |
| Central India        | :white_check_mark: Completed (10th June, 2025)                |
| Central US           | :white_check_mark: Completed (11th June, 2025)                |
| Chile Central        | :white_check_mark: Completed (13th June, 2025)             |
| East Asia            | :white_check_mark: Completed (7th June, 2025)            |
| East US              | :white_check_mark: Completed (13th June, 2025)                |
| East US 2            | :white_check_mark: Completed (13th June, 2025)                |
| France Central       | :white_check_mark: Completed (10th June, 2025)                  |
| France South         | :white_check_mark: Completed (10th June, 2025)                |
| Germany North        | :white_check_mark: Completed (10th June, 2025)                |
| Germany West Central | :white_check_mark: Completed (10th June, 2025)                  |
| Indonesia Central    | :white_check_mark: Completed (13th June, 2025)             |
| Israel North       | :white_check_mark: Completed (13th June, 2025)            |
| Israel Central       | :white_check_mark: Completed (13th June, 2025)            |
| Italy North          | :white_check_mark: Completed (13th June, 2025)          |
| Japan East           | :white_check_mark: Completed (10th June, 2025)                 |
| Japan West           | :white_check_mark: Completed (10th June, 2025)                |
| Jio India Central         | :white_check_mark: Completed (11th June, 2025)              |
| Jio India West           | :white_check_mark: Completed (11th June, 2025)                |
| Korea Central        | :white_check_mark: Completed (10th June, 2025)            |
| Korea South          | :white_check_mark: Completed (10th June, 2025)             |
| Malaysia South        | :white_check_mark: Completed (11th June, 2025)             |
| Malaysia West        | :white_check_mark: Completed (13th June, 2025)           |
| Mexico Central       | :white_check_mark: Completed (13th June, 2025)         |
| New Zealand North    | :white_check_mark: Completed (11th June, 2025)             |
| North Central US     | :white_check_mark: Completed (10th June, 2025)                 |
| North Europe         | :white_check_mark: Completed (9th June, 2025)             |
| Norway East          | :white_check_mark: Completed (10th June, 2025)                |
| Norway West          | :white_check_mark: Completed (10th June, 2025)                |
| Poland Central       | :white_check_mark: Completed (11th June, 2025)             |
| Qatar Central        | :white_check_mark: Completed (13th June, 2025)             |
| South Africa North   | :white_check_mark: Completed (10th June, 2025)                  |
| South Africa West    | :white_check_mark: Completed (10th June, 2025)                  |
| South Central US     | :white_check_mark: Completed (10th June, 2025)               |
| South India          | :white_check_mark: Completed (10th June, 2025)   |
| Southeast Asia       | :white_check_mark: Completed (8th June, 2025)            |
| Spain Central        | :white_check_mark: Completed (13th June, 2025)           |
| Sweden Central       | :white_check_mark: Completed (11th June, 2025)                |
| Sweden South         | :white_check_mark: Completed (11th June, 2025)                 |
| Switzerland North    | :white_check_mark: Completed (10th June, 2025)                  |
| Switzerland West     | :white_check_mark: Completed (10th June, 2025)                 |
| Taiwan West     | :white_check_mark: Completed (13th June, 2025)              |
| Taiwan North West     | :white_check_mark: Completed (13th June, 2025)           |
| UAE Central          | :white_check_mark: Completed (10th June, 2025)                  |
| UAE North            | :white_check_mark: Completed (10th June, 2025)                |
| UK South             | :white_check_mark: Completed (11th June, 2025)                 |
| UK West              | :white_check_mark: Completed (10th June, 2025)                 |
| US Central 2            | :white_check_mark: Completed (13th June, 2025)             |
| US SouthEast  |:white_check_mark: Completed (13th June, 2025)           |
| US SouthEast 3             | :white_check_mark: Completed (13th June, 2025)                |
| West Central US      | :white_check_mark: Completed (11th June, 2025)                 |
| West Europe          | :white_check_mark: Completed (10th June, 2025)                 |
| West India           | :white_check_mark: Completed (10th June, 2025)                 |
| West US              | :white_check_mark: Completed (11th June, 2025)                |
| West US 2            | :white_check_mark: Completed (11th June, 2025)                |
| West US 3            | :white_check_mark: Completed (10th June, 2025)                 |

