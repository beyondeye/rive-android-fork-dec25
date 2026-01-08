# Task Description
On January 8th we merged into the repo the changes in upstream repo of rive-android. The changes are described [here](../docs/merged_upstream_changes_jan_2026.md)
This changes require attention because the CommandQueue architecture that was also adopted in mprive kotlin multiplatform port
of original rive-android that was refactored to CommandQueueBridge is different. mprive should reflect the
new architecture and the change of architecture can actually make full port of rive-android to kotlin multiplatform easier.
