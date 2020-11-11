# Multi-Backup Megavault

This is code to run on a 4-megabit Flash memory (SST39SF040), which allows the
save and load of PC Engine backup memory to the flash memory on the cartridge.

The current version allows up to 32 backup memory slots to be saved along with
date infromation and a name/label for the save data.  There is substantially
more space to save information (both data and meta-data), with the possibility
of roughly 100 save areas plus paragraph-length descriptive information (however,
this would require some rethinking of the user interface).

As the NOR Flash is rated for 100 years and 100,000 write cycles, there is no
concern about lifetime under the usage in this application.  The techniques used
in this application can be used for other similar applications as well:
- usage counting
- save data within a cartridge, not relying on CDROM backup memory
- "protection" against usage on unauthorized hardware (check chip identification)

Note that as the special write commands involve specific read/write patterns at
specific addresses, the flash commands cannot be run from the flash memory itself; 
instead, these sepcific sequences have been relocated to execute from user SRAM.

This is prototype software, and as such has been minimally tested.
Use at your own risk.

Also in theis repository is the board layout information for EAGLE, in order
to get these PC board fabricated.

![Megavault HuCard](images/Megavault_Hu.jpg)


