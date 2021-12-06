# mail_system
* A system providing fault-tolerant distributed mail service.
* A data replication system targeted at E-Mail.

## What is special about this mail system?
### Maximized consistency with data efficiency
It is actaully a general data replication system, and we used it specifically for mail system here, what is special is how it can achieve maximized consistency with minimized data transfer.

### Dealt with Network Partition Recovery
Network partition is inevitable, it will happen at point, software cannot solve partition, but it helps recover from it. In this project, we deal with merging servers' date after partition using a knowledge exchange process, after which the servers could be recovered to a consistent state, i.e. every server has as much the same information of emails as possible.